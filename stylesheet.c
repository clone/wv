#include <stdio.h>
#include <stdlib.h>
#include "wv.h"
#include <string.h>

/*modify this to handle cbSTSHI < the current size*/
void wvGetSTSHI(STSHI *item,U16 cbSTSHI,FILE *fd)
	{
	U16 temp16;
	int i;
	U16 count=0;


	wvInitSTSHI(item);	/* zero any new fields that might not exist in the file*/

    item->cstd = read_16ubit(fd);
	count+=2;
	wvTrace(("there are %d std\n",item->cstd));
    item->cbSTDBaseInFile = read_16ubit(fd);
	count+=2;
	temp16 = read_16ubit(fd);
	count+=2;
    item->fStdStylenamesWritten = temp16 & 0x01;
    item->reserved = (temp16 & 0xfe) >> 1;
    item->stiMaxWhenSaved = read_16ubit(fd);
	count+=2;
    item->istdMaxFixedWhenSaved = read_16ubit(fd);
	count+=2;
    item->nVerBuiltInNamesWhenSaved = read_16ubit(fd);
	count+=2;
	for (i=0;i<3;i++)
		{
    	item->rgftcStandardChpStsh[i] = read_16ubit(fd);
		count+=2;
		if (count >= cbSTSHI) break;
		}

	while(count<cbSTSHI)
		{
		getc(fd);
		count++;
		}
	}

void wvInitSTSHI(STSHI *item)
	{
	int i;

    item->cstd = 0;
    item->cbSTDBaseInFile = 0;
    item->fStdStylenamesWritten = 0;
    item->reserved = 0;
    item->stiMaxWhenSaved = 0;
    item->istdMaxFixedWhenSaved = 0;
    item->nVerBuiltInNamesWhenSaved = 0;
	for (i=0;i<3;i++)
    	item->rgftcStandardChpStsh[i] = 0;
	}

void wvReleaseSTD(STD *item)
	{
	U8 i;
	if (!item)
		return;
	wvFree(item->xstzName);

	for (i=0;i<item->cupx;i++)
		{
		if (item->grupxf[i].cbUPX == 0)
			continue;

		if ((item->cupx == 1) || ((item->cupx == 2) && (i==1)))
			wvFree(item->grupxf[i].upx.chpx.grpprl);
		else if ((item->cupx == 2) && (i==0))
			wvFree(item->grupxf[i].upx.papx.grpprl);
		}

	if (item->sgc == sgcChp)
		if (item->grupe)
			wvReleaseCHPX(&(item->grupe[0].chpx));
	wvFree(item->grupxf);
	wvFree(item->grupe);
	}

void wvInitSTD(STD *item)
	{
	item->sti=0;
	item->fScratch=0;
	item->fInvalHeight=0;
	item->fHasUpe=0;
	item->fMassCopy=0;
	item->sgc=0;
	item->istdBase=istdNil;
	item->cupx=0;
	item->istdNext=0;
	item->bchUpe=0;
	item->fAutoRedef=0;
	item->fHidden=0;
	item->reserved=0;
	item->xstzName=NULL;
	item->grupxf=NULL;
	item->grupe=NULL;
	}

