#include <stdlib.h>
#include <stdio.h>
#include "wv.h"

/*
To find the beginning of the paragraph containing a character in a complex
document, it's first necessary to 

1) search for the piece containing the character in the piece table. 

2) Then calculate the FC in the file that stores the character from the piece 
	table information. 
	
3) Using the FC, search the FCs FKP for the largest FC less than the character's 
	FC, call it fcTest. 
	
4) If the character at fcTest-1 is contained in the current piece, then the 
	character corresponding to that FC in the piece is the first character of 
	the paragraph. 
	
5) If that FC is before or marks the beginning of the piece, scan a piece at a 
time towards the beginning of the piece table until a piece is found that 
contains a paragraph mark. 

(This can be done by using the end of the piece FC, finding the largest FC in 
its FKP that is less than or equal to the end of piece FC, and checking to see 
if the character in front of the FKP FC (which must mark a paragraph end) is 
within the piece.)

6) When such an FKP FC is found, the FC marks the first byte of paragraph text.
*/

/*
To find the end of a paragraph for a character in a complex format file,
again 

1) it is necessary to know the piece that contains the character and the
FC assigned to the character. 

2) Using the FC of the character, first search the FKP that describes the 
character to find the smallest FC in the rgfc that is larger than the character 
FC. 

3) If the FC found in the FKP is less than or equal to the limit FC of the 
piece, the end of the paragraph that contains the character is at the FKP FC 
minus 1. 

4) If the FKP FC that was found was greater than the FC of the end of the 
piece, scan piece by piece toward the end of the document until a piece is 
found that contains a paragraph end mark. 

5) It's possible to check if a piece contains a paragraph mark by using the 
FC of the beginning of the piece to search in the FKPs for the smallest FC in 
the FKP rgfc that is greater than the FC of the beginning of the piece. 

If the FC found is less than or equal to the limit FC of the
piece, then the character that ends the paragraph is the character
immediately before the FKP FC.
*/
int wvGetComplexParaBounds(int version,PAPX_FKP *fkp,U32 *fcFirst, U32 *fcLim, U32 currentcp,CLX *clx, BTE *bte, U32 *pos,int nobte,U32 piece,FILE *fd)
	{
	U32 currentfc;
	BTE entry;
	long currentpos;
	currentfc = wvConvertCPToFC(currentcp,clx);

	wvTrace("current fc is %x\n",currentfc);

	if (currentfc==0xffffffffL)
		{
		wvTrace("Para Bounds not found !, this is ok if this is the last para, otherwise its a disaster\n");
		return(-1);
		}

	if (0 != wvGetBTE_FromFC(&entry,currentfc, bte,pos,nobte))
		{
		wvError("BTE not found !\n");
		return(-1);
		}
	currentpos = ftell(fd);
	/*The pagenumber of the FKP is entry.pn */

	wvGetPAPX_FKP(version,fkp,entry.pn,fd);

	wvGetComplexParafcFirst(version,fcFirst,currentfc,clx, bte, pos,nobte,piece,fkp,fd);
	piece = wvGetComplexParafcLim(version,fcLim,currentfc,clx, bte, pos,nobte,piece,fkp,fd);

	fseek(fd,currentpos,SEEK_SET);
	return(piece);
	}

