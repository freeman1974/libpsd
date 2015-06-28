// created: 2007-01-27
#include <stdio.h>
#include <stdlib.h>
#include "libpsd.h"
#include "psd_system.h"

/*
函数malloc()和calloc()都可以用来动态分配内存空间,但两者稍有区别。
malloc()函数有一个参数,即要分配的内存空间的大小:
void*malloc(size_tsize);
calloc()函数有两个参数,分别为元素的数目和每个元素的大小,这两个参数的乘积就是要分配的内存空间的大小。
void*calloc(size_tnumElements,size_tsizeOfElement);
如果调用成功,函数malloc()和函数calloc()都将返回所分配的内存空间的首地址。
函数malloc()和函数calloc() 的主要区别是前者不能初始化所分配的内存空间,而后者能。如果由malloc()函数分配的内存空间原来没有被使用过，则其中的每一位可能都是0;反之, 如果这部分内存曾经被分配过,则其中可能遗留有各种各样的数据。也就是说，使用malloc()函数的程序开始时(内存空间还没有被重新分配)能正常进 行,但经过一段时间(内存空间还已经被重新分配)可能会出现问题。
函数calloc() 会将所分配的内存空间中的每一位都初始化为零,也就是说,如果你是为字符类型或整数类型的元素分配内存,那麽这些元素将保证会被初始化为0;如果你是为指 针类型的元素分配内存,那麽这些元素通常会被初始化为空指针;如果你为实型数据分配内存,则这些元素会被初始化为浮点型的零。
*/
void * psd_malloc(psd_int size)
{
	void *pMalloc=malloc(size);
	memset(pMalloc,0,size);		//add by yingcai, like calloc()
	return pMalloc;
}

void * psd_realloc(void * block, psd_int size)
{
	return realloc(block, size);
}

void psd_free(void * block)
{
	free(block);
	//block = NULL:		//add by freeman
}

void psd_freeif(void * block)
{
	if (block != NULL)
	{
		psd_free(block);
	//	block = NULL:		//add by freeman
	}
}

void * psd_fopen(psd_char * file_name)
{
	return (void *)fopen(file_name, "rb+");		//need allow to write
}

psd_int psd_fsize(void * file)
{
	psd_int offset, size;

	offset = ftell((FILE *)file);
	fseek((FILE *)file, 0, SEEK_END);
	size = ftell(file);
	fseek((FILE *)file, 0, SEEK_SET);
	fseek((FILE *)file, offset, SEEK_CUR);

	return size;
}

//we only read byte one by one. do NOT read word!!!
psd_int psd_fread(psd_uchar * buffer, psd_int count, void * file)
{
	return fread(buffer, 1, count, (FILE *)file);
}

psd_int psd_fseek(void * file, psd_int length)
{
	return fseek((FILE *)file, length, SEEK_CUR);
}

void psd_fclose(void * file)
{
	fclose((FILE *)file);
}
/*
函数原型:
size_t fwrite(   const void *buffer,   size_t size,   size_t count,   FILE *stream ); 
参数列表:
buffer    // 被写数据缓冲区的首地址,Pointer to data to be written    
size  //一次写入数据块的大小,Item size in bytes
count  // 写如数据快的次数,Maximum number of items  to be written
stream  // 文件结构指针,Pointer to FILE structure
eg: 
    fwrite(pBuffer, sizeof(unsigned long), 0x20000, fp_dma);
//从pBuffer 里搬0X20000个unsigned long类型的数据到fp_dma文件.
*/
psd_int psd_fwrite(psd_uchar * buffer, psd_int count, void * file)
{
	return fwrite(buffer, 1, count, (FILE *)file);
}
/*
ftell函数是用来获取文件的当前读写位置;
函数原型: long ftell(FILE *fp)
函数功能:得到流式文件的当前读写位置,其返回值是当前读写位置偏离文件头部的字节数.
*/
psd_int psd_ftell(void * file)
{
	return ftell((FILE *)file);
}
void *psd_fnew(psd_char * file_name)
{
	return (void *)fopen(file_name, "wb+");
}

//copy file if it exists, add by freeman
static int psd_fCopy(psd_char * filename)
{
	FILE *fpSrc=NULL,*fpDst=NULL;
	int ret=-1;
	psd_int offset, size;
	psd_uchar *buffer;
//	char DstFileName[128];

	//open source file to read
	fpSrc = fopen(filename, "rb");
	if (fpSrc == NULL)
	{
		ret = -1;
		goto End;
	}

	//create a tmp psd file in current dir to write
	fpDst = fopen("tmp.psd", "wb+");
	//get file size
	offset = ftell((FILE *)fpSrc);
	fseek((FILE *)fpSrc, 0, SEEK_END);
	size = ftell(fpSrc);
	fseek((FILE *)fpSrc, 0, SEEK_SET);
	fseek((FILE *)fpSrc, offset, SEEK_CUR);
	//malloc buffer
	buffer = malloc(size);
	if (buffer == NULL)
	{
		ret = -2;
		goto End;
	}
	fread(buffer, 1, size, (FILE *)fpSrc);	//read source file by byte
	fwrite(buffer, 1, size, (FILE *)fpDst);	//write to dst file by byte
	ret=0;
	
End:
	if (fpSrc)	fclose(fpSrc);
	if (fpDst)	fclose(fpDst);
	return ret;
}