int wvGetSTD(STD *item,U16 baselen,FILE *fd)
	{
	U16 temp16;
	U16 len,i,j;
	int pos;
	int ret=0;
	U16 count=0;

	wvInitSTD(item); /* zero any new fields that might not exist in the file*/

	wvTrace(("baselen set to %d\n",baselen));

	temp16 = read_16ubit(fd);
	count+=2;
	item->sti = temp16 & 0x0fff;
    item->fScratch = (temp16 & 0x1000) >> 12;
    item->fInvalHeight = (temp16 & 0x2000) >> 13;
    item->fHasUpe = (temp16 & 0x4000) >> 14;
    item->fMassCopy = (temp16 & 0x8000) >> 15;
	temp16 = read_16ubit(fd);
	count+=2;
    item->sgc = temp16 & 0x000f;
    item->istdBase = (temp16 & 0xfff0) >> 4;
	temp16 = read_16ubit(fd);
	count+=2;
    item->cupx = temp16 & 0x000f;
    item->istdNext = (temp16 & 0xfff0) >> 4;
    item->bchUpe = read_16ubit(fd);
	count+=2;
	if (count < baselen)		/* word 6 has only a count of 8 */
		{
		temp16 = read_16ubit(fd);
		count+=2;
		item->fAutoRedef = temp16 & 0x0001;
		item->fHidden = (temp16 & 0x0002) >> 1;
		item->reserved = (temp16 & 0xfffc) >> 2;

		while (count<baselen)	/* eat any new fields we might know about ourselves*/
			{
			getc(fd);
			count++;
			}
		}
	wvTrace(("count is %d, baselen is %d\n",count,baselen));


	pos=10;

	if (count < 10)
		{
		ret=1;
		len = getc(fd);
		pos++;
		}
	else
		{
		len = read_16ubit(fd);
		pos+=2;
		}

	wvTrace(("doing a std, str len is %d\n",len+1));
	item->xstzName = (U16 *)malloc((len+1) * sizeof(XCHAR));

	for(i=0;i<len+1;i++)
		{
		if (count < 10)
			{
			item->xstzName[i] = getc(fd);
			pos++;
			}
		else
			{
			item->xstzName[i] = read_16ubit(fd);
			pos+=2;
			}

		wvTrace(("sample letter is %c\n",item->xstzName[i]));
		}
	wvTrace(("string ended\n"));


	wvTrace(("cupx is %d\n",item->cupx));
	if (item->cupx == 0)
		{
		item->grupxf = NULL;
		item->grupe = NULL;
		return(0);
		}

	item->grupxf = (UPXF *)malloc(sizeof(UPXF) * item->cupx);
	if (item->grupxf == NULL)
		{
		wvError(("Couuldn't alloc %d bytes for UPXF\n",sizeof(UPXF) * item->cupx));
		return(0);
		}

	item->grupe = (UPE *)malloc(sizeof(UPE) * item->cupx);
	if (item->grupe == NULL)
		{
		wvError(("Couuldn't alloc %d bytes for UPE\n",sizeof(UPE) * item->cupx));
		return(0);
		}

	for (i=0;i<item->cupx;i++)
		{
		if ((pos+1)/2 != pos/2)
			{
			/*eat odd bytes*/
			fseek(fd,1,SEEK_CUR);
			pos++;
			}
		
		item->grupxf[i].cbUPX = read_16ubit(fd);
		wvTrace(("cbUPX is %d\n",item->grupxf[i].cbUPX));
		pos+=2;

		if (item->grupxf[i].cbUPX == 0)
			continue;

		if ((item->cupx == 1) || ((item->cupx == 2) && (i==1)))
			{
			item->grupxf[i].upx.chpx.grpprl = (U8 *)malloc(item->grupxf[i].cbUPX);
			for(j=0;j<item->grupxf[i].cbUPX;j++)
				{
				item->grupxf[i].upx.chpx.grpprl[j] = getc(fd);
				pos++;
				}
			}
		else if ((item->cupx == 2) && (i==0))
			{
			item->grupxf[i].upx.papx.istd = read_16ubit(fd);
			pos+=2;
			if (item->grupxf[i].cbUPX-2)
				item->grupxf[i].upx.papx.grpprl = (U8 *)malloc(item->grupxf[i].cbUPX-2);
			else
				item->grupxf[i].upx.papx.grpprl = NULL;
			for(j=0;j<item->grupxf[i].cbUPX-2;j++)
				{
				item->grupxf[i].upx.papx.grpprl[j] = getc(fd);
				pos++;
				}
			}
		else 
			{
			wvTrace(("Strange cupx option\n"));
			fseek(fd,item->grupxf[i].cbUPX,SEEK_CUR);
			pos+=item->grupxf[i].cbUPX;
			}
		}



	/*eat odd bytes*/
	if ((pos+1)/2 != pos/2)
		fseek(fd,1,SEEK_CUR);
	return(ret);
	}

void wvReleaseSTSH(STSH *item)
	{
	int i;
	for (i=0;i<item->Stshi.cstd;i++)
		{
		wvTrace(("Releaseing %d std\n",i));
		wvReleaseSTD(&(item->std[i]));
		}
	wvFree(item->std);
	}

