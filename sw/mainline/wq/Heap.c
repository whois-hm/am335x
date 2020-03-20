#include "_WQ.h"


#if defined (libwq_heap_testmode) && defined (_platform_linux32)
static int get_process(char *buffer, unsigned nbuffer)
{
	if(nbuffer < 256)
	{
		return -1;
	}
	memset(buffer, 0, nbuffer);

	__pid_t id = getpid();
	snprintf(buffer, 200, "/proc/%d/status", id);
	FILE *p = fopen(buffer, "r");
	if(!p)
	{
		return -1;
	}
	memset(buffer, 0, nbuffer);
	while(fgets(buffer, 200, p) != NULL)
	{
		if(!strncmp(buffer, "Name:", 5))
		{
			char *p = (buffer);
			p = p+5;
			while((*p == 0x20) ||
					(*p == 0x09))
			{
				p++;
			}

			char dump[nbuffer];
			memset(dump, 0, nbuffer);

			memcpy(dump, p, strlen(p));
			int len = strlen(dump);
			while(len--)
			{
				if(dump[len]  == 0x0d ||
						dump[len] == 0x0a)
				{
					dump[len] = 0;
				}
			}
			memset(buffer, 0, nbuffer);
			memcpy(buffer, dump, strlen(dump));
			break;
		}
		memset(buffer, 0, nbuffer);
	}
	fclose(p);
	return 1;
}


/*
	--------------------------------------------------------------- page boundary
    unsigned   (tail check sum)																<
	---------------------------------------------------------------						|
 	void[0]... (user block)																		|
 	 ---------------------------------------------------------------						|
    unsigned (front check sum)															|
 	 ---------------------------------------------------------------						|
 	size_t	(user block size)																	|        rw area
 	 ---------------------------------------------------------------						|      dynamic page align
 	void *(backtrace address)																|
 	 ---------------------------------------------------------------						|
 	void *(backtrace address)																|
 	 ---------------------------------------------------------------						|
 	void *(backtrace address)																|
 	 ---------------------------------------------------------------						|
 	void *(backtrace address)																|
 	 ---------------------------------------------------------------						|
 	void * (backtrace address)																|
 	 ---------------------------------------------------------------						|
 	unsigned (number of backtrace)													>
 	 ---------------------------------------------------------------4096		   <
 	void *	(pointer for next chunk)													|
 	 ---------------------------------------------------------------						|
 	page block																								|
 	 ---------------------------------------------------------------						|
 	page block																								|		read only
 	 ---------------------------------------------------------------						|      static page align
 	page block																								|
 	 ---------------------------------------------------------------						|
 	page block																								|
 	 ---------------------------------------------------------------0                  >
 */


#define page_size																						( _link._pagesize )
#define page_ro_next_address(pageblock) 								(((char *)pageblock + page_size)- sizeof(void *))
#define page_rw_backtrace_num_address(pageblock) 		((char *)pageblock + page_size)
#define page_rw_backtrace_ptr_address(pageblock) 		(page_rw_backtrace_num_address(pageblock) + sizeof(unsigned))
#define page_rw_userblocksize_address(pageblock)	 		(page_rw_backtrace_ptr_address(pageblock) + (sizeof(void *) * 5))
#define page_rw_frontchks_address(pageblock) 					(page_rw_userblocksize_address(pageblock) + sizeof(size_t))
#define page_rw_userblockptr_address(pageblock) 			(page_rw_frontchks_address(pageblock) + sizeof(unsigned))
#define page_rw_tailchks_address(pageblock) 						(page_rw_userblockptr_address(pageblock) + *((size_t *)(page_rw_userblocksize_address(pageblock))))



#define page_return_rw_frontchks_address(usrptr)			((char *)usrptr - (sizeof(unsigned)))
#define page_return_rw_userblocksize_address(usrptr)	(page_return_rw_frontchks_address(usrptr) - sizeof(size_t))
#define page_return_rw_backtrace_ptr_address(usrptr)	(page_return_rw_userblocksize_address(usrptr) - (sizeof(void *) * 5))
#define page_return_rw_backtrace_num_address(usrptr)(page_return_rw_backtrace_ptr_address(usrptr) - sizeof(unsigned))
#define page_return_rw_tailchks_address(usrptr)				((char *)usrptr + (*(unsigned *)page_return_rw_userblocksize_address(usrptr)))
#define page_return_ro_next_address(usrptr)						(page_return_rw_backtrace_num_address(usrptr) - sizeof(void *))
#define page_return_start_address(usrptr)								((page_return_ro_next_address(usrptr) +  sizeof(void *)) - page_size)


#define page_return_tailchks_address(usrptr)

#define page_chks (unsigned )0x1a4d4d1a


typedef struct
{
	void *_head;
	criticalsection _lock;
	int _pagesize;
	char proc_name[256];
}pagelink;
 static pagelink _link;

