#ifndef WEBMAIL_PARSER_H
#define WEBMAIL_PARSER_H
#include<stdio.h>
#include <pthread.h>
#include<string.h>
#include<stdlib.h>
#include<pcre.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "webmail_attach.h"
//#include "parsertrace.h"
#include "http_parser.h"
#include "plog.h"
#include "http_IM_parser.h"

/*存放正则匹配起始位置长度*/
#ifndef OVECCOUNT
#define OVECCOUNT 30
#endif
#ifndef MAX_REGEX_RESULT_LEN
#define MAX_REGEX_RESULT_LEN 128
#endif

#define NON_NUM '0'

/*uuid长度*/
#ifndef UUID_LENGTH
#define UUID_LENGTH 512
#endif

/*文件名长度*/
#ifndef FILE_NAME_LENGTH
#define FILE_NAME_LENGTH 128
#endif

/*post头的长度*/
#ifndef MAX_PT_LEN
#define MAX_PT_LEN 4096
#endif

#ifndef MAX_TO_LEN
#define MAX_TO_LEN 1024
#endif

/*时间长度*/
#ifndef MAX_TIME_LEN
#define MAX_TIME_LEN 20
#endif

/*boundary长度*/
#ifndef MAX_BOUNDARY_LEN
#define MAX_BOUNDARY_LEN 100
#endif

/*模式串长度*/
#ifndef PATTERN_LENGTH
#define PATTERN_LENGTH 256
#endif

/*四元组长度*/
#ifndef TUPLE_NAME_LEN
#define TUPLE_NAME_LEN 128
#endif

/*网易邮箱流返回值*/
#ifndef WYMAIL
#define WYMAIL 1
#endif

/*QQ邮箱流返回值*/
#ifndef QMAIL
#define QMAIL 2
#endif

/*新浪邮箱流返回值*/
#ifndef SINAMAIL
#define SINAMAIL 3
#endif

/*搜狐邮箱流返回值*/
#ifndef SOHUMAIL
#define SOHUMAIL 4
#endif

/*TOM邮箱流返回值*/
#ifndef TOMMAIL
#define TOMMAIL 5
#endif

/*21CN邮箱流返回值*/
#ifndef CNMAIL
#define CNMAIL 6
#endif

/*139邮箱流返回值*/
#ifndef MAIL139
#define MAIL139 7
#endif

/*189邮箱流返回值*/
#ifndef MAIL189
#define MAIL189 8
#endif

/*中国石油邮箱流返回值*/
#ifndef CNPCMAIL
#define CNPCMAIL 9
#endif

/*央视网邮箱流返回值*/
#ifndef CNTVMAIL
#define CNTVMAIL 10
#endif
/*用于存储webmail程序中长度与内容的结构体*/
typedef struct DLP_WEBMAIL_PARSER_STRING_AND_STRLEN {
	int len;
	char *string;
} webmail_string_and_len;

/*用于存放提取的webmail信息的结构体*/
typedef struct mail{
	enum
	{
	   no_jansson,/*不需要生成jansson串*/
	   build_jansson,/*需要生成jansson串*/
	   mail_end,/*提取大附件结束的标志，也许要生成jansson串*/
	   mail_ing,/*大附件还没结束，不需要生成jansson串*/
	}state;/*是否需要生成jansson串的状态标志*/
	int pro_id;/*协议号，webmail协议号码是157，http的协议号是7*/
	char Time[MAX_TIME_LEN]; /*邮件发送时间*/
	char boundary[MAX_BOUNDARY_LEN];/*mime格式的boundary*/
	char uuid[UUID_LENGTH];/*文件的存储名uuid*/
	char host_ori_ref[HTTP_FILE_HOST_MAX+HTTP_FILE_ORIGIN_MAX+HTTP_FILE_REFERER_MAX];/*将http头中的host、origin、referer放到一个数组里*/
	char *From; /*发件人*/
	char *To;/*收件人*/
	char *Subject;/*邮件主题*/
	char *Content;/*邮件正文*/
	struct att_node *filenode; /*附件链表*/
}dlp_webmail_info;

/*生成由四元组拼接成的字符串*/
int get_tuple_name(HttpSession*dlp_http , char tuple_name[128]  );

/*生成以四元组拼接成的字符串*/
char *itoa(int num,char*str,int radix);
/*获取系统时间*/
int system_time(char *nowtime);

/*10位数字时间戳转换*/
int time_convert(char* time1,char*s);

/*结构体初始化*/
webmail_string_and_len *init_webmail_string_and_len();

/*正则判断与提取*/
int mch( webmail_string_and_len ** var, webmail_string_and_len ** Sto,const char *pattern);