void wvGetSTSH(STSH *item,U32 offset,U32 len,FILE *fd)
	{
	U16 cbStshi,cbStd,i,word6,j,k;
	U16 *chains1;
	U16 *chains2;
	if (len == 0)
		{
		item->Stshi.cstd=0;
		item->std=NULL;
		return;
		}
	wvTrace(("stsh offset len is %x %d\n",offset,len));
	fseek(fd,offset,SEEK_SET);
	cbStshi = read_16ubit(fd);
	wvGetSTSHI(&(item->Stshi),cbStshi,fd);
	
	if (item->Stshi.cstd == 0)
		{
		item->std=NULL;
		return;
		}
	chains1 = (U16 *)malloc(sizeof(U16) * item->Stshi.cstd);
	chains2 = (U16 *)malloc(sizeof(U16) * item->Stshi.cstd);

	item->std = (STD *)malloc(sizeof(STD) * item->Stshi.cstd);
	if (item->std == NULL)
		{
		wvError(("No mem for STD list, of size %d\n",sizeof(STD) * item->Stshi.cstd));
		return;
		}

	for(i=0;i<item->Stshi.cstd;i++)
		wvInitSTD(&(item->std[i]));
		
	for(i=0;i<item->Stshi.cstd;i++)
		{
		cbStd = read_16ubit(fd);
		wvTrace(("index is %d,cbStd is %d, should end on %x\n",i,cbStd,ftell(fd)+cbStd));
		if (cbStd != 0)
			{
			word6 = wvGetSTD(&(item->std[i]),item->Stshi.cbSTDBaseInFile,fd);
			wvTrace(("istdBase is %d, type is %d, 6|8 version is %d\n",item->std[i].istdBase,item->std[i].sgc,word6));
			}
		wvTrace(("actually ended on %x\n",ftell(fd)));
		chains1[i] = item->std[i].istdBase;
		}

	
	/* 
	we will do number 10, (standard character style) first if possible, 
	some evil word docs attempt illegally to use sprmCBold etc with a 
	128 and 129 argument, which is supposedly not allowed, but happens
	anyway. In all examples so far it has been character style no 10 that
	they have attempted to access
	*/
	if (item->std[10].istdBase == istdNil)
		{
		wvTrace(("Generating istd no %d\n",i));
		wvGenerateStyle(item,10,word6);
		}
	for(i=0;i<item->Stshi.cstd;i++)
		{
		if ( (item->std[i].istdBase == istdNil) && (i!=10) )
			{
			wvTrace(("Generating istd no %d\n",i));
			wvGenerateStyle(item,i,word6);
			}
		wvTrace(("1: No %d,Base is %d\n",i,chains1[i]));
		}

	j=0;
	while(j<11)
		{
		int finished=1;
		for(i=0;i<item->Stshi.cstd;i++)
			{
			if ( (chains1[i] != istdNil) && (chains1[chains1[i]] == istdNil))
				{
				chains2[i] = istdNil;
				wvTrace(("Generating istd no %d\n",i));
				wvGenerateStyle(item,i,word6);
				finished=0;
				}
			else
				chains2[i] = chains1[i];
			wvTrace(("%d: No %d, Base is %d\n",j,i,chains2[i]));
			}
		for(i=0;i<item->Stshi.cstd;i++)
			chains1[i] = chains2[i];
		if (finished) break;
		j++;
		}

#if 0	
	for(i=0;i<item->Stshi.cstd;i++)
		{
		if (item->std[i].cupx == 0)
			{
			wvError(("Empty Slot %d\n",i));
			continue;
			}
		if ((item->std[i].istdBase >= i) && (item->std[i].istdBase != istdNil))
			{
			wvError(("ISTD's out of sequence, current no is %d, but base is %d\n",i,item->std[i].istdBase));
			switch (item->std[i].sgc)
				{
				case sgcPara:
					wvInitPAPFromIstd(&(item->std[i].grupe[0].apap),istdNil,item);
					wvInitCHPFromIstd(&(item->std[i].grupe[1].achp),istdNil,item);
					if (item->std[i].grupe[1].achp.istd != istdNormalChar)
						{
						wvWarning("chp should have had istd set to istdNormalChar, doing it manually\n");
						item->std[i].grupe[1].achp.istd = istdNormalChar;
						}
					break;
				case sgcChp:
					wvInitCHPXFromIstd(&(item->std[i].grupe[0].chpx),istdNil,item);
					break;
				}

			continue;
			}
		wvGenerateStyle(item,i,word6);
#endif

#if 0		
		switch (item->std[i].sgc)
			{
			case sgcPara:
				wvTrace(("doing paragraph, len is %d\n",item->std[i].grupxf[0].cbUPX));
				wvTrace(("doing paragraph, len is %d\n",item->std[i].grupxf[1].cbUPX));
				wvInitPAPFromIstd(&(item->std[i].grupe[0].apap),(U16)item->std[i].istdBase,item);
				if (word6)
					wvAddPAPXFromBucket6(&(item->std[i].grupe[0].apap),&(item->std[i].grupxf[0]),item);
				else
					wvAddPAPXFromBucket(&(item->std[i].grupe[0].apap),&(item->std[i].grupxf[0]),item,NULL);
					/* data is NULL because HugePAPX cannot occur in this circumstance, according to the
					docs */

				wvInitCHPFromIstd(&(item->std[i].grupe[1].achp),(U16)item->std[i].istdBase,item);

				wvTrace(("here1\n"));
				
				if (word6)
					wvAddCHPXFromBucket6(&(item->std[i].grupe[1].achp),&(item->std[i].grupxf[1]),item);
				else
					wvAddCHPXFromBucket(&(item->std[i].grupe[1].achp),&(item->std[i].grupxf[1]),item);

				wvTrace(("here2\n"));

				if (item->std[i].grupe[1].achp.istd != istdNormalChar)
					{
					wvWarning("chp should have had istd set to istdNormalChar, doing it manually\n");
					item->std[i].grupe[1].achp.istd = istdNormalChar;
					}

				break;
			case sgcChp:
				wvInitCHPXFromIstd(&(item->std[i].grupe[0].chpx),(U16)item->std[i].istdBase,item);

				if (word6)
					wvUpdateCHPXBucket(&(item->std[i].grupxf[0]));
					
				wvMergeCHPXFromBucket(&(item->std[i].grupe[0].chpx),&(item->std[i].grupxf[0]));
				/* UPE.chpx.istd is set to the style's istd */
				item->std[i].grupe[0].chpx.istd = i;	 /*?*/
				break;
			default:
				wvWarning("New document type\n");
				break;
			}
		}
#endif

	wvFree(chains1);
	wvFree(chains2);
	}


