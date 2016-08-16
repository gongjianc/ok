/************************************************************
Copyright (C), 2014, DLP
 FileName: http_IM_parser.c
 Author: Xu jiandong
 Version : V 0.2
 Date: 2014-10-22
 Description: 此模块主要是对一些主流的IM（即时通讯），网盘，微博，贴吧，BBS进行解析，所支持的功能如下：
			  1）能够QQ（5.3、5.4、6.3、6.4等版本）的离线文件传输，群共享文件传输进行解析和文件还原
			  2）能够对飞信离线文件传输和雅虎通文件传输进行解析和文件还原
			  3）能偶对QQ微云网盘客户端的文件传输进行解析和文件还原
			  4）能够对一些主流的微博、BBS、贴吧等进行解析和内容还原，如：新浪微博（包括桌面版）、
			  网易微博、腾讯微博、QQ说说和日志、百度贴吧、天涯论坛、猫扑等。
			  5）支持百度云管家（4.8.5）文件传输的解析和文件还原。
Version:V 0.2
Function List: 
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
<author> <time> <version > <desc>
 Xu jiandong 14/10/24 V 0.2 build this moudle 
 Xu Jiandong 14/12/08 V 0.2 build this module
 Xu Jiandong 14/12/15 V 0.2 build this module
 ***********************************************************/
#include "http_IM_parser.h"
#include "beap_hash.h"
#include "plog.h"
#include "read_IM_config.h"

//#include "parsertrace.h"
//#include "nids.h"

/*QQ 6.3、6.4 版本离线文件传输时的第一个POST的Content-Length*/
#define		QQ_6_3_MIN_BLOCK					1364 
/*QQ6.3、6.4 版本离线文件传输时从第二个POST开始的POST满载时的POST的Content-Length*/
#define		QQ_6_3_MAX_BLOCK					131436
/*QQ6.3、6.4 版本离线文件传输时，每个Message Body中在文件数据块之前的“乱码”长度*/
#define		QQ_6_3_HEADER						364
/*QQ6.3、6.4 版本离线文件传输时从第二个POST开始的POST满载时的POST的Content-Length*/
#define		QQ_6_6_MAX_BLOCK					262508					 
/*由于QQ(包括群共享和微云)文件传输的文件名是加密，所以暂不能解析的，就另外赋值*/
const char      *QQ_fn = "QQ_File_Have_No_Name";
/*识别某种IM，微博，BBS 的HSOT，URL*/
const char		*IS_QQ = "QQClient";//QQ离线文件传输，User Agent
const char		*IS_QQ_1 = "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)";//QQ5.4版本User Agent
const char		*IS_QQ_2014_GOS = "TXFTNActiveX_RawSocket";//QQ2014 群共享文件文件传输，User Agent
const char		*IS_WY_host = "tj.yunup.ftn.qq.com";//QQ微云网盘
const char		*IS_FX = "CloudFile";//飞信离线文件传输， User Agent

const char 		*feition_5_5_38_url = "/storageWeb/servlet/uploadFileServlet\\?uploadCode=.*&txType=.*&oprChannel=.*&dom=.*";
const char		*feition_5_5_38_host = "caiyun.feixin.10086.cn";
//雅虎通
const char		*IS_YM_url = "^/relay\\?token=.+&sender=.+&recver=.+";//正则表达式
	
/*const char      *baidu_yun_init_host = "pan.baidu.com";
const char      *baidu_yun_data_host = "c.pcs.baidu.com";*/

/*宏定义释放内存的函数*/
#define FREE(p)\
    if(p){\
        free(p);\
        p = NULL;\
    }

extern hash_table *ftp_session_tbl;
extern _IM_HOST_URL_REG  * IM_memcmp_match_head_Node;
extern _IM_HOST_URL_REG	 * IM_pcre_match_head_Node;
extern char *session_stream;

/*************************************************
Function: get_sys_time()
Description: 获取系统时间
Calls: 
Called By: 
	1. http_IM_pro_jsonstr(HttpSession* dlp_http, dlp_http_post_head* http_session, struct IM_file_inf *IM_session, char **http_IM_json)
	2. int http_QQ_2013(HttpSession *dlp_http, char **http_IM_json)
	3. void http_Yahoo_Messager(HttpSession *dlp_http, char **http_IM_json)
	4. int http_QQ_2014(HttpSession *dlp_http, char **http_IM_json)
	5. int http_QQ_2014_GOS(HttpSession *dlp_http, char **http_IM_json)
	6. int http_feition(HttpSession *dlp_http, char **http_IM_json)
	7. int blog(HttpSession *dlp_http, char **http_IM_json, int n)
Table Accessed: 
Table Updated: 
Input: 
Output: 系统时间
Return: 系统时间
Others: // 其它说明
*************************************************/
char *get_sys_time()
{
	time_t rawtime;
    struct tm *timeinfo = NULL;
    char *nowtime = NULL;
	nowtime = (char *)malloc(sizeof(char)*64);
	if(NULL == nowtime)
	{
//		fprintf(stderr,"Insufficient_memory");
        return NULL;
	}
	memset(nowtime, '\0', sizeof(char) * 64);
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );                                                                           
    sprintf (nowtime, "%d-%.2d-%.2d %.2d:%.2d:%.2d", 
              1900+timeinfo->tm_year,
              1+timeinfo->tm_mon,
              timeinfo->tm_mday,
              timeinfo->tm_hour,
              timeinfo->tm_min,
              timeinfo->tm_sec
            );
	return nowtime;
}
/*************************************************
Function: unsigned long IM_get_file_size(const char *path)  
Description: 获取文件大小
Calls: 
Called By: 
	1. int blog(HttpSession *dlp_http, char **http_IM_json, int n)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. const char *path，是文件名
Output: 文件大小
Return: 文件大小 
Others: // 其它说明
*************************************************/
unsigned long IM_get_file_size(const char *path)  
{  
	unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}

/*************************************************
Function: inline char hextoi(char hexch)
Description: 十六进制字符转整数
Calls: 
Called By: 
	1. char x2c( const char* hex )
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. char hexch，十六进制的字符
Output: 整数
Return: 整数
Others: 固定形式，用来解析URL
*************************************************/

inline char hextoi(char hexch)
{
	if((hexch>='0')&&(hexch<='9'))
		return hexch - '0';
	else if((hexch>='A')&&(hexch<='F'))
		return hexch - 'A' + 10;
	else if((hexch>='a')&&(hexch<='f'))
		return hexch - 'a' + 10;
	else
		return -1;
}

/*************************************************
Function: char x2c( const char* hex )
Description: 将 XX 形式的十六进制的数字(ASCII码)转换成字符
Calls: 
	1. inline char hextoi(char hexch)
Called By: 
	1. char* unescape( char* s)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. const char* hex，XX 形式的十六进制的数字(ASCII码)
Output: 字符
Return: 字符
Others: 固定形式，用来解析URL
*************************************************/
char x2c( const char* hex )
{
	char digit;
	digit = (hextoi(hex[0]));
	digit =  digit << 4;
	digit =  digit + hextoi(hex[1]);
	return(digit);
}

/*************************************************
Function: char* plustospace( char* str )
Description: 按 URL 编码规则解码，将'+' 替换成 ' ' (空格)
Calls: 
Called By: 
	1. char* unescape( char* s)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. char* str，需要转换的字符串
Output: 转换后的字符串
Return: 转换后的字符串
Others:固定形式，用来解析URL
*************************************************/
char* plustospace( char* str )
{
	int x;
	if(str == NULL)
	{
		return NULL;
	}
	for( x=0; str[x] ;x++ )
	{
		if(str[x] == '+')
		str[x] = ' ';
	}
	return( str );
}
//-----------------------------------------------------
//      按 URL 编码规则解码
//        %XX 转换成字符
//      然后将 + 替换成空格
//-----------------------------------------------------
/*************************************************
Function: char* unescape( char* s)
Description: 按 URL 编码规则解码，先将%XX 转换成字符，再将'+' 替换成 ' ' (空格)
Calls: 
	1. char x2c( const char* hex )
	2. char* plustospace( char* str )
Called By: 
	1. char *paser_blog(char *src, char *pattern)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. char* s，需要解析的URL
Output: 转换后的字符串
Return: 转换后的字符串
Others: 固定形式，用来解析URL
*************************************************/
char* unescape( char* s)
{
	int x, y;
	if(!s)
	{
		return NULL;
	}

	int length = strlen(s) - 2;

	for( x = 0, y = 0; s[y]; ++x, ++y )
	{
		if( ( s[x] = s[y] ) == '%')
		{
			s[x] = x2c( &s[y+1] );
			y += 2;
		}
	}
	s[x] = '\0';
	return plustospace(s);
}