int wvGetComplexParafcLim(int version,U32 *fcLim,U32 currentfc,CLX *clx, BTE *bte, U32 *pos,int nobte,U32 piece,PAPX_FKP *fkp,FILE *fd)
	{
	U32 fcTest,beginfc;
	BTE entry;
	*fcLim=0xffffffffL;
	fcTest = wvSearchNextSmallestFCPAPX_FKP(fkp,currentfc);

	wvTrace("fcTest is %x\n",fcTest);

	if (fcTest <= wvGetEndFCPiece(piece,clx))
		{
		*fcLim = fcTest/*-1*/;
		}
	else
		{
		wvTrace("piece is %d\n",piece);
		/*get end fc of previous piece*/
		while (piece < clx->nopcd) 
			{
			beginfc = wvNormFC(clx->pcd[piece].fc,NULL);
			if (0 != wvGetBTE_FromFC(&entry,currentfc, bte,pos,nobte))
				{
				wvError("BTE not found !\n");
				return(-1);
				}
			wvReleasePAPX_FKP(fkp);
			wvGetPAPX_FKP(version,fkp,entry.pn,fd);
			fcTest = wvSearchNextSmallestFCPAPX_FKP(fkp,beginfc);
			if (fcTest <= wvGetEndFCPiece(piece,clx))
				{
				*fcLim = fcTest/*-1*/;
				break;
				}
			piece++;
			}
		}
	wvTrace("fcLim is %x\n",*fcLim);
	if (piece == clx->nopcd)
		return(clx->nopcd-1);	/* test using this */
	return(piece);
	}


int wvGetComplexParafcFirst(int version,U32 *fcFirst,U32 currentfc,CLX *clx, BTE *bte, U32 *pos,int nobte,U32 piece,PAPX_FKP *fkp,FILE *fd)
	{
	U32 fcTest,endfc;
	BTE entry;
	fcTest = wvSearchNextLargestFCPAPX_FKP(fkp,currentfc);

	wvTrace("fcTest (s) is %x\n",fcTest);


	if (wvQuerySamePiece(fcTest-1,clx,piece))
		{
		wvTrace("same piece\n");
		*fcFirst = fcTest-1;
		}
	else
		{
		wvTrace("piece is %d\n",piece);
		/*
		get end fc of previous piece ??, or use the end of the current piece
		piece--;
		*/
		while (piece != 0xffffffffL) 
			{
			/*
			endfc = wvNormFC(clx->pcd[piece+1].fc,NULL);
			*/
			endfc = wvGetEndFCPiece(piece,clx);
			if (0 != wvGetBTE_FromFC(&entry,endfc, bte,pos,nobte))
				{
				wvError("BTE not found !\n");
				return(-1);
				}
			wvReleasePAPX_FKP(fkp);
			wvGetPAPX_FKP(version,fkp,entry.pn,fd);
			fcTest = wvSearchNextLargestFCPAPX_FKP(fkp,endfc);
			if (wvQuerySamePiece(fcTest-1,clx,piece))
				{
				*fcFirst = fcTest-1;
				break;
				}
			piece--;
			}
		
		}
	return(0);
	}


/* char properties version of the above -JB */
/* only difference is that we're using CHPX FKP pages,
 * and specifically just the Get and Release functions are
 * different between the two. We might be able to 
 * abstract the necessary functions to avoid duplicating them... */

int wvGetComplexCharBounds(int version, CHPX_FKP *fkp, U32 *fcFirst, 
			   U32 *fcLim, U32 currentcp,CLX *clx, BTE *bte, 
			   U32 *pos, int nobte, U32 piece, FILE *fd)
	{
	U32 currentfc;
	BTE entry;
	long currentpos;
	currentfc = wvConvertCPToFC(currentcp, clx);

	wvTrace("current fc is %x\n", currentfc);

	if (currentfc==0xffffffffL)
		{
		wvTrace("Char Bounds not found !, this is ok if this is the last char, otherwise its a disaster\n");
		return(-1);
		}

	if (0 != wvGetBTE_FromFC(&entry, currentfc, bte, pos, nobte))
		{
		wvError("BTE not found !\n");
		return(-1);
		}
	currentpos = ftell(fd);
	/*The pagenumber of the FKP is entry.pn */

	wvGetCHPX_FKP(version, fkp, entry.pn, fd);

	wvGetComplexCharfcFirst(version, fcFirst, currentfc, clx, bte, pos,
				nobte, piece, fkp, fd);
	piece = wvGetComplexCharfcLim(version, fcLim, currentfc, clx, bte, 
				      pos, nobte, piece, fkp, fd);

	fseek(fd,currentpos,SEEK_SET);
	return(piece);
	}

