/**
 * libpsdapp - Photoshop file formats (*.psd) decode application using library
 * Copyright (C) 2004-2010 Freeman.Tan (tanyc@126.com).
 *
 *
 * 
1. 只需要支持PNG格式。因为png有a值，这个阿尔法值就针对每个pixel的透明度。
2、新添加的图层，每层需要设置透明度，范围是［0，255］，0是全透明，255是不透明。
3、背景图就是psd格式，它装载进来时各个图层透明度为255.
4、每插入一层须指明在哪层之上。
5、文件只需要保存为psd格式，color mode只支持RGB = 3。
6、每个图层提供编辑功能，包括：旋转（90,180,270,360)功能，翻转，镜像。

程序总体设计思路：
1、先将psd文件load到RAM中，程序员就可以直接用文件句柄来操作它。
2、edit文件，比如增加或删除一个layer，flip/mirror某个图层,etc.
3、save文件到硬盘中。
4、close文件。
APIs的调用顺序应该follow上述设计思路:PsdNewFile()--->PsdNewLayer/

 */
//#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "png.h"
#include "libpsd.h"
#include "bmpfile.h"
#include "jpgfile.h"
#include "pngfile.h"
#include "psd_system.h"
#include "psdfile.h"

/*
#ifdef DEBUG_MSG
	#define DBG_MSG(fmt,args...) printf("cLibpsd:" fmt , ## args) 
	#define DBG_POS 	printf("Func:%s,Line:%d\n",__FUNCTION__,__LINE__); 
#else
	#define DBG_MSG(fmt,args...) 
	#define DBG_POS 
#endif
*/
typedef struct
{
	char sFileName[MAX_PSD_FILENAME_LEN];
	int fHandle;
	int max_width;	//decided by background pic size
	int max_height;
	psd_context	*pContext;
}PSD_FILE_TABLE;
static PSD_FILE_TABLE gPsdFileTable[MAX_PSD_FILE_TABLE_ITEM];
//#define TRANS_DOWN(rate)	((255-rate)/255)
//#define TRANS_UP(rate)		(rate/255)

#if 0
/*
   这个函数会将参数nptr字符串根据参数base来转换成长整型数。
   参数base范围从2至36，或0。参数base代表采用的进制方式，
   如base值为10则采用10进制，若base值为16则采用16进制等。
   当base值为0时则是采用10进制做转换，但遇到如’0x’前置
   字符则会使用16进制做转换、遇到’0’前置字符而不是’0x’
   的时候会使用8进制做转换。一开始strtol()会扫描参数nptr
   字符串，跳过前面的空格字符，直到遇上数字或正负符号才
   开始做转换，再遇到非数字或字符串结束时('\0')结束转换，
   并将结果返回。若参数endptr不为NULL，则会将遇到不合条件
   而终止的nptr中的字符指针由endptr返回。
*/
#define             STRLONG_MAX     2147483647L  /*0x7FFFFFFF*/
#define             STRLONG_MIN     (-2147483647L-1L) /*-0x80000000*/
static int mystrtol(const char *nptr, char **endptr, int base)
{
   const char *p = nptr;
   unsigned long ret;
   int ch;
   unsigned long Overflow;
   int sign = 0, flag, LimitRemainder;

   if (NULL == nptr)	return -1;	//add by yingcai
   /*
      跳过前面多余的空格，并判断正负符号。
      如果base是0，允许以0x开头的十六进制数，
      以0开头的8进制数。      如果base是16，则同时也允许以0x开头。
   */
   do
   {
      ch = *p++;
   } while (isspace(ch));
  
   if (ch == '-')
   {
      sign = 1;
      ch = *p++;
   }

   else if (ch == '+')
      ch = *p++;
   if ((base == 0 || base == 16) &&
      ch == '0' && (*p == 'x' || *p == 'X'))
   {
      ch = p[1];
      p += 2;
      base = 16;
   }
   if (base == 0)
      	base = ch == '0' ? 8 : 10;

   Overflow = sign ? -(unsigned long)STRLONG_MIN : STRLONG_MAX;
   LimitRemainder = Overflow % (unsigned long)base;
   Overflow /= (unsigned long)base;

   for (ret = 0, flag = 0;; ch = *p++)
   {
      /*把当前字符转换为相应运算中需要的值。*/
      if (isdigit(ch))
        ch -= '0';
      else if (isalpha(ch))
        ch -= isupper(ch) ? 'A' - 10 : 'a' - 10;
      else
        break;
      if (ch >= base)
        break;
 
      /*如果产生溢出，则置标志位，以后不再做计算。*/
      if (flag < 0 || ret > Overflow || (ret == Overflow && ch > LimitRemainder))
        flag = -1;
      else
      {
        flag = 1;
        ret *= base;
        ret += ch;
      }
   }

   /*
      如果溢出，则返回相应的Overflow的峰值。
      没有溢出，如是符号位为负，则转换为负数。
   */

   if (flag < 0)
      ret = sign ? STRLONG_MIN : STRLONG_MAX;
   else if (sign)
      ret = -ret;
  
   /*
      如字符串不为空，则*endptr等于指向nptr结束
      符的指针值；否则*endptr等于nptr的首地址。
   */

   if (endptr != 0)
      *endptr = (char *)(flag ?(p - 1) : nptr);

   return ret;

}

/* Find token in string
* Accepts: source pointer or NIL to use previous source
*            vector of token delimiters pointer
* Returns: pointer to next token
*/
static char *ts = NULL;               // string to locate tokens 
static char *mystrtok (char *s,char *ct)
{
  char *t;
  if (!s) s = ts;               // use previous token if none specified 
  if (!(s && *s)) return NULL;       // no tokens 
                                // find any leading delimiters 
  do for (t = ct, ts = NULL; *t; t++) if (*t == *s) {
    if (*(ts = ++s)) break;       // yes, restart seach if more in string 
    return ts = NULL;                // else no more tokens 
  } while (ts);                        // continue until no more leading delimiters 
                               // can we find a new delimiter?
  for (ts = s; *ts; ts++) for (t = ct; *t; t++) if (*t == *ts) {
    *ts++ = '\0';                // yes, tie off token at that point 
    return s;                        // return our token 
  }
  ts = NULL;                        //no more tokens 
  return s;                       // return final token 
}
#endif

/*
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
      )
{
    switch (ul_reason_for_call)
 {
  case DLL_PROCESS_ATTACH:
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
   break;
    }
    return TRUE;
}
*/

#if 0
/*
R = Y + 1.14V
G = Y - 0.39U - 0.58V
B = Y + 2.03U 

R = Y + 1.4075 *（V-128）
G = Y – 0.3455 *（U –128） – 0.7169 *（V –128）
B = Y + 1.779 *（U – 128）

R = Y + 1.402Cr
G = Y - 0.344Cb - 0.714Cr
B = Y + 1.772Cb

R = Y + 1.4075 *（V-128）
G = Y – 0.3455 *（U –128） – 0.7169 *（V –128）
B = Y + 1.779 *（U – 128）
*/
static unsigned int yuv2rgb(int y, int u, int v)
{
	unsigned int pixel32 = 0;
	unsigned char *pixel = (unsigned char *)&pixel32;
	int r, g, b;

	u -= 128;
	v -= 128;

	r = y + (1.370705 * v);
	g = y - (0.698001 * v) - (0.337633 * u);
	b = y + (1.732446 * u);

	//r = y + (1.4075 * v);
	//g = y - (0.7169 * v) - (0.3455 * u);
	//b = y + (1.779 * u);

	if(r > 255) r = 255;
	if(g > 255) g = 255;
	if(b > 255) b = 255;
	if(r < 0) r = 0;
	if(g < 0) g = 0;
	if(b < 0) b = 0;

	pixel[0] = r * 220 / 256;
	pixel[1] = g * 220 / 256;
	pixel[2] = b * 220 / 256;
	//pixel[0] = r;
	//pixel[1] = g;
	//pixel[2] = b;

	return pixel32;
}

static unsigned int rgb2yuv(int r, int g, int b)
{
	unsigned int pixel32 = 0;
	unsigned char *pixel = (unsigned char *)&pixel32;
	int y, u, v;

	y =   0.299 * (r - 128) + 0.587 * (g - 128) + 0.114 * (b - 128) + 128;
	u = - 0.147 * (r - 128) - 0.289 * (g - 128) + 0.436 * (b - 128) + 128;
	v =   0.615 * (r - 128) - 0.515 * (g - 128) - 0.100 * (b - 128) + 128;

	if(y > 255) y = 255;
	if(u > 255) u = 255;
	if(v > 255) v = 255;
	if(y < 0) y = 0;
	if(u < 0) u = 0;
	if(v < 0) v = 0;

	pixel[0] = y;
	pixel[1] = u;
	pixel[2] = v;

	return pixel32;
} 

#endif

static
#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
int convert_rgb_to_yuv_pixel(int r, int g, int b)
{
	unsigned int pixel32 = 0;
	unsigned char *pixel = (unsigned char *)&pixel32;
	int y, u, v;

	y =   0.299 * (r - 128) + 0.587 * (g - 128) + 0.114 * (b - 128) + 128;
	u = - 0.147 * (r - 128) - 0.289 * (g - 128) + 0.436 * (b - 128) + 128;
	v =   0.615 * (r - 128) - 0.515 * (g - 128) - 0.100 * (b - 128) + 128;

//	y = (299*r + 587*g+ 114*b)/1000;
//	u = (-147*r - 289*g + 436*b)/1000;
//	v = (615*r - 515*g - 100*b)/1000;

//	y  =	  (0.257 * r) + (0.504 * g) + (0.098 * b) + 16
//	v =  (0.439 * r) - (0.368 * g) - (0.071 * b) + 128
//	u = -(0.148 * r) - (0.291 *g) + (0.439 * b) + 128


	if(y > 255) y = 255;
	if(u > 255) u = 255;
	if(v > 255) v = 255;
	if(y < 0) y = 0;
	if(u < 0) u = 0;
	if(v < 0) v = 0;

	pixel[0] = y;
	pixel[1] = u;
	pixel[2] = v;

	return pixel32;
}

/*
RGB24--->YUV4:2:2
*/
int convert_rgb_to_yuv_buffer(unsigned char *rgb, unsigned char *yuv, unsigned int width, unsigned int height)
{
	unsigned int in, out = 0;
	unsigned int pixel32;
	int y0, u0, v0, y1, u1, v1;

	for(in = 0; in < width * height * 3; in += 6) {
		pixel32 = convert_rgb_to_yuv_pixel(rgb[in], rgb[in + 1], rgb[in + 2]);
		y0 = (pixel32 & 0x000000ff);
		u0 = (pixel32 & 0x0000ff00) >>  8;
		v0 = (pixel32 & 0x00ff0000) >> 16;

		pixel32 = convert_rgb_to_yuv_pixel(rgb[in + 3], rgb[in + 4], rgb[in + 5]);
		y1 = (pixel32 & 0x000000ff);
		u1 = (pixel32 & 0x0000ff00) >>  8;
		v1 = (pixel32 & 0x00ff0000) >> 16;

		yuv[out++] = y0;
		yuv[out++] = (u0 + u1) / 2;
		yuv[out++] = y1;
		yuv[out++] = (v0 + v1) / 2;
	}

	return 0;
}

int convert_rgb_to_yuv_file(char *rgbfile, char *yuvfile, unsigned int width, unsigned int height)
{
	FILE *in, *out;
	unsigned char *yuv, *rgb;

	in = fopen(rgbfile, "rb");
	if(!in)
		return 1;
	out = fopen(yuvfile, "wb");
	if(!out)
		return 1;

	rgb = malloc(width * height * 3);
	if(rgb == NULL)
		return 2;
	yuv = malloc(width * height * 2);
	if(yuv == NULL)
		return 2;

	fread(rgb, width * height * 3, 1, in);
	fclose(in);
	if(convert_rgb_to_yuv_buffer(rgb, yuv, width, height))
		return 3;
	fwrite(yuv, width * height * 2, 1, out);
	fclose(out);

	return 0;
}


int convert_yuv_to_rgb_pixel(int y, int u, int v)
{
	unsigned int pixel32 = 0;
	unsigned char *pixel = (unsigned char *)&pixel32;
	int r, g, b;

	u -= 128;
	v -= 128;

	r = y + 1.14*v;
	g = y - 0.39*u - 0.58*v;
	b = y + 2.03*u;

//	r = y + (1.370705 * v);
//	g = y - (0.698001 * v) - (0.337633 * u);
//	b = y + (1.732446 * u);

	//r = y + (1.4075 * v);
	//g = y - (0.7169 * v) - (0.3455 * u);
	//b = y + (1.779 * u);

	//r = y + v + ((v*103)>>8);
	//g = y - ((u*88)>>8) - ((v*183)>>8);
	//b = y + u + ((u*198)>>8);

	if(r > 255) r = 255;
	if(g > 255) g = 255;
	if(b > 255) b = 255;
	if(r < 0) r = 0;
	if(g < 0) g = 0;
	if(b < 0) b = 0;

	pixel[0] = r;
	pixel[1] = g;
	pixel[2] = b;
	pixel[3] = 0xFF;	//alpha channel

	return pixel32;
}

/*
use for YCbYCr mode. 
*/
int convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
	unsigned int in, out = 0;
	unsigned int pixel_16;
	unsigned char pixel_24[3];
	unsigned int pixel32;
	int y0, u, y1, v;

	for(in = 0; in < width * (height<<1); in += 4) {
		pixel_16 =
			yuv[in + 3] << 24 |
			yuv[in + 2] << 16 |
			yuv[in + 1] <<  8 |
			yuv[in + 0];

		y0 = (pixel_16 & 0x000000ff);
		u  = (pixel_16 & 0x0000ff00) >>  8;
		y1 = (pixel_16 & 0x00ff0000) >> 16;
		v  = (pixel_16 & 0xff000000) >> 24;

		pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
		pixel_24[0] = (pixel32 & 0x000000ff);
		pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
		pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;

		rgb[out++] = pixel_24[0];
		rgb[out++] = pixel_24[1];
		rgb[out++] = pixel_24[2];

		pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
		pixel_24[0] = (pixel32 & 0x000000ff);
		pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
		pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;

		rgb[out++] = pixel_24[0];
		rgb[out++] = pixel_24[1];
		rgb[out++] = pixel_24[2];
	}

	return 0;
}