/*************************************************
Function: int parser_QQ_url(char *src, struct IM_file_inf *f_inf)
Description: 解析QQ2013离线文件传输是的URL，提取其中的相关信息（filekey、filesize、range）
Calls: 
Called By: int http_QQ_2013(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. char *src, 需要解析的URL
	1. struct IM_file_inf *f_inf，存放提取到的有关QQ2013离线文件传输的信息
Output: 提取到的有关QQ2013离线文件传输的POST的信息
Return: 解析成功返回0；如果输入的URL不是QQ2013离线文件传输的POST的URL，则返回1；解析失败，则返回-1 
Others: 
*************************************************/
char ** IM_Match_Pcre(unsigned char *src, const char *pattern, unsigned int n)
{
	pcre		 *re = NULL; //被编译好的正则表达式的pcre内部表示结构 
    const char	 *error;     //输出参数，用来输出错误信息
	int			 erroffset;   //输出参数，pattern中出错位置的偏移量 
	int			 ovector[OVECCOUNT]; //输出参数，用来返回匹配位置偏移量的数组
    int			 rc;                 //匹配成功返回非负数，没有匹配返回负数 
	const char   *stringptr = NULL;         //指向那个提取到的子串的指针
	unsigned int i;
	char 		 **result = NULL;
	/*正则表达式*/
	//char pattern[] = ".*&filekey=(.*)&filesize=(.*)&bmd5=.*&range=(.*)";
    re = pcre_compile(pattern,     // pattern, 输入参数，将要被编译的字符串形式的正则表达式 
                       0,             // options, 输入参数，用来指定编译时的一些选项 
                       &error,         // errptr, 输出参数，用来输出错误信息 
                       &erroffset,     // erroffset, 输出参数，pattern中出错位置的偏移量 
                       NULL);         // tableptr, 输入参数，用来指定字符表，一般情况用NULL 
    // 返回值：被编译好的正则表达式的pcre内部表示结构 
    if (re == NULL)
    {//如果编译失败，返回错误信息 
        log_printf(ZLOG_LEVEL_ERROR, "PCRE compilation failed at offset %d: %s in -----%s------%s------%d", erroffset, error,__FILE__, __func__, __LINE__); 
        return NULL; 
    } 
    rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针 
                   NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针 
                   src,   // subject, 输入参数，要被用来匹配的字符串 
                   strlen(src),  // length, 输入参数，要被用来匹配的字符串的指针 
                   0,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量 
                   0,     // options, 输入参数，用来指定匹配过程中的一些选项 
                   ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组 
                   OVECCOUNT);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小 
    // 返回值：匹配成功返回非负数，没有匹配返回负数 
    if (rc < 0)
    {//如果没有匹配，返回错误信息 
        pcre_free(re);    
		return NULL; 
    }
    if( NULL == (result = (char **)malloc( n * sizeof(char *))))
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		pcre_free(re);
		return NULL;
	}
	memset(result, '\0', n * sizeof(char *));
    for(i = 0; i < n; i++)
    {
    	pcre_get_substring(src, ovector, rc, i + 1, &stringptr);
    	if( NULL == (result[i] = (char *)malloc( sizeof(char) * (strlen(stringptr) + 1))))
 		{
 			log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			pcre_free(re);
			return NULL;
 		}
 		memset(result[i], '\0', sizeof(char) * (strlen(stringptr) + 1));
    	memcpy(result[i], stringptr, strlen(stringptr));
    	pcre_free_substring(stringptr);
    } 
	pcre_free(re);   // 释放编译正则表达式re的内存
	return result;
}
//----------------------------------------------------------------------------------
//功能：产生JSON串
//----------------------------------------------------------------------------------
int http_IM_pro_jsonstr(HttpSession* dlp_http, dlp_http_post_head* http_session, struct IM_file_inf *IM_session, 
						char **http_IM_json, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	/*获取系统时间*/
	char *nowtime = get_sys_time();
	SESSION_BUFFER_NODE ft;
	/*定义JSON变量*/
	json_t		*objectmsg;
	json_t		*objectall;
	json_t		*myfile;
	json_t		*filelist;
	char		*result = NULL;
	
	/*char	fp[128] = {};
	char	filename2[384]={};
	char	plain_fp[384] = {};
	getcwd(fp, 128);*/
	
	/*定义数组，存放拼接后的URL（URL= “POST的url + Host”）*/
    char url[3096] = {};
    strcat(url, http_session->host);
    strcat(url, http_session->url);
	/*定义JSON对象*/
	objectmsg = json_object();
	objectall = json_object();
	myfile = json_object();
	/*定义JSON数组*/
	filelist= json_array();

	/*IP地址转换，整型IP格式转为字符串型IP（0.0.0.0）*/
	char	*ipaddr1=NULL;
	char	addr1[20] = {};
	struct	in_addr inaddr1;
	/*源IP转换*/
	inaddr1.s_addr=dlp_http->key.saddr;
	ipaddr1= inet_ntoa(inaddr1);
	strcpy(addr1,ipaddr1);
	json_object_set_new (objectall, "sip", json_string(addr1));
	/*目的IP转换*/
	char	*ipaddr2=NULL;
	char	addr2[20] = {};
	struct	in_addr inaddr2;
	inaddr2.s_addr=dlp_http->key.daddr;
	ipaddr2 = inet_ntoa(inaddr2);
	strcpy(addr2,ipaddr2);
	json_object_set_new (objectall, "dip", json_string(addr2));
	/*协议号*/
	json_object_set_new (objectall, "prt", json_integer(7));
	/*源端口*/
	json_object_set_new (objectall, "spt", json_integer(dlp_http->key.source));
	/*目的端口*/
	json_object_set_new (objectall, "dpt", json_integer(dlp_http->key.dest));
	/*解析完成后的系统时间*/
	json_object_set_new (objectall, "ts", json_string(nowtime));
	/*用户传输文件时间*/
	json_object_set_new (objectmsg, "ts", json_string(IM_session->ts));
	/*缺省*/
	json_object_set_new (objectmsg, "from", json_string(""));
	json_object_set_new (objectmsg, "to", json_string(""));
	json_object_set_new (objectmsg, "sbj", json_string(""));
	if(0 != strlen(IM_session->plain))
	{	
		//sprintf(plain_fp, "%s/%s", fp, IM_session->plain);
		json_object_set_new (objectmsg, "plain", json_string(IM_session->plain));
	}
	else
	{
		json_object_set_new (objectmsg, "plain", json_string(""));
	}
	/*拼接后的URL*/
	json_object_set_new (objectmsg, "url", json_string(url));
	/*把包含私有属性的JSON对象嵌入规定格式的对象*/
	json_object_set_new (objectall, "pAttr", objectmsg);
	json_object_set_new (myfile, "ft", json_string(""));
	/*文件大小*/
	json_object_set_new (myfile, "fs", json_integer(IM_session->filesize/1024));//by niulw 20160119
	json_object_set_new (myfile, "au", json_string(""));
	/*有附件*/
	if(0 != strlen(IM_session->uid))
	{	
		/*输出的UUID文件名（绝对路径+UUID）*/
		//sprintf(filename2, "%s/%s", fp, IM_session->uid);
		json_object_set_new (myfile, "fp", json_string(IM_session->uid));
		json_object_set_new (myfile, "fn", json_string(IM_session->fn));
	}
	/*没有附件*/
	else
	{
		json_object_set_new (myfile, "fp", json_string(""));
		json_object_set_new (myfile, "fn", json_string(""));
	}
	json_object_set_new (myfile, "ftp", json_string(""));
	json_array_append_new(filelist, myfile);
	json_object_set_new (objectall, "flist", filelist);

	/* JSON格式转换成字符串 */
   	result = json_dumps(objectall, JSON_PRESERVE_ORDER);
	//printf("\n************************* IM_json = %s\n", result);
	int size = strlen(result)+1;
	/*取出JSON串*/
	*http_IM_json = (char *)malloc(sizeof(char) * size);
	if(!(*http_IM_json))
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	memset(*http_IM_json, '\0', sizeof(char) * size);
	memcpy(*http_IM_json, result, strlen(result));
	ft.session_five_attr.ip_dst = dlp_http->key.daddr;
    ft.session_five_attr.ip_src = dlp_http->key.saddr;
    ft.session_five_attr.port_src = dlp_http->key.source;
    ft.session_five_attr.port_dst = dlp_http->key.dest;
    ft.attr = &(**http_IM_json);
    ft.session_five_attr.protocol = 7; 
    sbuff_putdata(iringbuffer_ID, ft);
	/* 释放json_dumps调用后的结果 */
	FREE(result);	
	//计数器5减一可以销毁数据, 创建的jansson对象最后调用
	
   	json_decref(objectall);
	FREE(nowtime);
	return 0;
}
/*************************************************
Function: int write_first_post(char *rs, const char *ws, unsigned long range, unsigned long len)
Description: 把文件传输时的第一POST的原始文件的数据块复制到相应的UUID文件
Calls: 
Called By: 
	1. int http_QQ_2013(HttpSession *dlp_http, char **http_IM_json)
	2. void http_Yahoo_Messager(HttpSession *dlp_http, char **http_IM_json)
	3. int http_feition(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. char *rs, 存放POST的Message Body的UUID文件
	2. const char *ws, 需要写入的UUID文件
	3. unsigned long range, Message Body 中的原始文件的数据块在原始文件的位置偏移量
	4. unsigned long len， POST的 Content-Length
Output: 
Return: 复制成功返回0，不成功返回-1
Others: // 其它说明
*************************************************/

