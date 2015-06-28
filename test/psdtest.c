/**
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * Copyright (C) 2016, Freeman.Tan (tanyc@126.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpsd.h"

//#pragma comment(lib,"dllpsd.lib")

extern int PngMerge(char *fPngName1,char *fPngName2);

/*
Big-Endian 一个Word中的高位的Byte放在内存中这个Word区域的低地址处。 
Little-Endian 一个Word中的低位的Byte放在内存中这个Word区域的低地址处。 
in VC++6.0, we use little(intel) endian mode. !!! But PSD file is using big endian as treaming order!!!	comment by freeman
*/
static int IsBigEndian(void)
{
	int ret=-1;

	union 
	{
		short Temp16;
		char  Temp8;
	}endian;

	endian.Temp16 = 0x0001;
	
	ret = (endian.Temp8 == 1)?0:1;	// 0:little endian, 1:big endian
	return ret;
}


int main(int argc, char* argv[])
{
	int ret=0,i;
	int fHandle;
	psd_context * context=NULL;
	//unsigned char *BmpBuf=NULL;
	unsigned int lwidth=10,lheight=10;

#ifdef LOG_MSG
	PSD_PRINT("%s() load...\n",__FUNCTION__);
#endif

	//PngAddBmp("layer6.bmp","addlayer5.png");

	//PngMerge("merged.png","canvas.png");
	
	//ret=Psd2Png("..\\..\\pic\\24ok.psd","..\\..\\pic\\24ok.png");
	
		fHandle=PsdLoadFile("..\\..\\pic\\test1.psd");
		//fHandle=Psd2Psd("..\\..\\pic\\tianye.psd","..\\..\\pic\\tianye1.psd");
		//fHandle = PsdNewFile("..\\..\\pic\\tianye.psd","..\\..\\pic\\tianye.bmp");
		//ret = PsdCreateLayer(fHandle,"..\\..\\pic\\ms12.png",0,200,190,200,1,255);
		//ret = PsdCreateLayer(fHandle,"..\\..\\pic\\ms12.png",100,200,190,200,1,50);
		ret=PsdSaveFile (fHandle);
		ret = PsdCloseFile(fHandle);

//	fHandle=Psd2Psd("..\\..\\pic\\24ok4_1.psd","..\\..\\pic\\24ok4_11.psd");
	fHandle=PsdLoadFile("..\\..\\pic\\002.psd");
//	ret=PsdSaveFile (fHandle);
	ret = PsdCloseFile(fHandle);

	fHandle = PsdLoadFile("..\\..\\pic\\24ok4_1.psd");
	if (fHandle>=0)
	{
		ret=PsdSaveFile (fHandle);
		ret = PsdCloseFile(fHandle);
		Psd2Png("..\\..\\pic\\tianye_brighness.psd","..\\..\\pic\\tianye_brighness.png");
	}
	
	
//	Psd2Png("..\\..\\pic\\24ok4_3.psd","..\\..\\pic\\24ok4_3.png");
//	Psd2Bmp("..\\..\\pic\\24ok4_3.psd","..\\..\\pic\\24ok4_3.bmp");
		
	if (!strcmp(argv[1],"brightness"))
	{
		fHandle = PsdNewFile("..\\..\\pic\\tianye_brighness.psd","..\\..\\pic\\tianye.bmp");
		//copy tianye_brighness.psd as backup for comparing
		if (fHandle>=0)
		{
			ret = PsdCreateLayer(fHandle,"..\\..\\pic\\ms12.png",50,50,190,200,1,200);	//create a new layer for comparing
			for (i=0;i<10;i++)		//test 10 times
			{
				PsdSetLayerBrightness(fHandle,1,100);PsdSetLayerBrightness(fHandle,1,-100);	//adjust brightness
				PsdSetLayerBrightness(fHandle,1,50);PsdSetLayerBrightness(fHandle,1,-50);
				PsdSetLayerBrightness(fHandle,1,70);
			}
				
			PsdSetLayerBrightness(fHandle,1,0);	//return to normal(original version)
			ret=PsdPreview(fHandle,"..\\..\\pic\\tianye_preview.bmp");
			PsdSaveFile(fHandle);
			PsdCloseFile(fHandle);		//must close before load again...
		}
	}
	
	else if (!strcmp(argv[1],"contrast"))
	{
		fHandle = PsdNewFile("..\\..\\pic\\tianye_contrast.psd","..\\..\\pic\\tianye.bmp");	
		//copy one

		if (fHandle>=0)
		{
			ret = PsdCreateLayer(fHandle,"..\\..\\pic\\ms12.png",50,50,190,200,1,200);
		
			for (i=0;i<10;i++)
			{
				PsdSetLayerContrast(fHandle,1,100);PsdSetLayerContrast(fHandle,1,-100);
				PsdSetLayerContrast(fHandle,1,50);PsdSetLayerContrast(fHandle,1,-50);
				PsdSetLayerContrast(fHandle,1,-100);
			}
				
			PsdSetLayerContrast(fHandle,1,0);	//return to normal(original version)
		
			PsdSaveFile(fHandle);
			PsdCloseFile(fHandle);		//must close firstly before load again...
		}
	}
	else
		;
	
	return 0;
}

