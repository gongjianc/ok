#ifndef HTTP_IM_PARSER_H_
#define HTTP_IM_PARSER_H_
/*************************************************
Copyright (C), 2014, Tech. Co., Ltd.
 File name: http_IM_parser.h
Author: 许建东
 Version: V 0.2
Date: 2014-10-22
Description: 它是解析IM、微博、论坛、网盘等内容程序的头文件。
	依赖文件：http_IM_parser.c,http_netdisk.c
Others:

Function List: // 主要函数列表，每条记录应包括函数名及功能简要说明
	1. int http_IM(HttpSession* dlp_http,  char **http_IM_json)
	功能：它是与底层HTTP解析模块的接口函数
	2. int http_QQ_2013(HttpSession *dlp_http,  char **http_IM_json)
	功能：它可以解析QQ（5.3、5.4版本）的离线文件传输
	3. int http_QQ_2014(HttpSession *dlp_http, char **http_IM_json)
	功能：它可以解析QQ（6.3、6.4版本）的离线文件传输
	4. int http_QQ_2014_GOS(HttpSession *dlp_http, char **http_IM_json)
	功能：解析QQ的群共享文件传输和微云文件上传
	5. void http_Yahoo_Messager(HttpSession *dlp_http, char **http_IM_json)
	功能：解析雅虎通的文件传输
	6. int http_feition(HttpSession *dlp_http, char **http_IM_json)
	功能：解析飞信离线文件传输
	7. int http_baidu_pan(HttpSession *dlp_http, char **http_IM_json)
	功能：解析百度云管家文件上传
	8. int blog(HttpSession *dlp_http, char **http_IM_json, int n)
	功能：对微博内容进行还原
	9. int http_IM_pro_jsonstr(HttpSession* dlp_http,  dlp_http_post_head* http_session,  struct IM_file_inf *IM_session,  char **http_IM_json
	功能：产生JSON串
 History:

1. Date:2014-12-15
 Author:许建东
 Modification:去掉了枚举和结构体数组的定义
 2. Date:2014-12-15
 Author:许建东
 Modification:
 *************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pcre.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include "http_parser.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include  "plog.h"

//#include "parsertrace.h"

#ifndef OVECCOUNT
#define OVECCOUNT 30
#endif

#ifndef UUID_LENGTH
#define UUID_LENGTH 100
#endif
#ifndef	TRUE
#define     TRUE                                1
#endif
#ifndef FALSE
#define     FALSE                               0
#endif
#ifndef IM_FILE_NAME_LENGTH
#define	IM_FILE_NAME_LENGTH 128
#endif
/*定义结构体，存放解析出的文件信息等信息*/
struct IM_file_inf
{
    unsigned int block;					//传输文件时的分块信息
    unsigned long filesize;				//文件大小
    unsigned long range;				//Message Body中文件数据在所传输文件的位置偏移量
	char ts[64];						//文件开始传输的时间
	char uid[IM_FILE_NAME_LENGTH];		//存放原始文件的UUID+fp
	char plain[IM_FILE_NAME_LENGTH];	//微博，BBS等内容
	char filekey[256];					//QQ2013离线文件传输是的file ID
	char fn[256];						//原始文件名
};
/*定义结构体，存放飞信传数据时的POST信息*/
struct feition_data_post
{
    int     index;	//飞信离线文件传输是的分块信息的索引号
    int     bp;		//飞信离线文件传输是的分块信息的Message Body中文件数据在所传输文件大块数据的位置偏移量
	char	s1[16];	//飞信会话ID
    char    s[128];
};

char *get_sys_time();
unsigned long IM_get_file_size(const char *path);
inline char hextoi(char hexch);
char x2c( const char* hex );
char* plustospace( char* str );
char* unescape( char* s);
int http_IM_pro_jsonstr(HttpSession* dlp_http, dlp_http_post_head* http_session, struct IM_file_inf *IM_session, char **http_IM_json, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
int write_first_post(char *rs, const char *ws, unsigned long range, unsigned long len);
int wirte_IM_file(char *rs, const char *ws, unsigned long range, unsigned long len);
int http_QQ_2013(HttpSession *dlp_http, char **http_IM_json,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
int http_Yahoo_Messager(HttpSession *dlp_http, char **http_IM_json ,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
int http_QQ_2014(HttpSession *dlp_http, char **http_IM_json ,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
char ** IM_Match_Pcre(unsigned char *src, const char *pattern, unsigned int n);
int match_IM_url(char *src, const char *pattern);
int http_QQ_2014_GOS(HttpSession *dlp_http, char **http_IM_json ,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
int parser_feition_init(struct IM_file_inf *f_inf, dlp_http_post_head *http_session);
int http_feition(HttpSession *dlp_http, char **http_IM_json ,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
char *paser_blog(char *src, const char *pattern);
int blog(HttpSession *dlp_http, char **http_IM_json, char *pattern, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
/*int parser_baidu_pan_init_url(char *src, struct IM_file_inf *f_inf);
int parser_baidu_pan_init_msg(char *src, struct IM_file_inf *f_inf);
int parser_baidu_pan_data_url(unsigned char *src, struct IM_file_inf *f_inf);*/
/*int parser_baidu_pan_data_msg(char *uuid, int post_num, int flag, dlp_http_post_head *http_session);
int http_baidu_pan(HttpSession *dlp_http, char **http_IM_json ,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);*/
int http_IM(HttpSession* dlp_http,char **IMjson ,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
int http_feition_5_5_38(HttpSession *dlp_http, char **http_IM_json,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);
#endif