int write_first_post(char *rs, const char *ws, unsigned long range, unsigned long len)
{
	FILE	*f_msg = NULL;//message body 指针
	FILE	*f_IM = NULL;//传输的文件
	char	*msg_buf = NULL;//临时存放需要写入的数据
	msg_buf = (char *)malloc(sizeof(char) * len);
	if(NULL == msg_buf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(msg_buf, '\0', sizeof(char) * len);

	//打开message body 文件
	f_msg = fopen(rs, "r");
	if(f_msg == NULL)
	{
		log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		FREE(msg_buf);	
		return -1;
	}

	fread ( msg_buf, len, 1, f_msg);
	fclose(f_msg);
	f_msg = NULL;
	//打开写要写如的文件
	f_IM = fopen(ws, "w");
	if(f_IM == NULL)
	{
		log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		FREE(msg_buf);		
		return -1;
	}
	fseek(f_IM, range, SEEK_SET);
	fwrite(msg_buf, len, 1, f_IM);

	fclose(f_IM);
	f_IM = NULL;
	FREE(msg_buf);
	return 0;

}
/*************************************************
Function: int wirte_IM_file(char *rs, const char *ws, unsigned long range, unsigned long len)
Description: 把文件传输时的从第二个POST开始的原始文件的数据块复制相应的UUID文件
Calls: // 被本函数调用的函数清单
Called By: 
	1. int http_QQ_2013(HttpSession *dlp_http, char **http_IM_json)
	2. int http_feition(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input: 
	1. char *rs, 存放POST的Message Body的UUID文件
	2. const char *ws, 需要写入的UUID文件
	3. unsigned long range, Message Body 中的原始文件的数据块在原始文件的位置偏移量
	4. unsigned long len， POST的 Content-Length
Output: // 对输出参数的说明。
Return: 复制成功返回0，不成功返回-1
Others: 在还原原始文件的时候，它需要和write_first_post这个函数配合使用。因为，POST是多线程分发的，
		所以，在原始文件中数据块靠前的数据块不一定最先写入到相应的文件。
*************************************************/

int wirte_IM_file(char *rs, const char *ws, unsigned long range, unsigned long len)
{
	FILE *f_msg = NULL;//message body 指针
	FILE *f_IM = NULL;//传输的文件
	char *msg_buf = NULL;//临时存放需要写入的数据
	msg_buf = (char *)malloc(sizeof(char) * len);
	if(NULL == msg_buf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(msg_buf, '\0', sizeof(char) * len);

	//打开message body 文件
	f_msg = fopen(rs, "r");
	if(f_msg == NULL)
	{
		log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		FREE(msg_buf);
		return -1;
	}

	fread ( msg_buf, len, 1, f_msg);
	fclose(f_msg);
	f_msg = NULL;
	//打开写要写如的文件
	f_IM = fopen(ws, "r+");
	if(f_IM == NULL)
	{
		log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		FREE(msg_buf);
		return -1;
	}
	fseek(f_IM, range, SEEK_SET);
	fwrite(msg_buf, len, 1, f_IM);

	fclose(f_IM);
	f_IM = NULL;
	FREE(msg_buf);
	return 0;
}

/*************************************************
Function: int http_QQ_2013(HttpSession *dlp_http, char **http_IM_json)
Description: 解析QQ（5.3、5.4版本）的离线文件传输
Calls: 
	1. int parser_QQ_url(char *src, struct IM_file_inf *f_inf)
	2. int write_first_post(char *rs, const char *ws, unsigned long range, unsigned long len)
	3. int wirte_IM_file(char *rs, const char *ws, unsigned long range, unsigned long len)
	4. int http_IM_pro_jsonstr(HttpSession* dlp_http, dlp_http_post_head* http_session, struct IM_file_inf *IM_session, char **http_IM_json)
	5. char *get_sys_time()
Called By: 
	1. int http_IM(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. HttpSession *dlp_http, 包含当前HTTP会话信息的结构体
	2. char **http_IM_json，取出产生的JSON串的二级指针
Output: 
Return: 解析成功返回1，否则返回其它整数（0或-1）
Others: 这个函数不仅可以解析QQ2013（5.3版本）、QQ2014（5.4版本）后续的版本可能也支持
*************************************************/

int http_QQ_2013(HttpSession *dlp_http, char **http_IM_json, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	dlp_http_post_head 		*http_session=NULL; //包含POST信息的结构体
	http_session = (dlp_http_post_head*)dlp_http->http; //强制类型转换，获得当前POST的信息
	//
	const char 				*pattern = "/\\?ver=.+&ukey=.+&filekey=(.*)&filesize=(.*)&bmd5=.*&range=(.*)";
	char 					**UrlInformation = NULL;
	unsigned int  			i;	
	/*存放生成的uuid*/
	char 					uuid_buf[70] = {};
	char 					IM_fp[256] = {};
	/*定义，结构体，存放文件信息*/
	struct IM_file_inf 		*f_inf = NULL;
	f_inf = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
	if(NULL == f_inf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(f_inf, '\0', sizeof(struct IM_file_inf));
	/*判断URL是否是关于QQ2013的离线文件传输的URL，如若不是返回0*/
	if( NULL == (UrlInformation = IM_Match_Pcre(http_session->url, pattern, 3)))
	{
		FREE(f_inf);
		return 0;
	}
	memcpy(f_inf->filekey, UrlInformation[0], strlen(UrlInformation[0]));
	/*获取filesize */
	f_inf->filesize = atoi(UrlInformation[1]);
	/*获取range*/
	f_inf->range = atoi(UrlInformation[2]);
	if(NULL != UrlInformation)
	{
		for(i = 0; i < 3;i++)
		{
			FREE(UrlInformation[i]);
		}
		FREE(UrlInformation);
	}
	
	/*该POST是关于QQ2013的离线文件传输的，则进行解析*/
	memcpy(f_inf->fn, QQ_fn, strlen(QQ_fn));
	/*定义数组，存放URL中的“Host” + “filekey”*/
	char inf_file[256] = {};
	sprintf(inf_file, "%s/%s%s.dlp", session_stream, http_session->host, f_inf->filekey);
	/*定义文件指针，该指针指向以inf_file命名的文件。
	这样就可以把所传输的文件信息和传输状态等信息写入此文件*/
	FILE *fqq = NULL;

	/*查看以inf_file命名的文件是否存在。若不存在，说明是新的文件传输过程，则进行初始化处理*/
	if((fqq = fopen(inf_file, "r")) == NULL)
	{
		/*产生UUID*/
		uuid_t randNumber;
	    uuid_generate(randNumber);
	    uuid_unparse(randNumber, uuid_buf);
	    sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
//		memcpy(f_inf->uid, uuid_buf, 70);
		/*获得系统时间*/
		char *nowtime = get_sys_time();
		memcpy(f_inf->ts, nowtime, 64);
		//free nowtime
		FREE(nowtime);
		//把UUID和文件开始传输的时间存放到文件
		fqq = fopen(inf_file, "w");
		if(NULL == fqq)
		{
			log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			FREE(f_inf);	
			return -1;
		}
		fwrite(uuid_buf, 70, 1, fqq);
		fseek(fqq, 70, SEEK_SET);
		fwrite(f_inf->ts, 64, 1, fqq);
		fclose(fqq);
		fqq = NULL;

		//把Message Body中所传输文件的数据写入UUID文件
		write_first_post(http_session->new_name, f_inf->uid, 0, http_session->content_length);
		//假如文件传输的第一POST的message body的大小等于整个文件的大小，则输出JSON串
		if(http_session->content_length == f_inf->filesize)
		{
			http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json, sbuff_putdata,  iringbuffer_ID);
			//删除产生的临时文件，假如删除不成功，返回-1
			if(remove(inf_file))
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				return -1;
			}
			FREE(f_inf);
			return 1;
		}
	}
	/*若已经存在，说明这个POST不是新的文件传输开始*/
	else
	{
		/*读取inf_file文件中的UUID*/
		fread(uuid_buf, 70, 1,fqq);
		fclose(fqq);
		fqq = NULL;
		sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
		/*把Message Body中所传输文件的数据写入UUID文件*/
		wirte_IM_file(http_session->new_name, f_inf->uid, f_inf->range, http_session->content_length);
		/*文件传输完毕产生json串*/
		if(f_inf->range + http_session->content_length == f_inf->filesize)
		{
			char qts[64] = {};//存放用户开始传输文件的时间
			/*读取用户开始传输文件的时间*/
			fqq = fopen(inf_file, "r");
			if(NULL == fqq)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				return -1;
			}
			fseek(fqq, 70, SEEK_SET);
			fread(qts, 64, 1, fqq);
			fclose(fqq);
			fqq = NULL;

			memcpy(f_inf->ts, qts, 64);
//			memcpy(f_inf->uid, uuid_buf, 70);
			/*产生JSON串*/
			http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json, sbuff_putdata, iringbuffer_ID);
			//删除产生的临时文件
			if(remove(inf_file))
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				return -1;
			}
		}
	}
	FREE(f_inf);
    return 1;	
}
/*************************************************
Function: void http_Yahoo_Messager(HttpSession *dlp_http, char **http_IM_json)
Description: 解析雅虎通件传输
Calls: 
	1.char *get_sys_time()
	2. int write_first_post(char *rs, const char *ws, unsigned long range, unsigned long len)
	3. int http_IM_pro_jsonstr(HttpSession* dlp_http, dlp_http_post_head* http_session, struct IM_file_inf *IM_session, char **http_IM_json)
Called By: 
	1. int http_IM(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. HttpSession *dlp_http, 包含当前HTTP会话信息的结构体
	2. char **http_IM_json，取出产生的JSON串的二级指针
Output: 
Return: 
Others: 
*************************************************/
int http_Yahoo_Messager(HttpSession *dlp_http, char **http_IM_json, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	dlp_http_post_head 		*http_session=NULL; //包含POST信息的结构体
	http_session = (dlp_http_post_head*)dlp_http->http; //强制类型转换，获得当前POST的信息
	char 					uuid_buf[70] = {}; //定义数组，存放UUID
	char 					*nowtime = NULL;   //系统时间
	const char				*YM_fn = "Yahoo_Message_File_Have_No_name";
	/*定义，结构体，存放文件信息*/
	struct IM_file_inf 		*f_inf = NULL;
	f_inf = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
	if(NULL == f_inf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(f_inf, '\0', sizeof(struct IM_file_inf));
	memcpy(f_inf->fn, YM_fn, strlen(YM_fn));
	/*产生UUID*/
	uuid_t randNumber;
	uuid_generate(randNumber);
	uuid_unparse(randNumber, uuid_buf);
	sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
	//memcpy(f_inf->uid, uuid_buf, 70);
	/*获取系统时间*/
	nowtime = get_sys_time();
	memcpy(f_inf->ts, nowtime, 64);
	//free nowtime
	FREE(nowtime);
	/*获取文件大小*/
	f_inf->filesize = http_session->content_length;
	/*把Message Body中所传输文件的数据写入UUID文件*/
	write_first_post(http_session->new_name, f_inf->uid, 0, http_session->content_length);
	/*产生JSON串*/
	http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json,  sbuff_putdata, iringbuffer_ID);
	
	FREE(f_inf);
    return 1;
}
/*************************************************
Function: int http_QQ_2014(HttpSession *dlp_http, char **http_IM_json)
Description: QQ2014（6.3、6.4版本）的离线文件传输
Calls: 
	1. char *get_sys_time()
	2. int http_IM_pro_jsonstr(HttpSession* dlp_http, dlp_http_post_head* http_session, struct IM_file_inf *IM_session, char **http_IM_json)
Called By: 
	1. int http_IM(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. HttpSession *dlp_http, 包含当前HTTP会话信息的结构体
	2. char **http_IM_json，取出产生的JSON串的二级指针
Output: 
Return: 解析成功返回1，否则返回其它整数（0或-1）
Others: 
*************************************************/
int http_QQ_2014(HttpSession *dlp_http, char **http_IM_json,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	dlp_http_post_head 		*http_session=NULL; //包含POST信息的结构体
	http_session = (dlp_http_post_head*)dlp_http->http; //强制类型转换，获得当前POST的信息
	char 					uuid_buf[70] = {};   //定义数组，存放UUID
	//该字符串QQ2014（6.3、6.4）离线文件传输POST的URL的前几个字节
	const char 				*qq_2014_url = "/ftn_handler?bmd5=";
	struct IM_file_inf 		*f_inf = NULL;//存放所传文件的信息
	f_inf = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
	if(NULL == f_inf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(f_inf, '\0', sizeof(struct IM_file_inf));
	/*匹配URL，若未匹配成功返回0*/
	if(memcmp(http_session->url, qq_2014_url, strlen(qq_2014_url)) != 0)
	{
		FREE(f_inf);
		return 0;
	}
	/*匹配成功，说明该POST是QQ2014（6.3、6.4）离线文件传输POST*/
	else
	{	
		memcpy(f_inf->fn, QQ_fn, strlen(QQ_fn));
		/*定义数组，存放文件名*/
		char inf_file[512] = {};
		sprintf(inf_file, "%s/%u%u%u%u.dlp", session_stream, dlp_http->key.saddr, dlp_http->key.daddr, dlp_http->key.source, dlp_http->key.dest);
		/*定义文件指针，该指针指向以inf_file命名的文件。
		这样就可以把所传输的文件信息和传输状态等信息写入此文件*/
		FILE *fqq = NULL;
		/*定义文件指针，其指向以UUID命名的文件，该文件存放原始文件*/
		FILE *f_IM = NULL;
		FILE *f_msg = NULL;//存放message body 的文件指针
		char *msg_buf = NULL;
		unsigned char qq_fs[4] = {};//POST中有关原始文件的大小，它是四个字节的16进制数
		/*Message Body中原始文件的数据块大小*/
		int len = http_session->content_length - QQ_6_3_HEADER;
		/*为POST中原始文件数据分配内存*/
		msg_buf = (char *)malloc(sizeof(char) * len);
		if(NULL == msg_buf)
		{
			log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			FREE(f_inf);
			return -1;	
		}
		memset(msg_buf, '\0', sizeof(char) * len);
		/*检查以inf_file命名的文件是否存在*/
		if((fqq = fopen(inf_file, "r")) == NULL)
		{
			/*产生UUID*/
			uuid_t randNumber;
		    uuid_generate(randNumber);
		    uuid_unparse(randNumber, uuid_buf);
		    sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
			//memcpy(f_inf->uid, uuid_buf, 70);
			/*获取系统时间*/
			char *nowtime = get_sys_time();
			memcpy(f_inf->ts, nowtime, 64);
			FREE(nowtime);
			/*把UUID和文件开始传输的时间写入以inf_file命名的文件*/
			fqq = fopen(inf_file, "w");
			if(NULL == fqq)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(msg_buf);
				FREE(f_inf);
				return -1;
			}
			fwrite(uuid_buf, 70, 1, fqq);
			fseek(fqq, 70, SEEK_SET);
			fwrite(f_inf->ts, 64, 1, fqq);
			fclose(fqq);
			fqq = NULL;
			/*读取message Body文件*/
			f_msg = fopen(http_session->new_name, "r");
			if(NULL == f_msg)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(msg_buf);
				FREE(f_inf);
				return -1;
			}
			/*读取表示原始文件大小的那四个字节的16进制数*/
			fseek(f_msg, QQ_6_3_HEADER - 20, SEEK_SET);
			fread (qq_fs, 4, 1, f_msg);
			/*计算原始文件大小*/
			f_inf->filesize = qq_fs[0] * 256 * 256 * 256 + qq_fs[1] * 256 * 256 + qq_fs[2] * 256 + qq_fs[3];
			/*读取Message Body中原始文件的数据*/
			fseek(f_msg, QQ_6_3_HEADER, SEEK_SET);
			fread (msg_buf, len, 1, f_msg);
			fclose(f_msg);
			f_msg = NULL;
			/*把读取的数据块写入UUID文件*/
			f_IM = fopen(f_inf->uid, "w");
			if(NULL == f_IM)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(msg_buf);
				FREE(f_inf);
				return -1;
			}
			fseek(f_IM, 0, SEEK_SET);
			fwrite(msg_buf, len , 1, f_IM);
			fclose(f_IM);
			f_IM = NULL;
			FREE(msg_buf);
			//假如，所传输的文件大小等于第一个post的message body中原始文件的数据块大小，则直接输出json串
			if(len == f_inf->filesize)
			{
				/*产生JSON串*/
				http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json,  sbuff_putdata,  iringbuffer_ID);
				/*删除临时文件*/
				if(remove(inf_file))
				{
					log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					return -1;
				}
			}
		}
		/*以inf_file命名的文件已经存在说明该POST属于之前的文件传输过程*/
		else
		{
			/*读取inf_file文件中的UUID*/
			fread(uuid_buf, 70, 1,fqq);
			fclose(fqq);
			fqq = NULL;
			/*定义数组存放POST序号*/
			unsigned char bp[2] = {};//存放偏移量,把URL中关于数据块的偏移量信息放入其中
			/**定义数组存放文件结束标志位*/
			char end_flag[1] = {};
			/*Message Body中原始文件的数据块在原始文件的偏移量*/
			unsigned long range = 0;
			unsigned int post_num = 0;//传输文件的POST的顺序
			//打开message body文件，读取其信息
			f_msg = fopen(http_session->new_name, "r");
			if(NULL == f_msg)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(msg_buf);
				FREE(f_inf);
				return -1;
			}
			//读取文件结束标志位
			fseek(f_msg, QQ_6_3_HEADER - 11, SEEK_SET);
			fread (end_flag, 1, 1, f_msg);

			//读取message body中所传输的数据块在原始文件中的位置信息
			fseek(f_msg, QQ_6_3_HEADER - 16, SEEK_SET);
			fread (bp, 2, 1, f_msg);
			//计算该数据块在文件中的位置
			post_num = bp[0] * 128 + ( (bp[1] & 0xf0) >>4 ) * 8 + (bp[1] & 0x0f)/2;
			range = post_num * (QQ_6_3_MAX_BLOCK - QQ_6_3_HEADER) + QQ_6_3_MIN_BLOCK - QQ_6_3_HEADER;
			//读取message body中文件数据块
			fseek(f_msg, QQ_6_3_HEADER, SEEK_SET);
			fread (msg_buf, len, 1, f_msg);
			fclose(f_msg);
			f_msg = NULL;
			//把读取到的数据块写入文件
			sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
			f_IM = fopen(f_inf->uid, "r+");
			if(NULL == f_IM )
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(msg_buf);
				FREE(f_inf);
				return -1;
			}
			fseek(f_IM, range, SEEK_SET);
			fwrite(msg_buf, len , 1, f_IM);
			fclose(f_IM);
			f_IM = NULL;
			FREE(msg_buf);
			//判断文件是否传输结束
			if(end_flag[0] != 0x02)
			{
				/*获取用户开始传输文件的时间*/
				char qts[64] = {};
				fqq = fopen(inf_file, "r");
				if(NULL == fqq)
				{
					log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					return -1;
				}
				fseek(fqq, 70, SEEK_SET);
				fread(qts, 64, 1, fqq);
				fclose(fqq);
				fqq = NULL;
				memcpy(f_inf->ts, qts, 64);
				//memcpy(f_inf->uid, uuid_buf, 70);

				//获取原始文件大小
				f_msg = fopen(http_session->new_name, "r");
				if(NULL == f_msg)
				{
					log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					return -1;
				}
				fseek(f_msg, QQ_6_3_HEADER - 20, SEEK_SET);
				fread (qq_fs, 4, 1, f_msg);
				fclose(f_msg);
				f_msg = NULL;
				//计算文件大小
				f_inf->filesize = qq_fs[0] * 256 * 256 * 256 + qq_fs[1] * 256 * 256 + qq_fs[2] * 256 + qq_fs[3];
				/*产生JSON串*/
				http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json,  sbuff_putdata, iringbuffer_ID);
				/*删除产生的临时文件，假如删除不成功，返回-1*/
				if(remove(inf_file))
				{
					log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					return -1;
				}
			}
		}
	}
	//free f_inf
	FREE(f_inf);
	return 1;
}
/*************************************************
Function: int match_IM_url(char *src, char *pattern)
Description: 匹配URL，判断该POST的归属，若未匹配成功返回0
Calls: 
Called By: 
	1. int http_QQ_2014_GOS(HttpSession *dlp_http, char **http_IM_json)
	2. int http_IM(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. char *src, 所要匹配的URL
	2. char *pattern，需要匹配的正则表达式
Output: 
Return: 匹配成功返回0，没有匹配返回1或-1
Others: 
*************************************************/

