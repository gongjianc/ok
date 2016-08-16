/*
 * =====================================================================================
 *
 *       Filename:  ftp_extract.h
 *
 *    Description: ftp协议的json串的生成函数
 *                 http upload协议的json串的生成函数
 *
 *  function list:
 *           get_file_size()
 *                 获取文件大小
 *           int_to_ipstr()
 *                 将网络格式的ip地址转换为点分十进制的字符串 
 *           ftp_jsonstr()
 *                 产生ftp的json串 
 *           httpupload_pro_jsonstr()
 *                 产生http upload的json串 
 *
 *        Version:  1.0
 *        Created:  08/30/2014 07:13:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  唐玉山, etangyushan@126.com
 *   Organization:  北京中测安华科技有限责任公司 
 *
 * =====================================================================================
 */
#ifndef FTP_EXTRACT_H_
#define FTP_EXTRACT_H_

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<jansson.h>
#include<ctype.h>
#include<dirent.h>
#include<sys/stat.h>
#include<pcre.h> 
#include<unistd.h>
#include<time.h>
#include<math.h>
#include"update_json.h"
#include"http_parser.h"
#include"plog.h"


/**
 * @brief        网络的ip转换为点分十进制
 *
 * @param src    源
 * @param str    输出
 *
 * @return  成功返回0，失败返回-1
 */
int int_to_ipstr(int src, char *str);

/**
 * @brief  产生ftp的json串
 *
 * @param pronodesrc   ftp信息结构体
 * @param jsonstr      产生的json串
 *
 * @return 成功返回0，失败返回-1
 */
int ftp_jsonstr(SESSION_BUFFER_NODE *pronode, char **jsonstr);


/**
 * @brief  产生http upload的json串
 *
 * @param pronodesrc   http upload信息结构体
 * @param jsonstr      产生的json串
 *
 * @return  成功返回0，失败返回-1
 */
int httpupload_pro_jsonstr(SESSION_BUFFER_NODE *pronodesrc, char **jsonstr);

#endif
