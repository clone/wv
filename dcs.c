#include <stdlib.h>
#include <stdio.h>
#include "wv.h"

void wvGetDCS_internal(DCS *item,FILE *fd, U8 *pointer)
	{
	U16 temp16;
	temp16 = dread_16ubit(fd,&pointer);
	item->fdct = temp16 & 0x0007;
	item->count = (temp16 & 0x00F8) >> 3;
	item->reserved = (temp16 & 0xff00) >> 8;
	}

void wvGetDCS(DCS *item,FILE *fd)
	{
	wvGetDCS_internal(item,fd,NULL);
	}

void wvGetDCSFromBucket(DCS *item,U8 *pointer)
	{
	wvGetDCS_internal(item,NULL,pointer);
	}

void wvCopyDCS(DCS *dest,DCS *src)
	{
	dest->fdct = src->fdct;
	dest->count = src->count;
	dest->reserved = src->reserved;
	}

void wvInitDCS(DCS *item)
	{
	item->fdct = 0;
	item->count = 0;
	item->reserved = 0;
	}
