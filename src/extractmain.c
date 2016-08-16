/*
 * =====================================================================================
 *
 *       Filename:  extractmain.c
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

extern char *session_json;
#include "extractmain.h"
extern int get_file_num(char *jsonstr, int *file_num);
#if 0
/* 消息队列的最大字节数 */
#ifndef MAX_MESSAGE_QUEUE
#define MAX_MESSAGE_QUEUE 20971520
#endif
#endif

#if 0
/* tika解析的文件最大长度 */
#ifndef MAX_FILESIZE_TIKA_PARSER
#define MAX_FILESIZE_TIKA_PARSER 1024*1024*100
#endif
#endif

/* 去掉tika解析 */
#if 0
#define NOTIKA 
#endif

#if 1
#define TANG_LOG
#endif

/* 发送消息给策略匹配的全局锁 */
pthread_mutex_t write_outlog_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef TANG_LOG
/* 记录发送消息个数 */
int outdata;
#endif


/**
 * @brief  调用tika解析 
 *
 * @param filefull_fname      文件全名
 * @param file_content_path   文件内容路径
 * @param tika_message        tika返回的信息
 *
 * @return  成功返回0，失败返回-1
 */
int tika_file_parser (char *filefull_fname, char *file_content_path, tika_message_s *tika_message)
{
		char *tikastr = NULL;
		int res = 0;
        log_printf (ZLOG_LEVEL_INFO, "get tika parser message...... ");
		log_printf (ZLOG_LEVEL_INFO, "filefull_fname=%s ", filefull_fname);
		log_printf (ZLOG_LEVEL_INFO, "file_content_path=%s ", file_content_path);
		log_printf (ZLOG_LEVEL_INFO, "call tika parse file list ");
		log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);

#ifdef NOTIKA 
		/* 用于自己测试不调用tika */
	    //char *str = "{\"filetype\":\"pptx\",\"isSuccess\":\"true\",\"fileAuthor\":\"陈洪波\",\"isEncrypted\":\"false\"}";
	    char *str = "{\"filetype\":\"pptx\",\"isSuccess\":\"true\",\"fileAuthor\":\"陈洪波\",\"isEncrypted\":\"true\"}";
	    tikastr = malloc(strlen(str)+1);
	    if (tikastr == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    strcpy(tikastr, str);
#else
		/* 调用tika进行文件识别和解析 */
	    tikastr = tika (filefull_fname, file_content_path);
#endif
	    if (tikastr == NULL || (strlen(tikastr) == 0))
	    {
			log_printf (ZLOG_LEVEL_ERROR, "tika() return NULL, tika parse error");
		    log_printf (ZLOG_LEVEL_ERROR, "filefull_fname=%s ", filefull_fname);
		    log_printf (ZLOG_LEVEL_ERROR, "file_content_path=%s ", file_content_path);
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
			return -1;
	    }

		log_printf (ZLOG_LEVEL_INFO, "tika parse return json string is: %s ", tikastr);
		log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);

		/* 获取tika解析出的文件属性信息:文件类型,作者,解析是否成功,文件是否加密 */
        res = get_file_type_author (tikastr, tika_message);
		if (res == -1)
		{
		    log_printf (ZLOG_LEVEL_ERROR, "get tika message error ");
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
			return -1;
		}
        if (tikastr)
		{
			free(tikastr);
			tikastr = NULL;
		}

		/* 解析失败，但是文件是非加密的，返回错误信息,删除文件 */
		if ((strcmp (tika_message->issuccess, "false") == 0) && (strcmp (tika_message->isEncrypted, "false") == 0))
		{
            log_printf (ZLOG_LEVEL_ERROR, "tika() return json string element issuccess is false, tika parse error isEncrypted is false, tika parse reult no return");
		    log_printf (ZLOG_LEVEL_ERROR, "filefull_fname=%s ", filefull_fname);
		    log_printf (ZLOG_LEVEL_ERROR, "file_content_path=%s ", file_content_path);
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);

			return -1;
		}

		/* 解析失败，但是文件是加密的，返回正确信息,不删除文件 */
		else if ((strcmp (tika_message->issuccess, "false") == 0) && (strcmp (tika_message->isEncrypted, "true") == 0))
		{
            log_printf (ZLOG_LEVEL_WARN, "tika() return json string element issuccess is false but isEncrypted is true, tika parse result return");
		    log_printf (ZLOG_LEVEL_WARN, "filefull_fname=%s ", filefull_fname);
		    log_printf (ZLOG_LEVEL_WARN, "file_content_path=%s ", file_content_path);
		    log_printf (ZLOG_LEVEL_WARN, "%s__%d ", __FILE__, __LINE__);

			return 0;
		}