int wvGetComplexCharfcLim(int version, U32 *fcLim, U32 currentfc, CLX *clx, 
			  BTE *bte, U32 *pos, int nobte, U32 piece, 
			  CHPX_FKP *fkp, FILE *fd)
	{
	U32 fcTest,beginfc;
	BTE entry;
	*fcLim=0xffffffffL;
	/* this only works with the initial rgfc array, which is the
	 * same for both CHPX and PAPX FKPs */
	fcTest = wvSearchNextSmallestFCPAPX_FKP((PAPX_FKP*)fkp, currentfc);

	wvTrace("fcTest is %x\n",fcTest);

	if (fcTest <= wvGetEndFCPiece(piece,clx))
		{
		*fcLim = fcTest/*-1*/;
		}
	else
		{
		wvTrace("piece is %d\n",piece);
		/*get end fc of previous piece*/
		while (piece < clx->nopcd) 
			{
			beginfc = wvNormFC(clx->pcd[piece].fc,NULL);
			if (0 != wvGetBTE_FromFC(&entry,currentfc, bte,pos,nobte))
				{
				wvError("BTE not found !\n");
				return(-1);
				}
			wvReleaseCHPX_FKP(fkp);
			wvGetCHPX_FKP(version,fkp,entry.pn,fd);
			/* this only works with the initial rgfc array, which is the
			 * same for both CHPX and PAPX FKPs */
			fcTest = wvSearchNextSmallestFCPAPX_FKP((PAPX_FKP*)fkp,beginfc);
			if (fcTest <= wvGetEndFCPiece(piece,clx))
				{
				*fcLim = fcTest/*-1*/;
				break;
				}
			piece++;
			}
		}
	wvTrace("fcLim is %x\n",*fcLim);
	if (piece == clx->nopcd)
		return(clx->nopcd-1);	/* test using this */
	return(piece);
	}


int wvGetComplexCharfcFirst(int version,U32 *fcFirst,U32 currentfc,CLX *clx, 
			    BTE *bte, U32 *pos,int nobte,U32 piece,CHPX_FKP *fkp, FILE *fd)
	{
	U32 fcTest,endfc;
	BTE entry;
	/* this only works with the initial rgfc array, which is the
	 * same for both CHPX and PAPX FKPs */
	fcTest = wvSearchNextLargestFCPAPX_FKP((PAPX_FKP*)fkp,currentfc);

	wvTrace("fcTest (s) is %x\n",fcTest);


	if (wvQuerySamePiece(fcTest-1,clx,piece))
		{
		wvTrace("same piece\n");
		*fcFirst = fcTest-1;
		}
	else
		{
		wvTrace("piece is %d\n",piece);
		/*
		get end fc of previous piece ??, or use the end of the current piece
		piece--;
		*/
		while (piece != 0xffffffffL) 
			{
			/*
			endfc = wvNormFC(clx->pcd[piece+1].fc,NULL);
			*/
			endfc = wvGetEndFCPiece(piece,clx);
			if (0 != wvGetBTE_FromFC(&entry,endfc, bte,pos,nobte))
				{
				wvError("BTE not found !\n");
				return(-1);
				}
			wvReleaseCHPX_FKP(fkp);
			wvGetCHPX_FKP(version,fkp,entry.pn,fd);
			/* this only works with the initial rgfc array, which is the
			 * same for both CHPX and PAPX FKPs */
			fcTest = wvSearchNextLargestFCPAPX_FKP((PAPX_FKP*)fkp,endfc);
			if (wvQuerySamePiece(fcTest-1,clx,piece))
				{
				*fcFirst = fcTest-1;
				break;
				}
			piece--;
			}
		
		}
	return(0);
	}