int match_IM_url(char *src, const char *pattern)
{ 
    pcre		*re = NULL; 
    const char	*error; 
	int			erroffset; 
	int			ovector[OVECCOUNT]; 
    int			rc;
    re = pcre_compile(pattern,     // pattern, 输入参数，将要被编译的字符串形式的正则表达式 
                       0,             // options, 输入参数，用来指定编译时的一些选项 
                       &error,         // errptr, 输出参数，用来输出错误信息 
                       &erroffset,     // erroffset, 输出参数，pattern中出错位置的偏移量 
                       NULL);         // tableptr, 输入参数，用来指定字符表，一般情况用NULL 
    // 返回值：被编译好的正则表达式的pcre内部表示结构 
    if (re == NULL)
    {//如果编译失败，返回错误信息 
        log_printf(ZLOG_LEVEL_ERROR, "PCRE compilation failed at offset %d: %s in -----%s------%s------%d", erroffset, error,__FILE__, __func__, __LINE__); 
        return -1; 
    } 

    rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针 
                   NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针 
                   src,   // subject, 输入参数，要被用来匹配的字符串 
                   strlen(src),  // length, 输入参数，要被用来匹配的字符串的指针 
                   0,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量 
                   0,     // options, 输入参数，用来指定匹配过程中的一些选项 
                   ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组 
                   OVECCOUNT);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小 
    // 返回值：匹配成功返回非负数，没有匹配返回负数 
    if (rc < 0)
    {//如果没有匹配，返回错误信息 i
        pcre_free(re);    
		return 1; 
    } 

	pcre_free(re);   // 释放编译正则表达式re的内存 
	return 0; 
}