int convert_yuv_to_rgb_file(char *yuvfile, char *rgbfile, unsigned int width, unsigned int height)
{
	FILE *in, *out;
	unsigned char *yuv, *rgb;

	in = fopen(yuvfile, "rb");
	if(!in)
		return 1;
	out = fopen(rgbfile, "wb");
	if(!out)
		return 1;

	yuv = malloc(width * height * 2);
	if(yuv == NULL)
		return 2;
	rgb = malloc(width * height * 3);
	if(rgb == NULL)
		return 2;

	fread(yuv, width * height * 2, 1, in);
	fclose(in);
	if(convert_yuv_to_rgb_buffer(yuv, rgb, width, height))
		return 3;
	fwrite(rgb, width * height * 3, 1, out);
	fclose(out);

	return 0;
}

psd_uchar *CreateRgbaBuf(psd_uchar *YBuf,psd_uint width,psd_uint height)
{
	psd_uchar *RgbaBuf=NULL;
	int i=0,PixelSize=width*height,k=0;

	RgbaBuf=psd_malloc(PixelSize<<2);

	for (i=0; i<PixelSize; i++)
	{
		k=i<<2;
		RgbaBuf[k]=YBuf[i];		//red
		RgbaBuf[k+1]=YBuf[i];		//green
		RgbaBuf[k+2]=YBuf[i];		//blue
		RgbaBuf[k+3]=0xFF;		//alpha
	}

	return RgbaBuf;
}


//static void inline swap_rgb24(unsigned char *mem, int n)   //颜色交换 
//memory order:BGRBGRBGRBGRBGR//it will be error if width is NOT 4x
static void swap_rgb24(unsigned char *mem, int n)   //颜色交换 
{
	unsigned char c;
	unsigned char *p = mem;
	int i = n ;
	while (--i)       //因为采集回来的数据是BGR格式的，要在VGB上正常显示，需要交换成RGB
	{			//V4L interface is BGR format
		c = p[0];
		p[0] = p[2];
		p[2] = c;
		p += 3;
	}
}

static void swap_rgba32(unsigned char *mem, int n)   //颜色交换 
{
	unsigned char c;
	unsigned char *p = mem;
	int i = n ;
	while (--i)       //因为采集回来的数据是BGR格式的，要在VGB上正常显示，需要交换成RGB
	{			//V4L interface is BGR format
		c = p[0];
		p[0] = p[2];
		p[2] = c;
		p += 4;
	}
}

//do flip. if width is not equal to 4x in bytes, error will happen
static void Rgb24_flip(unsigned char *pRgbBuf, int width,int height)
{
	unsigned char *pStartP=NULL;
	unsigned char *pEndP=NULL,c;
	int i=0,j=0;

	pStartP = pRgbBuf;	//end point
	pEndP = pRgbBuf + (height-1)*width*3;

	for(i=0; i<height/2; i++)
	{
		for (j=0; j<width; j++)
		{	
			c = pStartP[0];
			pStartP[0] = pEndP[0];
			pEndP[0] = c;
			c = pStartP[1];
			pStartP[1] = pEndP[1];
			pEndP[1] = c;
			c = pStartP[2];
			pStartP[2] = pEndP[2];
			pEndP[2] = c;			
			pStartP += 3;
			pEndP += 3;
		}
		pEndP -= 2*width*3;
	}
	return;
}


static void Rgba32_flip(unsigned char *pRgbBuf, int width,int height)
{
	unsigned char *pStartP=NULL;
	unsigned char *pEndP=NULL,c;
	int i=0,j=0;

	pStartP = pRgbBuf;	//end point
	pEndP = pRgbBuf + (height-1)*(width<<2);

	for(i=0; i<(height>>1); i++)
	{
		for (j=0; j<width; j++)
		{	
			c = pStartP[0];
			pStartP[0] = pEndP[0];
			pEndP[0] = c;

			c = pStartP[1];
			pStartP[1] = pEndP[1];
			pEndP[1] = c;

			c = pStartP[2];
			pStartP[2] = pEndP[2];
			pEndP[2] = c;

			c = pStartP[3];
			pStartP[3] = pEndP[3];
			pEndP[3] = c;
			
			pStartP += 4;
			pEndP += 4;
		}
		pEndP -= ((width<<2)<<1);	//2*(width*4)
	}
	return;
}


//do  mirror
static void Rgb24_mirror(unsigned char *pRgbBuf, int width,int height)
{
	unsigned char *pStartP=NULL;
	unsigned char *pEndP=NULL,c;
	int i=0,j=0;

	for(i=0; i<height; i++)
	{
		pStartP = pRgbBuf + (i*width*3);
		pEndP = pStartP + (width-1)*3;	
		for (j=0; j<width/2; j++)
		{	
			c = pStartP[0];
			pStartP[0] = pEndP[0];
			pEndP[0] = c;
			c = pStartP[1];
			pStartP[1] = pEndP[1];
			pEndP[1] = c;
			c = pStartP[2];
			pStartP[2] = pEndP[2];
			pEndP[2] = c;			
			pStartP += 3;
			pEndP -= 3;
		}
	}
	return;
}



//do  mirror
static void Rgba32_mirror(unsigned char *pRgbBuf, int width,int height)
{
	unsigned char *pStartP=NULL;
	unsigned char *pEndP=NULL,c;
	int i=0,j=0;

	for(i=0; i<height; i++)
	{
		pStartP = pRgbBuf + (i*(width<<2));
		pEndP = pStartP + ((width-1)<<2);	
		for (j=0; j<(width>>1); j++)
		{	
			c = pStartP[0];
			pStartP[0] = pEndP[0];
			pEndP[0] = c;
			
			c = pStartP[1];
			pStartP[1] = pEndP[1];
			pEndP[1] = c;

			c = pStartP[2];
			pStartP[2] = pEndP[2];
			pEndP[2] = c;			

			c = pStartP[3];
			pStartP[3] = pEndP[3];
			pEndP[3] = c;	

			pStartP += 4;
			pEndP -= 4;
		}
	}
	return;
}

static unsigned char *Rgba32_left90(unsigned char *pRgbBuf, int width,int height)
{
	unsigned char *pSrc=NULL;
	unsigned char *pDst=NULL;
	int i=0,j=0;
	int n = 0; 
	int linesize = width<<2; 

	pSrc = pRgbBuf;	
	pDst = psd_malloc(height*(width<<2));
	
	if (!pDst)
		return NULL;

	for (j=width; j>0; j--)
	{
		for (i=0; i<height; i++) 
		{ 
			memcpy(&pDst[n],&pSrc[linesize*i+(j<<2)-4],4); 
	  		n+=4;
		}
	}
	
	return pDst;
}

static unsigned char *Rgba32_right90(unsigned char *pRgbBuf, int width,int height)
{
	unsigned char *pSrc=NULL;
	unsigned char *pDst=NULL;
	int i=0,j=0;
	int n = 0; 
	int linesize = width<<2; 

	pSrc = pRgbBuf;	
	pDst = psd_malloc(height*(width<<2));
	
	if (!pDst)
		return NULL;

	for (j=0; j<width; j++)
	{
		for (i=height; i>0; i--) 
		{ 
			memcpy(&pDst[n],&pSrc[linesize*(i-1)+(j<<2)-4],4); 
	  		n+=4;
		}
	}
	
	return pDst;
}

//RGBRGBRGB--->RRRGGGBBB
static unsigned char *rgb24ToPlanar(unsigned char *mem, int PixelNum)
{
	unsigned char *PlanarBuff=psd_malloc(PixelNum*3);
	unsigned char *RBuf,*GBuf,*BBuf;
	unsigned char *p = mem;
	int i = 0;
	
	RBuf = PlanarBuff;
	GBuf = PlanarBuff + PixelNum;
	BBuf = PlanarBuff + PixelNum + PixelNum;
	
	for (i=0; i<PixelNum; )       
	{			
		*(RBuf+i) =  *(p+i);
		*(GBuf+i) =  *(p+i+1);
		*(BBuf+i) =  *(p+i+2);
		i+=3;
	}
	return PlanarBuff;
}

/*
ARGB--->RGB buffer??
BGRA--->BGR buffer?? yes
*/
static unsigned char *argb2rgb(unsigned char *ArgbBuff,int PixelNum)
{
	int i=0,m,n;
	unsigned char *RgbBuff=psd_malloc(PixelNum*3);

	for (i=0; i<PixelNum; i++)
	{
		m = i*3;
		n = i<<2;
		*(RgbBuff + m + 0) = *(ArgbBuff + n + 2);
		*(RgbBuff + m + 1) = *(ArgbBuff + n + 1);
		*(RgbBuff + m + 2) = *(ArgbBuff + n + 0);
	}

	return RgbBuff;
}


/*
BGR--->BGRA buffer?? yes
*/
static
#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
unsigned char *rgb2bgra(unsigned char *rgb_Buf,int PixelNum)
{
	int i=0,m,n;
	unsigned char *Rgba_Buf=psd_malloc(PixelNum<<2);

	for (i=0; i<PixelNum; i++)
	{
		m = i<<2;
		n = i*3;
		*(Rgba_Buf + m + 0) = *(rgb_Buf + n + 2);
		*(Rgba_Buf + m + 1) = *(rgb_Buf + n + 1);
		*(Rgba_Buf + m + 2) = *(rgb_Buf + n + 0);
		*(Rgba_Buf + m + 3) = 0xFF;
	}

	return Rgba_Buf;
}


/*RGB
RGB--->ARGB buffer
*/
static
#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
unsigned char *rgb2argb(unsigned char *rgb_Buf,int PixelNum)
{
	int i=0,m,n;
	unsigned char *Argb_Buf=psd_malloc(PixelNum<<2);

	for (i=0; i<PixelNum; i++)
	{
		m = i<<2;
		n = i*3;
		*(Argb_Buf + m + 0) = 0xFF;		
		*(Argb_Buf + m + 1) = *(rgb_Buf + n + 0);
		*(Argb_Buf + m + 2) = *(rgb_Buf + n + 1);
		*(Argb_Buf + m + 3) = *(rgb_Buf + n + 2);
	}

	return Argb_Buf;
}

#if 0
#define RGB565_MASK_RED 0xF800 
#define RGB565_MASK_GREEN 0x07E0 
#define RGB565_MASK_BLUE 0x001F 

unsigned short *pRGB16 = (unsigned short *)lParam; 
for(int i=0; i<176*144; i++) 
	{     unsigned short RGB16 = *pRGB16; 
     	g_rgbbuf[i*3+2] = (RGB16&RGB565_MASK_RED) >> 11;   
	 g_rgbbuf[i*3+1] = (RGB16&
RGB565_MASK_GREEN) >> 5; 11.     g_rgbbuf[i*3+0] = (RGB16&RGB565_MASK_BLUE); 12.     g_rgbbuf[i*3+2] <<= 3; 13.     
g_rgbbuf[i*3+1] <<= 2; 14.     g_rgbbuf[i*3+0] <<= 3; 15.     pRGB16++; 16.} 另一种：

01.rgb5652rgb888(unsigned char *image,unsigned char *image888) 02.{ 03.unsigned char R,G,B; 04.B=(*image) & 0x1F;//
000BBBBB 05.G=( *(image+1) << 3 ) & 0x38 + ( *image >> 5 ) & 0x07 ;//得到00GGGGGG00 06.R=( *(image+1) >> 3 ) & 0x1F; 
//得到000RRRRR 07.*(image888+0)=B * 255 / 63; // 把5bits映射到8bits去，自己可以优化一下算法，下同 08.*(image888+1)=G 
* 255 / 127; 09.*(image888+2)=R * 255 / 63; 10.} 


#endif


/*
png requires width must be 4 times. so revise it.
*/
#if 0
static unsigned char *ReviseRgbBuff(int width,int height,unsigned char* pData,int *NewWidth)
{
	int nByteAlign;
	int nBmpSize,i,j;
	unsigned char* pTmp,*pSrc,*bmp_buf;
	
	if (width %4 != 0) 
		nByteAlign = 4 - ((width) % 4);
	else
		nByteAlign = 0;

	*NewWidth = width + nByteAlign;
	nBmpSize = (width*3 + nByteAlign) * height;

	bmp_buf = psd_malloc(nBmpSize);
	memset(bmp_buf,255,nBmpSize);
	
	for(i =0;  i < height; i++)
	{
		pTmp = bmp_buf+(i*(width*3+nByteAlign));
		pSrc = pData + (i*width*3);

		for (j=0 ; j<width; j++)
		{
			pTmp[0] = pSrc[0];
			pTmp[1] = pSrc[1];
			pTmp[2] = pSrc[2];
			pTmp += 3;
			pSrc += 3;
		}
		pTmp -= 3;
		for(j=0; j<nByteAlign; j++)
		{	
			pTmp+=1;
			pTmp[0] = 0;
		}
	}

	return bmp_buf;
}
#endif

/*
BGRA--->RGB, do flip. this func supports NOT 4x width.
*/
static unsigned char *PngImg2Rgb(int width,int height,unsigned char* pData)
{
	int nByteAlign;
	int nBmpSize,i,j;
	unsigned char* pDst,*pSrc,*bmp_buf;
	
	if (width %4 != 0) 
		nByteAlign = 4 - ((width*3L) % 4);
	else
		nByteAlign = 0;
	nBmpSize = (width*3 + nByteAlign) * height;

	bmp_buf = psd_malloc(nBmpSize);
	memset(bmp_buf,255,nBmpSize);
	
	pSrc = pData + ((height-1)*(width<<2));	//back to last line

	for(i=0; i<height; i++)
	{
		pDst = bmp_buf+(i*(width*3+nByteAlign)); //start from 1st line
				
		for (j=0; j<width; j++)
		{
			pDst[0] = pSrc[2];
			pDst[1] = pSrc[1];
			pDst[2] = pSrc[0];
			pDst += 3;
			pSrc += 4;
		}
		for(j=0; j<nByteAlign; j++)
		{	
			pDst[0] = 0;
			pDst++;
		}
		pSrc -= 2L*(width<<2);
	}

	return bmp_buf;
}

