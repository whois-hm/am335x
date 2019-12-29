#include "_WQ.h"
static int conv_hex_to_string(unsigned char *pSource, int nSource_perByte, char *pDst, int nDst)
{
	unsigned char  nHighNibleTo_StringByte = 0x00;
	unsigned char  nLowNibleTo_StringByte  = 0x00;
	unsigned short tempBridge    = 0;
	unsigned char *pCastDst = (unsigned char *)pDst;

	if((!pSource) 				||
	   (nSource_perByte <= 0) 	||
	   (!pDst) 					||
	   (nDst < (nSource_perByte * 2) + 1)
	   ) return WQEINVAL;

	while(nSource_perByte-- > 0)
	{
		nHighNibleTo_StringByte = (((*(pSource + nSource_perByte)) & 0xF0) >> 4);
		nLowNibleTo_StringByte  = ((*(pSource + nSource_perByte))  & 0x0F);


		tempBridge = (nHighNibleTo_StringByte >= 0x00 && nHighNibleTo_StringByte <= 0x09) ? (nHighNibleTo_StringByte | 0x30)       : (nHighNibleTo_StringByte + 0x57);
		tempBridge |= (nLowNibleTo_StringByte >= 0x00 && nLowNibleTo_StringByte <= 0x09)  ? ((nLowNibleTo_StringByte | 0x30) << 8) : ((nLowNibleTo_StringByte + 0x57) << 8);


		*(pCastDst++) = (unsigned char )(tempBridge & 0x00FF);
		*(pCastDst++) = (unsigned char )((tempBridge >> 8) & 0x00FF);
	}
	return WQOK;
}

WQ_API void Printf_memory(void *mem, _dword size)
{
#define CONV_PRINT_ASCII(v)	(	((v >= 0x20) && (v <= 0x7e))	)
#define PVH(c) 				(	*(pBuffer + (nBf_Cs + c))		)
#define PVC(c)  			(CONV_PRINT_ASCII(*(pBuffer + (nBf_Cs + c))) ? PVH(c) : 0x2e)

	_dword nBf_Cs = 0;
	int nTot_Line = 0;
	int nTot_Rest = 0;
	unsigned char *pBuffer 			= (unsigned char *)mem;
	char RestBuffer[77] 	= {0, };
	int i = 0;
	_dword nStartAddress 	= 0;

	if(!pBuffer || size <= 0)
	{
		return;
	}

	nTot_Line = (size	>>	4);
	nTot_Rest = (size 	&	0x0000000F);

	printf("===========================================================================\n");
	printf("offset h  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Size [%uByte]\n", size);
	printf( "---------------------------------------------------------------------------\n");

	while(nTot_Line-- > 0)
	{
		printf("%08x  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
				(pBuffer + nBf_Cs),   	PVH(0),		 PVH(1), 	PVH(2), 	PVH(3), 	PVH(4), 	PVH(5), 	PVH(6), 	PVH(7),
										PVH(8), 	 PVH(9), 	PVH(10), 	PVH(11), 	PVH(12), 	PVH(13), 	PVH(14), 	PVH(15),

										PVC(0),		 PVC(1),	PVC(2),		PVC(3),		PVC(4),		PVC(5),		PVC(6),		PVC(7),
										PVC(8),		 PVC(9),	PVC(10),	PVC(11),	PVC(12),	PVC(13),	PVC(14),	PVC(15));
		nBf_Cs += 16;
	}

	if(nTot_Rest > 0 && nTot_Rest <= 16)
	{
		memset((void *)(RestBuffer), 0x20, (DIM(RestBuffer) - 2));
		nStartAddress = (_dword)(_dword *)pBuffer + nBf_Cs;


		conv_hex_to_string((unsigned char *)&nStartAddress, sizeof(nStartAddress), RestBuffer, ((sizeof(nStartAddress) << 1) + 1));

		for(; i < nTot_Rest; i++)
		{
			conv_hex_to_string((pBuffer + nBf_Cs), sizeof(unsigned char), (RestBuffer + (10 + ((i << 1) + i))), ((sizeof(unsigned char) << 1) + 1));
			*(RestBuffer + (59 + i)) = PVC(0);
			nBf_Cs++;
		}
		RestBuffer[59+nTot_Rest] = 0x0A;
		RestBuffer[59+nTot_Rest + 1] = 0x00;
		printf(RestBuffer);

	}

	printf("---------------------------------------------------------------------------\n");
}

WQ_API _dword Time_tickcount()
{
	return Platform_tick_count();
}
WQ_API void Time_sleep(_dword time)
{
	Platform_sleep(time);
}