static void page_ro_area_modecontrol(char *page, int flag)
{
	mprotect(page, page_size,flag);
}
static
unsigned page_require_test(size_t size)
{
	unsigned our_requiresize = sizeof(unsigned) +		/*the backtrace number*/
			(sizeof(void *) * 5) + 	/*adress for backtrace */
			sizeof(size_t) +				/*size for user block*/
			sizeof(unsigned) + 		/*checksum for front*/
			size + 									/*user requre size*/
			sizeof(unsigned);					/*checksum for tail*/
	unsigned ro_page_number = 1; /*default 'ro' area page */
	unsigned rw_page_number = 0;;
	do
	{
		rw_page_number ++;
	}while(our_requiresize >  (rw_page_number * page_size));

	return ro_page_number + rw_page_number;
}
 static void __link_page(char *page)
{
	 Platform_criticalsection_lock(&_link._lock, INFINITE);
	void * cur = _link._head;
	if(!cur)
	{

		_link._head = (void *)page;

		CS_unlock(&_link._lock);
		return;
	}

	while(cur &&  (*((int *)page_ro_next_address(cur))))
	{
		cur = (void *)*((int *)page_ro_next_address(cur));
	}

	page_ro_area_modecontrol((char *)cur, PROT_READ | PROT_WRITE);
	(*((int *)page_ro_next_address(cur))) = (int)page;
	page_ro_area_modecontrol((char *)cur, PROT_READ);


	Platform_criticalsection_unlock(&_link._lock);

}
void *__generate_page(size_t size)
{
	char *page_ptr = NULL;
	unsigned npage_block = 0;
	do
	{
		if(size <= 0)
		{
			printf("libwq heap bad alloc size zero\n");
			/*
					invalid parameter
			 */
			break;
		}

		npage_block = page_require_test(size);

		page_ptr = (char *)memalign(page_size, (page_size * npage_block));
		if(!page_ptr)
		{
			/*
			 	 	 can't get page block
			 */
			printf("libwq heap bad alloc detection\n");
			break;
		}
		/*
		 	 set prefixs
		 */
		memset(page_ptr, 0, (page_size * npage_block));
		*((unsigned *)page_rw_backtrace_num_address(page_ptr)) = backtrace((void **)page_rw_backtrace_ptr_address(page_ptr), 5);
		*((size_t *)page_rw_userblocksize_address(page_ptr)) = size;
		*((unsigned *)page_rw_frontchks_address(page_ptr)) = page_chks;
		*((unsigned *)page_rw_tailchks_address(page_ptr)) = page_chks;
		/*
		 	 linking page
		 */
		__link_page(page_ptr);
		/*
		 	 	 mode set readonly
		 */
		page_ro_area_modecontrol(page_ptr, PROT_READ);
	}while(0);
	if(!page_ptr)
	{
		printf("oops null pointer return....\n");
	}
	return page_ptr;
}
static int __unlink_page(char *ptr_usr)
{
	 threadid id= BackGround_id();
	Platform_criticalsection_lock(&_link._lock, INFINITE);
	void *find = NULL;
	void *pre = NULL;
	void *cur = _link._head;

	while(cur)
	{
		if(cur == page_return_start_address(ptr_usr))
		{
			if(!pre)
			{
				_link._head = (void *)*(int *)page_ro_next_address(cur);
			}
			else
			{
				page_ro_area_modecontrol((char *)pre, PROT_READ | PROT_WRITE);

				*((int *)page_ro_next_address(pre)) = (*(int *)page_ro_next_address(cur));

				page_ro_area_modecontrol((char *)pre, PROT_READ);
			}
			find = cur;
			break;
		}
		pre = cur;
		cur = (void *)*(int *)page_ro_next_address(cur);
	}

	Platform_criticalsection_unlock(&_link._lock);

	if(find == NULL)
	{
		return -1;
	}
	return 1;
}
static void __remove_exception_handler(int except)
{
	static char const *case1 = "null pointer free request";
	static char const *case2 = "no registed page  pointer free request";
	static char const *case3 = "broken head chks";
	static char const* case4 = "broken tail chks";

	char * case_what = NULL;

	if(except == -1)			 	case_what = case1;
	else if(except == -2) 		case_what = case2;
	else if(except == -3) 		case_what = case3;
	else if(except == -4) 		case_what = case4;
	/*
	 	 no case else
	 */
	void *backtrace_addr [10] = { NULL, };
	int size = 0;

	size = backtrace(backtrace_addr, 10);
	printf("libwq heap detection (%s)\n", case_what);
	if(size > 0)
	{
		printf("look up this scription\n");

		for(int i = 0; i < size; i++)
		{
			printf("addr2line -f -C -e %s %08x\n", _link.proc_name, backtrace_addr[i]);
		}
	}
	else
	{
		printf("can't look up scription\n");
	}
	exit(0);

}
void __remove_page(char *ptr_usr)
{
	/*
	 	 first, check front checksum
	 */
	int except = 0;
	do
	{
		if(!ptr_usr)
		{
			/*
			 	 null ptr require
			 */
			except = -1;
			break;
		}
		if(__unlink_page(ptr_usr) < 0)
		{
			/*
			 	 can't found our link
			 */
			except = -2;
			break;
		}
		if(*((unsigned *)page_return_rw_frontchks_address(ptr_usr)) != page_chks)
		{
			/*
			 	 broken head
			 */
			except = -3;
			break;
		}
		if(*((unsigned *)page_return_rw_tailchks_address(ptr_usr)) != page_chks)
		{
			/*
			 	 broken tail
			 */
			except = -4;
			break;
		}
		/*
		 	 now we guess page has safety using from user
		 */
		except = 0;
	}while(0);

	if(except< 0)
	{
		__remove_exception_handler(except);
		return;
	}

	char *cleanup_page = page_return_start_address(ptr_usr);
	page_ro_area_modecontrol(cleanup_page, PROT_READ|PROT_WRITE);
	free(cleanup_page);
}
static void print_linked_page()
{
	Platform_criticalsection_lock(&_link._lock, INFINITE);
	void * cur = _link._head;

	printf("----------------------------------------------------\n");
	printf("print linking page start\n");
	unsigned total_user_mallocblock_size = 0;
	unsigned total_user_mallocblock_count = 0;
	if(cur)
	{
		printf("from here allocation position! following script\n");
	}
	while(cur)
	{
		if(*((unsigned *)page_rw_frontchks_address(cur)) != page_chks)
		{
			printf("print_linked_page error (found front chks block )\n");
			printf("backtrace address is likely contaminated, so find free position self\n");
			__remove_exception_handler(-3);
			return;
		}
		if(*((unsigned *)page_rw_tailchks_address(cur)) != page_chks)
		{
			printf("print_linked_page error (found tail chks block )\n");
			printf("backtrace address is likely contaminated, but head position maybe safe, fllow scription\n");
			unsigned backtrace_num = *(unsigned *)page_rw_backtrace_num_address(cur);


			for(unsigned i = 0; i < backtrace_num; i++)
			{
				void **start_backtrace_address = (void **)page_rw_backtrace_ptr_address(cur);
				printf("addr2line -f -C -e %s  %08x\n", _link.proc_name, start_backtrace_address[i]);
			}
			__remove_exception_handler(-4);
			return;
		}
		total_user_mallocblock_size +=  *((size_t *)page_rw_userblocksize_address(cur));
		printf("%d. address : 0x%08x size : %d\n", total_user_mallocblock_count,
				page_rw_userblockptr_address(cur),
				*((size_t *)page_rw_userblocksize_address(cur)));

		unsigned backtrace_num = *(unsigned *)page_rw_backtrace_num_address(cur);


		for(unsigned i = 0; i < backtrace_num; i++)
		{
			void **start_backtrace_address = (void **)page_rw_backtrace_ptr_address(cur);
			printf("addr2line -f -C -e %s  %08x\n", _link.proc_name, start_backtrace_address[i]);
		}
		total_user_mallocblock_count++;
		cur = (void *)*((int *)page_ro_next_address(cur));
	}
	if(total_user_mallocblock_count)
	{
		printf("total count : %d total size : %d\n", total_user_mallocblock_count, total_user_mallocblock_size);
	}
	else
	{
		printf("no found block\n");
	}
	printf("end print linking page\n");
	printf("----------------------------------------------------\n");
	Platform_criticalsection_unlock(&_link._lock);
}


#endif

WQ_API void libwq_heap_testinit()
{
#if defined (libwq_heap_testmode) && defined (_platform_linux32)
	_link._head = NULL;
	_link._pagesize = sysconf(_SC_PAGE_SIZE);;
	get_process(_link.proc_name, 256);
	Platform_criticalsection_open(&_link._lock);
#else
    return;
#endif
}
WQ_API void libwq_heap_testdeinit()
{
#if defined (libwq_heap_testmode) && defined (_platform_linux32)
	print_linked_page();
	Platform_criticalsection_close(&_link._lock);
#else
    return ;
#endif

}
WQ_API void *libwq_malloc(size_t size)
{
#if defined (libwq_heap_testmode) && defined (_platform_linux32)
	void *page = __generate_page(size);
	return page ? page_rw_userblockptr_address(page) : NULL;
#else
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
#endif
}
WQ_API void libwq_free(void *mem)
{
#if defined (libwq_heap_testmode) && defined (_platform_linux32)
	__remove_page(mem);
#else
    free(mem);
#endif
}
WQ_API void libwq_print_heap()
{
#if defined (libwq_heap_testmode) && defined (_platform_linux32)
	print_linked_page();
#endif
}