#if 0
//malloc a new memory block, keep previous memory block.
static unsigned char *ReviseRgbBuff(int width,int height,unsigned char* pData)
{
	int nByteAlign;
	int nBmpSize,i,j;
	unsigned char* pDst,*pSrc,*bmp_buf;
	
	if (width %4 != 0) 
		nByteAlign = 4 - ((width*3L) % 4);
	else
		nByteAlign = 0;

	nBmpSize = (width*3 + nByteAlign) * height;
	bmp_buf = psd_malloc(nBmpSize);
	memset(bmp_buf,255,nBmpSize);
	
	for(i=0; i<height; i++)
	{
		pSrc = pData + (i*(width*3));
		pDst = bmp_buf + (i*(width*3+nByteAlign));
		for (j=0; j<width; j++)
		{
			pDst[0] = pSrc[0];
			pDst++;
			pSrc++;
		}
		for(j=0; j<nByteAlign; j++)
		{	
			pDst[0] = 0;
			pDst++;
		}
	}
	
	return bmp_buf;
}
#endif 

/*
RGB--->BGRA, do flip and mirror.
*/
static unsigned char *Rgb2PngImg(int width,int height,unsigned char* pData)
{
	int i,j;
	unsigned char *pSrc,*pDst,*png_buf;
	int nByteAlign,size;
	if (width %4 != 0) 
		nByteAlign = 4 - ((width*3L)%4);
	else
		nByteAlign = 0;

	size = ((width<<2)+0)*height;
	png_buf = psd_malloc(size);
	//memset(png_buf,0,size);

	//pSrc = pData + (height-i)*(width*3+nByteAlign);
	for(i=0; i<height; i++)
	{
	//	pSrc = pData + (i*(width*3+nByteAlign);	//start from 1st line
	//	pDst = png_buf + ((width<<2)*(height-i));	
		pSrc = pData + (height-i)*(width*3+nByteAlign);
		pDst = png_buf + ((width<<2))*i;
		for (j=0; j<width; j++)
		{
			pDst[0] = pSrc[2];
			pDst[1] = pSrc[1];
			pDst[2] = pSrc[0];
			pDst[3] = 0xFF;		//alpha value
			pSrc += 3;
			pDst += 4;
		}
		for(j=0; j<nByteAlign; j++)
		{
			
		}
		//pSrc -= 2*(width*3+nByteAlign);
	}

	return png_buf;
}


/*
RGBA--->RGB, do flip.
*/
static unsigned char *PsdImg2Rgb(int width,int height,unsigned char* pData)
{
	int nByteAlign;
	int nBmpSize,i,j;
	unsigned char* pDst,*pSrc,*bmp_buf;
	
	if (width %4 != 0) 
		nByteAlign = 4 - ((width*3L) % 4);	//make sure bytes in one line are 4x
	else
		nByteAlign = 0;
	nBmpSize = (width*3 + nByteAlign) * height;

	bmp_buf = psd_malloc(nBmpSize);
	memset(bmp_buf,255,nBmpSize);
	
	pSrc = pData + ((height-1)*(width<<2));		//back to last line of pixel

	for(i=0;  i<height; i++)
	{
		pDst = bmp_buf+(i*(width*3+nByteAlign));
				
		for (j=0 ;j<width; j++)
		{
			pDst[0] = pSrc[2];	//B
			pDst[1] = pSrc[1];	//G
			pDst[2] = pSrc[0];	//R
			pDst += 3;
			pSrc += 4;	//Alpha is skipped
		}
		//pDst -= 3;
		for(j=0; j<nByteAlign; j++)
		{
			pDst[0] = 0;
			pDst++;
		}
		pSrc -= 2L*(width<<2);
	}

	return bmp_buf;
}


/*
RGB--->BGRA, do flip. BGRA is for psd using.
*/
static unsigned char *Rgb2PsdImg(int width,int height,unsigned char* pData)
{
	int i,j;
	unsigned char *pSrc,*pDst,*png_buf;
	int nByteAlign,size;
	if (width %4 != 0) 
		nByteAlign = 4 - ((width*3L)%4);
	else
		nByteAlign = 0;

	size = ((width<<2)+0)*height;
	png_buf = psd_malloc(size);
	//memset(png_buf,0,size);

	//pSrc = pData + (height-i)*(width*3+nByteAlign);
	for(i=0; i<height; i++)
	{
	//	pSrc = pData + (i*(width*3+nByteAlign);	//start from 1st line
	//	pDst = png_buf + ((width<<2)*(height-i));	
		pSrc = pData + (height-1-i)*(width*3+nByteAlign);
		pDst = png_buf + ((width<<2))*i;
		for (j=0; j<width; j++)
		{
			pDst[0] = pSrc[2];
			pDst[1] = pSrc[1];
			pDst[2] = pSrc[0];
			pDst[3] = 0xFF;		//alpha value
			pSrc += 3;
			pDst += 4;
		}
		for(j=0; j<nByteAlign; j++)
		{
			
		}
		//pSrc -= 2*(width*3+nByteAlign);
	}

	return png_buf;
}

/*
设置透明度，范围是［0，255］，0是全透明，255是不透明.
透明度，其它是针对该图层下面的图层来说的，是否允许下面的图层显现出来。
*/
static void PngAddTransparency(psd_uchar *PngImgBuf,int PixelNum,psd_uchar ucTransParency)
{
	int i=PixelNum;
	unsigned char *pTemp=PngImgBuf;

	while (--i)	//count pixel
	{
		pTemp[0] = (pTemp[0]*ucTransParency)/255;	//B byte
		pTemp[1] = (pTemp[1]*ucTransParency)/255;	//G byte
		pTemp[2] = (pTemp[2]*ucTransParency)/255;	//R byte
		pTemp +=4;	//skip alpha byte
	}
}

/*
y =   0.299 * (r - 128) + 0.587 * (g - 128) + 0.114 * (b - 128) + 128 = 0.299*r + 0.587*g+ 0.114*b;
u = - 0.147 * (r - 128) - 0.289 * (g - 128) + 0.436 * (b - 128) + 128 = - 0.147*r - 0.289*g + 0.436*b+ 128;
v =   0.615 * (r - 128) - 0.515 * (g - 128) - 0.100 * (b - 128) + 128 = 0.615*r - 0.515*g - 0.100*b+ 128;
char Brightness: [-100,+100] like photoshop bright adjust
在调整亮度和对比度时，只要保证像素灰度不饱和，就可以实现可逆的完全无损的调节。
所有象素rgb分量都要介于0和255当有值为0的点的分量出现时如果再调黑就是不可逆的了。
*/
static void PngSetBrightness(unsigned char *PngImgBuf,int PixelNum,signed char Brightness)
{
	int i=PixelNum;
	unsigned char *pTemp=PngImgBuf;
	//unsigned char r=0x104,g=0x215,b=0x100;  gcc will just cut the high byte, 0x215 will be 0x15, stupid!!!
	unsigned short r,g,b;

	while (--i)	//count pixel
	{
		r = ((pTemp[0]*(255+Brightness))/255);	//B byte
		g = ((pTemp[1]*(255+Brightness))/255);	//G byte
		b = ((pTemp[2]*(255+Brightness))/255);	//R byte
		//r = pTemp[0]+Brightness;	//B byte
		//g = pTemp[1]+Brightness;	//G byte
		//b = pTemp[2]+Brightness;	//R byte

		if ((r<255)&&(g<255)&&(b<255))
		{
//			pTemp[0] = 0x110;		pTemp[0] = 0x10,??yes
			pTemp[0] = r;
			pTemp[1] = g;
			pTemp[2] = b;
		}
		
		pTemp +=4;	//skip alpha byte
	}

}

/*
inline void IncreaseContrast(BYTE *pByte, const int Low, const int High, const float Grad)
{
    if(*pByte <= Low)
    {
        *pByte = 0;
    }
    else if((Low < *pByte) && (*pByte <  High))
    {
        *pByte = (BYTE)( (*pByte - Low) / Grad);
    }
    else
    {  // pElem->rgbGreen >= High
        *pByte = 255;
    }

}

inline void DecreaseContrast(BYTE *pByte, const int Level, const float Grad)
{
    *pByte = (BYTE) ( ((int) (*pByte / Grad)) - Level);
}
*/

/*
对比度定义是：在暗室中，白色画面(最亮时)下的亮度除以黑色画面(最暗时)下的亮度。
因此白色越亮、黑色越暗，对比度就越高。
signed char Contrast, [-100,+100]
对比度的含义就是，一幅画，保持平均亮度不变。使亮的更亮，暗的更暗（对比度增加），或 使亮的更暗，暗的更亮（对比度减少）。
所以算法就是：1、计算平均亮度
              2、每点与平均亮度比较，得到差值。
              3、新的亮度 = 此点的亮度 + 系数 * 此点的亮度 * 差值
              4、根据新的亮度，计算出新的rgb(保持色调不变)


*/
static void PngSetContrast(unsigned char *PngImgBuf,int PixelNum,signed char Contrast)
{
	int Low=0,High=255,average=128;
//	float Grad;
	int y,u,v;
	int r,g,b;
	int i=PixelNum,pixel32;
	unsigned char *pTemp=PngImgBuf;

	if (Contrast >= 0)	//more light, more dark
	{
		while (--i)	//count pixel
		{
			r=pTemp[0];g=pTemp[1];b=pTemp[2];
			
			pixel32 = convert_rgb_to_yuv_pixel(r,g,b);
			y = (pixel32 & 0x000000ff);
			u = (pixel32 & 0x0000ff00) >>  8;
			v = (pixel32 & 0x00ff0000) >> 16;
			
			if (y>average)
			{
				y = y+ (High-y)*Contrast/100;
			}
			else
			{
				y = y-(y-Low)*Contrast/100;	
			}
			pixel32=convert_yuv_to_rgb_pixel(y,u,v);
			r= (pixel32 & 0x000000ff);
			g= (pixel32 & 0x0000ff00) >> 8;
			b= (pixel32 & 0x00ff0000) >> 16;
		
			pTemp[0] = r;	pTemp[1] = g;pTemp[2] = b;
			
			pTemp +=4;	//skip alpha byte
		}		
	}
	else		//less light,less dark
	{
		Contrast = -Contrast;
		
		while (--i)	//count pixel
		{
			r=pTemp[0];g=pTemp[1];b=pTemp[2];
			pixel32 = convert_rgb_to_yuv_pixel(r,g,b);
			y = (pixel32 & 0x000000ff);
			u = (pixel32 & 0x0000ff00) >>  8;
			v = (pixel32 & 0x00ff0000) >> 16;			

			if (y>average)
			{
				y = y - (y-average)*Contrast/100;
			}
			else
			{
				y = y + (average-y)*Contrast/100;	
			}
			pixel32=convert_yuv_to_rgb_pixel(y,u,v);
			r= (pixel32 & 0x000000ff);
			g= (pixel32 & 0x0000ff00) >> 8;
			b= (pixel32 & 0x00ff0000) >> 16;
			
			pTemp[0] = r;	pTemp[1] = g;pTemp[2] = b;
			
			pTemp +=4;	//skip alpha byte
		}
	}

	return;
}

//channel is 4, for rgba buffer, png. NOW we only support 50% or 200%
static
#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
int ZoomImg(unsigned char *SrcPng,unsigned int SrcWidth, unsigned int SrcHeight
	,unsigned char *DstPng,unsigned int DstWidth, unsigned int DstHeight)
{
	int i,j;
//	int DivPixel_w,DivPixel_h;
	unsigned char *pSrc=SrcPng,*pDst=DstPng;

	if (DstWidth==SrcWidth)		// no zoom
	{
		memcpy(DstPng,SrcPng,DstWidth*(DstHeight<<2));	
		return 0;
	}
	else if (DstWidth==(SrcWidth<<1))		//zoom in
	{
		memset(DstPng,0xFF,DstWidth*DstHeight*4);	//init

		for (i=0; i<SrcHeight; i++)	
		{
			for (j=0; j<SrcWidth; j++)	
			{
				pDst[0] = pSrc[0];pDst[1] = pSrc[1];pDst[2] = pSrc[2];pDst[3] = pSrc[3];
				pDst+=4; 
				pDst[0] = pSrc[0];pDst[1] = pSrc[1];pDst[2] = pSrc[2];pDst[3] = pSrc[3];
				pDst+=4; 
				pSrc+=4;
			}

			memcpy(pDst,pDst-(DstWidth<<2),DstWidth<<2);
			pDst+=(DstWidth<<2);
		}
	}
	else	 if (DstWidth==(SrcWidth>>1))	//zoom out
	{
		memset(DstPng,0xFF,DstWidth*(DstHeight<<2));	//init
		
		for (i=0; i<SrcHeight; i+=2)	
		{
			pSrc+=(SrcWidth<<2);	//skip odd line
			
			if ((i+1)==SrcHeight)
					break;
			
			for (j=0; j<SrcWidth; j+=2)	
			{
				pSrc+=4;	//skip odd pixel
				
				if ((j+1)==SrcWidth)
					break;
				pDst[0] = pSrc[0];pDst[1] = pSrc[1];pDst[2] = pSrc[2];pDst[3] = pSrc[3];
				pDst+=4; 
				pSrc+=4;	
			}
		}
	}
	else
		;		//do nothing

	return 0;		//success
}


