/************************************************************
Copyright (C), 2014, DLP
 FileName: http_IM_parser.c
 Author: Xu jiandong
 Version : V 0.2
 Date: 2014-10-22
 Description: 
	
Version:V 0.2
Function List: 
	
History: 
<author> <time> <version > <desc>
 
 ***********************************************************/
#include "http_IM_parser.h"
#include "read_IM_config.h"

//#include "nids.h"
//#include "parsertrace.h"

extern char *session_stream;

#define FREE(p)\
    if(p){\
        free(p);\
        p = NULL;\
    }

 int http_feition_5_5_38(HttpSession *dlp_http, char **http_IM_json,http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
 {
 	dlp_http_post_head 		*http_session=NULL; //包含POST信息的结构体
	http_session = (dlp_http_post_head*)dlp_http->http; //强制类型转换，获得当前POST的信息
	char 					uuid_buf[70] = {};   //定义数组，存放UUID
	char 					feition_5_5_38_tem_file[512] = {};
	char					begin[64] = {};
	char					end[64] = {};
	FILE 					*f_tem_file = NULL;
	struct IM_file_inf 		*f_inf = NULL;//存放所传文件的信息

	f_inf = (struct IM_file_inf *)malloc(sizeof(struct IM_file_inf));
	if(NULL == f_inf)
	{
		log_printf(ZLOG_LEVEL_ERROR, "Insufficient_memory: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
        return -1;
	}
	memset(f_inf, '\0', sizeof(struct IM_file_inf));
	
	/*产生UUID*/
	uuid_t randNumber;
	uuid_generate(randNumber);
	uuid_unparse(randNumber, uuid_buf);

	sprintf(feition_5_5_38_tem_file, "%s/feition_5_5_38%s.dlp", session_stream, http_session->UploadtaskID);
	if(access(feition_5_5_38_tem_file, F_OK) != 0)
	{
		char *nowtime = get_sys_time();
		memcpy(f_inf->ts, nowtime, 64);
		FREE(nowtime);

		if(NULL == (f_tem_file = fopen(feition_5_5_38_tem_file, "w+")))
		{
			log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			FREE(f_inf);
			return -1;
		}

		uuid_t randNumber;
		uuid_generate(randNumber);
		uuid_unparse(randNumber, uuid_buf);
		
		fseek(f_tem_file, 0, SEEK_SET);
		fwrite(uuid_buf, 70, 1, f_tem_file);
		fseek(f_tem_file, 70, SEEK_SET);
		fwrite(f_inf->ts, 64, 1, f_tem_file);
		fclose(f_tem_file);
		f_tem_file = NULL;

		sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
		write_first_post(http_session->new_name, f_inf->uid, 0, http_session->content_length);

		sscanf(http_session->range, "bytes=%*[^-]-%s", end);

		if(atoi(end) + 1 == http_session->contentSize)
		{
			f_inf->filesize = http_session->contentSize;
			sscanf(http_session->content_type, "%*[^=]=%s", f_inf->fn);
			http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json, sbuff_putdata, iringbuffer_ID);
			if(remove(feition_5_5_38_tem_file))
            {
				log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
                FREE(f_inf);
                return -1;
            }
		}			
	}
//////////////////////////////////////////////////////////////////////////////////////
	else
	{
		if(NULL == (f_tem_file = fopen(feition_5_5_38_tem_file, "r")))
		{
			log_printf(ZLOG_LEVEL_ERROR, "failed to open file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			FREE(f_inf);
			return 0;
		}

		fseek(f_tem_file, 0, SEEK_SET);
		fread(uuid_buf, 70, 1, f_tem_file);
		fseek(f_tem_file, 70, SEEK_SET);
		fread(f_inf->ts, 64, 1, f_tem_file);
		fclose(f_tem_file);
		f_tem_file = NULL;

		sscanf(http_session->range, "bytes=%[^-]-%s", begin, end);
		sprintf(f_inf->uid, "%s/%s", session_stream, uuid_buf);
		wirte_IM_file(http_session->new_name, f_inf->uid, atoi(begin), http_session->content_length);

		if(atoi(end) + 1 == http_session->contentSize)
		{
			f_inf->filesize = http_session->contentSize;
			sscanf(http_session->content_type, "%*[^=]=%s", f_inf->fn);
			http_IM_pro_jsonstr(dlp_http, http_session, f_inf, http_IM_json, sbuff_putdata, iringbuffer_ID);
			if(remove(feition_5_5_38_tem_file))
            {
				log_printf(ZLOG_LEVEL_ERROR, "failed to delete file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
                FREE(f_inf);
                return -1;
            }
		}		
	}
	FREE(f_inf);
	return 0;
 }
