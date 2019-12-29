#pragma once

/*
 	 throw!
 */
struct throw_if
{
	void operator ( )  ( bool likely,
			const char *str = "runtime error",
			unsigned  backtrace_dep = 10)
	{
		if(likely)
		{

				void *backtrace_addr [backtrace_dep] = { nullptr, };
				int size = 0;
				if(backtrace_dep)
				{
					size = backtrace(backtrace_addr, backtrace_dep);
				}

				if(size > 0)
				{
					printf("terminate called (%s) : look up addr \n", str);

					for(int i = 0; i < size; i++)
					{
						printf("addr2line -f -C -e livemedia++ %08x\n", backtrace_addr[i]);
					}
				}
				else
				{
					printf("terminate called (%s): look up unknown  addr\n", str);
				}
				exit(0);
//				throw std::runtime_error
//				(
//						except_msg
//				);
			}
		}
};
struct throw_register_sys_except
{
	static void sys_except(int nsig)
	{
		throw_if ti;
		ti(1, nsig == SIGSEGV ? "sigsegv" : "sigabrt");
	}
	throw_register_sys_except()
	{
		signal(SIGSEGV, throw_register_sys_except::sys_except);
		signal(SIGABRT, throw_register_sys_except::sys_except);
	}
};

inline void* operator new(std::size_t size)
{
	void *userblock = __base__malloc__(size);

	return  userblock;
}
inline void* operator new[](std::size_t size)
{
	void *userblock = __base__malloc__(size);

	return  userblock;
}
inline void operator delete(void *ptr)
{

	return __base__free__(ptr);
}
inline void operator delete[](void *ptr)
{ return __base__free__(ptr); }