#if 0
int main(int argc, char* argv[])
{
	int ret=0;
	int fHandle;
	psd_context * context=NULL;

	ret = IsBigEndian();
	if (!ret)
		printf("we are using little endian, psd image file stream byte order is big endian! need convert\n");
	else
		printf("we are using big endian\n");
	
	if (argc == 1)
	{
		printfHelp();
		return 0;
	}

	if (!strcmp(argv[1],"psd2bmp"))
	{
		Psd2Bmp("..\\..\\pic\\tianye_all.psd","..\\..\\pic\\tianye_all.bmp");
		Psd2Bmp("..\\..\\pic\\tianye_new.psd","..\\..\\pic\\tianye_new.jpg");
		Psd2Png("..\\..\\pic\\tianye_all.psd","..\\..\\pic\\tianye_all.png");
	}
	else if (!strcmp(argv[1],"merge"))
	{
		MergeTwoPngFile("..\\..\\pic\\bg.png","..\\..\\pic\\tianye.png","..\\..\\pic\\merge.png",180);
	}
	else if (!strcmp(argv[1],"bmp2psd"))
	{
		//CreateDemoPsdFile("..\\..\\pic\\tianye_new.psd","..\\..\\pic\\tianye.png");
		fHandle = PsdNewFile("..\\..\\pic\\tianye_1.psd","..\\..\\pic\\tianye.bmp");
		printf("new one psd file, ret=%d\n",fHandle);
		if (ret>=0)
		{
			ret = PsdNewLayer(fHandle,"..\\..\\pic\\ms12.png",0,0,1,200);
//			ret = PsdNewLayer(fHandle,"..\\..\\pic\\ms12.png",200,200,2,200);
//			PsdSetLayerDirection(fHandle,1,RIGHT90);
//			PsdSetLayerDirection(fHandle,1,LEFT90);
			//PsdSetLayerBrightness(fHandle,1,-0);
			//PsdSetLayerContrast(fHandle,1,-50);
			PsdSaveFile(fHandle);
			PsdCloseFile(fHandle);		//must close firstly before load again...
		}

//		Psd2Bmp("..\\..\\pic\\tianye_new.psd","..\\..\\pic\\tianye_new_tmp.bmp");
		Psd2Png("..\\..\\pic\\tianye_1.psd","..\\..\\pic\\tianye_1_tmp.png");
		//pngzoom("..\\..\\pic\\tianye_new_tmp.png",50);

	}
	else if (!strcmp(argv[1],"psd2psd"))
	{
		//fHandle = PsdLoadFile("..\\..\\pic\\tianye_1.psd");	//tianye_1.psd has two layers already
		fHandle = PsdNewFile("..\\..\\pic\\tianye_1.psd","..\\..\\pic\\tianye.bmp");
		if (fHandle>=0)
		{
			//ret = PsdNewLayer(fHandle,"..\\..\\pic\\ms12.png",200,200,2,200);
			ret = PsdCreateLayer(fHandle,"..\\..\\pic\\ms12.png",200,200,100,200,2,200);
			ret = PsdSaveFile(fHandle);
			ret = PsdCloseFile(fHandle);
			Psd2Png("..\\..\\pic\\tianye_1.psd","..\\..\\pic\\tianye_1.png");
		}
		else
		{
			printf("fail to load psd file. ret=%d\n",ret);
		}		
	}
	else if (!strcmp(argv[1],"bmp2bmp"))
	{
		ret = bmp2bmp("..\\..\\pic\\tianye.bmp","..\\..\\pic\\tianye_tmp.bmp");
		if (ret>=0)
		{
			printf("bmp convert to bmp successfully!\n");
		}
	}		
	else if (!strcmp(argv[1],"jpg2bmp"))
	{
		//ret = jpg2bmp("..\\..\\pic\\testimg.jpg","..\\..\\pic\\testimg.bmp");
		ret = jpg2bmp("..\\..\\pic\\puke.jpg","..\\..\\pic\\puke_tmp.bmp");
		if (ret>=0)
		{
			printf("jpg convert to bmp successfully!\n");
		}
	}	
	else if (!strcmp(argv[1],"bmp2jpg"))
	{
		ret = bmp2jpg("..\\..\\pic\\tianye.bmp","..\\..\\pic\\tianye.jpg");
		if (ret>=0)
		{
			printf("bmp convert to jpg successfully!\n");
		}
	}
	else if (!strcmp(argv[1],"png2bmp"))
	{
		//ret = png2bmp("..\\..\\pic\\mxp.png","..\\..\\pic\\mxp_tmp.bmp");
		ret = png2bmp("..\\..\\pic\\ms12.png","..\\..\\pic\\ms12_tmp.bmp");

//		ret = png2bmp("..\\..\\pic\\lisa.png","..\\..\\pic\\lisa_tmp.bmp");		//can NOT load lisa.png into RAM
		if (ret>=0)
		{
			printf("png convert to bmp successfully!\n");
		}
	}	
	else if (!strcmp(argv[1],"bmp2png"))
	{
		ret = bmp2png("..\\..\\pic\\Bliss.bmp","..\\..\\pic\\Bliss.png");
		if (!ret)
		{
			printf("bmp convert to png successfully!\n");
		}
	}		
	else if (!strcmp(argv[1],"pngcrop"))
	{
		ret = bmp2png("..\\..\\pic\\Bliss.bmp","..\\..\\pic\\Bliss.png");
		//ret = pngzoom("..\\..\\pic\\Bliss.png",100);
		ret=pngcrop("..\\..\\pic\\Bliss.png",600,400);
	}		

	else
		;

	return 0;
}

#endif
