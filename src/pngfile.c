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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pngfile.h"

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif


/* Check to see if a file is a PNG file using png_sig_cmp().  png_sig_cmp()
 * 0:success or equal to, -1:fail.
 */
#define PNG_BYTES_TO_CHECK 4
int png_check(char *file_name)
{
	char buf[PNG_BYTES_TO_CHECK];
	FILE *fp;
	int ret=-1;

	if ((fp = fopen(file_name, "rb")) == NULL)
		return -1;

	if (fread(buf, 1, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK)
	{
		ret=-1;
	}
	else
	{
		ret = png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK);
	}

	fclose(fp);
	return ret;
}

#if 0
static void user_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
//	if (fread(data,1,length) != length) 
//		png_error(png_ptr, "Read Error");
}


static void user_write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
//	if (fwrite(data,1,length) != length) 
//		png_error(png_ptr, "Write Error");
}


static void user_error_fn(png_structp png_ptr,png_const_charp error_msg)
{
	strncpy((char*)png_ptr->error_ptr,error_msg,255);
	longjmp(png_ptr->jmpbuf, 1);
}

#endif

/*
func desp: this func supports alpha channel. buffer is BGRA format

*/
unsigned char *png_load(char *filename,unsigned int *width,unsigned int *height)
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	int ret=-1;
	png_bytep row_buf;
	png_uint_32 y;
	int num_pass, pass;

	//check to see if png type file
	ret = png_check(filename);
	if (ret)
	{
		printf( "%s is not exist or NOT png format. pls check \n", filename);
		return NULL;		//it is NOT a png file.
	}
   	//printf( "libpng version is:%s\n", PNG_LIBPNG_VER_STRING);

	//prepare to read png file
	fp = fopen(filename, "rb");		//has pass test. fp will not equal to NULL.
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (void *)NULL,(png_error_ptr)NULL,  (png_error_ptr)NULL);
	if (png_ptr == NULL)
	{
		printf("preparing png_ptr... fail\n");
		fclose(fp);
		return NULL;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("preparing info_ptr... fail\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return NULL;
	}

	//png_read_png(png_ptr, info_ptr,png_transforms_flag,NULL);		//high level  func
	//init stream I/O func
	png_init_io(png_ptr, fp);	//using standard C API
	png_read_info(png_ptr, info_ptr);		//read the file information

	//tell libpng to strip 16 bit depth files down to 8 bits.  
	if (info_ptr->bit_depth == 16)	
	{
		printf("NOT be supported 16 bit-depth png file operation!\n");
		png_set_strip_16(png_ptr);
	}

	if (info_ptr->pixel_depth != 32){
		printf("NOT be supported non-4-bpp png file operation!\n");
		return NULL;
	}
	
	/*
	If there is no interlacing (check interlace_type == PNG_INTERLACE_NONE), 
	this is simple:
	png_read_rows(png_ptr, row_pointers, NULL, number_of_rows);
	*/
	if (info_ptr->interlace_type)
	{
		num_pass = png_set_interlace_handling(png_ptr);
	}
	else
	{
		num_pass = 1;
	}

	// Allocate the memory to hold the image using the fields of info_ptr.
	row_buf = (png_bytep)malloc(info_ptr->rowbytes * info_ptr->height *num_pass);

	for (pass = 0; pass < num_pass; pass++)
	{
		for (y = 0; y < info_ptr->height; y++)
		{
			//png_read_row(png_structp png_ptr, png_bytep row, png_bytep dsp_row)
			png_read_row(png_ptr,row_buf+(info_ptr->rowbytes*y) + (info_ptr->rowbytes*info_ptr->height*pass), NULL);
		}
	}
	

	if (width)
		*width = info_ptr->width;
	if (height)
		*height = info_ptr->height;
	
	png_read_end(png_ptr, info_ptr);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return ((unsigned char *)row_buf);
}

