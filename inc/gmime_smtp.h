/*
 * =====================================================================================
 *
 *       Filename:  gmime_smtp.h
 *
 *    Description:  解析eml格式文件，产生smtp属性的json串 
 *
 *  function list:
 *           init_name_nodelist()
 *                  初始化文件名称存储链表,带头节点的链表，头节点不存放信息
 *           add_name_nodelist()
 *                  添加文件名称存储链表节点 
 *           free_name_nodelist()
 *                  释放文件名称存储链表空间
 *           show_name_nodelist()
 *                  显示文件名称存储链表信息
 *           write_part()
 *                  解析eml数据文件,得到附件的原始名称
 *           pro_smtp_jsonstr()
 *                  产生json串没有信头的收件人发件人
 *           parse_message()
 *                  产生一个gmime的流对象
 *           mysmtp()
 *                  解析eml文件,产生smtp协议属性的json串
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

#ifndef GMIME_SMTP_H_
#define GMIME_SMTP_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pcre.h>   
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gmime/gmime.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <math.h>
#include "plog.h"
#include "smtpfunc.h"

#define OVECCOUNT 30 /* should be a multiple of 3 */   
//#pragma  comment(lib,"pcre.lib")

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE 
#define FALSE 0
#endif

#if 1
#define TANG_DEBUG
#endif

struct namenode 
{
    char *filename;
    char *uuidname;
	char *filepath;
	unsigned char contentflag;
	struct namenode* next;
};

struct _envelope {
	char *date;
	char *subject;
	char *from;
	char *sender;
	char *reply_to;
	char *to;
	char *cc;
	char *bcc;
	char *in_reply_to;
	char *message_id;
};

struct _bodystruct {
	struct _bodystruct *next;
	
	struct {
		char *type;
		char *subtype;
		GMimeParam *params;
	} content;
	struct {
		char *type;
		GMimeParam *params;
	} disposition;
	char *encoding;
	struct _envelope *envelope;
	struct _bodystruct *subparts;
};

struct smtp_data
{
   int start;
   int end;
   struct smtp_data *next;
};

#define int_ntoa(x)	inet_ntoa(*((struct in_addr *)&x))

/**
 * @brief 解析eml数据文件,得到附件的原始名称
 *
 * @param part        gmime的对象
 * @param nodelist    文件名称链表
 * @param spec        eml嵌套层次标识
 *
 * @return void
 */
void write_part (GMimeObject *part, struct namenode* nodelist, const char *spec);


/**
 * @brief 初始化链表,带头节点的链表，头节点不存放信息
 *
 * @return  返回链表头节点
 */
struct namenode *init_name_nodelist();

/**
 * @brief 添加链表节点 
 *
 * @param nodelistsrc 链表头
 * @param filename    附件原始名称 
 * @param uuidname    gmime库解析生成的文件名称 
 * @param filepath    文件存放路径
 * @param contentflag 是否是正文的标志
 *
 * @return  成功返回0，失败返回-1
 */
int add_name_nodelist (struct namenode* nodelistsrc, const char *filename, const char *uuidname, const char *filepath, unsigned char contentflag);

/**
 * @brief 释放链表空间
 *
 * @param nodelist 链表头
 *
 * @return  成功返回0，失败返回-1
 */
int free_name_nodelist (struct namenode *nodelist);

/**
 * @brief 显示链表信息
 *
 * @param nodelist 链表头
 *
 * @return  成功返回0，失败返回-1
 */
int show_name_nodelist (struct namenode* nodelist);

/**
 * @brief 产生一个gmime的流对象
 *
 * @param fd 文件描述符
 *
 * @return  返回的gmime对象
 */
GMimeMessage *parse_message (int fd);

/**
 * @brief 产生json串没有信头的收件人发件人
 *
 * @param message  gmime解析出的信头信息
 * @param emlpath  eml文件存放绝对路径 
 * @param nodelist gmime解析出的文件链表
 * @param projson  产生的json串
 *
 * @return  成功返回0，失败返回-1
 */
int pro_smtp_jsonstr(GMimeMessage *message, char *emlpath, struct namenode* nodelist, char **projson);


/**
 * @brief 解析eml数据文件,产生smtp协议属性的json串
 *
 * @param emlpath  数据文件路径
 * @param projson  产生json串
 *
 * @return  成功返回0，失败返回-1
 */
int mysmtp (char *emlpath, char **projson);

#endif