//channel is 4, for rgba buffer, png. 
static int CropImg(unsigned char *SrcPng,unsigned int SrcWidth, unsigned int SrcHeight
	,unsigned char *DstPng,unsigned int DstWidth, unsigned int DstHeight)
{
	int i,j;
	unsigned char *pSrc=SrcPng,*pDst=DstPng;

	if (DstWidth==SrcWidth)		// cut some lines
	{
		memcpy(DstPng,SrcPng,DstWidth*(DstHeight<<2));	
		return 0;
	}
	else 
	{
		memset(DstPng,0xFF,DstWidth*(DstHeight<<2));	//init

		for (i=0; i<DstHeight; i++)	
		{
			for (j=0; j<DstWidth; j++)	
			{
				pDst[0] = pSrc[0];pDst[1] = pSrc[1];pDst[2] = pSrc[2];pDst[3] = pSrc[3];
				pDst+=4; 
				pSrc+=4;
			}

			//memcpy(pDst,pDst-(DstWidth<<2),DstWidth<<2);
			pSrc+=((SrcWidth-DstWidth)<<2);
		}
	}

	return 0;		//success
}

//width of bmp buffer should be a multiple of 4. if NOT, error will be happened.
int jpg2bmp(char *fJpgName, char *fBmpName)
{
	int ret=-1;
	int width,height;
	unsigned char *pBuff;

	pBuff = jpg_load(fJpgName,&width,&height);
	if (pBuff)
	{
		printf("Load %s successfully!\n",fJpgName);
		swap_rgb24(pBuff,width*height);			//BGR-->RGB
		ret=bmp_save(fBmpName,pBuff,width,height,24);
		psd_free(pBuff);
	}

	return ret;
}

//width of bmp buffer should be a multiple of 4. if NOT, error will be happened.
int bmp2jpg(char *fBmpName,char *fJpgName)
{
	int ret=-1;
 	BMP_HEADER *pHeader;
 	unsigned char *pBuff;		
	int width,height;

	ret = bmp_load(fBmpName,&pHeader,&pBuff);
	width=pHeader->imgwidth;
	height=pHeader->imgheight;

	if (ret>=0)
	{
		printf("Load %s successfully!\n",fBmpName);
		swap_rgb24(pBuff,width*height);			//RGB-->BGR
		Rgb24_flip(pBuff,width,height);
		//Rgb24_mirror(pBuff,width,height);
		ret=jpg_save(fJpgName,pBuff,width,height,80);
		psd_free(pBuff);
	}

	return ret;
}

/*

*/
int bmp2bmp(char *fSrcBmp,char *fDstBmp)
{
	int ret=-1;
 	BMP_HEADER *pHeader;
 	unsigned char *pBuff,*yuv;
	int width,height;

//	int nPad;	//pad pixel or pad byte
//	int nBmpSize,i,j;
//	unsigned char* pDst,*pSrc,*bmp_buf;
	
	ret = bmp_load(fSrcBmp,&pHeader,&pBuff);
	width=pHeader->imgwidth;
	height=pHeader->imgheight;

	if (ret>=0)
	{
		printf("Load %s successfully!\n",fSrcBmp);
/*
		if ((width%4) != 0)
			nPad = 4 - (width%4);
		else
			nPad = 0;
		nBmpSize = (width+nPad)*3*height;
		bmp_buf = psd_malloc(nBmpSize);
		memset(bmp_buf,0,nBmpSize);
		
		for(i=0; i<height; i++)
		{
			pSrc = pBuff + (i*3*width);
			pDst = bmp_buf+(i*3*(width+nPad)); //start from 1st line

			for (j=0; j<(width*3); j++)	//COUNT pixel number
			{
				pDst[0] = pSrc[0];
				pDst+= 1;
				pSrc+= 1;
			}
		}
*/
	//	Rgb24_flip(pBuff,width,height);
		
		yuv = malloc(width * (height<<1));
		convert_rgb_to_yuv_buffer(pBuff, yuv, width, height);
		convert_yuv_to_rgb_buffer(yuv, pBuff, width, height);	

		//Rgb24_mirror(pBuff,width,height);
		ret=bmp_save(fDstBmp,pBuff,width,height,24);	//RGB
		psd_free(pBuff);
	}

	return ret;
}

int bmp2png(char *fBmpName,char *fPngName)
{
	int ret=-1;
 	BMP_HEADER *pHeader;
 	unsigned char *pBuff,*ImgBuf;		
	int width,height;

	ret = bmp_load(fBmpName,&pHeader,&pBuff);
	width=pHeader->imgwidth;
	height=pHeader->imgheight;

	if (!ret)
	{
		printf("Load %s successfully!\n",fBmpName);

		ImgBuf = Rgb2PngImg(width,height,pBuff);			//RGB-->BGRA, flip/mirror
		Rgba32_flip(ImgBuf,width,height);
		Rgba32_mirror(ImgBuf,width,height);
		ret=png_save(fPngName,ImgBuf,width,height);
		psd_free(pBuff);
		psd_free(ImgBuf);
	}
	else
		printf("Fail to load %s !!!\n",fBmpName);

	return ret;
}


int png2bmp(char *fPngName, char *fBmpName)
{
	int ret=-1;
	int width,height;
	unsigned char *pBuff;
	unsigned char *RgbBuff;

	pBuff = png_load(fPngName,&width,&height);		//get BGRA buffer
	if (pBuff)
	{
		printf("Load %s successfully!\n",fPngName);
		
		//RgbBuff = argb2rgb(pBuff,width*height);		//BGRA--->RGB, error here for flip/mirror reason
		RgbBuff = PngImg2Rgb(width,height,pBuff);
		ret=bmp_save(fBmpName,RgbBuff,width,height,24);
		psd_free(pBuff);
		psd_free(RgbBuff);
	}
	else
		printf("Fail to load %s !!!\n",fPngName);

	return ret;
}


int  pngcrop(char *fPngName, int width, int height)
{
	int ret=-1;
	int Src_w=0xFFFF,Src_h=0xFFFF,Dst_w=width,Dst_h=height;
	unsigned char *pSrcPng,*pDstPng;
		
	pSrcPng = png_load(fPngName,&Src_w,&Src_h);		//get BGRA buffer
	if (pSrcPng)
	{
		//printf("Load %s successfully!\n",fPngName);
		if (width>Src_w)
			Dst_w=Src_w;
		if (height>Src_h)
			Dst_h=Src_h;

		pDstPng=malloc(Dst_w*(Dst_h<<2));

		CropImg(pSrcPng,Src_w,Src_h,pDstPng,Dst_w,Dst_h);
	
		png_save(fPngName,pDstPng,Dst_w,Dst_h);
		psd_freeif(pSrcPng);
		psd_freeif(pDstPng);
		ret=0;
	}
	else
	{
		printf("Fail to load %s !!!\n",fPngName);
		ret=-2;
	}

	return ret;
}


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
	if (ret>=0)
	{
		pngzoom("..\\pic\\tianye.png",50);
		PsdNewLayer(fHandle,"..\\pic\\tianye.png",20,80,1,155);
		PsdSaveFile(fHandle);
		PsdCloseFile(fHandle);		//must close firstly before load again...
	}
	
*/
DLLEXPORT int  pngzoom(char *fPngName, unsigned char ucZoomRate)
{
	int ret=-1;
	int Src_w,Src_h,Dst_w,Dst_h;
	unsigned char *pSrcPng,*pDstPng;
/*
	char fPngNewName[256];
	int len;

	len = strlen(fPngName);
	if (len<5)
		return -2;
	if ((((fPngName[len-3]=='P')) && ((fPngName[len-2]=='N')) && ((fPngName[len-1]=='G')))
	|| (((fPngName[len-3]=='p')) && ((fPngName[len-2]=='n')) && ((fPngName[len-1]=='g'))))
	{
		strncpy(fPngNewName,fPngName,len-4);
		strcat(fPngNewName,"_tmp.png");		//merge two string as one			
	}
	else
	{
		return -3;
	}
*/
	if ((ucZoomRate !=50) && (ucZoomRate !=200) && (ucZoomRate !=100))
	{
		return -1;		//do NOT support
	}	
	pSrcPng = png_load(fPngName,&Src_w,&Src_h);		//get BGRA buffer
	if (pSrcPng)
	{
//		printf("Load %s successfully!\n",fPngName);
		
		Dst_w = Src_w*ucZoomRate/100;
		Dst_h= Src_h*ucZoomRate/100;
		pDstPng=malloc(Dst_w*(Dst_h<<2));

		ZoomImg(pSrcPng,Src_w,Src_h,pDstPng,Dst_w,Dst_h);
		png_save(fPngName,pDstPng,Dst_w,Dst_h);
		psd_free(pSrcPng);
		psd_free(pDstPng);
		ret=0;
	}
	else
	{
		printf("Fail to load %s !!!\n",fPngName);
		ret=-2;
	}

	return ret;
}



/*
函数原型	
功能说明	transform psd to jpg(quality=80) format and save it.
入口参数	
出口参数	无
返回值		0:success, other negative value:error code
调用事项	
*/
DLLEXPORT int Psd2Jpg(char *fPsdFile,char *fJpgFile)
{
	int ret=-1;
	psd_context *context = NULL;
	int width,height;
	unsigned char *psd_buf,*bmp_buf;
	psd_status status;

	if ((!fPsdFile)||(!fJpgFile))
		return -1;	//input para error
	
	status = psd_image_load(&context, fPsdFile);		//file will keep open...
	if (status == psd_status_done)
	{
		psd_buf = (unsigned char*)context->merged_image_data;
		width = context->width;
		height = context->height;
		bmp_buf = PsdImg2Rgb(width,height,psd_buf);
		
		swap_rgb24(bmp_buf,width*height);
		Rgb24_flip(bmp_buf,width,height);
		
		ret=jpg_save(fJpgFile,bmp_buf,width,height,80);
		psd_free(bmp_buf);
	}
	else		//fail to read
	{
		ret = status;
	}

	psd_image_free(context);
	return ret;
}



/*
函数原型	
功能说明	transform psd to bmp(bpp=24) format and save it.
入口参数	
出口参数	无
返回值		0:success, other negative value:error code
调用事项	
*/
DLLEXPORT int Psd2Bmp(char *fPsdFile,char *fBmpFile)
{
	int ret=-1;
	psd_context *context = NULL;
	int width,height;
	unsigned char *psd_buf,*bmp_buf;
	psd_status status;

	if ((!fPsdFile)||(!fBmpFile))
		return -1;	//input para error
	
	status = psd_image_load(&context, fPsdFile);		//file will keep open...
	if (status == psd_status_done)
	{
		psd_buf = (unsigned char*)context->merged_image_data;
		width = context->width;
		height = context->height;
		bmp_buf = PsdImg2Rgb(width,height,psd_buf);
		ret = bmp_save(fBmpFile,bmp_buf,width,height,24);	//only support 24 depth
		psd_free(bmp_buf);
	}
	else		//fail to read
	{
		ret = status;
	}

	psd_image_free(context);
	return ret;
}

/*
函数原型	
功能说明	transform .psd to .png format.
入口参数	
出口参数	无
返回值		0:success, other negative value:error code
调用事项	
*/
DLLEXPORT int  Psd2Png(char *fPsdFile,char *fPngFile)
{
	int ret=-1;
	psd_context *context = NULL;
	int width,height;
	unsigned char *psd_buf,*Png_buf;
	psd_status status;
//	int NewWidth,NewHeight;

	if ((!fPsdFile)||(!fPngFile))
		return -1;	//input para error
		
//	printf("input psdfilename is:%s, pngfilename is:%s\n",fPsdFile,fPngFile);
		
	status = psd_image_load(&context, fPsdFile);		//file will keep open...
	if (status == psd_status_done)
	{
		psd_buf = (unsigned char*)context->merged_image_data;
		width = context->width;
		height = context->height;
		
		//swap_rgba32(psd_buf,width*height);	//why psd to png need swap R and B?? confuse
		//PngAddTransparency(psd_buf,width*height,60);
		//PngSetBrightness(psd_buf,width*height,-128);
		//PngSetContrast(psd_buf,width*height,0);

		ret = png_save(fPngFile,psd_buf,width,height);

	/*		
		NewWidth = width*0.5;
		NewHeight = height*0.5;
		Png_buf=malloc(NewWidth*NewHeight*4);
		ZoomImg(psd_buf,width,height,Png_buf,NewWidth,NewHeight);
		ret = png_save(fPngFile,Png_buf,NewWidth,NewHeight);
	*/
	}
	else		//fail to read
	{
		ret = status;
	}

	psd_image_free(context);
	return ret;
}

static unsigned long fGetSize(char *file_name)
{
	unsigned long offset, size;
	FILE *file=fopen(file_name, "rb+");

	if (file==NULL)
		return 0;

	offset = ftell(file);
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	fseek(file, offset, SEEK_CUR);

	fclose(file);
	return size;
}


static int CreateCanvas(int width,int height)
{
	int ret=-1;
	unsigned char *png_buf;
	int size;

	size = (width<<2)*height;
	png_buf = psd_malloc(size);
	if (png_buf)
	{
		memset(png_buf,0xFF,size);
		ret=png_save("canvas.png",png_buf,width,height);
	}
	
	return ret;
}


