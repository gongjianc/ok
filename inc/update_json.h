/*
 * =====================================================================================
 *
 *       Filename:  update_json.h
 *
 *    Description:  更新获取json串的指定的key对应的value值 
 *
 *
 *  function list:
 *          int_to_ipstr()
 *               网络的ip转换为点分十进制
 *          get_file_path_absolute()
 *               获取需要解析的文件绝对路径 
 *          remove_flist()
 *               删除文件链表中的项
 *          show_session()
 *               显示从抓包收到的协议结构体信息
 *          get_file_num()
 *               json串中文件的个数
 *          get_protocol_type()
 *               获取协议类型
 *          get_file_type_author()
 *               获取tika产生的文件作者和文件类型
 *          freeold_copyfromnew_freenew()
 *               释放并重新申请oldstr的内存，将newstr的字符串拷贝到oldstr中，释放newstr的内存资源
 *          set_tika_message()
 *               设置tika解析出到属性信息 
 *          set_tika_message_bak()
 *               设置tika解析出到属性信息 
 *          last_mark()
 *               找到最后的slash(/)
 *          get_file_path()
 *               获取json串中文件的路径
 *          get_file_value()
 *               获取json串中文件的路径
 *          update_filelist_int_value()
 *               更新json串中的flist文件属性数组的key对应的value,value是int
 *          update_filelist_value()
 *               更新json串中的flist文件属性数组的key对应的value,value 是string
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
#ifndef UPDATE_JSON_H
#define UPDATE_JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include "session_buf.h"
#include "ring_buffer.h"
#include "plog.h"

#define int_ntoa(x)	inet_ntoa(*((struct in_addr *)&x))
#define NAMELEN  512

/**
 * @brief tika parse return message struct
 */
typedef struct tika_message 
{
	char filetype[16];   //file type
	char issuccess[16];    //tika parse success is true otherwise false  
	char isEncrypted[16];  //file is encrypted  is true otherwise false
    char author[512];
} tika_message_s;
#if 0
typedef struct five_attr 
{
    unsigned int ip_src;     //原IP
    unsigned int ip_dst;     //目的IP
    unsigned short port_src;   //源端口
    unsigned short port_dst;   //目的端口
    unsigned int protocol;   //本文件网络传输的协议
} session_ft;             //五元组信息
typedef struct session_buffer_node_tag
{
    session_ft  session_five_attr;  //五元组信息
    char        strname[128];       //本地存储文件名
    char        strpath[128];       //文件路径
	char        orgname[128];       //网络上传输的原始文件名
	FILE        *old_file;
	void        *attr;

}SESSION_BUFFER_NODE;
#endif

/**
 * @brief 获取需要解析的文件绝对路径 
 *
 * @param jsonsrc         json 串
 * @param filefull_fname  文件绝对路径
 * @param file_flag       文件列表flist中的位置
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_path_absolute (char *jsonsrc, char *filefull_fname, int file_flag);


/**
 * @brief 删除文件链表中的项
 *
 * @param jsonstr     原json串
 * @param jsondest    处理后的json串
 * @param file_flag   flist文件索引值
 *
 * @return 成功返回0，失败返回-1
 */
int remove_flist (char *jsonstr, char **jsondest, int file_flag);


/**
 * @brief 显示从抓包收到的协议结构体信息
 *
 * @param pronode 协议属性信息结构体
 *
 * @return 成功返回0，失败返回-1
 */
int show_session(SESSION_BUFFER_NODE *pronode);

/**
 * @brief json串中文件的个数
 *
 * @param jsonstr    json串
 * @param file_num   文件个数
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_num(char *jsonstr, int *file_num);

/**
 * @brief           获取协议类型
 *
 * @param jsonstr   json串
 *
 * @return          协议号
 */
int get_protocol_type (char *jsonstr);


/**
 * @brief              获取tika产生的文件作者和文件类型
 *
 * @param jsonstr      tika产生的json串
 * @param tika_message tika解析出的文件信息结构体
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_type_author (char *jsonstr, tika_message_s *tika_message);


/**
 * @brief 释放并重新申请oldstr的内存，将newstr的字符串拷贝到oldstr中，释放newstr的内存资源
 *
 * @param oldstr  老的内存块
 * @param newstr  新的内存块
 *
 * @return 成功返回0，失败返回-1
 */
int freeold_copyfromnew_freenew (char **oldstr, char **newstr);

/**
 * @brief  设置tika解析出到属性信息 
 *
 * @param jsonsrc            源json串
 * @param jsondest           目的json串
 * @param tika_message       tika parser message
 * @param file_content_path  内容路径
 * @param file_flag          flist文件索引值
 *
 * @return 成功返回0，失败返回-1
 */
int set_tika_message (char *jsonsrc, char **jsondest, tika_message_s tika_message, char *file_content_path, int file_flag);


/**
 * @brief  设置tika解析出到属性信息 
 *
 * @param jsonsrc            源json串
 * @param jsondest           目的json串
 * @param tika_message       tika parser message
 * @param file_content_path  内容路径
 * @param file_flag          flist文件索引值
 *
 * @return 成功返回0，失败返回-1
 */
int set_tika_message_bak (char *jsonsrc, char **jsondest, tika_message_s tika_message, char *file_content_path, int file_flag);


/**
* @brief 找到最后的slash(/)
*
* @param str  字符串
* @param mark 标记
*
* @return 成功返回0，失败返回-1
*/
int last_mark (char *str, char mark);


/**
 * @brief 获取json串中文件的路径
 *
 * @param jsonstr     json信息
 * @param key         json中flist项的key值
 * @param file_value  json中flist项的key对应的value值
 * @param file_flag   flist的第几个文件属性,file_flag = 0是第一个
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_path (char *jsonstr,  char *filepath, int file_flag);


/**
 * @brief 获取json中给定key的值
 *
 * @param jsonstr     json信息
 * @param key         json中flist项的key值
 * @param file_value  json中flist项的key对应的value值
 * @param file_flag   flist的第几个文件属性
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_value(char *jsonstr, char *key, char *file_value, int file_flag);


/**
 * @brief 更新json串中的flist文件属性数组的key对应的value,value是int
 *
 * @param jsonsrc      源json
 * @param jsondest     更新后的json
 * @param key          更新的value对应的key
 * @param value        更新的value
 * @param file_flag    flist的第几个文件属性,file_flag = 0是第一个
 *
 * @return 成功返回0，失败返回-1
 */
int update_filelist_value (char *jsonsrc, char **jsondest, char *key, char *value, int file_flag);


/**
 * @brief 更新json串中的flist文件属性数组的key对应的value,value 是string
 *
 * @param jsonsrc      源json
 * @param jsondest     更新后的json
 * @param key          更新的value对应的key
 * @param value        更新的value
 * @param file_flag    flist的第几个文件属性,file_flag = 0是第一个
 *
 * @return 成功返回0，失败返回-1
 */
int update_filelist_int_value (char *jsonsrc, char **jsondest, char *key, int value, int file_flag);
	
#endif