#if 0
		/* 不存在这种情况，tika外层解析失败，就识别不出内部的嵌套 */
        else if ((strcmp (tika_message->issuccess, "false") == 0) && (strcmp (tika_message->isEncrypted, "embededEncrypted") == 0))
		{
            log_printf (ZLOG_LEVEL_WARN, "tika() return json string element issuccess is false but isEncrypted is embededEncrypted, tika parse result return");
		    log_printf (ZLOG_LEVEL_WARN, "filefull_fname=%s ", filefull_fname);
		    log_printf (ZLOG_LEVEL_WARN, "file_content_path=%s ", file_content_path);
		    log_printf (ZLOG_LEVEL_WARN, "%s__%d ", __FILE__, __LINE__);

			return 0;
		}
#endif

#if 0
        if ((tika_message->author != NULL) && (strlen (tika_message->author) != 0))
		{
           log_printf (ZLOG_LEVEL_DEBUG, "author is: %s", tika_message->author);
		   log_printf (ZLOG_LEVEL_DEBUG, "%s__%d", __FILE__, __LINE__);
		}

        if ((tika_message->filetype == NULL) || (strlen (tika_message->filetype) == 0))
		{
           log_printf (ZLOG_LEVEL_DEBUG, "file type unknown");
		   log_printf (ZLOG_LEVEL_DEBUG, "filefull_fname=%s", filefull_fname);
		   log_printf (ZLOG_LEVEL_DEBUG, "file_content_path=%s", file_content_path);
		   log_printf (ZLOG_LEVEL_DEBUG, "%s__%d", __FILE__, __LINE__);
		}
		else
		{
           log_printf (ZLOG_LEVEL_DEBUG, "file type is: %s", tika_message->filetype);
		   log_printf (ZLOG_LEVEL_DEBUG, "%s__%d", __FILE__, __LINE__);
		}
#endif
		return 0;
}



/**
 * @brief 记录发送消息队列个数和内容日志
 *
 * @param logname 日志文件名称
 * @param thing   记录的内容
 * @param data    数据个数
 *
 * @return  成功返回0，失败返回-1
 */
int record_log (char *logname, char *thing, int *data)
{
	/* 判断文件是否存在，不存在就重置全局计数器 */
    if (access (logname, F_OK) != 0)
	{
		*data = 1;
	}
	else
	{
		*data = *data + 1;
	}


	/* 追加写入日志信息 */
	FILE *fp = fopen (logname, "a+");
	int res = 0;
    if (fp == NULL)
    {
    	log_printf (ZLOG_LEVEL_ERROR, "open record log");
		log_printf (ZLOG_LEVEL_ERROR, "logname:%s ", logname);
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
    }

	if ((res = fwrite(thing, strlen(thing), 1, fp)) != 1)
	{
    	log_printf (ZLOG_LEVEL_ERROR, "fwrite error");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}

	if ((res = fwrite("\n", strlen("\n"), 1, fp)) != 1)
	{
    	log_printf (ZLOG_LEVEL_ERROR, "fwrite error");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}
    char num[128] = {};
    sprintf (num, "%d", *data);
	if ((res = fwrite(num, strlen(num), 1, fp)) != 1)
	{
    	log_printf (ZLOG_LEVEL_ERROR, "fwrite error");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}
	if ((res = fwrite("\n", strlen("\n"), 1, fp)) != 1)
	{
    	log_printf (ZLOG_LEVEL_ERROR, "fwrite error");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}
	if (fp)
	{
  	    fclose(fp);
        fp = NULL;	
	}
	return 0;
}

/**
 * @brief 获得文件大小
 *
 * @param path 文件路径
 *
 * @return   文件大小
 */
