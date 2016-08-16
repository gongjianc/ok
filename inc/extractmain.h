/*
 * =====================================================================================
 *
 *       Filename:  extractmain.h
 *
 *    Description: 协议解析到文件解析的调用接口实现
 *                 ftp协议的json串的生成、调用tika解析、发送给策略匹配消息队列
 *                 http upload协议的json串的生成、调用tika解析、发送给策略匹配消息队列
 *
 *       function list:
 *           tika_file_parser()
 *                 得到tika解析文件后产生的文件属性信息:文件类型、是否解析成功、是否加密、文件作者
 *           record_log()
 *                 将发送给策略匹配的json串的内容和个数写入一个日志文件:session_capture.log
 *           get_file_size()
 *                 获得文件大小的函数
 *           random_uuid()
 *                 取得uuid值
 *           send_msgqueue()
 *                 发送消息队列给策略匹配
 *           file_extract_pthread()
 *                 提取协议解析产生的json串中的文件信息，交给tika解析后，产生一个新的完整的json串,
 *                 并且把json串发送消息队列到策略匹配
 *           is_not_null()
 *                 判断传入的结构体数据是否为空
 *           send_json()
 *                 产生ftp格式json串，经过tika解析，发送消息给策略匹配
 *           http_upload_json()
 *                 产生http upload格式json串，经过tika解析，发送消息给策略匹配
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
#ifndef EXTRACTMAIN_H__
#define EXTRACTMAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <signal.h>
#include <uuid/uuid.h>
#include "update_json.h"
#include "fileparse.h"
#include "ftp_extract.h"
#include "plog.h"

/* 消息队列通信的key */
#define NIDS_PRO_KEY (key_t)0x1fff
#define PROTOCOL_KEY (key_t)0x2fff
#define FILE_KEY (key_t)0x2840

/* 协议json串最大长度 */
#define PROTOCOL_JSONLEN 1024 

/* 传输的信息 */
struct path_msg
{
   char name[80]; /* 存储json串的文件名称 */
   char dir[128];  /* 存储json串的文件路径*/
   char uuid[70]; /* uuid,策略匹配需要使用，这边不用*/
};

/* 文件消息队列结构体 */
struct nids_pm_msg 
{
  long mtype;			/* message type, must be > 0 */
  //struct path_msg pathmsg;
  char topath[128];
};


/**
 * @brief  调用tika解析 
 *
 * @param filefull_fname      文件全名
 * @param file_content_path   文件内容路径
 * @param tika_message        tika返回的信息
 *
 * @return  成功返回0，失败返回-1
 */
int tika_file_parser (char *filefull_fname, char *file_content_path, tika_message_s *tika_message);


/**
 * @brief 记录日志
 *
 * @param logname 日志文件名称
 * @param thing   记录的内容
 * @param data    数据个数
 *
 * @return  成功返回0，失败返回-1
 */
int record_log (char *logname, char *thing, int *data);


/**
 * @brief 获得文件大小
 *
 * @param path 文件路径
 *
 * @return   文件大小
 */
unsigned long get_file_size (const char *path);


/**
 * @brief 产生uuid
 *
 * @param buf[128] 存储uuid
 *
 * @return  成功返回0，失败返回-1
 */
int random_uuid(char buf[128]);


/**
 * @brief 发送消息给策略匹配
 *
 * @param jsondest  解析的内容json串,
 * @param strpath   写入文件，给策略匹配发送json的路径
 *
 * @return  成功返回0，失败返回-1
 */
int send_msgqueue (char *jsondest, char *strpath);

/**
 * @brief 
 * @brief 文件解析
 *
 * @param jsonstr       输入协议解析串
 * @param strpath       存储json串的文件路径
 * @param max_filesize  最大解析到文件大小
 *
 * @return  成功返回0，失败返回-1
 */
int file_extract_pthread (char *jsonstr, char *strpath, unsigned long max_filesize);


/**
 * @brief 判断传入的结构体数据是否为空
 *
 * @param pronod ftp结构体
 *
 * @return  成功返回0，失败返回-1
 */
int is_not_null(SESSION_BUFFER_NODE *pronode);


/**
 * @brief 发送ftp属性信息的json串给策略匹配
 *
 * @param pronode        ftp属性信息
 * @param max_filesize   tika最大解析文件大小,超过这个值将被丢弃
 *
 * @return  成功返回0，失败返回-1
 */
int send_json(SESSION_BUFFER_NODE *pronode, unsigned long max_filesize);


/**
 * @brief http upload 发送http upload属性信息的json串给策略匹配
 *
 * @param pronode        ftp属性信息
 * @param max_filesize   tika最大解析文件大小,超过这个值将被丢弃
 *
 * @return  成功返回0，失败返回-1
 */
int http_upload_json (SESSION_BUFFER_NODE *pronode, unsigned long max_filesize);

#endif
