#include <stdlib.h>
#include <stdio.h>
#include "wv.h"

void wvGetWKB(WKB *item,FILE *fd)
	{
	U16 temp16;
	item->fn = (S16)read_16ubit(fd);
	item->grfwkb = read_16ubit(fd);;
	item->lvl = (S16)read_16ubit(fd);
	temp16 = read_16ubit(fd);
	item->fnpt = temp16 & 0x000F;
	item->fnpd = (temp16 & 0xFFF0) >> 4;
	item->doc = (S32)read_32ubit(fd);
	}
