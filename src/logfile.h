#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#ifdef __cplusplus
       extern "C" {
#endif


#define	LOG_MSG	1
#define   PSD_LOG_FILE	"libpsd.log"

#define	LOG_FILE_LIMIT		(2000*1024)
#define	LOG_FILE_DIR		""
#define   LOG_LINE_LIMIT 	1000

//return errcode or written string length
int Log_MsgLine(char *LogFileName,char *sLine);

//return errcode or written string length
int Log_MsgLine2(char *Dir,char *FileName,char *sLine,unsigned int FileSizeLimit);

#define PSD_PRINT(fmt,args...)  do{char info[256];sprintf(info,":" fmt , ## args);Log_MsgLine(PSD_LOG_FILE,info);}while(0)


#ifdef __cplusplus
}
#endif 

#endif			//__LOGFILE_H__