/*************************************************
Function:int http_QQ_2014_GOS(HttpSession *dlp_http, char **http_IM_json)
Description: 解析QQ的群共享文件传输和微云文件上传
Calls: 
	1. int match_IM_url(char *src, char *pattern)
	2. char *get_sys_time()
	
Called By: 
	1.int http_IM(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. HttpSession *dlp_http, 包含当前HTTP会话信息的结构体
	2. char **http_IM_json，取出产生的JSON串的二级指针
Output: 
Return: 匹配成功返回1，没有匹配返回0或-1
Others: 
*************************************************/

int http_QQ_2014_GOS(HttpSession *dlp_http, char **http_IM_json,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	dlp_http_post_head		*http_session=NULL; //包含POST信息的结构体
	char					uuid_buf[70] = {};	//定义数组，存放UUID
	//正则表达式，匹配该POST的URL是否是传输QQ共享文件传输或者微云文件上传的URL
  	char 					pattern[] = "^/ftn_handler/.+bmd5=.+";
  	int 					max_block;
  	char 					end[1] = {};
  	int 					endFlag=-1;
	/*定义结构体，存放所传文件信息*/
	struct IM_file_inf *f_inf = NULL;
	f_inf = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
	if(NULL == f_inf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;	
	}
	memset(f_inf, '\0', sizeof(struct IM_file_inf));
	http_session = (dlp_http_post_head*)dlp_http->http; //强制类型转换，获得当前POST的信息
	//检测该URL是否是群共享的文件传输或者微云文件上传的URL,不是则返回0，是则解析
	if(match_IM_url(http_session->url, pattern) != 0)
	{
		FREE(f_inf);
		return 0;
	}

	else
	{	
		memcpy(f_inf->fn, QQ_fn, strlen(QQ_fn));
		/*定义数组存放由四元组拼接的字符串*/
		char			inf_file[512] = {};
		/*定义文件指针，该指针指向以inf_file命名的文件。
		这样就可以把所传输的文件信息和传输状态等信息写入此文件*/
		FILE			*fqq = NULL;
		/*定义文件指针，其指向以UUID命名的文件，该文件存放原始文件*/
		FILE			*f_IM = NULL;
		FILE			*f_msg = NULL;//存放message body 的文件指针
		/*数据缓存*/
		char			*msg_buf = NULL;
		/*文件结束的标志位*/
		char			end_flag[1] = {};
		/*Message Body中原始文件的数据块大小*/
		unsigned int	len = http_session->content_length - QQ_6_3_HEADER;
		/*把四元组拼接成字符串*/
		sprintf(inf_file, "%s/%u%u%u%u.dlp", session_stream, dlp_http->key.saddr, dlp_http->key.daddr, dlp_http->key.source, dlp_http->key.dest);
		msg_buf = (char *)malloc(sizeof(char) * len);
		if(NULL == msg_buf)
		{
			log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			FREE(f_inf);
			return -1;		
		}
		memset(msg_buf, '\0', sizeof(char) * len);
		/*判断以inf_file命名的文件是否存在*/
		if((fqq = fopen(inf_file, "r")) == NULL)
		{
			/*产生UUID*/
			uuid_t randNumber;
		    uuid_generate(randNumber);
		    uuid_unparse(randNumber, uuid_buf);
		    sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
			//memcpy(f_inf->uid, uuid_buf, 70);
			/*获取系统时间*/
			char *nowtime = get_sys_time();
			memcpy(f_inf->ts, nowtime, 64);
			FREE(nowtime);
				
			//UUID和文件开始传输时间写入以inf_file命名的文件
			fqq = fopen(inf_file, "w");
			if(NULL == fqq)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				FREE(msg_buf);
				return -1;
			}
			fwrite(uuid_buf, 70, 1, fqq);
			fseek(fqq, 70, SEEK_SET);
			fwrite(f_inf->ts, 64, 1, fqq);
			//
			if (http_session->content_length > QQ_6_3_MAX_BLOCK)
			{
				fseek(fqq, 134, SEEK_SET);
				fwrite("4", 1, 1, fqq);
				endFlag = 4;	
			}
			else 
			{
				fseek(fqq, 134, SEEK_SET);
				fwrite("2", 1, 1, fqq);	
				endFlag = 2;
			}
			fclose(fqq);
			fqq = NULL;
			//读取message body所在的文件
			f_msg = fopen(http_session->new_name, "r");
			if(NULL == f_msg)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				FREE(msg_buf);
				return -1;
			}
			//读取文件是否传输结束的标志位
			fseek(f_msg, QQ_6_3_HEADER - 11, SEEK_SET);
			fread (end_flag, 1, 1, f_msg);
			//读取原始文件文件的数据块
			fseek(f_msg, QQ_6_3_HEADER, SEEK_SET);
			fread (msg_buf, len, 1, f_msg);
			fclose(f_msg);
			f_msg = NULL;
			//把数据块写入以UUID命名的文件
			f_IM = fopen(f_inf->uid, "w");
			if(NULL == f_IM)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				FREE(msg_buf);
				return -1;
			}
			fseek(f_IM, 0, SEEK_SET);
			fwrite(msg_buf, len , 1, f_IM);
			fclose(f_IM);
			f_IM = NULL;
			
			FREE(msg_buf);
			//假如,检测到文件传输完毕的标志，则产生JSON串
			if(((end_flag[0] != 0x02) && (endFlag == 2)) || ((end_flag[0] != 0x04) && (endFlag == 4)))
			{
				/*获取文件大小*/
				f_inf->filesize = len;
				/*产生JSON串*/
				http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json, sbuff_putdata,  iringbuffer_ID);
				/*删除产生的临时文件，假如删除不成功，返回-1*/
				if(remove(inf_file))
				{
					log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					return -1;
				}
			}
		}
		/*以inf_file命名的文件已经存在说明该POST属于之前的文件传输过程*/
		else
		{
			/*读取inf_file文件中的UUID*/
			fread(uuid_buf, 70, 1,fqq);
			fseek(fqq, 134, SEEK_SET);
			fread(end, 1, 1,fqq);
			fclose(fqq);
			fqq = NULL;
			/*定义数组存放POST序号*/
			unsigned char	bp[2] = {};
			/*Message Body中原始文件的数据块在原始文件的偏移量*/
			unsigned int	range = 0;
			/*PSOT序号*/
			unsigned int	post_num = 0;

			f_msg = fopen(http_session->new_name, "r");
			if(NULL == f_msg)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				FREE(msg_buf);
				return -1;
			}
			fseek(f_msg, QQ_6_3_HEADER - 16, SEEK_SET);
			fread (bp, 2, 1, f_msg);
			if(end[0] == '2')
			{
				//计算这是第几个post
				post_num = bp[0] * 128 + ( (bp[1] & 0xf0) >>4 ) * 8 + (bp[1] & 0x0f)/2;

				//计算数据块的偏移量
				range = post_num * (QQ_6_3_MAX_BLOCK - QQ_6_3_HEADER);

				endFlag = 2;
			}
			else if(end[0] == '4')
			{
				//计算这是第几个post
				post_num = bp[0] * 64 + ( (bp[1] & 0xf0) >>4 ) * 4 + (bp[1] & 0x0f)/4;

				//计算数据块的偏移量
				range = post_num * (QQ_6_6_MAX_BLOCK - QQ_6_3_HEADER);

				endFlag = 4;
			}
			else
			{
				return -1;
			}
			//读取文件结束的标志位
			fseek(f_msg, QQ_6_3_HEADER - 11, SEEK_SET);
			fread (end_flag, 1, 1, f_msg);
			/*读取Message Body中原始文件的数据*/
			fseek(f_msg, QQ_6_3_HEADER, SEEK_SET);
			fread (msg_buf, len, 1, f_msg);
			fclose(f_msg);
			f_msg = NULL;
			//把数据块写入以UUID 命名的文件
			sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
			f_IM = fopen(f_inf->uid, "r+");
			if(NULL == f_IM )
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				FREE(msg_buf);
				return -1;
			}
			fseek(f_IM, range, SEEK_SET);
			fwrite(msg_buf, len , 1, f_IM);
			fclose(f_IM);
			f_IM = NULL;
			FREE(msg_buf);
			//假如,检测到文件传输完毕的标志，则产生JSON串
			if(((end_flag[0] != 0x02) && (endFlag == 2)) || ((end_flag[0] != 0x04) && (endFlag == 4)))
			{
				/*读取inf_file文件中原始文件的信息*/
				char qts[64] = {};
				fqq = fopen(inf_file, "r");
				if(NULL == fqq)
				{
					FREE(f_inf);
					return -1;
				}
				fseek(fqq, 70, SEEK_SET);
				/*文件开始传输时间*/
				fread(qts, 64, 1, fqq);
				fclose(fqq);
				fqq = NULL;

				memcpy(f_inf->ts, qts, 64);
				//memcpy(f_inf->uid, uuid_buf, 70);

				//读取Message Body文件，获取文件大小
				unsigned char qq_fs[4] = {};
				f_msg = fopen(http_session->new_name, "r");
				if(NULL == f_msg)
				{
					FREE(f_inf);
					return -1;
				}
				fseek(f_msg, QQ_6_3_HEADER - 20, SEEK_SET);
				fread (qq_fs, 4, 1, f_msg);
				fclose(f_msg);
				f_msg = NULL;
				//计算原始文件大小
				f_inf->filesize = qq_fs[0] * 256 * 256 * 256 + qq_fs[1] * 256 * 256 + qq_fs[2] * 256 + qq_fs[3];
				/*产生JSON串*/
				http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json,  sbuff_putdata,  iringbuffer_ID);
				/*删除产生的临时文件，假如删除不成功，返回-1*/
				if(remove(inf_file))
				{
					log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					return -1;
				}
			}
		}
	}

	FREE(f_inf);
	return 1;
}