/*
how this works,
we seek to the beginning of the text, we loop for a count of charaters that is stored in the fib.

the piecetable divides the text up into various sections, we keep track of our location vs
the next entry in that table, when we reach that location, we seek to the position that
the table tells us to go.

there are special cases for coming to the end of a section, and for the beginning and ends of
pages. for the purposes of headers and footers etc.
*/
void wvDecodeComplex(wvParseStruct *ps)
	{
	U32 piececount=0,i,j,temppiece;
	U32 beginfc,endfc,discard;
	U32 begincp,endcp;
	int chartype;
	U16 eachchar;
	U32 para_fcFirst=0,para_fcLim=0xffffffffL,para_tfcLim=0xffffffffL;
	U32 char_fcFirst=0,char_fcLim=0xffffffffL,char_tfcLim=0xffffffffL;
	BTE *btePapx=NULL, *bteChpx=NULL;
	U32 *posPapx=NULL, *posChpx=NULL;
	U32 para_intervals, char_intervals;
	U16 charset;
	U8 state=0;
	int cpiece=0;
	STSH stsh;
	PAPX_FKP para_fkp;
	PAP apap;
	CHPX_FKP char_fkp;
	CHP achp;
	int para_pendingclose=0, char_pendingclose=0;

	/*we will need the stylesheet to do anything useful with layout and look*/
	wvGetSTSH(&stsh,ps->fib.fcStshf,ps->fib.lcbStshf,ps->tablefd);

	/*we will need the table of names to answer questions like the name of the doc*/
	if ( (wvQuerySupported(&ps->fib,NULL) == 2) || (wvQuerySupported(&ps->fib,NULL) == 3) )
		wvGetSTTBF6(&ps->anSttbfAssoc,ps->fib.fcSttbfAssoc,ps->fib.lcbSttbfAssoc,ps->tablefd);
	else
		wvGetSTTBF(&ps->anSttbfAssoc,ps->fib.fcSttbfAssoc,ps->fib.lcbSttbfAssoc,ps->tablefd);

	wvGetCLX(wvQuerySupported(&ps->fib,NULL),&ps->clx,ps->fib.fcClx,ps->fib.lcbClx,ps->tablefd);

	if ((ps->fib.ccpFtn) || (ps->fib.ccpHdd))
		wvTrace("Special ending\n");
	/*
	we will need the paragraph and character bounds table to make decisions as 
	to where a table begins and ends
	*/
	if ( (wvQuerySupported(&ps->fib,NULL) == 2) || (wvQuerySupported(&ps->fib,NULL) == 3) )
	    {
	       wvGetBTE_PLCF6(&btePapx,&posPapx,&para_intervals,ps->fib.fcPlcfbtePapx,ps->fib.lcbPlcfbtePapx,ps->tablefd);
	       wvListBTE_PLCF(&btePapx,&posPapx,&para_intervals);
	       wvGetBTE_PLCF6(&bteChpx,&posChpx,&char_intervals,ps->fib.fcPlcfbteChpx, ps->fib.lcbPlcfbteChpx,ps->tablefd);
	       wvListBTE_PLCF(&bteChpx,&posChpx,&char_intervals);
	    }
	else 
	     {
		wvGetBTE_PLCF(&btePapx,&posPapx,&para_intervals,ps->fib.fcPlcfbtePapx,ps->fib.lcbPlcfbtePapx,ps->tablefd);
		wvGetBTE_PLCF(&bteChpx,&posChpx,&char_intervals,ps->fib.fcPlcfbteChpx,ps->fib.lcbPlcfbteChpx,ps->tablefd);
	     }
	charset = wvAutoCharset(&ps->clx);

	wvInitPAPX_FKP(&para_fkp);
	wvInitCHPX_FKP(&char_fkp);

	wvHandleDocument(ps,DOCBEGIN);


	
	/*for each piece*/
	for (piececount=0;piececount<ps->clx.nopcd;piececount++)
		{
		chartype = wvGetPieceBoundsFC(&beginfc,&endfc,&ps->clx,piececount);
		wvGetPieceBoundsCP(&begincp,&endcp,&ps->clx,piececount);
		wvTrace("begin end %x %x %x %x\n",beginfc,endfc,begincp,endcp);
		para_fcLim = char_fcLim = 0xffffffffL;
		fseek(ps->mainfd,beginfc,SEEK_SET);
		for (i=begincp,j=beginfc;i<endcp;i++,j += wvIncFC(chartype))
			{

			/* paragraph properties */
			if (j == para_fcLim)
				{
				wvHandleElement(ps,PARAEND, (void*)&apap);
				para_pendingclose=0;
				}

			if ((para_fcLim == 0xffffffffL) || (para_fcLim == j))
				{
				wvTrace("before tests j is %x\n",j);
				wvReleasePAPX_FKP(&para_fkp);
				cpiece = wvGetComplexParaBounds(wvQuerySupported(&ps->fib,NULL),&para_fkp,&para_fcFirst,&para_fcLim,i,&ps->clx, btePapx, posPapx, para_intervals,piececount,ps->mainfd);
				wvTrace("fcLim is %x, fcFirst is %x\n",para_fcLim,para_fcFirst);
				/*
				para_fcLim = 0xfffffffeL;
				*/
				}
	

			if (j == para_fcFirst)
				{
				wvAssembleSimplePAP(&apap,para_fcLim,&para_fkp,&stsh);
				wvTrace("cpiece is %d, but full no is %d\n",cpiece,ps->clx.nopcd);
				wvAssembleComplexPAP(wvQuerySupported(&ps->fib,NULL),&apap,cpiece,&stsh,&ps->clx);
				wvHandleElement(ps,PARABEGIN, (void*)&apap);
				para_pendingclose=1;
				}

			/* character properties */
			if (j == char_fcLim)
				{
				wvHandleElement(ps,CHARPROPEND, (void*)&achp);
				char_pendingclose=0;
				}

			if ((char_fcLim == 0xffffffffL) || (char_fcLim == j))
				{
				wvTrace("before tests j is %x\n",j);
				wvReleaseCHPX_FKP(&char_fkp);
				cpiece = wvGetComplexCharBounds(wvQuerySupported(&ps->fib,NULL),&char_fkp,&char_fcFirst,&char_fcLim,i,&ps->clx, bteChpx, posChpx, char_intervals,piececount,ps->mainfd);
				wvTrace("fcLim is %x, fcFirst is %x\n",char_fcLim,char_fcFirst);
				/*
				char_fcLim = 0xfffffffeL;
				*/
				}
	

			if (j == char_fcFirst)
				{
				wvAssembleSimpleCHP(&achp,char_fcLim,&char_fkp,&stsh);
				wvTrace("cpiece is %d, but full no is %d\n",cpiece,ps->clx.nopcd);
				wvAssembleComplexCHP(wvQuerySupported(&ps->fib,NULL),&achp,cpiece,&stsh,&ps->clx);
				wvHandleElement(ps,CHARPROPBEGIN, (void*)&achp);
				char_pendingclose=1;
				}

			eachchar = wvGetChar(ps->mainfd,chartype);

			/* previously, in place of ps there was a NULL,
			 * but it was crashing Abiword. Was it NULL for a
			 * reason? -JB */
			wvOutputTextChar(eachchar,chartype,charset,&state,ps);
			}

		if (j == para_fcLim)
			{
			wvHandleElement(ps,PARAEND, (void*)&apap);
			para_pendingclose=0;
			}
		if (j == char_fcLim)
			{
			wvHandleElement(ps,CHARPROPEND, (void*)&achp);
			char_pendingclose=0;
			}
		}

	if (char_pendingclose)
		{
		wvHandleElement(ps,CHARPROPEND, (void*)&achp);
		}

	if (para_pendingclose)
		{
		wvHandleElement(ps,PARAEND, (void*)&apap);
		}

	wvReleasePAPX_FKP(&para_fkp);
	wvReleaseCHPX_FKP(&char_fkp);
	   
	wvHandleDocument(ps,DOCEND);
	wvReleaseSTTBF(&ps->anSttbfAssoc);
	wvReleaseCLX(&ps->clx);
	wvReleaseSTSH(&stsh);
	wvFree(btePapx);
	wvFree(posPapx);
	wvFree(bteChpx);
	wvFree(posChpx);
	}