unsigned long get_file_size (const char *path)
{

	/* 判断文件是否存在 */
	if (access(path, F_OK) != 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "file not exist, get_file_size() function error");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d, file path:%s ", __FILE__, __LINE__, path);
		return -1;
	}
	unsigned long filesize = -1;
	struct stat statbuff;
	if (stat (path, &statbuff) < 0)
	{
		log_printf (ZLOG_LEVEL_DEBUG, "stat(), file not exist :filesize=%d ", filesize);
		log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
		return filesize;
	}
	else
	{
		filesize = statbuff.st_size;
		log_printf (ZLOG_LEVEL_DEBUG, "stat(), filesize=%d ", filesize);
		log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
		return filesize;
	}
	return 0;
}


/**
 * @brief 产生uuid
 *
 * @param buf[128] 存储uuid
 *
 * @return  成功返回0，失败返回-1
 */
int random_uuid(char buf[128]) 
{
	uuid_t randNumber;
    uuid_generate(randNumber);
	uuid_unparse(randNumber, buf);
	log_printf (ZLOG_LEVEL_DEBUG, "UUID generate = %s ", buf);
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
	return 0;
}

/**
 * @brief 发送消息给策略匹配
 *
 * @param jsondest  解析的内容json串,
 * @param strpath   写入文件，给策略匹配发送json的路径
 *
 * @return  成功返回0，失败返回-1
 */
int send_msgqueue (char *jsondest, char *strpath)
{
	static   long long err_i=0;
	if (jsondest == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "jsondest is NULL");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}
	if (strpath == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "strpath is NULL");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}
	int size = 0;
	int res = 0;
    char jsonbuf[128] = {};
	int randnum = 0;
    int msgid = 0;
    key_t key;

    /* 发送json信息到消息队列 */
	struct nids_pm_msg fmsg;
	memset (&fmsg, 0, sizeof(struct nids_pm_msg));
	//memset (&(fmsg.pathmsg), 0, sizeof(struct path_msg));
      memset (&(fmsg.topath), 0, 128);


#if 0
	/* 获取文件路径 */
    char  strpath[128] = {};
	/* flist中的第0个 */
	int file_flag = 0;
	res = get_file_path (jsondest, strpath, file_flag);
	if (res == -1)
	{
        log_printf (ZLOG_LEVEL_ERROR, "没有附件列表，或者文件为空");
	}