//could we use inline?? LayerTransparency: 0:see through, 255:opacity
/*
color_channels=3,width=637,height=461,depth=8,color_mode=3
layer0(top:0,left:0,width=637,height=461,transpancy=255),ch_len=293657,ch_num=3
layer1(top:159,left:158,width=242,height=182,transpancy=255),ch_len=44044,ch_num=4
layer2(top:272,left:38,width=184,height=138,transpancy=255),ch_len=25392,ch_num=4
layer3(top:0,left:0,width=637,height=461,transpancy=255),ch_len=293657,ch_num=4
layer4(top:161,left:0,width=315,height=249,transpancy=255),ch_len=78435,ch_num=4
layer5(top:213,left:254,width=256,height=256,transpancy=255),ch_len=65536,ch_num=4
layer6(top:0,left:38,width=220,height=337,transpancy=255),ch_len=74140,ch_num=4
layer7(top:113,left:187,width=426,height=319,transpancy=158),ch_len=135894,ch_num=4
layer8(top:0,left:0,width=637,height=461,transpancy=255),ch_len=293657,ch_num=4
layer9(top:130,left:29,width=239,height=239,transpancy=255),ch_len=57121,ch_num=4
layer10(top:185,left:233,width=215,height=215,transpancy=255),ch_len=46225,ch_num=4
layer11(top:272,left:362,width=278,height=185,transpancy=255),ch_len=51430,ch_num=4
layer12(top:69,left:541,width=72,height=23,transpancy=255),ch_len=1656,ch_num=4
global layer mask length is:0
begin to merge layer...layer count=13
NOTE: only support channel=4, error will occur if channels=3

Func: Layer image is on the top of background image.
*/	
static
#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
void  MergeLayer1(unsigned char *pLayerBuf,int LayerWidth,int LayerHeight,
	unsigned char *bgBuf,int bgWidth,int bgHeight,
	int offx,int offy,psd_uchar LayerTransparency)
{
	psd_uchar *pImage=NULL,*pLayer=NULL;
	psd_uchar ImgA,ImgB,ImgG,ImgR,LayA,LayB,LayG,LayR;
	int x,y,ImgOff,LayOff;

	for (y=MAX(offy,0); y<MIN(bgHeight,(LayerHeight+offy)); y++)	//line
	{
		pImage = bgBuf+(y<<2)*bgWidth;
		pLayer = pLayerBuf+((y-offy)<<2)*LayerWidth;
		
		for (x=MAX(offx,0); x<MIN(bgWidth,(LayerWidth+offx)); x++)	//column
		{
			ImgOff=x<<2; LayOff=(x-offx)<<2;
			ImgA=pImage[ImgOff];ImgB=pImage[ImgOff+1];
			ImgG=pImage[ImgOff+2];ImgR=pImage[ImgOff+3];
			LayA=pLayer[LayOff];LayB=pLayer[LayOff+1];
			LayG=pLayer[LayOff+2];LayR=pLayer[LayOff+3];			

			pImage[ImgOff] = (ImgA*(255-LayerTransparency)
				+LayA*LayerTransparency)>>8;
			pImage[ImgOff+1] = (ImgB*(255-LayerTransparency)
				+LayB*LayerTransparency)>>8;
			pImage[ImgOff+2] = (ImgG*(255-LayerTransparency)
				+LayG*LayerTransparency)>>8;
			pImage[ImgOff+3] = (ImgR*(255-LayerTransparency)
				+LayR*LayerTransparency)>>8;
		}
	}

}


//could we use inline?? LayerTransparency: 0:see through, 255:opacity
/*
layer count=5
layer0(top:0,left:0,width=2400,height=1800,transpancy=255),ch_len=4320000,ch_num=4
layer1(top:0,left:-4,width=2505,height=1915,transpancy=255),ch_len=4797075,ch_num=4
layer2(top:0,left:-4,width=2505,height=1915,transpancy=255),ch_len=4797075,ch_num=4
layer3(top:0,left:-4,width=2505,height=1915,transpancy=255),ch_len=4797075,ch_num=4
layer4(top:28,left:426,width=1772,height=1774,transpancy=255),ch_len=3143528,ch_num=4

*/	
static
#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
void MergeLayer2(unsigned char *pLayerBuf,int LayerWidth,int LayerHeight,
	unsigned char *bgBuf,int bgWidth,int bgHeight,
	int offx,int offy,psd_uchar LayerTransparency)
{
	psd_uchar *pImage=NULL,*pLayer=NULL;
	psd_uchar ImgA,ImgB,ImgG,ImgR,LayA,LayB,LayG,LayR;
	int x,y,ImgOff,LayOff;
	int a,b,g,r;

	for (y=MAX(offy,0); y<MIN(bgHeight,(LayerHeight+offy)); y++)	//line
	{
		pImage = bgBuf+(y<<2)*bgWidth;
		pLayer = pLayerBuf+((y-offy)<<2)*LayerWidth;
		
		for (x=MAX(offx,0); x<MIN(bgWidth,(LayerWidth+offx)); x++)	//column
		{
			ImgOff=x<<2; LayOff=(x-offx)<<2;
			ImgA=pImage[ImgOff];ImgB=pImage[ImgOff+1];
			ImgG=pImage[ImgOff+2];ImgR=pImage[ImgOff+3];
			LayA=pLayer[LayOff];LayB=pLayer[LayOff+1];
			LayG=pLayer[LayOff+2];LayR=pLayer[LayOff+3];			

			pImage[ImgOff] = (LayA*(255-LayerTransparency)
				+ImgA*LayerTransparency)>>8;
			pImage[ImgOff+1] = (LayB*(255-LayerTransparency)
				+ImgB*LayerTransparency)>>8;
			pImage[ImgOff+2] = (LayG*(255-LayerTransparency)
				+ImgG*LayerTransparency)>>8;
			pImage[ImgOff+3] = (LayR*(255-LayerTransparency)
				+ImgR*LayerTransparency)>>8;
		}
	}

}

static
#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
void MergeLayer3(unsigned char *pLayerBuf,int LayerWidth,int LayerHeight,
	unsigned char *bgBuf,int bgWidth,int bgHeight,
	int offx,int offy,psd_uchar LayerTransparency)
{
	psd_uchar *pImage=NULL,*pLayer=NULL;
	psd_uchar ImgA,ImgB,ImgG,ImgR,LayA,LayB,LayG,LayR;
	int x,y,ImgOff,LayOff;
	int a,b,g,r;

	for (y=MAX(offy,0); y<MIN(bgHeight,(LayerHeight+offy)); y++)	//line
	{
		pImage = bgBuf+(y<<2)*bgWidth;
		pLayer = pLayerBuf+((y-offy)*3)*LayerWidth;
		
		for (x=MAX(offx,0); x<MIN(bgWidth,(LayerWidth+offx)); x++)	//column
		{
			ImgOff=x<<2; LayOff=(x-offx)*3;
			ImgA=pImage[ImgOff];ImgB=pImage[ImgOff+1];
			ImgG=pImage[ImgOff+2];ImgR=pImage[ImgOff+3];
			LayA=0xFF;LayB=pLayer[LayOff];
			LayG=pLayer[LayOff+1];LayR=pLayer[LayOff+2];

			pImage[ImgOff] = ImgA+LayA;
			pImage[ImgOff+1] = (LayB+ImgB);
			pImage[ImgOff+2] = (LayG+ImgG);
			pImage[ImgOff+3] = (LayR+ImgR);
		}
	}

}



int PngAddBmp(char *fBmpName1,char *fPngName2)
{
	int width1,height1,width2,height2;
	unsigned char *pPng1Buf=NULL,*pPng2Buf=NULL,*ImgBuf;
	int ret=-1;
 	BMP_HEADER *pHeader;
 	
	ret = bmp_load(fBmpName1,&pHeader,&pPng1Buf);
	width1=pHeader->imgwidth;
	height1=pHeader->imgheight;
	ImgBuf = Rgb2PngImg(width1,height1,pPng1Buf);			//RGB-->BGRA, flip/mirror
	Rgba32_flip(ImgBuf,width1,height1);
	Rgba32_mirror(ImgBuf,width1,height1);
	
	pPng2Buf = png_load(fPngName2,&width2,&height2);

	MergeLayer3(pPng1Buf,width1,height1,pPng2Buf,width2,height2,0,0,255);
		
	ret = png_save("png_merged.png",pPng2Buf,width2,height2);
	
	return ret;
}


int PngMerge(char *fPngName1,char *fPngName2)
{
	int width1,height1,width2,height2;
	unsigned char *pPng1Buf=NULL,*pPng2Buf=NULL;
	int ret=-1;
	
	pPng1Buf = png_load(fPngName1,&width1,&height1);
	if (!pPng1Buf)
	{
		return -1;
	}

	pPng2Buf = png_load(fPngName2,&width2,&height2);
	if (!pPng2Buf)
	{
		psd_freeif(pPng1Buf);
		return -2;
	}

	MergeLayer3(pPng1Buf,width1,height1,pPng2Buf,width2,height2,0,0,255);
		
	ret = png_save("png_merged.png",pPng2Buf,width2,height2);
	
	return ret;
}

int MergeTwoPngFile(char *fPngName1,char *fPngName2
		,char *fMergedName,psd_uchar ucTransparency)
{
	int FrameWidth=1280,FrameHeight=1024,width1,height1,width2,height2;
	//FILE *fp1,fp2;
	unsigned char *pPng1Buf=NULL,*pPng2Buf=NULL,*DstPngBuf=NULL;
//	unsigned char *pDst,*p1,*p2;
	int ret=-1;
	int x,y,offx1=0,offy1=0,offx2=10,offy2=0;
	
	pPng1Buf = png_load(fPngName1,&width1,&height1);

	if (!pPng1Buf)
	{
		return -1;
	}

	pPng2Buf = png_load(fPngName2,&width2,&height2);
	if (!pPng2Buf)
	{
		psd_freeif(pPng1Buf);
		return -2;
	}

	if ( (width1>FrameWidth)||(width2>FrameWidth)
		||(height1>FrameHeight)||(height2>FrameHeight) )
	{
			printf("file size is too big to fit in.\n");
			ret = -3;
			goto Exit1;
	}
	else
	{
		DstPngBuf = psd_malloc(FrameWidth*FrameHeight*4);
		if (DstPngBuf == NULL)
		{
			ret = -4;
			goto Exit1;
		}

		memset(DstPngBuf,0x00,FrameWidth*FrameHeight*4); // 0:nothing, 0xFF:obsolete

		/*
			for (y=offy1; y<min(FrameHeight,(height1+offy1)); y++)
			{
				pDst = DstPngBuf+(y<<2)*FrameWidth;
				p1 = pPng1Buf+((y-offy1)<<2)*width1;
				for (x=offx1; x<min(FrameWidth,(width1+offx1)); x++)
				{
					pDst[x<<2] += p1[(x-offx1)<<2];
					pDst[(x<<2)+1] += p1[((x-offx1)<<2)+1];
					pDst[(x<<2)+2] += p1[((x-offx1)<<2)+2];
					pDst[(x<<2)+3] = 0xFF;
				}
			}
		*/
		MergeLayer1(pPng1Buf,width1,height1,DstPngBuf,FrameWidth,FrameHeight,offx1,offy1,255);
		/*
			for (y=offy2; y<min(FrameHeight,(height2+offy2)); y++)
			{
				pDst = DstPngBuf+(y<<2)*FrameWidth;
				p2 = pPng2Buf+((y-offy2)<<2)*width2;
				for (x=offx2; x<min(FrameWidth,(width2+offx2)); x++)
				{
					pDst[x<<2] = (pDst[x<<2]*(255-ucTransparency)
						+p2[(x-offx2)<<2]*ucTransparency)/255;
					pDst[(x<<2)+1] = (pDst[(x<<2)+1]*(255-ucTransparency)
						+p2[((x-offx2)<<2)+1]*ucTransparency)/255;
					pDst[(x<<2)+2] = (pDst[(x<<2)+2]*(255-ucTransparency)
						+p2[((x-offx2)<<2)+2]*ucTransparency)/255;
					pDst[(x<<2)+3] = (pDst[(x<<2)+3]*(255-ucTransparency) 
						+ p2[((x-offx2)<<2)+3]*ucTransparency)/255;
				}
			}
		*/

		MergeLayer1(pPng2Buf,width2,height2,DstPngBuf,FrameWidth,FrameHeight,offx2,offy2,ucTransparency);
			
		ret = png_save(fMergedName,DstPngBuf,FrameWidth,FrameHeight);
	
	}

Exit1:
	psd_freeif(DstPngBuf);
	psd_freeif(pPng1Buf);
	psd_freeif(pPng2Buf);	
	return ret;
}

//create a psd file without any layer(header and merged data section),it's the very simple psd file.
int CreateDemoPsdFile(char *fDemoPsdName,char *fAnyPngFile)
{
	psd_context context;
	void *fp=NULL;
	int ret=-1;
	unsigned char *pPngBuf=NULL;
	int width,height;

	fp = (void *)fopen(fDemoPsdName,"wb+");
	if (fp == NULL)		//fail to open
	{
	 	return -1;		//open file error
	}

	pPngBuf = png_load(fAnyPngFile,&width,&height);		//get BGRA buffer, flip
	if (!pPngBuf)
	{
		return -2;
	}	
	
	context.width = width;
	context.height = height;
	context.depth = 8;		//NOT 24, bpp per channel
	context.color_mode = psd_color_mode_rgb; //??
	context.channels = 4;	//add alpha channel.
	context.color_channels=3;	//rgba
	context.alpha_channels = 1;
	context.temp_image_data = pPngBuf;
	context.per_channel_length = width*height;
	psd_set_file_header(&context,fp);	//section 1
	printf("fp position is on:%d\n",psd_ftell(fp));
	
	context.color_map_length = 0;
	psd_set_color_mode_data(&context,fp);	//section 2
	printf("fp position is on:%d\n",psd_ftell(fp));

	context.ImageResLen = 0;
	psd_set_image_resource(&context,fp);	//section 3
	printf("fp position is on:%d\n",psd_ftell(fp));

	context.LayerandMaskLen = 0;
	psd_set_layer_and_mask(&context,fp);	//section 4
	printf("fp position is on:%d\n",psd_ftell(fp));

	context.MergedImageLen = context.width * context.height * context.channels;		//depth must be 8
	psd_set_image_data(&context,fp);		//section 5
	printf("fp position is on:%d\n",psd_ftell(fp));

	fclose(fp);
	psd_freeif(pPngBuf);
	return 0;
}	