/*************************************************
Function:int parser_feition_init(struct IM_file_inf *f_inf, dlp_http_post_head *http_session)
Description: 解析飞信离线文件传输初始化POST的Message Body，提取其中关于文件传输的信息
Calls: 
	
Called By: 
	1. int http_feition(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. struct IM_file_inf *f_inf, 包含文件信息的结构体
	2. dlp_http_post_head *http_session，包含POST信息的结构体
Output: 
Return: 匹配成功返回0，没有匹配返回1或-1
Others: 
*************************************************/
int parser_feition_init(struct IM_file_inf *f_inf, dlp_http_post_head *http_session)
{ 
    pcre		*re = NULL; 
    const char	*error; 
	int			erroffset; 
	int			ovector[OVECCOUNT]; 
    int			rc;
	const char	*stringptr = NULL;
	FILE		*f_msg = NULL;//message body 文件指针
	char		*msg_buf = NULL;//message body 数据缓存
				
	msg_buf = (char *)malloc(sizeof(char) * http_session->content_length);
	if(NULL == msg_buf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(msg_buf, '\0', sizeof(char) * http_session->content_length);
	/*读取Message Body文件的数据*/
	f_msg = fopen(http_session->new_name, "r");
	if(NULL == f_msg)
	{
//		printf(" 没有打开飞信，初始化post的 messagebody 的文件\n");
		FREE(msg_buf);
		return -1;
	}
	fread ( msg_buf, http_session->content_length, 1, f_msg);
	fclose(f_msg);
	f_msg = NULL;
	/*正则表达式*/
	char pattern1[] = ".*size=\x22(.*)\x22.*name=\x22(.*)\x22.*is-expiration-valid.*<chunk id=\"(.*)\" md5=\".*\"/>.*";
    re = pcre_compile(pattern1,     // pattern, 输入参数，将要被编译的字符串形式的正则表达式 
                       0,             // options, 输入参数，用来指定编译时的一些选项 
                       &error,         // errptr, 输出参数，用来输出错误信息 
                       &erroffset,     // erroffset, 输出参数，pattern中出错位置的偏移量 
                       NULL);         // tableptr, 输入参数，用来指定字符表，一般情况用NULL 
    // 返回值：被编译好的正则表达式的pcre内部表示结构 
    if (re == NULL)

    {//如果编译失败，返回错误信息 
            log_printf(ZLOG_LEVEL_ERROR, "PCRE compilation failed at offset %d: %s in -----%s------%s------%d", erroffset, error,__FILE__, __func__, __LINE__); 
			FREE(msg_buf);
			return -1; 
    } 
    rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针 
                   NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针 
                   msg_buf,   // subject, 输入参数，要被用来匹配的字符串 
                   http_session->content_length,  // length, 输入参数，要被用来匹配的字符串的指针 
                   0,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量 
                   0,     // options, 输入参数，用来指定匹配过程中的一些选项 
                   ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组 
                   OVECCOUNT);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小 
    // 返回值：匹配成功返回非负数，没有匹配返回负数 
    if (rc < 0)

    {//如果没有匹配，返回错误信息 
			FREE(msg_buf);
            pcre_free(re); 
            return 1; 
    } 
	//获取原始文件大小
	pcre_get_substring(msg_buf, ovector, rc, 1, &stringptr);
	f_inf->filesize = atoi(stringptr);
	pcre_free_substring(stringptr);
	//获取原始文件名
	pcre_get_substring(msg_buf, ovector, rc, 2, &stringptr);
	memcpy(f_inf->fn, stringptr, strlen(stringptr));
	pcre_free_substring(stringptr);
	//获取传输原始文件时的分块数量
	pcre_get_substring(msg_buf, ovector, rc, 3, &stringptr);
	f_inf->block = atoi(stringptr);
	pcre_free_substring(stringptr);

	pcre_free(re);   // 编译正则表达式re的内存 
	FREE(msg_buf);
	return 0; 
}