#endif

	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
	//strcpy (fmsg.pathmsg.dir, strpath);

	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
	char uuid_buf[80] = {};
    random_uuid(uuid_buf);
	log_printf (ZLOG_LEVEL_DEBUG, "uuid=%s ", uuid_buf);
	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
	//strcpy (fmsg.pathmsg.uuid, uuid_buf);

	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
	/* 文件名称去重 */
    do
	{
		randnum++;
	    sprintf (jsonbuf, "%d_%s_json_string", randnum, uuid_buf);
		log_printf (ZLOG_LEVEL_DEBUG, "size=%d ", strlen(jsonbuf));
		log_printf (ZLOG_LEVEL_DEBUG, "jsonbuf=%s ", jsonbuf);
	    log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
	    //strcpy (fmsg.pathmsg.name, jsonbuf);
	    sprintf (jsonbuf, "%s/%d_%s_json_string", strpath, randnum, uuid_buf);
	} while (access(jsonbuf, F_OK) == 0);

      strcpy (fmsg.topath, jsonbuf);
	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);

	if (strlen(jsonbuf) != 0)
	{
		log_printf (ZLOG_LEVEL_INFO, "json string path：%s ", jsonbuf);
	    log_printf (ZLOG_LEVEL_INFO, "%s____%d ", __FILE__, __LINE__);

#if 1
		/* json串存储的文件 */
		FILE* fp = fopen(jsonbuf, "a+");
		if (fp == NULL)
		{
			log_printf (ZLOG_LEVEL_ERROR, "fopen");
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
			return -1;
		}

	    log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
		res = fwrite (jsondest, strlen(jsondest), 1, fp); 
		if (res != 1)
		{
			log_printf (ZLOG_LEVEL_ERROR, "write");
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
			if (fp)
			{
				fclose(fp);
				fp = NULL;
			}
	        return -1;
		}

		res = fwrite ("\n", strlen("\n"), 1, fp); 
		if (res != 1)
		{
			log_printf (ZLOG_LEVEL_ERROR, "fwrite");
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
            if (fp)
			{
				fclose(fp);
				fp = NULL;
			}
	        return -1;
		}
		if (fp)
		{
		    fclose(fp);
			fp = NULL;
		}

#endif



	    log_printf (ZLOG_LEVEL_INFO, "send message count ########################################################outdata = %d ", outdata);
	    log_printf (ZLOG_LEVEL_INFO, "%s____%d ", __FILE__, __LINE__);

        /* 创建消息队列 */
        key = FILE_KEY;
	    log_printf (ZLOG_LEVEL_INFO, "file parse send message queue to PolicyMatching........... ");
	    log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ",__FILE__, __LINE__);
        msgid = msgget (key, 0666 | IPC_CREAT);
        if (msgid == -1) 
        {
           log_printf (ZLOG_LEVEL_ERROR, "msgget");
		   log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
	       return -1;
        }
        
        fmsg.mtype = 1;
	    log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ",__FILE__, __LINE__);

        /* 发送 */
	    size = sizeof(struct nids_pm_msg);
		log_printf (ZLOG_LEVEL_DEBUG, "sizeof struct= %d ", size);
	    log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ",__FILE__, __LINE__);
		/* 判断消息队列是否满了  MAX_MESSAGE_QUEUE*/
	    struct msqid_ds ds;
	    msgctl(msgid, IPC_STAT, &ds);
		//if ((ds.__msg_cbytes + size) > MAX_MESSAGE_QUEUE)
		/* 判断当前消息队列的总大小是否超出设置的最大值 */
		if ((ds.__msg_cbytes + size) > ds.msg_qbytes)
		{
              log_printf (ZLOG_LEVEL_ERROR, "message queue is max not allow send continue !!!!!!!!!!!!!!!!!!!!!!!!!!!");
              log_printf (ZLOG_LEVEL_ERROR, "start delnoson.sh delete 24 hours ago file");
              log_printf (ZLOG_LEVEL_ERROR, "__msg_cbytes= %ld\n", ds.__msg_cbytes);
              log_printf (ZLOG_LEVEL_ERROR, "msg_qbytes = %ld\n", ds.msg_qbytes);
		      log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
			  /* 启动定时删除脚本 */
			  //system ("chmod u+x delnoson.sh");
			  //system ("./delnoson.sh");

		      return -1;
		}
        res = msgsnd (msgid, &fmsg, size, 0);
        if (res == -1)
        {

				 log_printf(ZLOG_LEVEL_INFO,"\n\n#### Message error  = %d\n\n",err_i++);
				perror("Msg send Policy Error\n");
				 log_printf(ZLOG_LEVEL_INFO,"\n\n#### Send error Info %s\n\n",strerror(res));
           log_printf (ZLOG_LEVEL_ERROR, "msgsnd");
	       log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
	       return  -1;
        }
				else
				{
					log_printf(ZLOG_LEVEL_INFO,"\n\n#### Message send ok = %d\n\n",err_i);
					log_printf (ZLOG_LEVEL_DEBUG, "send to PolicyMaching json string is=%s ", jsondest);
					log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
				}

#ifdef TANG_LOG
		/* 发送消息队列的日志 */
		char *caplog = "./session_capture.log";
        log_printf (ZLOG_LEVEL_INFO, "session_capture.log, send message log path is:%s ", caplog);
	    log_printf (ZLOG_LEVEL_INFO, "%s____%d ", __FILE__, __LINE__);
        if (access (caplog, F_OK) != 0)
	    {
	    	outdata = 1;
	    }
        /* 记录发送的json串内容，写入文件 */
        record_log (caplog, jsondest, &outdata);
#endif

	}

	log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ",__FILE__, __LINE__);

	return 0;
}

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
int file_extract_pthread (char *jsonstr, char *strpath, unsigned long max_filesize)
{     //printf("json=============================%s\n",jsonstr);
	log_printf (ZLOG_LEVEL_INFO, "file_extract_pthread() beging...... ");
	log_printf (ZLOG_LEVEL_INFO, "%s____%d ", __FILE__, __LINE__); 
    if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "json string is null ");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}
	log_printf (ZLOG_LEVEL_INFO, "%s__%d, received json string is: %s ", __FILE__, __LINE__, jsonstr);

    int res = 0, size = 0;