//create a psd file having one layer(header and merged data section),it's the very simple psd file.
static void CreateBackgroundPsdFile(psd_context *context,char *PsdFileName,
	int width,int height,unsigned char *pRgbaBuf)
{
	psd_uint i=0,LayerCnt=1;
	char *pFileName=NULL;
	psd_layer_record *layer=NULL;
	psd_channel_info *channel_info=NULL;

	//pFileName = (char *)psd_malloc(128);
	//context->file_name = pFileName;
	//strcpy(pFileName,PsdFileName);
	
	context->file_name = PsdFileName;	//do not use strcpy.
	layer = (psd_layer_record *)psd_malloc(sizeof(psd_layer_record)*LayerCnt);
	context->layer_records = layer;
	
	//init
	//context->color_channels = context->channels - context->alpha_channels;

	context->width = width;
	context->height = height;
	context->depth = 8;		//NOT 24, bpp per channel
	context->color_mode = psd_color_mode_rgb; //??
	context->channels = 4;	//add alpha channel.
	context->color_channels=3;	//rgba
	context->alpha_channels = 1;
	//psd_set_file_header(context,context->file);	//section 1
	
	context->color_map_length = 0;
	context->color_map=NULL;	
	//psd_set_color_mode_data(context,context->file);	//section 2

	context->ImageResLen = 0;
	//psd_set_image_resource(context,context->file);	//section 3

	//prepare for lay and layer mask info section
	context->layer_count = LayerCnt;	//merged data is as one layer
	layer->per_channel_length = width*height;	//???
	layer->layer_type = psd_layer_type_normal;
	layer->top = 0;
	layer->left = 0;
	layer->right = width;
	layer->bottom = height;
	layer->width = width;
	layer->height = height;
	layer->number_of_channels = 4;	// only  support ARGB (4 channel)
	channel_info = (psd_channel_info *)psd_malloc(sizeof(psd_channel_info)*layer->number_of_channels);
	layer->channel_info = channel_info;

	(layer->channel_info+0)->channel_id=0;	// -1:alpha mask, 0:red,1:green,2:blue
	(layer->channel_info+0)->data_length=
		layer->per_channel_length + 2;
	(layer->channel_info+1)->channel_id=1;
	(layer->channel_info+1)->data_length=
		layer->per_channel_length + 2;
	(layer->channel_info+2)->channel_id=2;
	(layer->channel_info+2)->data_length=
		layer->per_channel_length + 2;
	if (layer->number_of_channels ==3)
	{
		; //default value
	}
	else	// channel number = 4
	{
		(layer->channel_info+3)->channel_id=-1;
		(layer->channel_info+3)->data_length=layer->per_channel_length + 2;
	}

	layer->blend_mode=psd_blend_mode_normal;	
	layer->opacity=255;
	layer->clipping=0;
	layer->transparency_protected=1;
	layer->visible=0;	//if set 1 will see nothing. should set zero
	layer->obsolete=0;
	layer->pixel_data_irrelevant=0;		//why?
	layer->transparency_shapes_layer=1;
	layer->divider_blend_mode = psd_blend_mode_pass_through;
	layer->layer_blending_ranges.number_of_blending_channels=0;
	layer->blend_clipped = psd_true;
	layer->fill_opacity = 255;

	//strcpy(layer->layer_name,"freeman007");
	
	//memcpy(layer->image_data,pRgbaBuf,ImgSize);
	//layer->image_data = pRgbaBuf;
	layer->temp_image_data = pRgbaBuf;	//need to transform
	
	layer->LayerMaskLen=0;		//not support, 0, 20, 36
	layer->LayerBlendRangeLen = 0;	//must support
	layer->LayerExtraLen = 0;		//not support	
	layer->LayerLen = 34 + layer->LayerExtraLen 
		+ ( layer->number_of_channels *(6+2+width*height) );
	layer->layer_info_count = 0;
	
	context->fill_alpha_channel_info = 0;
	//context->alpha_channels = 1;
	context->alpha_channel_info=NULL;
	
	context->global_layer_mask.kind=128;
	context->global_layer_mask.opacity=100;

	//context->LayerExtraLen= (8+context->LayerMaskLen + context->LayerBlendRangeLen);
	//	+strlen(layer->layer_name) );// /4*4;
	//layer structure + layer extra data + channel structure + channel image data
	context->LayerLen = 2 + layer->LayerLen;
	//context->gLayerMaskLen = 14;	//NOT including length itself
	context->gLayerMaskLen = 0;	//for testing only
	context->LayerandMaskLen = context->LayerLen + context->gLayerMaskLen + 8;
	//context->LayerandMaskLen = 0;	//for testing only
	//psd_set_layer_and_mask(context,context->file);	//section 4

	context->MergedImageLen = context->width * context->height * context->channels;		//depth must be 8
	
	context->temp_image_data = psd_malloc(context->MergedImageLen);
	memcpy(context->temp_image_data,pRgbaBuf,context->MergedImageLen);	//strore temp merged image data
	context->merged_image_data=NULL;
	//context->per_channel_length = width*height;
	//psd_set_image_data(context,context->file);		//section 5

	//psd_image_save(context);		//it's an easy way to save psd image.
}	

static void CreateWriteContext(psd_context *context)
{
	context->MergedImageLen = context->width*context->height*4;
	psd_freeif(context->temp_image_data);
	context->temp_image_data = psd_malloc(context->MergedImageLen);
	memcpy((void *)context->temp_image_data,(void *)context->merged_image_data
		,context->MergedImageLen);	

	context->fill_alpha_channel_info = 0;
	context->alpha_channels = 1;
	context->alpha_channel_info=NULL;	
	context->channels = context->color_channels+context->alpha_channels;
	if (context->channels >4)
		context->channels =4;

	context->color_map_length = 0;
	context->ImageResLen = 0;
	context->gLayerMaskLen = 0;
	context->global_layer_mask.kind=128;
	context->global_layer_mask.opacity=100;	
}


/*
Prototype: int  GetImgFileSize(char *fImgName,unsigned int *pWidth,unsigned int *pHeight)
Func desc: get image file size(width and height)
Input: 	char *fImgName, file name(abstract directory)	
Output:	unsigned short *pWidth,unsigned short *pHeight
Return:	0:success, others:fail.
Note:	only support .bmp, .jpg, .png, .psd
Example:	

*/
DLLEXPORT int  GetImgFileSize(char *fImgName,unsigned int *pWidth,unsigned int *pHeight)
{
	int len=0,ret=0;
	BMP_HEADER *pHeader;
	unsigned char *pBmpBuf=NULL,*pPngBuf=NULL,*pJpgBuf=NULL,*pPsdBuf=NULL;	
	psd_context *InContext=NULL;psd_status status;
	unsigned int width=0,height=0;

	if ((pWidth==NULL)||(pHeight==NULL))
		return -1;
	
	len = strlen(fImgName);
	if (len <= 4)
	{
		ret = -1;		//file name error
		goto end_err1;
	}
	
	if ((((fImgName[len-3]=='B')) && ((fImgName[len-2]=='M')) && ((fImgName[len-1]=='P')))
	|| (((fImgName[len-3]=='b')) && ((fImgName[len-2]=='m')) && ((fImgName[len-1]=='p'))))
	{
		ret = bmp_load(fImgName,&pHeader,&pBmpBuf);		//get RGB buffer,normal
		if (!pBmpBuf)
		{
			ret=-2;	//load file error
			goto end_err1;
		}
		width=pHeader->imgwidth;
		height=pHeader->imgheight;
		psd_free(pBmpBuf);
	}
	else if ((((fImgName[len-3]=='P')) && ((fImgName[len-2]=='N')) && ((fImgName[len-1]=='G')))
	|| (((fImgName[len-3]=='p')) && ((fImgName[len-2]=='n')) && ((fImgName[len-1]=='g'))))
	{
		pPngBuf = png_load(fImgName,&width,&height);		//get BGRA buffer, flip
		if (!pPngBuf)
		{
			ret=-3;	//load file error
			goto end_err1;
		}
		psd_free(pPngBuf);
	}
	else if ((((fImgName[len-3]=='J')) && ((fImgName[len-2]=='P')) && ((fImgName[len-1]=='G')))
	|| (((fImgName[len-3]=='j')) && ((fImgName[len-2]=='p')) && ((fImgName[len-1]=='g'))))
	{
		pJpgBuf = jpg_load(fImgName,&width,&height);		//get BGR buffer,flip
		if (!pJpgBuf)
		{
			ret=-4;	//load file error
			goto end_err1;
		}
		psd_free(pJpgBuf);
	}
	else if ((((fImgName[len-3]=='P')) && ((fImgName[len-2]=='S')) && ((fImgName[len-1]=='D')))
	|| (((fImgName[len-3]=='p')) && ((fImgName[len-2]=='s')) && ((fImgName[len-1]=='d'))))
	{
		status = psd_image_load(&InContext,fImgName);		//get BGRA buffer, flip
		if (status != psd_status_done)	//fail to read
		{
			psd_image_free(InContext);
			ret=-5;	//load file error
			goto end_err1;
		}
		width = InContext->width;
		height = InContext->height;
		psd_image_free(InContext);	//free all the malloc memory and close file
	}
	else
	{
		printf("file type wrong!(only support .bmp.jpg.png.psd file)\n");
		ret = -6;	//file type error
		goto end_err1;
	}

	*pWidth=width;
	*pHeight=height;
	printf("%s width=%d,height=%d\n",fImgName,*pWidth,*pHeight);

	return 0;
end_err1:
	*pWidth=0;
	*pHeight=0;
	return ret;
	
}

/*
transform a .psd file to my psd file, so libpsd can parse it correctly.
*/
DLLEXPORT psd_handle  Psd2Psd(char *PsdSrc,char *PsdDst)
{
	psd_status status;
	psd_context *InContext=NULL;
	int ret=-1;
	int i=0,width=0,height=0;
	psd_handle fHandle=-1;
	char layerImgName[32];
	psd_layer_record *layer=NULL;
	
#ifdef LOG_MSG
	PSD_PRINT("Psd2Psd(%s,%s) is called\n",PsdSrc,PsdDst);
#endif

	if ( (!PsdSrc)||(!PsdDst) )
		return -1;

	status = psd_image_load(&InContext,PsdSrc);		//get BGRA buffer, flip
	if (status != psd_status_done)	//fail to read
	{
		psd_image_free(InContext);
		return -2;
	}
	width = InContext->width;
	height = InContext->height;

	//pPsdBuf=psd_malloc(width*height*4);
	//memcpy(pPsdBuf,(unsigned char*)InContext->merged_image_data,width*height*4);
	//CreateBackgroundPsdFile(context,PsdDst,width,height,pPsdBuf);

	//CreateCanvas(width,height);
	
	fHandle=PsdNewFile(PsdDst,"layer0.png");
	for(i=1, layer=InContext->layer_records+1; i<InContext->layer_count; i++, layer++)
	{
		sprintf(layerImgName,"layer%d.png",i);
		ret = PsdNewLayer(fHandle,layerImgName
			,layer->left,layer->top,i,layer->opacity);
	}
/*
	fHandle=PsdNewFile(PsdDst,"merged.png");
	for(i=0, layer=InContext->layer_records; i<InContext->layer_count; i++, layer++)
	{
		sprintf(layerImgName,"layer%d.png",i);
		ret = PsdNewLayer(fHandle,layerImgName
			,layer->left,layer->top,i+1,layer->opacity);
	}
*/
	psd_image_free(InContext);	//free all the malloc memory and close file
	return fHandle;

}

DLLEXPORT psd_handle  PsdLoadFile(char *PsdFileName)
{
	psd_handle fHandle=-1;
#ifdef LOG_MSG
	PSD_PRINT("PsdLoadFile(%s) is called\n",PsdFileName);
#endif

	if (!PsdFileName)
		return -1;

	//fHandle=Psd2Psd(PsdFileName,PsdFileName);
	fHandle=Psd2Psd(PsdFileName,"temp.psd");
	
	return fHandle;
}

#if 0
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
DLLEXPORT psd_handle  PsdLoadFile(char *PsdFileName)
{
	psd_status status;
	int ret=-1,i;
	psd_context *context = NULL;
#ifdef LOG_MSG
	PSD_PRINT("PsdLoadFile(%s) is called\n",PsdFileName);
#endif

	if (!PsdFileName)
		return -1;
	
	status = psd_image_load(&context, PsdFileName);		//file will keep open...

	if (status == psd_status_done)
	{
		for (i=0; i<MAX_PSD_FILE_TABLE_ITEM; i++)
		{
			if (gPsdFileTable[i].pContext == NULL)
				break;
		}
		if (i == MAX_PSD_FILE_TABLE_ITEM)
		{
		#ifdef LOG_MSG
			PSD_PRINT("open too many files!\n");
		#endif
		
			ret = -3;		//valid file handle.
			psd_image_free(context);
		}
		else
		{
			//since we only plan to support rgba(4 channels) image, so we must handle as follows.
			CreateWriteContext(context);
		
			strcpy(gPsdFileTable[i].sFileName,PsdFileName);		//must < 128
			gPsdFileTable[i].fHandle = i;
			gPsdFileTable[i].max_width = context->width;
			gPsdFileTable[i].max_height = context->height;
			gPsdFileTable[i].pContext = context;	//malloc memory
			ret=i;			
		}
	}
	else		//fail to read
	{
		psd_image_free(context);
		ret=status;
	}

	return ret;
}
#endif