/*************************************************
Function: int http_feition(HttpSession *dlp_http, char **http_IM_json)
Description: 解析飞信离线文件传输
Calls: 
	1. int parser_feition_init(struct IM_file_inf *f_inf, dlp_http_post_head *http_session)
	2. char *get_sys_time()
	3. int parser_feition_url(char *src, struct feition_data_post *f_inf)
	
Called By: 
	1.int http_IM(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. HttpSession *dlp_http, 包含当前HTTP会话信息的结构体
	2. char **http_IM_json，取出产生的JSON串的二级指针
Output: 
Return: 匹配成功返回1，没有匹配返回0或-1
Others: 
*************************************************/
int http_feition(HttpSession *dlp_http, char **http_IM_json,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{	
	/*哈希桶*/
	hash_bucket					*pBucket=NULL;
	/*定义包含POST信息的结构体*/
	dlp_http_post_head			*http_session=NULL;
	/*强制类型转换，获得当前POST的信息*/
	http_session = (dlp_http_post_head*)dlp_http->http;
	/*定义有关当前会话信息的结构体*/
	http_IM_session				*IM_session = NULL;
	const char					*fx = "feition";
	const char					*fin = "fin=";
	//初始化post的URL前几个字符
	const char					*f_init = "/cfp/initializesession";
	const char					*pattern = "^/cfp/postdata\\?s=(.*)/(.*)&sp=.&i=.*&index=(.*)&bp=(.*)&ep=.*";
	/*传输结束时的POST的URL的前几个字符*/
	const char					*f_end = "/cfp/finishsession";
	char						**UrlInformation = NULL;
	unsigned int  				i;
	//存放飞信文件信息的文件名
	char						fei_inf_file[512] = {};
	/*包含所传文件信息的结构体*/
	struct IM_file_inf			*f_inf = NULL;
	/*存放在传输文件数据时POST的URL中提取到的信息，如："bp"、"s"、"range"*/
	struct feition_data_post    *post_data_inf = NULL;
	//定义五元组，用来产生hash桶，存放初始文件传输的post的信息
	struct  http_IM_tuple5		*IM_key = NULL;

	f_inf = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
	if(NULL == f_inf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(f_inf, '\0', sizeof(struct IM_file_inf));
	
	post_data_inf = (struct feition_data_post *)malloc(sizeof(struct feition_data_post ));
	if(NULL == post_data_inf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        FREE(f_inf);
        return -1;
	}	
	memset(post_data_inf, '\0', sizeof(struct feition_data_post ));
    
	IM_key = (struct http_IM_tuple5 *)malloc(sizeof(struct http_IM_tuple5));
	if(NULL == IM_key)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        FREE(post_data_inf);
		FREE(f_inf);
        return -1;
	}
    memset(IM_key, '\0', sizeof(struct http_IM_tuple5));
	/*给五元组赋值*/
    IM_key->IM_src = dlp_http->key.source;
    IM_key->IM_dst = dlp_http->key.dest;
    IM_key->IM_sad = dlp_http->key.saddr;
    IM_key->IM_dad = dlp_http->key.daddr;   
    memcpy(IM_key->post_id, fx, 7); 
/*****************************************************************************************************/	
	/*匹配传输data是的POST的URL*/
	if(NULL != (UrlInformation = IM_Match_Pcre(http_session->url , pattern, 4)))
	{

		memcpy(post_data_inf->s, UrlInformation[0], strlen(UrlInformation[0]));
		memcpy(post_data_inf->s1, UrlInformation[1], strlen(UrlInformation[1]));
		//获取大块数据的索引号
    	post_data_inf->index = atoi(UrlInformation[2]);
		//  获取小块数据的偏移量
    	post_data_inf->bp = atoi(UrlInformation[3]);
    	if(NULL != UrlInformation)
		{
			for(i = 0; i < 4;i++)
			{
				FREE(UrlInformation[i]);
			}
			FREE(UrlInformation);
		}
		/*拼接存放所传文件信息的文件名*/
		sprintf(fei_inf_file, "%s/%s%s%s.dlp", session_stream, http_session->host, post_data_inf->s, post_data_inf->s1);
		FILE *ff = NULL;
		//查找存放飞信文件信息的文件是否存在, 如果不存在就查找hash桶
		if(access(fei_inf_file, F_OK) != 0)
		{

			//查找hash桶，把初始化的post中关于文件的信息写入文件
			if((pBucket = find_hash(ftp_session_tbl, IM_key, sizeof(struct http_IM_tuple5))) != NULL)
			{
				IM_session=(http_IM_session *)pBucket->content;
				
				ff = fopen(fei_inf_file, "a+");
				if(NULL == ff)
				{
					FREE(f_inf);
					FREE(post_data_inf);
					FREE(IM_key);
					log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					return -1;
				}

				char	p_num[16] = {};//存放文件的分块数量
				char	sfs[64] = {};//存放文件大小
				//把分块信息写入文件
				sprintf(p_num, "%d", IM_session->block + 1);
				fseek(ff, 0, SEEK_SET);
				fwrite(p_num, 16, 1, ff);
				//UUID
				fseek(ff, 16, SEEK_SET);
				fwrite(IM_session->uid, 70, 1, ff);	
				//系统时间
				fseek(ff, 86, SEEK_SET);
				fwrite(IM_session->ts, 64, 1, ff);
				//文件名
				fseek(ff, 150, SEEK_SET);
				fwrite(IM_session->fn, 256, 1, ff);
				//文件大小
				fseek(ff, 406, SEEK_SET);
				sprintf(sfs, "%d", IM_session->fs);
				fwrite(sfs, 64, 1, ff);

				fclose(ff);
				ff = NULL;

				//计算post 的message body数据在整个文件的偏移量
				unsigned long range = post_data_inf->index*2097152 + post_data_inf->bp;
				//把数据写入文件
				sprintf(f_inf->uid, "%s/%s", session_stream, IM_session->uid);
				write_first_post(http_session->new_name, f_inf->uid, range, http_session->content_length);

				//假如第一个post 就把文件传输完毕，就输出结果
				if(memcmp(http_session->url + strlen(http_session->url) -4, fin, 4) == 0)
				{
					//memcpy(f_inf->uid, IM_session->uid, 70);
					memcpy(f_inf->ts, IM_session->ts, 64);
					memcpy(f_inf->fn, IM_session->fn, 256);
					f_inf->filesize = IM_session->fs;
					/*产生JSON串*/
					http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json, sbuff_putdata, iringbuffer_ID);
					/*删除产生的临时文件，假如删除不成功，返回-1*/
					if(remove(fei_inf_file))
					{
						log_printf(ZLOG_LEVEL_ERROR, "failed to delele file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
						FREE(f_inf);
						FREE(post_data_inf);
						FREE(IM_key);
						return -1;
					}
				}
			}
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*找到fei_inf_file，说明该POST属于之前的文件传输*/
		else
		{
			int		flen = 0; //所传输的文件大小
			char    fir_buf[16] = {}; //存放分块信息
			
			ff = fopen(fei_inf_file, "a+");
			if( NULL == ff)
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				FREE(f_inf);
				FREE(post_data_inf);
				FREE(IM_key);
				return -1;
			}
			/*检测是否有大块的数据传输完毕，所有计数加1*/
			if(memcmp(http_session->url + strlen(http_session->url) - 4, fin, 4) == 0)
			{
				fseek(ff, 0, SEEK_END);
				fwrite("1", 1, 1, ff);
				flen = ftell(ff);
			}
			/*读取ff中的计数信息*/
			fseek(ff, 0, SEEK_SET);
			fread(fir_buf , 16, 1, ff);
			/*读取UUID*/
			char uuid_buf[70] = {};
			fseek(ff, 16, SEEK_SET);
			fread(uuid_buf, 70, 1, ff);
			fclose(ff);
			ff = NULL;
			sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
			/*计算Message Body中原始文件的数据块在原始文件的偏移量*/
			unsigned long range = (unsigned long)(post_data_inf->index*2097152) + (unsigned long)post_data_inf->bp;
			/*把Message Body的数据复制到UUID文件相应位置*/
			wirte_IM_file(http_session->new_name, f_inf->uid, range, http_session->content_length);
			//判断文件是否传输完毕，假如完毕，开始输出结果
			if(flen - 470 == atoi(fir_buf))
			{
				ff = fopen(fei_inf_file, "r");
				if(NULL == ff)
				{
				    log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					FREE(post_data_inf);
					FREE(IM_key);
					return -1;
				}
				/*文件开始传输时间*/
				fseek(ff, 86, SEEK_SET);
				fread(f_inf->ts, 64, 1, ff);
				/*原始文件名*/
				fseek(ff, 150, SEEK_SET);
				fread(f_inf->fn, 256, 1, ff);
				/*原始大小*/
				fseek(ff, 406, SEEK_SET);
				char ssfs[64] = {};
				fread(ssfs, 64, 1, ff);

				fclose(ff);
				ff = NULL;
				/*把原始文件大小由字符串转为整数*/
				f_inf->filesize = atoi(ssfs);
				/*产生JSON串*/
				http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json, sbuff_putdata, iringbuffer_ID);
				/*删除产生的临时文件，假如删除不成功，返回-1*/
				if(remove(fei_inf_file))
				{
					log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					FREE(f_inf);
					FREE(post_data_inf);
					FREE(IM_key);
					return -1;
				}

			}
		}

	}
/*****************************************************************************************************/	
	/*判断POST是否是飞信离线文件传输的初始化POST，如果是以五元组建立哈希桶，存放所传文件的信息*/
	else if((memcmp(http_session->url ,f_init, strlen(f_init)) == 0) && parser_feition_init(f_inf, http_session) == 0)
    {   
		
		if((pBucket = find_hash(ftp_session_tbl, IM_key, sizeof(struct http_IM_tuple5))) == NULL)
		{
			/*分配内存*/
			LS_MEM_MALLOC(pBucket, hash_bucket);
			LS_MEM_MALLOC(IM_session, http_IM_session);
			memset(pBucket, '\0', sizeof(hash_bucket));
			memset(IM_session, '\0', sizeof(http_IM_session));
			memcpy(&IM_session->key, IM_key, sizeof(struct http_IM_tuple5));
			/*把所传文件的分块数量和文件大小存到哈希桶*/
			IM_session->block = f_inf->block;
			IM_session->fs = f_inf->filesize;
			/*产生UUID*/
			char uuid_buf[70] = {};
		    uuid_t randNumber;
	        uuid_generate(randNumber);
	        uuid_unparse(randNumber, uuid_buf);
			memcpy(IM_session->uid, uuid_buf, 70);
			/*把所传文件的名存到哈希桶*/
			memcpy(IM_session->fn, f_inf->fn, 256);
			/*获得系统时间*/
			char *nowtime = get_sys_time();
			memcpy(IM_session->ts, nowtime, 64);
			FREE(nowtime);
			pBucket->content = (void *)IM_session;
			pBucket->key = &(IM_session->key);
			/*插入哈希桶*/
			insert_hash(ftp_session_tbl, pBucket, sizeof(struct http_IM_tuple5));
		}		
	}

/*****************************************************************************************************/	
	//文件传输的会话结束，释放hash
	else if(memcmp(http_session->url, f_end, strlen(f_end)) == 0)
	{
		if((pBucket = find_hash(ftp_session_tbl, IM_key, sizeof(struct http_IM_tuple5))) != NULL)
		{
			//读桶完毕，释放桶
			remove_hash(ftp_session_tbl, pBucket, sizeof(struct http_IM_tuple5));
		    if(pBucket->content)
				free(pBucket->content);
	        if(pBucket)
				free(pBucket);
        	pBucket->content = NULL;
        	pBucket = NULL;
		}
	}
/*****************************************************************************************************/	
	else 
	{
 		FREE(f_inf);
		FREE(post_data_inf);
		FREE(IM_key);       
		return FALSE;
	}
/***************************************************************************************************/
	FREE(f_inf);
	FREE(post_data_inf);
	FREE(IM_key); 
	return TRUE;	
}
/*************************************************
Function: char *paser_blog(char *src, char *pattern)
Description: 还原微博、贴吧、BBS等内容
Calls: 
	1. char* unescape( char* s)
	
Called By: 
	1.int blog(HttpSession *dlp_http, char **http_IM_json, int n)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. char *src, Message Body数据
	2. char *pattern，需要匹配的正则表达式
Output: 
Return: 解析成功返回相应内容，错误返回NULL
Others: 
*************************************************/
char *paser_blog(char *src, const char *pattern)
{
	pcre		*re = NULL; 
    const char	*error; 
	int			erroffset; 
	int			ovector[OVECCOUNT]; 
    int			rc;
	const char  *stringptr;

	re = pcre_compile(pattern,     // pattern, 输入参数，将要被编译的字符串形式的正则表达式 
                       0,             // options, 输入参数，用来指定编译时的一些选项 
                       &error,         // errptr, 输出参数，用来输出错误信息 
                       &erroffset,     // erroffset, 输出参数，pattern中出错位置的偏移量 
                       NULL);         // tableptr, 输入参数，用来指定字符表，一般情况用NULL 
    // 返回值：被编译好的正则表达式的pcre内部表示结构 
    if (re == NULL)
    {//如果编译失败，返回错误信息 
//            printf("PCRE compilation failed at offset %d: %s\n", erroffset, error); 
            return NULL; 
    } 
    rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针 
                   NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针 
                   src,   // subject, 输入参数，要被用来匹配的字符串 
                   strlen(src),  // length, 输入参数，要被用来匹配的字符串的指针 
                   0,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量 
				   0,     // options, 输入参数，用来指定匹配过程中的一些选项 
                   ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组 
                   OVECCOUNT);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小 
    // 返回值：匹配成功返回非负数，没有匹配返回负数 
    if (rc < 0)

    {//如果没有匹配，返回错误信息 
            pcre_free(re); 
            return NULL; 
    } 
	//获取包含微博内容的子串
	pcre_get_substring(src, ovector, rc, 1, &stringptr);
	//由于子串是URL，所以需要解析该URL
	char *szText = NULL;
	szText = (char *)malloc(sizeof(char)*(strlen(stringptr) + 1));
	if(NULL == szText)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        pcre_free(re);
        return NULL;
	}
	memset(szText, '\0', sizeof(char)*(strlen(stringptr) + 1));
	memcpy(szText, stringptr, sizeof(char)*strlen(stringptr));
	/*解析URL*/
	unescape(szText);
	pcre_free_substring(stringptr);
	pcre_free(re);
	return szText;
}
/*************************************************
Function: int blog(HttpSession *dlp_http, char **http_IM_json, int n)
Description: 解析微博、贴吧、BBS等
Calls: 
	1. char *paser_blog(char *src, char *pattern)
	
Called By: 
	int http_IM(HttpSession *dlp_http, char **http_IM_json)
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. HttpSession *dlp_http, 包含当前HTTP会话信息的结构体
	2. char **http_IM_json，取出产生的JSON串的二级指针
	3. int n, 确定该POST的归属，即属于那个微博、贴吧等
Output: 
Return: 解析成功返回0，错误返回-1
Others: 
*************************************************/
int blog(HttpSession *dlp_http, char **http_IM_json, char *pattern,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	FILE					*f_IM = NULL;//存放微博内容的文件指针
	FILE					*f_msg = NULL;//message body 文件指针
	char					*msg_buf = NULL; //Message Body数据缓存
	char					uuid_buf[70] = {};//UUID
	char					*nowtime = NULL;	//系统时间
	struct IM_file_inf		*IM_session = NULL;	//存放微博、贴吧等信息的结构体
	IM_session = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
	if(NULL == IM_session)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(IM_session, '\0', sizeof(struct IM_file_inf));
	dlp_http_post_head *http_session = NULL;	
	http_session = (dlp_http_post_head*)dlp_http->http;	//强制类型转换，获得当前POST的信息
	/*产生UUID*/
	uuid_t randNumber;
	uuid_generate(randNumber);
	uuid_unparse(randNumber, uuid_buf);
	sprintf(IM_session->plain, "%s/%s", session_stream, uuid_buf);
	//memcpy(IM_session->plain, uuid_buf, 70);
	/*获取系统时间*/
	nowtime = get_sys_time();
	memcpy(IM_session->ts, nowtime, 64);
	FREE(nowtime);
	
	msg_buf = (char *)malloc(http_session->content_length + 1);
	if(NULL == msg_buf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        FREE(IM_session);
        return -1;
	}
	memset(msg_buf, '\0', http_session->content_length + 1);
	/*读取Message Body文件中的数据*/
	f_msg = fopen(http_session->new_name, "r");
	if(NULL == f_msg)
	{
		log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		FREE(msg_buf);
		FREE(IM_session);
		return -1;
	}
	fread (msg_buf, http_session->content_length, 1, f_msg);
	fclose(f_msg);
	f_msg = NULL;
	char *result = NULL;//指出取出提取到的内容
	result = paser_blog(msg_buf, pattern);	
	//假如result为空说明没有提取内容成功
	if(result == NULL)
	{
		FREE(msg_buf);
		FREE(IM_session);
		return 0;
	}
	//把提取到的内容写入文件
	f_IM = fopen(IM_session->plain, "a+");

	if(NULL == f_IM)
	{
		FREE(msg_buf);
		FREE(IM_session);
		FREE(result);
		log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}

	fwrite(result, strlen(result) , 1, f_IM);
	FREE(result);
	fclose(f_IM);
	f_IM = NULL;
	//产生JSON串
	http_IM_pro_jsonstr(dlp_http, http_session, IM_session, http_IM_json, sbuff_putdata, iringbuffer_ID);
	FREE(msg_buf);
	FREE(IM_session);
	return 0;
}
/*************************************************
Function: int http_IM(HttpSession *dlp_http, char **http_IM_json)
Description: 解析IM、微博、贴吧、BBS、网盘等的入口函数
Calls: 
	1. int blog(HttpSession *dlp_http, char **http_IM_json, int n)
	2. int http_QQ_2013(HttpSession *dlp_http,  char **http_IM_json)
	3. int http_QQ_2014(HttpSession *dlp_http, char **http_IM_json)
	4. int http_QQ_2014_GOS(HttpSession *dlp_http, char **http_IM_json)
	5. void http_Yahoo_Messager(HttpSession *dlp_http, char **http_IM_json)
	6. int http_feition(HttpSession *dlp_http, char **http_IM_json)
	7. int http_baidu_pan(HttpSession *dlp_http, char **http_IM_json)
	
Called By: 
	int do_http_file(const char *packet_content, int len, struct tuple4 *key);
Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
Input:
	1. HttpSession *dlp_http, 包含当前HTTP会话信息的结构体
	2. char **http_IM_json，取出产生的JSON串的二级指针
Output: 
Return: 解析成功返回0
Others: 
*************************************************/
int http_IM(HttpSession *dlp_http, char **http_IM_json, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	dlp_http_post_head			*http_session=NULL;
    int 				Stype_Of_Weibo_BBS_TieBa = -1;

    http_session = (dlp_http_post_head*)dlp_http->http;
//-------------------------------QQ离线文件传输--------------------------------------------------------
    if((memcmp(http_session->user_agent, IS_QQ, strlen(IS_QQ)) == 0)
		||(memcmp(http_session->user_agent, IS_QQ_1, strlen(IS_QQ_1)) == 0))
	{
		//2013 QQ	
		if(http_QQ_2013(dlp_http, http_IM_json, sbuff_putdata, iringbuffer_ID) == TRUE)
		{
			return 0;
		}

		else if(http_QQ_2014(dlp_http, http_IM_json, sbuff_putdata, iringbuffer_ID) == TRUE)
		{
			return 0;
		}
	}
//-------------------------------QQ群共享文件传输以及微云网盘文件上传------------------------------------

	else if((memcmp(http_session->user_agent, IS_QQ_2014_GOS, strlen(IS_QQ_2014_GOS)) == 0)
			||(memcmp(http_session->host, IS_WY_host, strlen(IS_WY_host)) == 0))
	{
		if(http_QQ_2014_GOS(dlp_http, http_IM_json, sbuff_putdata, iringbuffer_ID) == TRUE)
		{
			return 0;
		}
	}

//-------------------------------飞信离线文件传输--------------------------------------------------------
	else if(memcmp(http_session->user_agent, IS_FX, strlen(IS_FX)) == 0)
	{
		if(http_feition(dlp_http, http_IM_json, sbuff_putdata, iringbuffer_ID) == TRUE)
			return 0;
	}

//----------------------------------------------------------------------------------------------------

	else if((strstr(http_session->host, feition_5_5_38_host) != NULL)
			&&(match_IM_url(http_session->url, feition_5_5_38_url) == 0))
	{
		http_feition_5_5_38(dlp_http, http_IM_json, sbuff_putdata, iringbuffer_ID);
		return 0;
	}
	//-------------------------------雅虎通文件传输-----------------------------------------------------
	else if(match_IM_url(http_session->url, IS_YM_url) == 0)
	{
		if(http_Yahoo_Messager(dlp_http, http_IM_json, sbuff_putdata, iringbuffer_ID) == 1)
			return 0;	
	}
//------------------------------百度云管家---------------------------------------------------------
	/* else if((memcmp(http_session->host, baidu_yun_init_host, strlen(baidu_yun_init_host)) == 0)
			||(memcmp(http_session->host, baidu_yun_data_host, strlen(baidu_yun_data_host)) == 0))
	{
		if(http_baidu_pan(dlp_http, http_IM_json, sbuff_putdata, iringbuffer_ID) == TRUE)
			return 0;
	} */

	_IM_HOST_URL_REG 	*IM_memcmp_match_nodelist;
	_IM_HOST_URL_REG	*IM_pcre_match_nodelist;

	IM_memcmp_match_nodelist = IM_memcmp_match_head_Node;
	IM_pcre_match_nodelist = IM_pcre_match_head_Node;

	while(IM_memcmp_match_nodelist->next != NULL)
	{
		IM_memcmp_match_nodelist = IM_memcmp_match_nodelist->next;
		if(	(memcmp(http_session->host, IM_memcmp_match_nodelist->host, strlen(IM_memcmp_match_nodelist->host)) == 0)
			&& (memcmp(http_session->url, IM_memcmp_match_nodelist->url, strlen(IM_memcmp_match_nodelist->url)) == 0))
		{
			blog(dlp_http, http_IM_json, IM_memcmp_match_nodelist->reg, sbuff_putdata, iringbuffer_ID);
			return 0;
		}
	}

	while(IM_pcre_match_nodelist->next != NULL)
	{
		IM_pcre_match_nodelist = IM_pcre_match_nodelist->next;
		if((memcmp(http_session->host, IM_pcre_match_nodelist->host, strlen(IM_pcre_match_nodelist->host)) == 0)
			&& (match_IM_url(http_session->url, IM_pcre_match_nodelist->url) == 0))
		{
			blog(dlp_http, http_IM_json, IM_pcre_match_nodelist->reg, sbuff_putdata, iringbuffer_ID);
			return 0;
		}
	}

	return 0;	
}