/*
The process thus far has created a PAP that describes
what the paragraph properties of the paragraph were at the last full save.

1) Now it's necessary to apply any paragraph sprms that were linked to the
piece that contains the paragraph's paragraph mark. 

2) If pcd.prm.fComplex is 0, pcd.prm contains 1 sprm which should only be 
applied to the local PAP if it is a paragraph sprm. 

3) If pcd.prm.fComplex is 1, pcd.prm.igrpprl is the index of a grpprl in the 
CLX.  If that grpprl contains any paragraph sprms, they should be applied to 
the local PAP.
*/
void wvAssembleComplexPAP(int version,PAP *apap,U32 cpiece,STSH *stsh,CLX *clx)
	{
	/*
	Sprm Sprm;
	*/
	U16 sprm,pos=0,i=0;
	U8 *pointer;
	U16 index;
	U8 val;

	if (clx->pcd[cpiece].prm.fComplex == 0)
		{
		val = clx->pcd[cpiece].prm.para.var1.val;
		pointer = &val;
		wvApplySprmFromBucket(version,wvGetrgsprmPrm(clx->pcd[cpiece].prm.para.var1.isprm),
		apap,NULL, NULL,stsh,pointer,&pos);
		}
	else
		{
		index = clx->pcd[cpiece].prm.para.var2.igrpprl;
		wvTrace("index is %d, piece is %d\n",index,cpiece);
		while (i < clx->cbGrpprl[index])   
			{
			if (version == 0)
				sprm = bread_16ubit(clx->grpprl[index]+i,&i);
			else
				{
				sprm = bgetc(clx->grpprl[index]+i,&i);
				wvTrace("sprm (word 6) is %x\n",sprm);
				sprm = wvGetrgsprmWord6(sprm);
				}
			wvTrace("sprm is %x\n",sprm);
			pointer = clx->grpprl[index]+i;
			wvApplySprmFromBucket(version,sprm,apap,NULL,NULL,stsh,pointer,&i);
			}
		}
	}