DLLEXPORT psd_handle  PsdGetLayers(char *PsdSrc)
{
	psd_status status;
	psd_context *InContext=NULL;
	int ret=-1;
	
#ifdef LOG_MSG
	PSD_PRINT("PsdGetLayers(%s) is called\n",PsdSrc);
#endif

	if ( (!PsdSrc) )
		return -1;

	status = psd_image_load(&InContext,PsdSrc);
	if (status != psd_status_done)
		ret=-2;
	else
		ret=0;
	
	psd_image_free(InContext);	//free all the malloc memory and close file
	return ret;
}


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
DLLEXPORT psd_handle  PsdNewFile(char *PsdFileName,char *bgPic)
{
	BMP_HEADER *pHeader;
	unsigned char *pBmpBuf=NULL,*pPngBuf=NULL,*pJpgBuf=NULL,*pPsdBuf=NULL;	
	psd_context *context = NULL,*InContext=NULL;psd_status status;
	int ret=-1;
	int i=0,bgFileType=-1,width,height,len=0;

#ifdef LOG_MSG
	PSD_PRINT("PsdNewFile(%s,%s) is called\n",PsdFileName,bgPic);
#endif

	if ((PsdFileName == NULL) ||(bgPic == NULL))
		return -1;	//input para error

	context = (psd_context *)psd_malloc(sizeof(psd_context));
	if(context == NULL)
	{
		return -2;	//apply memory error
	}

	context->file = (void *)fopen(PsdFileName,"wb+");
	if (context->file == NULL)		//fail to open
	{
	 	return -3;		//open file error
	}

	len = strlen(bgPic);
	if (len <= 4)
	{
		ret = -4;		//file name error
		goto end_err1;
	}
	for (i=0; i<MAX_PSD_FILE_TABLE_ITEM; i++)
	{
		if (gPsdFileTable[i].pContext == NULL)
		break;
	}
	if (i == MAX_PSD_FILE_TABLE_ITEM)
	{
		ret = -5;		//lack of resources to handle this operation
		goto end_err1;
	}
	
	if ((((bgPic[len-3]=='B')) && ((bgPic[len-2]=='M')) && ((bgPic[len-1]=='P')))
	|| (((bgPic[len-3]=='b')) && ((bgPic[len-2]=='m')) && ((bgPic[len-1]=='p'))))
	{
		bgFileType = 0;	//bmp file
		ret = bmp_load(bgPic,&pHeader,&pBmpBuf);		//get RGB buffer,normal
		if (!pBmpBuf)
		{
			ret=-6;	//load file error
			goto end_err1;
		}
		width=pHeader->imgwidth;
		height=pHeader->imgheight;
	}
	else if ((((bgPic[len-3]=='P')) && ((bgPic[len-2]=='N')) && ((bgPic[len-1]=='G')))
	|| (((bgPic[len-3]=='p')) && ((bgPic[len-2]=='n')) && ((bgPic[len-1]=='g'))))
	{
		bgFileType = 1;	//png file
		pPngBuf = png_load(bgPic,&width,&height);		//get BGRA buffer, flip
		if (!pPngBuf)
		{
			ret=-6;	//load file error
			goto end_err1;
		}		
	}
	else if ((((bgPic[len-3]=='J')) && ((bgPic[len-2]=='P')) && ((bgPic[len-1]=='G')))
	|| (((bgPic[len-3]=='j')) && ((bgPic[len-2]=='p')) && ((bgPic[len-1]=='g'))))
	{
		bgFileType = 2;	//jpg file
		pJpgBuf = jpg_load(bgPic,&width,&height);		//get BGR buffer,flip
		if (!pJpgBuf)
		{
			ret=-6;	//load file error
			goto end_err1;
		}		
	}
	else if ((((bgPic[len-3]=='P')) && ((bgPic[len-2]=='S')) && ((bgPic[len-1]=='D')))
	|| (((bgPic[len-3]=='p')) && ((bgPic[len-2]=='s')) && ((bgPic[len-1]=='d'))))
	{
		bgFileType = 3;	//psd file
		status = psd_image_load(&InContext,bgPic);		//get BGRA buffer, flip
		if (status != psd_status_done)	//fail to read
		{
			psd_image_free(InContext);
			ret=-6;	//load file error
			goto end_err1;
		}
		width = InContext->width;
		height = InContext->height;
	}
	else
	{
		printf("file type wrong!(only support .bmp.jpg.png file)\n");
		ret = -7;	//file type error
		goto end_err1;
	}

	switch(bgFileType)
	{
		case 0:	//bmp file
			pPsdBuf = Rgb2PsdImg(width,height,pBmpBuf);	//RGB-->BGRA
			CreateBackgroundPsdFile(context,PsdFileName,width,height,pPsdBuf);
			psd_freeif(pBmpBuf);
			//psd_freeif(pPsdBuf);
			break;
		case 1:	//png file
			pPsdBuf = pPngBuf;
			CreateBackgroundPsdFile(context,PsdFileName,width,height,pPsdBuf);
			//psd_freeif(pPsdBuf);			
			break;
		case 2:	//jpg file
			swap_rgb24(pJpgBuf,width*height);	//BGR-->RGB
			Rgb24_flip(pJpgBuf,width,height);
			pPsdBuf = Rgb2PsdImg(width,height,pJpgBuf);	//RGB-->BGRA
			CreateBackgroundPsdFile(context,PsdFileName,width,height,pPsdBuf);
			psd_freeif(pJpgBuf);
			//psd_freeif(pPsdBuf);			
			break;
		case 3:	//psd file
			pPsdBuf = (unsigned char*)InContext->merged_image_data;
			//swap_rgba32(pPsdBuf,width*height);	//why we should swap R and B??
			CreateBackgroundPsdFile(context,PsdFileName,width,height,pPsdBuf);
			psd_image_free(InContext);	//free all the malloc memory and close file
			//psd_freeif(pPsdBuf);			
			break;			
		default:
			goto end_err1;	//error
	}
	strcpy(gPsdFileTable[i].sFileName,PsdFileName);		//must < 128
	gPsdFileTable[i].fHandle = i;
	gPsdFileTable[i].max_width = width;
	gPsdFileTable[i].max_height = height;	
	gPsdFileTable[i].pContext = context;	 //malloc memory
	ret=i;
				
	return ret;

end_err1:
	fclose(context->file);
	psd_free(context);
	return ret;
}


/*
函数原型	int PsdCloseFile(psd_handle fHandle)
功能说明	关闭一个打开的psd file
入口参数	psd_handle fHandle，这是通过PsdLoadFile得到的psd 文件handle。
出口参数	无
返回值		0:success, other negative value:error code
调用事项	There is no dllmain, so you must close all the psd file when you exist your program.
*/
DLLEXPORT int  PsdCloseFile(psd_handle fHandle)
{
	int ret=-1,i=0;
	psd_context *context = gPsdFileTable[fHandle].pContext;
#ifdef LOG_MSG
	PSD_PRINT("PsdCloseFile(%d) is called\n",fHandle);
#endif	
	
	if ((fHandle <0 ) || (fHandle >= MAX_PSD_FILE_TABLE_ITEM))
		return -1;

	if (context != NULL)
	{
//		psd_freeif(context->file_name);	//add by freeman, it is a servious bug
		psd_image_free(context);	//free all the malloc memory and close file
		gPsdFileTable[fHandle].pContext = NULL;		//recyle for use again.
		gPsdFileTable[i].max_width = 0;
		gPsdFileTable[i].max_height = 0;
		ret = 0;
	}

	return ret;
}


/*
函数原型	int PsdSaveFile(psd_handle fHandle)
功能说明	save psd file
入口参数	psd_handle fHandle: psd file handler, get it by PsdLoadFile() or PsdNewFile()
出口参数	无
返回值		0:success, other negative value:error code
调用事项	before close psd file, you must call PsdSaveFile() to save modification.
*/
DLLEXPORT int  PsdSaveFile(psd_handle fHandle)
{
	 int ret=-1,i;
	 psd_context *context = gPsdFileTable[fHandle].pContext;
	 psd_layer_record *layer=NULL;
	 int channels=context->channels;//-context->alpha_channels;
#ifdef LOG_MSG
	char layerImgName[64];
	PSD_PRINT("PsdSaveFile(%d) is called\n",fHandle);	//in VC6, we can not use __FUNCTION__
#endif
	 
	 if ( (fHandle < 0)||(fHandle >= MAX_PSD_FILE_TABLE_ITEM) )
		 return -1;
	 
	 if (context != NULL)
	 {
#ifdef LOG_MSG
	PSD_PRINT("begin to merge layer...layer count=%d\n",context->layer_count);
	
	psd_assert(channels==4);
	ret=png_save("bg.png",context->temp_image_data,context->width,context->height);	
#endif	 
	 	//merge layers before save image. 
		for(i=1,layer=context->layer_records+1; i<context->layer_count; i++, layer++)
		{
#if 0
	sprintf(layerImgName,"psdsave_layer%d.png",i);
	ret=png_save(layerImgName,layer->temp_image_data,layer->width,layer->height);
#endif	
//	if ( (layer->width>context->width) || (layer->height>context->height) )
//		MergeLayer2(layer->temp_image_data,layer->width,layer->height,
//			context->temp_image_data,context->width,context->height
//			,layer->left,layer->top,layer->opacity);
//	else		
		MergeLayer1(layer->temp_image_data,layer->width,layer->height,
			context->temp_image_data,context->width,context->height
			,layer->left,layer->top,layer->opacity);
#ifdef LOG_MSG
	sprintf(layerImgName,"addlayer%d.png",i);
	psd_assert(channels==4);
	ret=png_save(layerImgName,context->temp_image_data,context->width,context->height);	
#endif		
		}

		psd_image_save(context);

	 	ret = 0;
	 }
	 
 	return ret;
}


DLLEXPORT int  PsdPreview(psd_handle fHandle,char *fBmpFile)
{
	int ret=-1,i;
	psd_context *context = gPsdFileTable[fHandle].pContext;
	psd_layer_record *layer=NULL;
	int width = 0;
	int height = 0;
	psd_uchar *psd_buf=NULL;	//rgba
	psd_uchar *bmp_buf=NULL;	//rgb

#ifdef LOG_MSG
	PSD_PRINT("PsdPreview(%d,%s) is called\n",fHandle,fBmpFile);
#endif	
	if ( (fHandle < 0)||(fHandle >= MAX_PSD_FILE_TABLE_ITEM) )
		 return -1;

	width = context->width;
	height = context->height;
	psd_buf=psd_malloc(width*height*4);	//rgba
	bmp_buf=psd_malloc(width*height*3);	//rgb
	
	if ( (!psd_buf)||(!bmp_buf) )
	 	return -2;
	 
	if (context != NULL)
	{
		memcpy(psd_buf,context->temp_image_data,width*height*context->color_channels);
		for(i=1,layer=context->layer_records+1; i<context->layer_count; i++, layer++)
		{
			MergeLayer1(layer->temp_image_data,layer->width,layer->height,
				psd_buf,width,height
				,layer->left,layer->top,layer->opacity);
		}

		bmp_buf = PsdImg2Rgb(width,height,psd_buf);
		ret = bmp_save(fBmpFile,bmp_buf,width,height,24);	//create a tmp bmp file
		psd_free(psd_buf);
		psd_free(bmp_buf);

	 	//ret = 0;
	}
	 
	return ret;
}



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
DLLEXPORT int  PsdNewLayer(psd_handle fHandle, char *sPngFileName
//	,int PosX,int PosY, int lwidth, int lheight
	,int PosX,int PosY
	,unsigned char ucLayerIndex,unsigned char Transpancy)
{
	psd_layer_record *layer=NULL,*OldLayer=NULL;
	int ret=-1;
	psd_context *context = NULL;
	int width=0,height=0,i=0;
	unsigned char *pPngBuf=NULL,*pTemp=NULL;
	psd_uint LayerImageSize=0;
	psd_uchar *layer_image_data=NULL;		//save for contrast adjustment using.
#ifdef LOG_MSG
	PSD_PRINT("PsdNewLayer(%d,%s,%d,%d,%d,%d) is called\n",fHandle,sPngFileName,PosX,PosY
		,ucLayerIndex,Transpancy);
#endif		
	 
	if ( (fHandle <0 )||(fHandle >= MAX_PSD_FILE_TABLE_ITEM)||(sPngFileName==NULL) 
		||(ucLayerIndex==0) )
	{
		return -1;	//handle is error
	}
		 
	if (gPsdFileTable[fHandle].pContext == NULL)
	{
		return -2;	//handle is wrong
	}

	context = gPsdFileTable[fHandle].pContext;	//get image in RAM

	if (ucLayerIndex >= context->layer_count)
		ucLayerIndex = context->layer_count;	//take as the top layer

	psd_assert(ucLayerIndex > 0);	//ucLayerIndex can not = 0. 0 is reserved for background layer.

	pPngBuf = png_load(sPngFileName,&width,&height);		//get BGRA buffer
	if (!pPngBuf)
	{
		return -3;
	}

/*	if ( ((width+PosX)>context->width) || ((height+PosY)>context->height) )
	{
		ret=-4;	//layer size is too big
		printf("layer size is too big!\n");
		goto Err_end;
	}
*/
	context->layer_count++;	//add one layer
	context->LayerLen = 2;		//layer count len is 2(word)
	OldLayer = context->layer_records;	
	pTemp = (unsigned char *)OldLayer;	//save it temp, need to free
	context->layer_records = 
		(psd_layer_record *)psd_malloc(sizeof(psd_layer_record)*context->layer_count);

	if ( (pTemp==NULL) || (context->layer_records==NULL) )
	{
		ret=-5;	//malloc memory fail
		printf("fail to malloc memory!\n");
		goto Err_end;
	}

	//can not skip layer0:background layer
	for(i=0,layer=context->layer_records; i<context->layer_count; i++, layer++)
	{
		if (i != ucLayerIndex)
		{
			memcpy((void *)layer,(void *)OldLayer,sizeof(psd_layer_record));
			OldLayer++;	//next
		}
		else		//insert a layer
		{
			//prepare for lay and layer mask info section
			layer->per_channel_length = width*height;	//???
			layer->layer_type = psd_layer_type_normal;
			layer->top = PosY;
			layer->left = PosX;
			layer->width = width;
			layer->height = height;
			layer->right = layer->left +  layer->width;
			layer->bottom = layer->top + layer->height;

			layer->number_of_channels = 4;	// only  support ARGB (4 channel)
			layer->channel_info = 
				(psd_channel_info *)psd_malloc(sizeof(psd_channel_info)*layer->number_of_channels);

			(layer->channel_info+0)->channel_id=0;	// -1:alpha mask, 0:red,1:green,2:blue
			(layer->channel_info+0)->data_length=layer->per_channel_length + 2;
			(layer->channel_info+1)->channel_id=1;
			(layer->channel_info+1)->data_length=layer->per_channel_length + 2;
			(layer->channel_info+2)->channel_id=2;
			(layer->channel_info+2)->data_length=layer->per_channel_length + 2;
			if (layer->number_of_channels ==3)
			{
				; //default value
			}
			else	// channel number = 4
			{
				(layer->channel_info+3)->channel_id=-1;
				(layer->channel_info+3)->data_length=layer->per_channel_length + 2;
			}

			layer->blend_mode=psd_blend_mode_normal;	
			layer->opacity=Transpancy;
			layer->clipping=0;
			layer->transparency_protected=1;
			layer->visible=0;	//if set 1 will see nothing. should set zero
			layer->obsolete=0;
			layer->pixel_data_irrelevant=1;
			layer->layer_blending_ranges.number_of_blending_channels=0;
			//strcpy(layer->layer_name,"freeman007");
			
			layer->temp_image_data = pPngBuf;	//need to transform, ch image data
			LayerImageSize=width*height*4;
			layer_image_data=psd_malloc(LayerImageSize);
			memcpy(layer_image_data,pPngBuf,LayerImageSize);
			layer->image_data=(psd_argb_color *)layer_image_data;		//save  for image adjustment using.

			layer->layer_info_count = 0;

			layer->LayerMaskLen=0;		//not support, 0, 20, 36
			layer->LayerExtraLen = 0;
			layer->LayerBlendRangeLen = 0;	//must support
			layer->LayerLen = 34 + layer->LayerExtraLen + layer->number_of_channels *(6+2+width*height) ;
		}

		context->LayerLen += layer->LayerLen;	
	}

	//context->LayerExtraLen= (8+context->LayerMaskLen + context->LayerBlendRangeLen);
	//	+strlen(context->layer_records->layer_name) );// /4*4;
	//layer structure + layer extra data + channel structure + channel image data
	//context->gLayerMaskLen = 14;	//NOT including length itself
	context->gLayerMaskLen = 0;	//for testing only
	context->LayerandMaskLen = context->LayerLen + context->gLayerMaskLen + 8;

/*
	if (context->layer_count == 1)
	{
		psd_freeif(pPngBuf);		//do not insert new layer
	}
	else
	{
		;	//do not merge layer here
	}
*/			
	psd_freeif(pTemp);
	return 0;	//success		
Err_end:
	
 	return ret;	
}


