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
#ifndef __JPGFILE_H__
#define __JPGFILE_H__
#ifdef __cplusplus
extern "C" {
#endif

#pragma pack (1)		//byte allign start


#pragma pack ()		//byte allign end

int jpg_save(char * filename,unsigned char *image_buffer,int image_width,int image_height,int quality);
unsigned char *jpg_load(char *fileName,unsigned int *width,unsigned int *height);


#ifdef __cplusplus
}
#endif
#endif