/* CHP version of the above. follows the same rules -JB */
void wvAssembleComplexCHP(int version,CHP *achp,U32 cpiece,STSH *stsh,CLX *clx)
	{
	/*
	Sprm Sprm;
	*/
	U16 sprm,pos=0,i=0;
	U8 *pointer;
	U16 index;
	U8 val;

	if (clx->pcd[cpiece].prm.fComplex == 0)
		{
		val = clx->pcd[cpiece].prm.para.var1.val;
		pointer = &val;
		wvApplySprmFromBucket(version,wvGetrgsprmPrm(clx->pcd[cpiece].prm.para.var1.isprm),
		NULL, achp, NULL,stsh,pointer,&pos);
		}
	else
		{
		index = clx->pcd[cpiece].prm.para.var2.igrpprl;
		wvTrace("index is %d, piece is %d\n",index,cpiece);
		while (i < clx->cbGrpprl[index])   
			{
			if (version == 0)
				sprm = bread_16ubit(clx->grpprl[index]+i,&i);
			else
				{
				sprm = bgetc(clx->grpprl[index]+i,&i);
				wvTrace("sprm (word 6) is %x\n",sprm);
				sprm = wvGetrgsprmWord6(sprm);
				}
			wvTrace("sprm is %x\n",sprm);
			pointer = clx->grpprl[index]+i;
			wvApplySprmFromBucket(version,sprm,NULL,achp,NULL,stsh,pointer,&i);
			}
		}
	}

