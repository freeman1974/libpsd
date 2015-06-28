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
#ifndef __LIB_PSDAPP_H__
#define __LIB_PSDAPP_H__
#ifdef __cplusplus
extern "C" {
#endif


#pragma pack (1)		//byte allign start
/*PS 3.0 psd format as below:
File Header				//fixed length,26bytes
Color Mode Data			//variable in length.
Image Resources			//variable in length.
Layer and Mask Information	//variable in length.
Image Data				//variable in length.

When writing one of these sections, you should write all fields in the section.
Photoshop 3.0 files under Windows .All data is stored in big endian byte order; under Windows you must byte
swap short and long integers when reading or writing. shit!!!

Color mode data section
Table 2–8: Color mode data
Length Name Description
4 bytes Length		:The length of the following color data. 
Variable Color data		:The color data.
Only indexed color and duotone have color mode data. For all other modes,this section is just 4 bytes: the length field, which is set to zero.

Table 2–9: Image resources section
Length Name Description
4 bytes Length 		:Length of image resource
Variable Resources 	:Image resources.

Table 2–10: Layer and mask information
Length Name Description
4 bytes Length 			:Length of the miscellaneous information section.
Variable Layers 			:Layer info. See table 2–12.
Variable Globallayer mask	:Global layer mask info. See table 2–19.

Table 2–11: Image data
Length Name Description
2 bytes Compression Compression method.		:Raw data = 0, RLE compressed = 1.
Variable Data 								:The image data, Planar order = RRR GGG BBB, etc. no pad bytes.

===========================layer&mask info is very important--------------------
Table 2–12: Layer info section
Length 					Name 			Description
4 bytes 					Length 			Length of the layers info section, rounded up to a multiple of 2.
Variable 					Layers structure     Data about each layer in the document. See table 2–13.
Variable 					Pixel data 		Channel image data for each channel in the order listed in 
the layers structure section. See table 2–18.

Table 2–13: Layer structure
Length 		Name 			Description
2 bytes	 	Count 			Number of layers. If <0, then number of layers is absolute value,and the first alpha channel contains the transparency 
data for the merged result.
Variable 		Layer 			Information about each layer (table 2–14).


*/

#ifndef psd_handle
#define	psd_handle 	int
#endif

#pragma pack ()		//byte allign end
#include "libpsd.h"

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#define DLLCALL 	__cdecl		//__stdcall
#else
#define DLLEXPORT 
#define DLLIMPORT 
#define DLLCALL 	
#endif
#define	DLLAPI	DLLEXPORT


#define	MAX_PSD_FILE_TABLE_ITEM	20
#define	MAX_PSD_FILENAME_LEN	128
//#define 	DEBUG_MSG	1		//debug message switch

typedef enum
{
	RIGHT90=0,	//right turn 90
	LEFT90,		//left turn 90
	FLIP,		//flip, exchange up and down
	MIRROR		//mirror, exchange left and right
}IMAGE_DIRECTION;

#define MAX(a,b) (a>b?a:b)
#define MIN(a,b) (a>b?b:a)

//---------------------declare the APIs of interface 
/*
Prototype: int  GetImgFileSize(char *fImgName,unsigned int *pWidth,unsigned int *pHeight)
Func desc: get image file size(width and height)
Input: 	char *fImgName, file name(abstract directory)	
Output:	unsigned short *pWidth,unsigned short *pHeight
Return:	0:success, others:fail.
Note:	only support .bmp, .jpg, .png, .psd
Example:	

*/
DLLEXPORT  int  GetImgFileSize(char *fImgName,unsigned int *pWidth,unsigned int *pHeight);

/*
transform a .psd file to my psd file, so libpsd can parse it correctly.
*/
DLLEXPORT psd_handle  Psd2Psd(char *PsdSrc,char *PsdDst);

/*
函数原型 FILE *PsdNewFile(char *PsdFileName,char *bgPic)
功能说明 生成一张新的psd格式图片，可以带一张背景图片。
入口参数 char *PsdFileName, 新psd文件的名称，包括全路径。
    char *bgPic，用作背景的图片。support bmp/jpg/png/psd
出口参数 无
返回值		<0:error code, 0,>0:file handler number,[0,19]. 
调用事项	1. psd file must exist already.
			2. psdfile will be open. do NOT open it again.
			3. new other layers' dimention should <= bgPic(widthxheight)
example:
			psd_handle fHandle=-1;
			fHandle = PsdNewFile("..\\pic\\tianye_new.psd","..\\pic\\tianye.bmp");
			if (fHandle >= 0)
			{
				printf("new psd file success!\n");
			}
*/
DLLAPI psd_handle  PsdNewFile(char *PsdFileName,char *bgPic);
DLLEXPORT psd_handle  PsdGetLayers(char *PsdSrc);
/*
函数原型psd_handle  PsdLoadFile(char *PsdFileName)
功能说明load a psd file into memory
入口参数char *PsdFileName, psd file name with full diretory
出口参数 无
返回值	 file handler[0,19], others:error code
调用事项1. psd file must exist already.
			2. psdfile will be open. do NOT open it again.
example:
		fHandle = PsdLoadFile("..\\..\\pic\\tianye_new.psd");	//tianye_new.psd has two layers already
		if (fHandle>=0)
		{
			ret = PsdNewLayer(fHandle,"..\\..\\pic\\ms12.png",200,200,2,200);
			ret = PsdSaveFile(fHandle);
			ret = PsdCloseFile(fHandle);
			Psd2Png("..\\..\\pic\\tianye_new.psd","..\\..\\pic\\tianye_new.png");
		}
		else
		{
			printf("fail to load psd file. ret=%d\n",ret);
		}
*/
DLLAPI psd_handle  PsdLoadFile(char *PsdFileName);
/*
函数原型	int PsdPreview(psd_handle fHandle,char *fBmpFile)
功能说明	prview psd file, not save it
入口参数	psd_handle fHandle: psd file handler, get it by PsdLoadFile() or PsdNewFile()
					char *fBmpFile: bmp file to be saved
出口参数	none
返回值		0:success, other negative value:error code
调用事项	
example:	
		int ret=-1;
		
		fHandle = PsdNewFile("..\\..\\pic\\tianye_brighness.psd","..\\..\\pic\\tianye.bmp");
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
			if (!ret)
				printf("succcess\n");
			else
				printf("fail, errcode is:%d\n",ret);
				
			PsdSaveFile(fHandle);
			PsdCloseFile(fHandle);		//must close firstly before load again...				
		}
*/
DLLEXPORT int  PsdPreview(psd_handle fHandle,char *fBmpFile);

/*
函数原型	int PsdSaveFile(psd_handle fHandle)
功能说明	save psd file
入口参数	psd_handle fHandle: psd file handler, get it by PsdLoadFile() or PsdNewFile()
出口参数	无
返回值		0:success, other negative value:error code
调用事项	before close psd file, you must call PsdSaveFile() to save modification.
*/
DLLAPI  int  PsdSaveFile(psd_handle fHandle);

/*
函数原型	int PsdCloseFile(psd_handle fHandle)
功能说明	关闭一个打开的psd file
入口参数	psd_handle fHandle，这是通过PsdLoadFile得到的psd 文件handle。
出口参数	无
返回值		0:success, other negative value:error code
调用事项	There is no dllmain, so you must close all the psd file when you exist your program.
*/
DLLAPI  int  PsdCloseFile(psd_handle fHandle);

/*transform .psd to .png/.bmp/.jpg format.
Psd2Bmp("..\\..\\pic\\tianye_all.psd","..\\..\\pic\\tianye_all.bmp");
Psd2Bmp("..\\..\\pic\\tianye_new.psd","..\\..\\pic\\tianye_new.jpg");
Psd2Png("..\\..\\pic\\tianye_all.psd","..\\..\\pic\\tianye_all.png");
*/
DLLAPI  int  Psd2Png(char *fPsdFile,char *fPngFile);
DLLAPI int Psd2Bmp(char *fPsdFile,char *fBmpFile);
DLLAPI int Psd2Jpg(char *fPsdFile,char *fJpgFile);


/*
函数原型	psd_status PsdNewLayer(psd_handle fHandle, char *sPngFileName,int PosX,int PosY,int Width,int Height, char Transpancy)
功能说明	use一张图片(png) as 一个新的psd图层.
入口参数	psd_handle fHandle，这是通过PsdLoadFile得到的psd background文件handle。
			char *sPngFileName，这是准备载入新图层的图片，only支持png。
			int PosX，起始位置X（单位：像素）
			int PosY，起始位置Y
			unsigned char ucLayerIndex, layer index number [1,n], 0 is reserved for background layer.
			char Transpancy，透明度[0,255]，0：全透明；255：不透明）。
出口参数	无
返回值		0:success, other negative value:error code
调用事项	必须要先new或load了一个psd文件后，才可以new一个新图层。
examples:
		fHandle = PsdNewFile("..\\pic\\tianye_new.psd","..\\pic\\tianye.bmp");
		printf("new one psd file, fHandle=%d\n",fHandle);
		if (fHandle>=0)
		{
			ret = PsdNewLayer(fHandle,"..\\pic\\ms12.png",20,80,1,155);
			PsdSaveFile(fHandle);
			PsdCloseFile(fHandle);		//must close firstly before load again...
		}

		Psd2Png("..\\pic\\tianye_new.psd","..\\pic\\tianye_new_tmp.png");
*/
DLLAPI  int  PsdNewLayer(psd_handle fHandle, char *sPngFileName \
	,int PosX,int PosY,unsigned char ucLayerIndex,unsigned char Transpancy);

/*
函数原型	psd_status PsdCreateLayer(psd_handle fHandle, char *fPngName,int PosX,int PosY,int Width,int Height, char Transpancy)
功能说明	use一张图片(png) as 一个新的psd图层.
入口参数	psd_handle fHandle，这是通过PsdLoadFile得到的psd background文件handle。
			char *fPngName，这是准备载入新图层的图片，only支持png。
			int PosX，起始位置X（单位：像素）
			int PosY，起始位置Y
			 int lwidth,  layer width
			 int lheight, layer height
			unsigned char ucLayerIndex, layer index number [1,n], 0 is reserved for background layer.
			char Transpancy，透明度[0,255]，0：全透明；255：不透明）。
出口参数	无
返回值		0:success, other negative value:error code
调用事项	必须要先new或load了一个psd文件后，才可以create一个新图层。
examples:
		fHandle = PsdNewFile("..\\..\\pic\\tianye_1.psd","..\\..\\pic\\tianye.bmp");
		if (fHandle>=0)
		{
			//ret = PsdNewLayer(fHandle,"..\\..\\pic\\ms12.png",200,200,2,200);
			ret = PsdCreateLayer(fHandle,"..\\..\\pic\\ms12.png",200,200,100,200,2,200);
			ret = PsdSaveFile(fHandle);
			ret = PsdCloseFile(fHandle);
			Psd2Png("..\\..\\pic\\tianye_1.psd","..\\..\\pic\\tianye_1.png");
		}
*/
DLLEXPORT int  PsdCreateLayer(psd_handle fHandle, char *fPngName \
		,unsigned int PosX,unsigned int PosY, unsigned int lwidth, unsigned int lheight \
	,unsigned char ucLayerIndex,unsigned char Transpancy);

/*
函数原型	int PsdSetLayerDirection(psd_handle fHandle,unsigned char WhichLayer, IMAGE_DIRECTION ImgDir)
功能说明	
入口参数	
出口参数	
返回值		0：成功。-1：失败。
调用事项	
example:
fHandle = PsdNewFile("..\\pic\\tianye_new.psd","..\\pic\\tianye.bmp");
if (fHandle>=0)
{
	PsdSetLayerDirection(fHandle,1,RIGHT90);
	PsdSetLayerDirection(fHandle,1,LEFT90);
	PsdSaveFile(fHandle);
	PsdCloseFile(fHandle);		//must close firstly before load again...
}

Psd2Png("..\\pic\\tianye_new.psd","..\\pic\\tianye_new_tmp.png");

*/
DLLAPI  int  PsdSetLayerDirection(psd_handle fHandle,unsigned char WhichLayer, IMAGE_DIRECTION ImgDir);
/*
函数原型	PsdSetLayerBrightness(psd_handle fHandle,unsigned char WhichLayer, signed char Brightness)
功能说明	
入口参数	unsigned char WhichLayer,layer index number [1,n], 0 is reserved for background layer.
				signed char Brightness,[-100,+100]
出口参数	
返回值		0：成功。-1：失败。
调用事项	
example:
fHandle = PsdNewFile("..\\pic\\tianye_new.psd","..\\pic\\tianye.bmp");
if (fHandle>=0)
{
	ret = PsdNewLayer(fHandle,"..\\..\\pic\\ms12.png",0,0,1,200);
	PsdSetLayerBrightness(fHandle,1,50);
	PsdSaveFile(fHandle);
	PsdCloseFile(fHandle);		//must close firstly before load again...
}

Psd2Png("..\\pic\\tianye_new.psd","..\\pic\\tianye_new_tmp.png");

*/
DLLAPI  int  PsdSetLayerBrightness(psd_handle fHandle,unsigned char WhichLayer, signed char Brightness);
/*
函数原型	PsdSetLayerContrast(psd_handle fHandle,unsigned char WhichLayer, signed char Contrast)
功能说明	
入口参数	unsigned char WhichLayer,layer index number [1,n], 0 is reserved for background layer.
				signed char Contrast, [-100,+100]
出口参数	
返回值		0：成功。-1：失败。
调用事项	
example:
fHandle = PsdNewFile("..\\pic\\tianye_new.psd","..\\pic\\tianye.bmp");
if (fHandle>=0)
{
	ret = PsdNewLayer(fHandle,"..\\..\\pic\\ms12.png",0,0,1,200);
	PsdSetLayerContrast(fHandle,1,50);
	PsdSaveFile(fHandle);
	PsdCloseFile(fHandle);		//must close firstly before load again...
}

Psd2Png("..\\pic\\tianye_new.psd","..\\pic\\tianye_new_tmp.png");

*/
DLLAPI  int  PsdSetLayerContrast(psd_handle fHandle,unsigned char WhichLayer, signed char Contrast);


/*
函数原型	int pngzoom(char *fPngName, unsigned char ucZoomRate)
功能说明	open png file, zoom it and save it.
入口参数	char *fPngName, png file name like "..\\pic\\tianye_new_tmp.png"
				unsigned char ucZoomRate, limit to [50,200]
出口参数	无
返回值		0:success, other negative value:error code
调用事项	this png file will be modified.
example:
	fHandle = PsdNewFile("..\\pic\\tianye_new.psd","..\\pic\\tianye.bmp");
	if (fHandle>=0)
	{
		pngzoom("..\\pic\\tianye.png",50);
		PsdNewLayer(fHandle,"..\\pic\\tianye.png",20,80,1,155);
		PsdSaveFile(fHandle);
		PsdCloseFile(fHandle);		//must close firstly before load again...
	}
	
*/
DLLAPI  int  pngzoom(char *fPngName, unsigned char ucZoomRate);



/////////////////////////////////////////////////////////////////////////////
//--------------internal using only---------------------------------
int bmp2bmp(char *fSrcBmp,char *fDstBmp);
int bmp2jpg(char *fBmpName,char *fJpgName);
int jpg2bmp(char *fJpgName, char *fBmpName);
int bmp2png(char *fBmpName,char *fPngName);
int png2bmp(char *fPngName, char *fBmpName);
int MergeTwoPngFile(char *fPngName1,char *fPngName2,char *fMergedName,psd_uchar ucTransparency);
int CreateDemoPsdFile(char *fDemoPsdName,char *fAnyPngFile);
int  pngcrop(char *fPngName, int width, int height);

//////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif
#endif