DLLEXPORT int  PsdCreateLayer(psd_handle fHandle, char *fPngName
	,unsigned int PosX,unsigned int PosY, unsigned int lwidth, unsigned int lheight
	,unsigned char ucLayerIndex,unsigned char Transpancy)
{
	int ret=-1;
	psd_context *context = NULL;
	unsigned int Src_w=0xFFFF,Src_h=0xFFFF,Dst_w=lwidth,Dst_h=lheight;
	unsigned char *pSrcPng,*pDstPng;
	char *fPngTmp="crop_tmp.png";
#ifdef LOG_MSG
	PSD_PRINT("PsdCreateLayer(%d,%s,%d,%d,%d,%d,%d,%d) is called\n",fHandle,fPngName,PosX,PosY
		,lwidth,lheight,ucLayerIndex,Transpancy);
#endif
	
	if ( (fHandle <0 )||(fHandle >= MAX_PSD_FILE_TABLE_ITEM)||(fPngName==NULL) 
		||(ucLayerIndex==0) )
	{
		return -1;	//handle is error
	}
		 
	if (gPsdFileTable[fHandle].pContext == NULL)
	{
		return -2;	//handle is wrong
	}
	context = gPsdFileTable[fHandle].pContext;	//get image in RAM

	if (ucLayerIndex >= context->layer_count)
		ucLayerIndex = context->layer_count;	//take as the top layer
	psd_assert(ucLayerIndex > 0);	//ucLayerIndex can not = 0. 0 is reserved for background layer.

	pSrcPng = png_load(fPngName,&Src_w,&Src_h);		//get BGRA buffer
	if (!pSrcPng)
	{
		return -3;
	}

	if (Dst_w>Src_w)
		Dst_w=Src_w;		//cut some width
	if (Dst_h>Src_h)
		Dst_h=Src_h;		//cut some lines
	if ((Dst_w+PosX)>context->width)	//greater than bg
	{
		Dst_w=context->width - PosX;	//cut
	}
	if ((Dst_h+PosY)>context->height)
	{
		Dst_h=context->height - PosY;	//cut
	}

	if ( (Dst_w>Src_w)||(Dst_h>Src_h)
		||((Dst_w+PosX)>context->width)||((Dst_h+PosY)>context->height) )
	{
	#ifdef LOG_MSG
		PSD_PRINT("Error!Image can not load, image dimention should <= background!\n");
	#endif
		return -4;
	}

	psd_assert(Dst_w>0);
	psd_assert(Dst_h>0);

	pDstPng=psd_malloc(Dst_w*(Dst_h<<2));
	if (pDstPng)
	{
		CropImg(pSrcPng,Src_w,Src_h,pDstPng,Dst_w,Dst_h);	//shaping png image
		png_save(fPngTmp,pDstPng,Dst_w,Dst_h);		//save a temp png file under current dir
		psd_freeif(pDstPng);
	#ifdef LOG_MSG
		PSD_PRINT("layer image is shaped,saved as %s\n",fPngTmp);
	#endif			
		ret = PsdNewLayer(fHandle,fPngTmp,PosX,PosY,ucLayerIndex,Transpancy);
	}

	psd_freeif(pSrcPng);
 	return ret;	
}


/*
函数原型	int PsdSetLayerDirection(psd_handle fHandle,unsigned char WhichLayer, IMAGE_DIRECTION ImgDir)
功能说明	unsigned char WhichLayer,layer index number [1,n], 0 is reserved for background layer.
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
DLLEXPORT int  PsdSetLayerDirection(psd_handle fHandle,unsigned char WhichLayer, IMAGE_DIRECTION ImgDir)
{
	psd_layer_record * layer=NULL;
	int ret=-1;
	psd_context *context = gPsdFileTable[fHandle].pContext;
	int width=0,height=0;
	unsigned char *pTemp=NULL;
#ifdef LOG_MSG
	PSD_PRINT("PsdSetLayerDirection(%d,%d,%d) is called\n",fHandle,WhichLayer,ImgDir);
#endif		
 	 
	if ( (fHandle <0 )||(fHandle >= MAX_PSD_FILE_TABLE_ITEM) )
		return -1;
	 
	if (context != NULL)
	{
		if (WhichLayer >= context->layer_count)		//layer number is wrong
			ret = -2;
		else
		{
			layer = context->layer_records + WhichLayer;
			width = layer->width;
			height = layer->height;
			switch(ImgDir)
			{
				case RIGHT90:
					pTemp = Rgba32_right90(layer->temp_image_data,width,height);
					layer->width = layer->top;
					layer->top = layer->left;
					layer->left = layer->width;
					layer->width = height;
					layer->height = width;
					layer->right = layer->left +  layer->width;
					layer->bottom = layer->top + layer->height;
					psd_freeif(layer->temp_image_data);
					layer->temp_image_data = pTemp;
					break;

				case LEFT90:
					pTemp = Rgba32_left90(layer->temp_image_data,width,height);
					layer->width = layer->top;
					layer->top = layer->left;
					layer->left = layer->width;
					layer->width = height;
					layer->height = width;
					layer->right = layer->left +  layer->width;
					layer->bottom = layer->top + layer->height;
					psd_freeif(layer->temp_image_data);
					layer->temp_image_data = pTemp;					
					break;

				case FLIP:
					Rgba32_flip(layer->temp_image_data,width,height);
					break;

				case MIRROR:
					Rgba32_mirror(layer->temp_image_data,width,height);
					break;

				default:
					break;
			}
			
			ret = 0;
		}
	}
	 
 	return ret;	
}


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
	PsdSetLayerBrightness(fHandle,1,50);
	PsdSaveFile(fHandle);
	PsdCloseFile(fHandle);		//must close firstly before load again...
}

Psd2Png("..\\pic\\tianye_new.psd","..\\pic\\tianye_new_tmp.png");

*/
DLLEXPORT int  PsdSetLayerBrightness(psd_handle fHandle,unsigned char WhichLayer, signed char Brightness)
{
	psd_layer_record * layer=NULL;
	int ret=-1;
	psd_context *context = gPsdFileTable[fHandle].pContext;
	int width=0,height=0;
	unsigned char *pTemp=NULL;
#ifdef LOG_MSG
	PSD_PRINT("PsdSetLayerBrightness(%d,%d,%d) is called\n",fHandle,WhichLayer,Brightness);
#endif	

	if ( (fHandle <0 )||(fHandle >= MAX_PSD_FILE_TABLE_ITEM) )
		return -1;
	 
	if (context != NULL)
	{
		if (WhichLayer >= context->layer_count)		//layer number is wrong
			ret = -2;
		else
		{
			layer = context->layer_records + WhichLayer;
			width = layer->width;
			height = layer->height;

			memcpy((psd_uchar *)layer->temp_image_data,(psd_uchar *)layer->image_data,width*height*4);		//to avoid damage using original copy
			PngSetBrightness(layer->temp_image_data,width*height,Brightness);

			ret = 0;
		}
	}
	 
 	return ret;	
}

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
	PsdSetLayerContrast(fHandle,1,50);
	PsdSaveFile(fHandle);
	PsdCloseFile(fHandle);		//must close firstly before load again...
}

Psd2Png("..\\pic\\tianye_new.psd","..\\pic\\tianye_new_tmp.png");

*/
DLLEXPORT int  PsdSetLayerContrast(psd_handle fHandle,unsigned char WhichLayer, signed char Contrast)
{
	psd_layer_record * layer=NULL;
	int ret=-1;
	psd_context *context = gPsdFileTable[fHandle].pContext;
	int width=0,height=0;
	unsigned char *pTemp=NULL;
#ifdef LOG_MSG
	PSD_PRINT("PsdSetLayerContrast(%d,%d,%d) is called\n",fHandle,WhichLayer,Contrast);
#endif		 
	if ( (fHandle <0 )||(fHandle >= MAX_PSD_FILE_TABLE_ITEM) )
		return -1;
	 
	if (context != NULL)
	{
		if (WhichLayer >= context->layer_count)		//layer number is wrong
			ret = -2;
		else
		{
			layer = context->layer_records + WhichLayer;
			width = layer->width;
			height = layer->height;
			
			memcpy((psd_uchar *)layer->temp_image_data,(psd_uchar *)layer->image_data,width*height*4);		//to avoid damage using original copy
			PngSetContrast(layer->temp_image_data,width*height,Contrast);

			ret = 0;
		}
	}
	 
 	return ret;	
}


#if 0

/*
函数原型	
功能说明	
入口参数	unsigned char WhichLayer,layer index number [1,n], 0 is reserved for background layer.
				unsigned char ucZoomRate, limit to [50,100,200]
出口参数	
返回值		0：成功。-1：失败。
调用事项	:
				[放大和缩小只能按矩形来进行，其它形状不能支持]
example:
fHandle = PsdNewFile("..\\pic\\tianye_new.psd","..\\pic\\tianye.bmp");
if (ret>=0)
{
	ret=PsdZoomLayer(fHandle,1,50);
	if (ret==0)
		printf("zoom success\n");
	else
		printf("zoom fail\n");
		
	PsdSaveFile(fHandle);
	PsdCloseFile(fHandle);		//must close firstly before load again...
}

*/
DLLEXPORT int  PsdZoomLayer(psd_handle fHandle,unsigned char WhichLayer, unsigned char ucZoomRate)
{
	psd_layer_record * layer=NULL;
	int ret=-1,i;
	psd_context *context = gPsdFileTable[fHandle].pContext;
	int width=0,height=0;
	unsigned char *pTemp=NULL;
	int NewWidth,NewHeight;
	unsigned char *Png_buf=NULL;
	 
	if ( (fHandle <0 )||(fHandle >= MAX_PSD_FILE_TABLE_ITEM) )
		return -1;
	 
	if (context != NULL)
	{
		if (WhichLayer >= context->layer_count)		//layer number is wrong
			ret = -2;
		else
		{
			layer = context->layer_records + WhichLayer;
			width = layer->width;
			height = layer->height;
			NewWidth=width*ucZoomRate/100;
			NewHeight=height*ucZoomRate/100;
			if ( (NewWidth>context->width) || (NewHeight>context->height) )
			{
				ret=-3; //layer size is too big
				printf("layer size is too big!\n");
			}
			else
			{
				Png_buf=malloc(NewWidth*NewHeight*4);
				ZoomImg(layer->temp_image_data,width,height,Png_buf,NewWidth,NewHeight);

				//register the new layer image buffer and width/height. free the old one.
				//Do NOT need to merged the layers. PsdSaveFile() will do the merge work.
				psd_free(layer->temp_image_data);
				layer->temp_image_data=Png_buf;
				layer->width=NewWidth;
				layer->height=NewHeight;

				ret = 0;
			}
		}
	}
	
 	return ret;	

}



/*
函数原型	int PsdSetLayerColor(FILE *fp, int Layer, int AddRed,int AddGreen,int AddBlue)
功能说明	改变psd文件中某个图层的(R,G,B)三色比例，用于完成色彩的预置功能。
入口参数	FILE *fp，文件指针
				int Layer，图层编号
				int AddRed，增加或减少红色的比例，取值范围[-100,+100]
				int AddGreen，增减绿色成分比例。
				int AddBlue，增减蓝色成分比例。
				比如，红色成分占30%，那可以减少为占0%，增加为占60%。如果此项为0，则不增也不减。
出口参数	无
返回值		0：成功。-1：失败。
调用事项	如果要预置图层1为10%偏红色，则调用方法为：
example:			PsdSetLayerColor(fp,1,+10,0,0);
*/

DLLEXPORT int PsdSetLayerColor(FILE *fp, int Layer, int AddRed,int AddGreen,int AddBlue)
{
	return 0;
}


#endif


