#ifndef _M_LOGGER_H//防止头文件重复包含
#define _M_LOGGER_H

#include<cstdio>
#include<ctime>

#define INF 0
#define DBG 1
#define ERR 2
#define DEFAULT_LOG_LEVEL DBG


#define LOG(level,format,...) do{\
    if(level<DEFAULT_LOG_LEVEL)break;\
    time_t t=time(NULL);\
    struct tm*formattime=localtime(&t);\
    char buf[32]={0};\
    strftime(buf,31,"%H:%M:%S",formattime);\
    fprintf(stdout,"[%s:%s:%d] " format "\n",buf,__FILE__,__LINE__,##__VA_ARGS__);}while(0)//...中的东西传给__VA_ARGS__

#define ILOG(format,...) LOG(INF,format,##__VA_ARGS__)
#define DLOG(format,...) LOG(DBG,format,##__VA_ARGS__)
#define ELOG(format,...) LOG(ERR,format,##__VA_ARGS__)

#endif