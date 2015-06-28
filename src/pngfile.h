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

#ifndef __PNGFILE_H__
#define __PNGFILE_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "png.h"

#pragma pack (1)		//byte allign start



#pragma pack ()		//byte allign end

extern unsigned char *png_load(char *filename,unsigned int *width,unsigned int *height);
extern int png_save(char * filename,unsigned char *img_buf,int width,int height);

extern int png2png(char *inname, char *outname);


#ifdef __cplusplus
}
#endif
#endif