#if 1
    char file_content_path [NAMELEN] = {};
    char filefull_fname[NAMELEN] = {};
	tika_message_s tika_message;
	memset (&tika_message, 0, sizeof(tika_message_s));

    char *jsondest = NULL;

	int filenum = 0;
	int file_flag = 0;

    /* 获取的文件个数 */
	res = get_file_num (jsonstr, &filenum);
    if ((res == -1) || (filenum <= 0))
	{
	    log_printf (ZLOG_LEVEL_WARN, "no file parsed from json string ");
	    log_printf (ZLOG_LEVEL_WARN, "%s_%d ",__FILE__, __LINE__);
	}

	size = strlen(jsonstr); 
	jsondest = malloc (size+1);
	if (jsondest == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (jsondest, 0, size+1);
	strcpy (jsondest, jsonstr);
	char *jsontmp = jsondest;

    file_flag = 0;
	while(1)
	{
        /* 获取解析成功后的的文件个数 */
		log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, parsing json string, jsontmp = %s ", __FILE__, __LINE__, jsontmp);
	    res = get_file_num (jsontmp, &filenum);
        if ((res == -1) || (filenum <= 0))
	    {
			/* ftp协议json串中传送的只有文件属性信息，如果文件列表flist为空，则没有发送给策略匹配的必要 */
	    	if (get_protocol_type(jsontmp) == 175)
	    	{
	             log_printf (ZLOG_LEVEL_WARN, "no file parsed from json string , 175 is ftp protocol not send ");
	             log_printf (ZLOG_LEVEL_WARN, "%s_%d ",__FILE__, __LINE__);
                 return -1;
	    	}
	    	else
	    	{
			     /* 其他协议json串中传送的信息不只是文件属性信息，如果文件列表flist为空，还是需要发送给策略匹配 */
	             log_printf (ZLOG_LEVEL_WARN, "no file parsed from json string , not ftp protocol send it ");
	             log_printf (ZLOG_LEVEL_WARN, "%s_%d ",__FILE__, __LINE__);
				 break;
	    	}
	    }

		/* 遍历完整个json数组跳出循环 */
        if (file_flag >= filenum)
		{
			log_printf (ZLOG_LEVEL_INFO, "traverse all file list finish ");
	        log_printf (ZLOG_LEVEL_INFO, "%s____%d ", __FILE__, __LINE__);
			break;
		}
		
        /* 从json串中提获取需要解析的文件绝对路径 */
        res = get_file_path_absolute (jsontmp, filefull_fname, file_flag);
        if (res == -1)
		{
			log_printf (ZLOG_LEVEL_ERROR, "get_file_path_absolute(), get file path error");
			log_printf (ZLOG_LEVEL_ERROR, "delete flist element");
	        log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
			/* 删除json串中的flist的对应项 */
            remove_flist (jsontmp, &jsondest, file_flag);
            if (jsontmp)
	        {
                free(jsontmp);
                jsontmp= NULL;
	        }
		    jsontmp = jsondest;
			/* 删除原始文件 */
			log_printf (ZLOG_LEVEL_ERROR, "delete store file");
	        log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
            //remove (filefull_fname);

            /* 下一个文件 */
			continue;
		}

		/* 筛选文件大小符合规定的文件交给tika解析 */
		int filesize = get_file_size (filefull_fname);
		/* filesize cat not <=0 or >=100M */
	    //unsigned long maxfilesize = 1024*1024*100;
	    unsigned long maxfilesize = max_filesize;
		if ((filesize <= 0) || (filesize > maxfilesize))
		{
			if (filesize <= 0)
			{
			    log_printf (ZLOG_LEVEL_ERROR, "file size is 0,no parse ,delete file of null, remove error element from json string");
	            log_printf (ZLOG_LEVEL_ERROR, "%s_%d, filefull_fname:%s ",__FILE__, __LINE__, filefull_fname);
			}
			else if (filesize > maxfilesize)
			{
			    log_printf (ZLOG_LEVEL_ERROR, "file size  > max file size");
	            log_printf (ZLOG_LEVEL_ERROR, "%s_%d, filefull_fname:%s ",__FILE__, __LINE__, filefull_fname);
			}
			/* 删除json串中的flist的对应项 */
			log_printf (ZLOG_LEVEL_ERROR, "delete flist element");
	        log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
			remove_flist (jsontmp, &jsondest, file_flag);
            if (jsontmp)
	        {
                free(jsontmp);
                jsontmp= NULL;
	        }
		    jsontmp = jsondest;
			log_printf (ZLOG_LEVEL_ERROR, "delete store file");
	        log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
            //remove (filefull_fname);
            /* 下一个文件 */
			continue;
		}

		/* 文件内容路径 */
		sprintf (file_content_path, "%s.txt", filefull_fname);

        /* 调用tika解析 */
        res = tika_file_parser (filefull_fname, file_content_path, &tika_message);
#if 0 //by niulw mask for xxx
        if (res == -1)
		{
			log_printf (ZLOG_LEVEL_ERROR, "tika parse error, delete file and remove error element from json string ");
			log_printf (ZLOG_LEVEL_ERROR, "delete flist element");
			//log_printf (ZLOG_LEVEL_ERROR, "error json string is %s", jsonstr);
	        log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
			/* 删除json串中的flist的对应项 */
			remove_flist (jsontmp, &jsondest, file_flag);
            if (jsontmp)
	        {
                free(jsontmp);
                jsontmp= NULL;
	        }
		    jsontmp = jsondest;
			log_printf (ZLOG_LEVEL_ERROR, "delete store file add content file");
	        log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
            //remove (filefull_fname);
            //remove (file_content_path);

            /* 下一个文件 */
			continue;
		}
#endif


		
        /* 设置文件类型和作者内容路径 */
        res = set_tika_message (jsontmp, &jsondest, tika_message, file_content_path, file_flag);
		if (res == -1)
		{
			log_printf (ZLOG_LEVEL_ERROR, "set file type author add file content path error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
            /* 下一个文件 */
		    file_flag++;
			continue;
		}
        if (jsontmp)
	    {
            free(jsontmp);
            jsontmp= NULL;
	    }
		jsontmp = jsondest;

        /* 下一个文件 */
		file_flag++;
		
	}

    /* 获取解析成功后的的文件个数 */
	res = get_file_num (jsondest, &filenum);
    if ((res == -1) || (filenum <= 0))
	{
		if (get_protocol_type(jsondest) == 175)
		{
	         log_printf (ZLOG_LEVEL_WARN, "no file parsed from json string , is ftp protocol not send ");
			 log_printf (ZLOG_LEVEL_WARN, "%s__%d ", __FILE__, __LINE__);
             return -1;
		}
		else
		{
	         log_printf (ZLOG_LEVEL_WARN, "no file parsed from json string , not ftp protocol send it ");
			 log_printf (ZLOG_LEVEL_WARN, "%s__%d ", __FILE__, __LINE__);
		}
	}
	else
	{
		log_printf (ZLOG_LEVEL_INFO, "tika parsed  success file count is: %d", filenum);
		log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
	}
			
    /* 发送消息给策略匹配 */
	if ((jsondest == NULL) || (strlen(jsondest) == 0))
	{
	    log_printf (ZLOG_LEVEL_ERROR, "to Policymatching json string is null");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}

	/* 多线程共享资源加锁 */
	log_printf (ZLOG_LEVEL_DEBUG, "jsondest=%s ", jsondest);
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
	log_printf (ZLOG_LEVEL_INFO, "mutex lock");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
#endif 
    pthread_mutex_lock (&write_outlog_lock);

	/* 发送消息给策略匹配 */
    //res = send_msgqueue (jsonstr, strpath);
    res = send_msgqueue (jsondest, strpath); //by niulw add
	if (res == -1)
	{
        log_printf (ZLOG_LEVEL_ERROR, "send_msgqueue error");
	    log_printf (ZLOG_LEVEL_ERROR, "mutex unlock reason error ");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
        pthread_mutex_unlock (&write_outlog_lock);
		return -1;
	}
    log_printf (ZLOG_LEVEL_INFO, "\033[5;33m");
	log_printf (ZLOG_LEVEL_INFO, "send json string:");
    //log_printf (ZLOG_LEVEL_INFO, "json string len is：%d", strlen(jsondest));
    //log_printf (ZLOG_LEVEL_INFO, "json string content is：%s", jsondest);
	log_printf (ZLOG_LEVEL_INFO, "%s____%d ", __FILE__, __LINE__);
	log_printf (ZLOG_LEVEL_INFO, "\033[0m ");  

	log_printf (ZLOG_LEVEL_INFO, "mutex unlock");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
    pthread_mutex_unlock (&write_outlog_lock);

	/* 释放 */
	/*if (jsondest)
	{
        free(jsondest);
        jsondest = NULL;
	}*/

	log_printf (ZLOG_LEVEL_INFO, "file parse is finish ...... ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
    return 0;
}


/**
 * @brief 判断传入的结构体数据是否为空
 *
 * @param pronod ftp结构体
 *
 * @return  成功返回0，失败返回-1
 */
int is_not_null(SESSION_BUFFER_NODE *pronode)
{
    if (pronode == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "SESSION_BUFFER_NODE struct is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}
    else if (pronode->strname == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "pronode->strname, store name is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}
    else if (pronode->orgname == NULL) 
	{
		log_printf (ZLOG_LEVEL_ERROR, "pronode->orgname, original name is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}
    else if (pronode->strpath == NULL) 	
	{
		log_printf (ZLOG_LEVEL_ERROR, "pronode->strpath, store path is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}
    else if (strlen(pronode->strname) == 0) 
	{
		log_printf (ZLOG_LEVEL_ERROR, "strlen(pronode->strname), store name len is 0");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}
    else if (strlen(pronode->orgname) == 0) 
	{
		log_printf (ZLOG_LEVEL_ERROR, "strlen(pronode->orgname), original name len is 0");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}
    else if (strlen(pronode->strpath) == 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "strlen(pronode->strpath), file path len is 0");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}

	return 0;
}

#if 1
/**
 * @brief 发送ftp属性信息的json串给策略匹配
 *
 * @param pronode        ftp属性信息
 * @param max_filesize   tika最大解析文件大小,超过这个值将被丢弃
 *
 * @return  成功返回0，失败返回-1
 */
int send_json(SESSION_BUFFER_NODE *pronode,unsigned long max_filesize)
{	
	log_printf (ZLOG_LEVEL_INFO, "%s__%d, ftp call send_json()", __FILE__, __LINE__);
	char *return_json = NULL;
	int res = 0;

#if 1
    //显示结构体信息
	log_printf (ZLOG_LEVEL_DEBUG, "print SESSION_BUFFER_NODE struct message ");
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
    show_session(pronode);
#endif
    log_printf (ZLOG_LEVEL_INFO, "strname:%s, orgname:%s", pronode->strname, pronode->orgname);
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);

    //判断传入的结构体数据是否为空
    res = is_not_null(pronode);
	if (res == -1)
	{
		log_printf (ZLOG_LEVEL_ERROR, " SESSION_BUFFER_NODE  struct some element is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}

	log_printf (ZLOG_LEVEL_INFO, "call protocol parse to make json string...... ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
#if 1
	/* 协议解析 */
    res = ftp_jsonstr (pronode, &return_json);
	if (res == -1)
	{
	   	log_printf (ZLOG_LEVEL_ERROR, "ftp_jsonstr(), call ftp protocol parser function error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
        if (return_json)
	    {
	       free(return_json);
	       return_json = NULL;
	    }
	    return -1;
	}
#endif

#if 0
    char *jsonstr = "{\"prt\": 7, \"sip\": \"213.220.50.0\", \"dip\": \"28.209.67.0\", \"spt\": 12345, \"dpt\": 54321, \"ts\": \"NULL\", \"pAttr\": {\"url\": \"10.63.58.121/UploadProject/servlet/uploadFile\", \"typ\": \"POST\", \"ctl\": \"NULL\", \"sinf\": \"NULL\", \"Cki\": \"JSESSIONID=5D1A92D1104B2FDBF6CC9FDEE67380AE\"}, \"flist\": [{\"ft\": \"NULL\", \"fn\": \"test.docx\", \"au\": \"NULL\", \"fs\": 239860, \"fp\": \"/root/codesmtp/test.docx\", \"ftp\": \"NULL\"}]}";
	return_json = malloc(strlen(jsonstr)+1);
	strcpy (return_json, jsonstr);
#endif



#if 1

	log_printf (ZLOG_LEVEL_INFO, "ftp_jsonstr(), call ftp protocol parser function success");
	log_printf (ZLOG_LEVEL_INFO, "the json string is: %s  ", return_json);
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);

	log_printf (ZLOG_LEVEL_INFO, "call file parse function...... ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
    /* 调用文件解析函数 */
    res = file_extract_pthread (return_json, session_json, max_filesize);
	if (res == -1)
	{
	 	log_printf (ZLOG_LEVEL_ERROR, "call file parse function send to PolicyMatching error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		if (return_json)
	    {
	    	free(return_json);
	    	return_json = NULL;
	    }
		return -1;
	}

	log_printf (ZLOG_LEVEL_INFO, "call file parse function send to PolicyMatching success ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);

	if (return_json == NULL || strlen(return_json) == 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "josn string is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
	}

#endif
	
	log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ",__FILE__, __LINE__);
    
    if (return_json)
	{
		free(return_json);
		return_json = NULL;
	}

	log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ",__FILE__, __LINE__);
	return 0;
}
#endif


#if 1
/**
 * @brief http upload 发送http upload属性信息的json串给策略匹配
 *
 * @param pronode        ftp属性信息
 * @param max_filesize   tika最大解析文件大小,超过这个值将被丢弃
 *
 * @return  成功返回0，失败返回-1
 */
int http_upload_json (SESSION_BUFFER_NODE *pronode, unsigned long max_filesize)
{
	log_printf (ZLOG_LEVEL_INFO, "%s__%d, http_upload_json ", __FILE__, __LINE__);
	char *return_json = NULL;
	int res = 0;

	log_printf (ZLOG_LEVEL_DEBUG, "print SESSION_BUFFER_NODE struct message ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
    show_session(pronode);
    
    //判断传入的结构体数据是否为空
    res = is_not_null(pronode);
	if (res == -1)
	{
		log_printf (ZLOG_LEVEL_ERROR, "SESSION_BUFFER_NODE struct element some  is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		return -1;
	}

	log_printf (ZLOG_LEVEL_INFO, "call http upload protocol parse make json string...... ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
#if 1
	/* 协议解析 */
    res = httpupload_pro_jsonstr (pronode, &return_json);
	if (res == -1)
	{
	   	log_printf (ZLOG_LEVEL_ERROR, "call http upload protocol parse error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
        if (return_json)
	    {
	       free(return_json);
	       return_json = NULL;
	    }
	    return -1;
	}
#endif

#if 0
    char *jsonstr = "{\"prt\": 7, \"sip\": \"213.220.50.0\", \"dip\": \"28.209.67.0\", \"spt\": 12345, \"dpt\": 54321, \"ts\": \"NULL\", \"pAttr\": {\"url\": \"10.63.58.121/UploadProject/servlet/uploadFile\", \"typ\": \"POST\", \"ctl\": \"NULL\", \"sinf\": \"NULL\", \"Cki\": \"JSESSIONID=5D1A92D1104B2FDBF6CC9FDEE67380AE\"}, \"flist\": [{\"ft\": \"NULL\", \"fn\": \"test.docx\", \"au\": \"NULL\", \"fs\": 239860, \"fp\": \"/root/codesmtp/test.docx\", \"ftp\": \"NULL\"}]}";
	return_json = malloc(strlen(jsonstr)+1);
	strcpy (return_json, jsonstr);
#endif

#if 1

	log_printf (ZLOG_LEVEL_INFO, "call http upload protocol parse  ");
	log_printf (ZLOG_LEVEL_INFO, " http upload json string is: %s  ", return_json);
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);

	log_printf (ZLOG_LEVEL_INFO, "call file parse function...... ");
    /* 调用文件解析函数 */
    res = file_extract_pthread (return_json, pronode->strpath, max_filesize);
	if (res == -1)
	{
	 	log_printf (ZLOG_LEVEL_ERROR, "file_extract_pthread(), file parse send Policymatching function error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
		if (return_json)
	    {
	    	free(return_json);
	    	return_json = NULL;
	    }
		return -1;
	}

	log_printf (ZLOG_LEVEL_INFO, "file_extract_pthread(), file parse send Policymatching function success ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);

	if (return_json == NULL || strlen(return_json) == 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "josn string is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ",__FILE__, __LINE__);
	}

#endif
	
    if (return_json)
	{
		free(return_json);
		return_json = NULL;
	}

	return 0;
}
#endif