void wvGenerateStyle(STSH *item,U16 i,U16 word6)
	{
	if (item->std[i].cupx == 0)
		{
		wvTrace(("Empty Slot %d\n",i));
		return;
		}

	switch (item->std[i].sgc)
		{
		case sgcPara:
			wvTrace(("doing paragraph, len is %d\n",item->std[i].grupxf[0].cbUPX));
			wvTrace(("doing paragraph, len is %d\n",item->std[i].grupxf[1].cbUPX));
			wvInitPAPFromIstd(&(item->std[i].grupe[0].apap),(U16)item->std[i].istdBase,item);
			if (word6)
				wvAddPAPXFromBucket6(&(item->std[i].grupe[0].apap),&(item->std[i].grupxf[0]),item);
			else
				wvAddPAPXFromBucket(&(item->std[i].grupe[0].apap),&(item->std[i].grupxf[0]),item,NULL);
				/* data is NULL because HugePAPX cannot occur in this circumstance, according to the
				docs */

			wvInitCHPFromIstd(&(item->std[i].grupe[1].achp),(U16)item->std[i].istdBase,item);

			wvTrace(("here1\n"));
			
			if (word6)
				wvAddCHPXFromBucket6(&(item->std[i].grupe[1].achp),&(item->std[i].grupxf[1]),item);
			else
				wvAddCHPXFromBucket(&(item->std[i].grupe[1].achp),&(item->std[i].grupxf[1]),item);

			wvTrace(("here2\n"));

			if (item->std[i].grupe[1].achp.istd != istdNormalChar)
				{
				wvWarning("chp should have had istd set to istdNormalChar, doing it manually\n");
				item->std[i].grupe[1].achp.istd = istdNormalChar;
				}

			break;
		case sgcChp:
			wvInitCHPXFromIstd(&(item->std[i].grupe[0].chpx),(U16)item->std[i].istdBase,item);

			if (word6)
				wvUpdateCHPXBucket(&(item->std[i].grupxf[0]));
				
			wvMergeCHPXFromBucket(&(item->std[i].grupe[0].chpx),&(item->std[i].grupxf[0]));
			/* UPE.chpx.istd is set to the style's istd */
			item->std[i].grupe[0].chpx.istd = i;	 /*?*/
			break;
		default:
			wvWarning("New document type\n");
			break;
		}
	}
