#ifndef __LOGFILE_C__
#define __LOGFILE_C__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>

#include "logfile.h"

#ifdef __cplusplus
       extern "C" {
#endif


static char gsLine[LOG_LINE_LIMIT+32];

static inline unsigned long fGetSize(const char *filename)
{
	struct stat buf;

	if(stat(filename, &buf)<0)
	{
		return 0;
	}
	return (unsigned long)buf.st_size;
}

//return errcode or written string length
int Log_MsgLine2(char *Dir,char *FileName,char *sLine,unsigned int FileSizeLimit)
{
	int ret=0;
	
	FILE *fp=NULL;
	unsigned long fSize=0;
	static char FullFileName[128]={0};
	struct tm *pLocalTime=NULL;
	time_t timep;

	if ( (!Dir)||(!FileName)||(!sLine)||(FileSizeLimit<=0) )
	{
		ret=-1;goto End;
	}

	ret=strlen(sLine);		//return write string length if success!
	if (ret>LOG_LINE_LIMIT)	//too long to write
	{
		ret=-2;goto End;
	}
		
	time(&timep);		//get current time
	pLocalTime=localtime(&timep);	//convert to local time to create timestamp
	sprintf(gsLine,"TS:%02d:%02d:%02d %s",pLocalTime->tm_hour,pLocalTime->tm_min,pLocalTime->tm_sec,sLine);
	
	strcpy(FullFileName,Dir);
	strcat(FullFileName,FileName);
	
	fSize=fGetSize(FullFileName);
	
	if ((fSize>FileSizeLimit) ||(fSize==0) )		//too large or no logfile exist, create it
		fp = fopen(FullFileName, "w+");		//reset to zero
	else
		fp = fopen(FullFileName, "a+");		//append to the end of logfile
	
	if (fp)
	{
		fputs(gsLine,fp);		//write a line to file
		fflush(fp);		//only block device has buffer(cache) requiring flush operation.
		fclose(fp);
		ret=fSize;
	}
	
End:
	return ret;
}

//return errcode or written string length
int Log_MsgLine(char *LogFileName,char *sLine)
{
	int ret=0;
	
	ret=	Log_MsgLine2(LOG_FILE_DIR,LogFileName,sLine,LOG_FILE_LIMIT);

	return ret;
}


#ifdef __cplusplus
}
#endif 

#endif	// __LOGFILE_C__
