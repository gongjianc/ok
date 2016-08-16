/**
 * @file http_netdisk.c
 * @brief 支持百度云盘
 * @author xujiandong
 * @version 2014.10.17
 * @date 2014-10.17
 */
#include"http_IM_parser.h"
#include"beap_hash.h"
//#include "nids.h"

#define		BAIDU_PAN_BLOCK		4194504

#if 1
#define FREE(p)\
    if(p){\
        free(p);\
        p = NULL;\
    }
#endif

extern char *session_stream;
//---------------------------------------------------------------------------------------
//功能：提取百度云盘文件传输的文件数据块
//输入：uuid是重组后的文件名， post_num是POST 的有关文件的数据块在原始文件的位置顺序，
//flag是有关文件写入信息的标志位， http_session是包含POST 信息的结构提
//返回：解析成功返回0
//---------------------------------------------------------------------------------------
int parser_baidu_pan_data_msg(char *uuid, int post_num, int flag, dlp_http_post_head *http_session)
{
    pcre			*re = NULL;
    const char		*error;
    int				erroffset;
    int				ovector[OVECCOUNT];
    int				rc;
	char			*src = NULL;
    const char		*stringptr = NULL;
	char			*substring_start = NULL;
	unsigned long	substring_length[3] = {};
	FILE			*fuid = NULL;
	FILE			*fmsg = NULL;
	unsigned		range = 0;
	char			pattern[1024] = {};
	sprintf(pattern, "(^--%s\\r\\nContent-Disposition:.+\\r\\nContent-Type:.+\\r\\n\\r\\n)([\\s\\S]*)(\\r\\n--%s--\\r\\n)", http_session->boundary, http_session->boundary);
	fmsg = fopen(http_session->new_name, "r");
	if(NULL == fmsg)
	{
		log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	src = (char *)malloc(sizeof(char) * http_session->content_length);
	if(NULL == src)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	memset(src, '\0', sizeof(char) * http_session->content_length);
	fread(src, http_session->content_length, 1, fmsg);
	fclose(fmsg);
	fmsg = NULL;

	re = pcre_compile(pattern,     // pattern, 输入参数，将要被编译的字符串形式的正则表达式 
                       0,             // options, 输入参数，用来指定编译时的一些选项 
                       &error,         // errptr, 输出参数，用来输出错误信息 
                       &erroffset,     // erroffset, 输出参数，pattern中出错位置的偏移量 
                       NULL);         // tableptr, 输入参数，用来指定字符表，一般情况用NULL 
    // 返回值：被编译好的正则表达式的pcre内部表示结构 
    if (re == NULL)
    {//如果编译失败，返回错误信息  
            log_printf(ZLOG_LEVEL_ERROR, "PCRE compilation failed at offset %d: %s in -----%s------%s------%d", erroffset, error,__FILE__, __func__, __LINE__); 
			FREE(src);
            return -1;
    }
    rc = pcre_exec(re,    // code, 输入参数，用pcre_compile编译好的正则表达结构的指针 
                   NULL,  // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针 
                   src,   // subject, 输入参数，要被用来匹配的字符串 
                   http_session->content_length,  // length, 输入参数，要被用来匹配的字符串的指针 
                   0,     // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量 
                   0,     // options, 输入参数，用来指定匹配过程中的一些选项 
                   ovector,        // ovector, 输出参数，用来返回匹配位置偏移量的数组 
                   OVECCOUNT);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小 
    // 返回值：匹配成功返回非负数，没有匹配返回负数 
    if (rc < 0)

    {//如果没有匹配，返回错误信息 
        pcre_free(re);
		FREE(src)
        return 1;
    }

	substring_length[0] = ovector[3] - ovector[2];
	//传输的文件的数据块
	substring_start = src + ovector[4];
	substring_length[1] = ovector[5] - ovector[4];
	
	substring_length[2] = ovector[7] - ovector[6];
	if(flag == 1)
	{	
		fuid = fopen(uuid, "w");
	}
	else
	{
		fuid = fopen(uuid, "r+");
	}
	if(NULL == fuid)
	{
		FREE(src);
		pcre_free(re);
		log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	range = (BAIDU_PAN_BLOCK - substring_length[0] - substring_length[2]) * post_num;
	fseek(fuid, range, SEEK_SET);
	fwrite(substring_start, substring_length[1], 1, fuid);
	fclose(fuid);
	fuid = NULL;
	FREE(src);
    pcre_free(re);   // 编译正则表达式re 释放内存 
    return 0;
}
//---------------------------------------------------------------------------------------
//功能：解析百度云传输文件的主函数
//输入：dlp_http是包含POST 信息的结构提指针
//返回：解析成功返回0
//---------------------------------------------------------------------------------------

int http_baidu_pan(HttpSession *dlp_http, char **http_IM_json,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
	dlp_http_post_head 	*http_session = NULL;
    http_session = (dlp_http_post_head*)dlp_http->http;
	char 				uuid_buf[70] = {};
	const char 			*init_host = "pan.baidu.com";   //文件传输初始化POST 的HOST
	const char 			*data_host = "c.pcs.baidu.com"; //传输数据的POST的HOST
	const char 			*InitUrlReg = "^/api/precreate.+&logid=(.+)";
	const char 			*InitMsgReg = "^path=/(.+)&size=(.+)&isdir.+";
	const char 			*DataUrlReg = "^/rest/2.0/pcs/superfile2.+&partseq=(.+)&client_ip=.+&logid=(.+)";
	char 				**UrlInformation = NULL;
	FILE				*fid = NULL;					//存放有关文件信息的文件指针	
	FILE				*fuid = NULL;					//重组后的文件的指针
	FILE				*fmsg = NULL;					//post 的message body
	char				*buffer = NULL;					//mesaage body 缓存
	unsigned char		post_num[4] = {};				//POST 顺序
	unsigned char		fs_buf[16] = {};				//原始文件大小
	char				*nowtime = NULL;				//系统时间
	char 				tempfile[512] = {};				//存放文件信息的临时文件

    /*定义，结构体，存放文件信息*/
    struct IM_file_inf 	*f_inf = NULL;
    f_inf = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
    if(NULL == f_inf)
    {
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
    }
    memset(f_inf, '\0', sizeof(struct IM_file_inf));
	//解析传输数据部分的URL
	if((memcmp(http_session->host, data_host, strlen(data_host)) == 0)
		&&(NULL != (UrlInformation = IM_Match_Pcre(http_session->url, DataUrlReg, 2))))
	{
		f_inf->block = atoi(UrlInformation[0]); 
    	memcpy(f_inf->filekey, UrlInformation[1], strlen(UrlInformation[1]));
		sprintf(tempfile, "%s/%s.dlp", session_stream, f_inf->filekey);
    	FREE(UrlInformation[0]);
		FREE(UrlInformation[1]);
		FREE(UrlInformation);
		//判断存放原始文件信息的文件是否存在
		if(access(tempfile, F_OK) != 0)
		{
			FREE(f_inf);
			return FALSE;
		}

		else
		{
			fid = fopen(tempfile, "r+");
			if(NULL == fid)
			{
				FREE(f_inf);
		        log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				return -1;
			}
			fseek(fid, 0, SEEK_SET);
			fread(post_num, 4, 1, fid);
			fseek(fid, 4, SEEK_SET);
			fread(uuid_buf, 70, 1, fid);
			sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
			fseek(fid, 0, SEEK_END);
			int flen = ftell(fid);
			int flag = 0;
			if(flen == 410)
			{
				flag = 1;
			}
			fwrite("1", 1, 1, fid);
			fclose(fid);
			fid = NULL;
			//提取POST 的message body 中有关原始文件的数据块
			if(parser_baidu_pan_data_msg(f_inf->uid, f_inf->block, flag, http_session) != 0)
			{
				FREE(f_inf);
				return FALSE;
			}
			//判断原始文件是否传输完毕
			if(flen - 410 == atoi(post_num))
			{
				fid = fopen(tempfile, "r");
				if(NULL == fid)
				{
					FREE(f_inf);
					log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
					return -1;
				}
				fseek(fid, 74, SEEK_SET);
				fread(f_inf->ts, 64, 1, fid);
				fseek(fid, 138, SEEK_SET);
				fread(fs_buf, 16, 1, fid);
				f_inf->filesize = strtoul(fs_buf, NULL, 10);
				fseek(fid, 154, SEEK_SET);
				fread(f_inf->fn, 256, 1, fid);
				fclose(fid);
				fid = NULL;

				http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json,sbuff_putdata, iringbuffer_ID);
				
				if(remove(tempfile))
                {
					log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
                    FREE(f_inf);
                    return -1;
                }
			}
		}
	}
	//解析传输原始文件的初始化的POST 的有关原始文件的信息
	else if((memcmp(http_session->host, init_host, strlen(init_host)) == 0)
		&&(NULL != (UrlInformation = IM_Match_Pcre(http_session->url, InitUrlReg, 1))))
	{
		memcpy(f_inf->filekey, UrlInformation[0], strlen(UrlInformation[0]));
		FREE(UrlInformation[0]);
		FREE(UrlInformation);
		uuid_t randNumber;
        uuid_generate(randNumber);
        uuid_unparse(randNumber, uuid_buf);
		fmsg = fopen(http_session->new_name, "r");
		if(NULL == fmsg)
		{
			FREE(f_inf);
		    log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			return -1;
		}
		buffer = (char *)malloc( sizeof(char) * http_session->content_length);
		if(NULL == buffer)
		{
			log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			return -1;
		}
		memset(buffer, '\0', sizeof(char) * http_session->content_length);
		fread(buffer, http_session->content_length, 1, fmsg);
		fclose(fmsg);
		fmsg = NULL;

		if(NULL == (UrlInformation = IM_Match_Pcre(buffer, InitMsgReg, 2)))
		{
			FREE(f_inf);
			FREE(buffer);
			return FALSE;
		}
		memcpy(f_inf->fn, UrlInformation[0], strlen(UrlInformation[0]));
		unescape(f_inf->fn);
    	f_inf->filesize = strtoul(UrlInformation[1], NULL, 10);

    	FREE(UrlInformation[0]);
    	FREE(UrlInformation[1]);
		FREE(UrlInformation);
		FREE(buffer);

		sprintf(post_num, "%d", (int)(f_inf->filesize / BAIDU_PAN_BLOCK));
		sprintf(fs_buf, "%u", f_inf->filesize);
		sprintf(tempfile, "%s.dlp", f_inf->filekey);
		fid = fopen(tempfile, "w");
		if(NULL == fid)
		{
			FREE(f_inf);
 			log_printf(ZLOG_LEVEL_ERROR, "failed open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
 			return -1;
		}
		nowtime = get_sys_time();
		fseek(fid, 0, SEEK_SET);
		fwrite(post_num, 4, 1, fid);
		fseek(fid, 4, SEEK_SET);
		fwrite(uuid_buf, 70, 1, fid);
		fseek(fid, 74, SEEK_SET);
		fwrite(nowtime, 64, 1, fid);
		FREE(nowtime);
		fseek(fid, 138, SEEK_SET);
		fwrite(fs_buf, 16, 1, fid);
		fseek(fid, 154, SEEK_SET);
		fwrite(f_inf->fn, 256, 1, fid);
		fclose(fid);
		fid = NULL;
	}
	FREE(f_inf);
	return TRUE;
}
