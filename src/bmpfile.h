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
#ifndef __BMPFILE_H__
#define __BMPFILE_H__
#ifdef __cplusplus
extern "C" {
#endif

#pragma pack (1)		//byte allign start
#define BMP_HEADERSIZE 54
typedef struct
{
	char bmpid[2];  		//BM
	unsigned int filesize; 	//file size = header len+datasize
	unsigned int reserved1; //must be 0L
	unsigned int imgoff; 	//general 54L
	unsigned int headsize; 	//40L
	unsigned int imgwidth;
	unsigned int imgheight;
	unsigned short planes; 	//must be 1L
	unsigned short bitcount;
	unsigned int compression;
	unsigned int datasize;	//raw data len
	unsigned int hres;
	unsigned int vres;
	unsigned int colors;
	unsigned int impcolors; 
}BMP_HEADER;
//}__attribute__((packed,aligned(1)));

typedef struct tagRGBQUAD{     //定义每个像素的数据类型
        unsigned char  rgbBlue;
        unsigned char  rgbGreen;
        unsigned char  rgbRed;
}RGBQUAD;

#pragma pack ()		//byte allign end

int bmp_save(char* filename,unsigned char *buffer,int width,int height,int bpp);
int bmp_load(char* filename,BMP_HEADER **pBmpHeader,unsigned char **pBmpBuff);




#ifdef __cplusplus
}
#endif
#endif