WQ_API struct Dictionary *Dictionary_create()
{
	struct Dictionary *dict = NULL;
	dict = (struct Dictionary *)libwq_malloc(sizeof(struct Dictionary));
	if(!dict)
	{
		return NULL;
	}
	dict->kvs = NULL;

	return dict;
}
WQ_API void Dictionary_delete(struct Dictionary **pd)
{
	struct Dictionary *dict = NULL;
	if(!pd ||
			!(!pd))
	{
		return;
	}
	dict = *pd;
	*pd = NULL;

	struct Keyvalue *cur = dict->kvs;
	struct Keyvalue *next = NULL;
	while(cur)
	{
		next = cur->next;
		if(cur->k) libwq_free(cur->k);
		if(cur->vc) libwq_free(cur->vc);
		libwq_free(cur);
		cur = next;
	}
	libwq_free(dict);
}
WQ_API _dword Dictionary_add_int(struct Dictionary *d, char *k, int v)
{
	struct Keyvalue *kv = NULL;
	struct Keyvalue *cur = NULL;
	int dup = 0;
	_dword ret = WQOK;
	if(!d ||
			!k)
	{
		return WQEINVAL;
	}

	do
	{
		kv = (struct Keyvalue *)libwq_malloc(sizeof(struct Keyvalue));
		if(!kv)
		{
			ret = WQENOMEM;
			break;
		}

		memset(kv, 0, sizeof(struct Keyvalue));
		if(k)
		{
			kv->k = strdup(k);
			if(!kv->k)
			{
				ret = WQENOMEM;
				break;
			}
		}
		kv->vi = v;

		cur = d->kvs;
		while(cur)
		{
			if(!strcmp(cur->k, k))
			{
				dup = 1;
				break;
			}
			cur = cur->next;
		}
		if(dup)
		{
			ret = WQEALREADY;
			break;
		}

		cur = d->kvs;
		if(!cur)
		{
			d->kvs = kv;
		}
		else
		{
			while(cur->next)
			{
				cur = cur->next;
			}
			cur->next = kv;
		}
		return ret;
	}while(0);

	if(kv)
	{
		if(kv->k)libwq_free(kv->k);
		if(kv->vc)libwq_free(kv->vc);
		libwq_free(kv);
		kv = NULL;
	}
	return ret;
}
WQ_API _dword Dictionary_add_char(struct Dictionary *d, char *k, char *v)
{
	struct Keyvalue *kv = NULL;
	struct Keyvalue *cur = NULL;
	int dup = 0;
	_dword ret = WQOK;

	if(!d ||
			!k)
	{
		return WQEINVAL;
	}

	do
	{
		kv = (struct Keyvalue *)libwq_malloc(sizeof(struct Keyvalue));
		if(!kv)
		{
			ret = WQENOMEM;
			break;
		}
		memset(kv, 0, sizeof(struct Keyvalue));

		if(k)
		{
			kv->k = strdup(k);
			if(!kv->k)
			{
				ret = WQENOMEM;
				break;
			}
		}
		if(v)
		{
			kv->vc = strdup(v);
			if(!kv->vc)
			{
				ret = WQENOMEM;
				break;
			}
		}

		cur = d->kvs;
		while(cur)
		{
			if(!strcmp(cur->k, k))
			{
				dup = 1;
				break;
			}
			cur = cur->next;
		}
		if(dup)
		{
			ret = WQEALREADY;
			break;
		}
		cur = d->kvs;
		if(!cur)
		{
			d->kvs = kv;
		}
		else
		{
			while(cur->next)
			{
				cur = cur->next;
			}
			cur->next = kv;
		}
		return ret;
	}while(0);

	if(kv)
	{
		if(kv->k)libwq_free(kv->k);
		if(kv->vc)libwq_free(kv->vc);
		libwq_free(kv);
		kv = NULL;
	}
	return ret;
}
WQ_API void Dictionary_remove(struct Dictionary *d, char *k)
{
	struct Keyvalue *cur = NULL;
	struct Keyvalue *prev = NULL;
	if(!d ||
			!k)
	{
		return;
	}

	cur = d->kvs;

	while(cur)
	{
		if(!strcmp(cur->k, k))
		{
			if(!prev) d->kvs = cur->next;
			else prev->next = cur->next;

			if(cur->k)libwq_free (cur->k);
			if(cur-> vc)libwq_free (cur->vc);
			libwq_free(cur);
		}
		prev = cur;
		cur = cur->next;
	}
}
WQ_API char *Dictionary_refchar(struct Dictionary *d, char *k)
{
	struct Keyvalue *cur = NULL;
	char *ref = NULL;
	if(!d ||
			!k)
	{
		return NULL;
	}

	cur = d->kvs;

	while(cur)
	{
		if(!strcmp(cur->k, k))
		{
			ref = cur->vc;
			break;
		}
		cur = cur->next;
	}
	return ref;

}
WQ_API char *Dictionary_allocchar(struct Dictionary *d, char *k)
{
	struct Keyvalue *cur = NULL;
	char *alloc = NULL;
	if(!d ||
			!k)
	{
		return NULL;
	}
	cur = d->kvs;

	while(cur)
	{
		if(!strcmp(cur->k, k))
		{
			if(cur->vc)
			{
				alloc = strdup(cur->vc);
			}
			break;
		}
		cur = cur->next;
	}
	return alloc;
}
WQ_API void Dictionary_freechar(char **allocchar)
{
	if(allocchar ||
			*allocchar)
	{
		char *a = *allocchar;
		libwq_free(a);
		*allocchar = NULL;
	}
}
WQ_API int Dictionary_copyint(struct Dictionary *d, char *k)
{
	struct Keyvalue *cur = NULL;
	int copy = 0;
	if(!d ||
			!k)
	{
		return 0;
	}

	cur = d->kvs;

	while(cur)
	{
		if(!strcmp(cur->k, k))
		{
			copy = cur->vi;
			break;
		}
		cur = cur->next;
	}
	return copy;
}
WQ_API int Dictionary_haskey(struct Dictionary *d, char *k)
{
	struct Keyvalue *cur = NULL;
	int ret = -1;
	if(!d ||
			!k)
	{
		return ret;
	}

	cur = d->kvs;

	while(cur)
	{
		if(!strcmp(cur->k, k))
		{
			ret = 1;
			break;
		}
		cur = cur->next;
	}
	return ret;
}