/*
This func only support 4 bpp BGRA format.
*/
int png_save(char * filename,unsigned char *img_buf,int width,int height)
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	int number_passes = 1,pass,y;
	int bytes_per_pixel=4;

	fp = fopen(filename, "wb+");
	if (fp == NULL)	return (-1);

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if (png_ptr == NULL)
	{
		fclose(fp);
		return (-2);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return (-3);
	}

	png_init_io(png_ptr, fp);

	info_ptr->width = width;
	info_ptr->height = height;
	info_ptr->pixel_depth = 32;
	info_ptr->channels = 4;
	info_ptr->bit_depth = 8;
	info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;		//  4 means COLORTYPE_ALPHA
	info_ptr->compression_type = info_ptr->filter_type = info_ptr->interlace_type=0;
	info_ptr->valid = 0;
	info_ptr->interlace_type=PNG_INTERLACE_NONE;
	info_ptr->rowbytes = width*4;	

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
    */
   png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   // Write the file header information.  REQUIRED 
   png_write_info(png_ptr, info_ptr);

   for (pass = 0; pass < number_passes; pass++)
   {
	  //png_write_row(png_structp png_ptr, png_bytep row)
        for (y = 0; y < height; y++)
           png_write_row(png_ptr, img_buf + (info_ptr->rowbytes * y));
   }

   // It is REQUIRED to call this to finish writing the rest of the file 
   png_write_end(png_ptr, info_ptr);

   png_destroy_write_struct(&png_ptr, &info_ptr);
   fclose(fp);
   return 0;
}


/*
For testing purpose, do NOT support alpha channel. 
*/
int png2png(char *inname, char *outname)
{
	FILE *fpin, *fpout;
	png_structp read_ptr;
	png_structp write_ptr;
	png_infop info_ptr;
	png_infop end_info;
	png_bytep row_buf;
	png_uint_32 rowbytes;
	png_uint_32 y;
	int num_pass, pass;

	fpin = fopen(inname, "rb");
	if (!fpin)
	{
		printf( "Could not find input file %s\n", inname);
		return -1;
	}

	fpout = fopen(outname, "wb");
	if (!fpout)
	{
		printf( "Could not open output file %s\n", outname);
		fclose(fpin);
		return -1;
	}

	read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (void *)NULL,(png_error_ptr)NULL,  (png_error_ptr)NULL);
	write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (void *)NULL,(png_error_ptr)NULL, (png_error_ptr)NULL);
	info_ptr = png_create_info_struct(read_ptr);
	end_info = png_create_info_struct(read_ptr);

	if (setjmp(read_ptr->jmpbuf))
	{
		printf( "libpng read error\n");
		png_destroy_read_struct(&read_ptr, &info_ptr, &end_info);
		png_destroy_write_struct(&write_ptr, (png_infopp)NULL);
		fclose(fpin);
		fclose(fpout);
		return -2;
	}

	if (setjmp(write_ptr->jmpbuf))
	{
		printf( "libpng write error\n");
		png_destroy_read_struct(&read_ptr, &info_ptr, &end_info);
		png_destroy_write_struct(&write_ptr, (png_infopp)NULL);
		fclose(fpin);
		fclose(fpout);
		return -2;
	}

	png_init_io(read_ptr, fpin);
	png_init_io(write_ptr, fpout);

	png_read_info(read_ptr, info_ptr);
	png_write_info(write_ptr, info_ptr);

/*
	if ((info_ptr->color_type & PNG_COLOR_TYPE_PALETTE)==PNG_COLOR_TYPE_PALETTE)
		channels = 1;
	else
		channels = 3;
	
	if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA)
		channels++;
*/
	//rowbytes = info_ptr->width * info_ptr->pixel_depth;
	
	rowbytes = info_ptr->rowbytes;
	row_buf = (png_bytep)malloc((size_t)rowbytes);
	if (!row_buf)
	{
		printf( "No memory to allocate row buffer\n");
		png_destroy_read_struct(&read_ptr, &info_ptr, &end_info);
		png_destroy_write_struct(&write_ptr, (png_infopp)NULL);
		fclose(fpin);
		fclose(fpout);
		return -3;
	}

/*
 If there is no interlacing (check interlace_type == PNG_INTERLACE_NONE), 
this is simple:
    png_read_rows(png_ptr, row_pointers, NULL, number_of_rows);
*/
   if (info_ptr->interlace_type)
   {
      num_pass = png_set_interlace_handling(read_ptr);
      num_pass = png_set_interlace_handling(write_ptr);
   }
   else
   {
      num_pass = 1;
   }

   for (pass = 0; pass < num_pass; pass++)
   {
      for (y = 0; y < info_ptr->height; y++)
      {
         png_read_rows(read_ptr, (png_bytepp)&row_buf, (png_bytepp)0, 1);
         png_write_rows(write_ptr, (png_bytepp)&row_buf, 1);	//reuse the same row pointer
      }
   }

   png_read_end(read_ptr, end_info);
   png_write_end(write_ptr, end_info);

   png_destroy_read_struct(&read_ptr, &info_ptr, &end_info);
   png_destroy_write_struct(&write_ptr, (png_infopp)NULL);

   fclose(fpin);
   fclose(fpout);

   free((void *)row_buf);

   return 0;
}