/*正则判断与提取,存入的为数组*/
int mch_t( webmail_string_and_len ** var, char Sto[MAX_REGEX_RESULT_LEN],const char *pattern);

/*针对mime格式的附件流,提取附件用到sundy算法*/
int mch_sundy_research(webmail_string_and_len * var, webmail_string_and_len ** Sto1, int Rloca, const char pattern1[PATTERN_LENGTH]);

/*针对mime格式的附件流,提取附件，放到链表里*/
int mch_att(webmail_string_and_len ** var,dlp_webmail_info *webmail);

/*将二进制转换为拾进制*/
int hex2num(char c);

/*URLdecode解码*/
int URLDecode(const char* str, const int strSize, char* result, const int resultSize);

/*针对网易邮箱发送正文邮件流，提取收件人等信息*/
int wymail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对网易邮箱发送附件流，附件提取*/
int wyattach(HttpSession* dlp_http, webmail_string_and_len *StoPT, webmail_string_and_len *StoMB,dlp_webmail_info *webmail) ;

/*针对QQ邮箱发送正文邮件流，提取收件人等信息*/
int QQmail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对QQ邮箱群邮件发送正文邮件流，提取收件人等信息*/
int QQmail_qun(webmail_string_and_len *StoCK,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对QQ邮箱发送有乱码的附件流，附件提取*/
int QQattach(HttpSession* dlp_http, webmail_string_and_len *StoPT, webmail_string_and_len *StoBY, webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对QQ邮箱发送的分会话分post的附件流，附件提取*/
int QQattach2(HttpSession* dlp_http,webmail_string_and_len *StoPT,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对QQ邮箱发送的无乱码但会话分post与单纯mime格式的附件流，附件提取*/
int QQattachF(HttpSession* dlp_http,webmail_string_and_len *StoCK,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对新浪邮箱发送正文邮件流，提取收件人等信息*/
int sina_mail(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对sina邮箱发送附件流，附件提取*/
int sina_attach(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对sohu邮箱发送正文邮件流，提取收件人等信息*/
int sohu_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对sohu邮箱发送普通附件流，附件提取*/
int sohu_attach(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对sohu邮箱批量上传到U盘的分会话分post附件流，附件提取*/
int sohu_attach2(HttpSession* dlp_http,webmail_string_and_len *StoPT,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对tom邮箱发送正文邮件流，提取收件人等信息*/
int tom_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对tom邮箱发送附件流，附件提取*/
int tom_attach(webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对21cn邮箱发送正文邮件流，提取收件人等信息*/
int cn_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对21cn邮箱发送附件流，附件提取*/
int cn_attach(webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对139邮箱发送正文邮件流，提取收件人等信息*/
int mail_139(webmail_string_and_len *StoCK,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对139多会话传送附件*/
int mch_att_139_multi_session( webmail_string_and_len *StoMB , webmail_string_and_len *StoBY , dlp_webmail_info *webmail);

/*针对139邮箱通过天翼云发送的单post的mime格式的附件流，附件提取*/
int attach_139(webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对139邮箱单会话多post的mime格式的附件流，附件提取*/
int attach2_139(HttpSession* dlp_http,webmail_string_and_len *StoPT,webmail_string_and_len *StoMB, webmail_string_and_len *StoBY,dlp_webmail_info *webmail);

/*针对139邮箱单会话多post的明文的附件流，附件提取*/
int attach3_139(HttpSession* dlp_http,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对189邮箱发送正文邮件流，提取收件人等信息*/
int mail_189(webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对189邮箱发送附件流，附件提取*/
int attach_189(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对中国石油邮箱发送正文邮件流，提取收件人等信息*/
int cnpc_mail(webmail_string_and_len *StoPT,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对中国石油邮箱发送附件流，附件提取*/
int cnpc_attach(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对央视网邮箱发送正文邮件流，提取收件人等信息*/
int cntv_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*针对央视网邮箱发送附件流，附件提取*/
int cntv_attach(webmail_string_and_len *StoCK,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail);

/*判别是发送邮件流还是附件流*/
int jdmail_att(HttpSession* dlp_http,dlp_webmail_info *webmail,char*host_ori_ref,char**webmailjson, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);

/*判别是何种邮箱*/
int jdmail_type(char*host_ori_ref);

/*判断是webmail数据流*/
int jdmail(char*host_ori_ref);

/* webmail程序的主接口函数 */
int webmail_mail(HttpSession* dlp_http,char **webmailjson, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);

/*产生jansson串*/
int webmail_pro_jsonstr(HttpSession* dlp_http, dlp_webmail_info *webmail, char **webmail_json_result, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID);

/*申请空间*/
int c_malloc(void **obj,int len);

/*释放空间*/
void c_free(char**obj);

#endif
