/*
 * =====================================================================================
 *
 *       Filename:  gmime_smtp.c
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
#include "gmime_smtp.h"
extern char *session_stream;
#if 1
/**
 * @brief 初始化链表,带头节点的链表，头节点不存放信息
 *
 * @return  返回链表头节点
 */
struct namenode *init_name_nodelist()
{
	struct namenode *headlist = NULL;
	int size = sizeof(struct namenode);
	if ((headlist = malloc(size)) == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "malloc error");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return NULL;
	}
					
	memset (headlist, 0, size);
	
	headlist->filename = NULL;
	headlist->uuidname = NULL;
	headlist->filepath= NULL;
	headlist->next = NULL;

	return headlist;
}


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
int add_name_nodelist (struct namenode* nodelistsrc, const char *filename, const char *uuidname, const char *filepath, unsigned char contentflag)
{
	if (nodelistsrc == NULL)	
	{
	    nodelistsrc = init_name_nodelist();	
	}

	struct namenode* nodelist = nodelistsrc; 
	int size = 0;
	size = sizeof(struct namenode);
	struct namenode* newnode = malloc(size);
	if (newnode == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "malloc error");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (newnode, 0, size);

	while (nodelist->next != NULL)
	{
		nodelist = nodelist->next;	
	}

    if (uuidname)
	{
		size = strlen(uuidname);
	    newnode->uuidname = malloc(size+1);
		if (newnode->uuidname == NULL)
		{
		    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
		}
	    memset(newnode->uuidname, 0, size+1);
	    strcpy(newnode->uuidname, uuidname);
    }

	if (filename)
	{
		size = strlen(filename);
	    newnode->filename = malloc(size+1);
		if (newnode->filename == NULL)
		{
		    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
		}
	    memset(newnode->filename, 0, size+1);
	    strcpy(newnode->filename, filename);
    }	

    if (filepath)
	{
		size = strlen(filepath);
	    newnode->filepath = malloc(size+1);
		if (newnode->filepath == NULL)
		{
		    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
		}
	    memset(newnode->filepath, 0, size+1);
	    strcpy(newnode->filepath, filepath);
    }
    newnode->contentflag = contentflag;
	newnode->next = NULL;

	nodelist->next = newnode;
	return 0;
}

/**
 * @brief 释放链表空间
 *
 * @param nodelist 链表头
 *
 * @return  成功返回0，失败返回-1
 */
int free_name_nodelist (struct namenode *nodelist)
{
	if (nodelist == NULL)	
	{
		return -1;
	}

	struct namenode *now = nodelist;
	struct namenode *tmp = NULL;

	while(now->next != NULL)
	{
		tmp = now->next; 
		now->next = tmp->next;

        if (tmp->filename)
		{
		    free (tmp->filename);
		    tmp->filename = NULL;
		}

        if (tmp->uuidname)
		{
		    free (tmp->uuidname);
		    tmp->uuidname = NULL;
		}

        if (tmp->filepath)
		{
		    free (tmp->filepath);
		    tmp->filepath = NULL;
		}

		if (tmp)
		{
		    free(tmp);
		    tmp = NULL;
		}
		log_printf (ZLOG_LEVEL_INFO, "free............... ");
	}
    if (nodelist)
	{
	    free(nodelist);
	    nodelist = NULL;
	}
	return 0;
}

/**
 * @brief 显示链表信息
 *
 * @param nodelist 链表头
 *
 * @return  成功返回0，失败返回-1
 */
int show_name_nodelist (struct namenode* nodelist)
{
	struct namenode *tmp = nodelist;
	if (nodelist == NULL)
	{
		return -1;
	}

	while (tmp->next != NULL)
	{
		tmp = tmp->next; 
		log_printf (ZLOG_LEVEL_INFO, "filename=%s ", tmp->filename);	
		log_printf (ZLOG_LEVEL_INFO, "uuidname=%s ", tmp->uuidname);	
		log_printf (ZLOG_LEVEL_INFO, "filepath=%s ", tmp->filepath);	
		log_printf (ZLOG_LEVEL_INFO, "contentflag=%d ", tmp->contentflag);	
	}

	return 0;
}
#endif

/**
 * @brief 解析eml数据文件,得到附件的原始名称
 *
 * @param part        gmime的对象
 * @param nodelist    文件名称链表
 * @param spec        eml嵌套层次标识
 *
 * @return void
 */
void write_part (GMimeObject *part, struct namenode* nodelist, const char *spec)
{
	GMimeStream *istream;
	GMimeStream *ostream;
	GMimeStream *fstream;
	GMimeStream *ostream2;
	GMimeMultipart *multipart;
	GMimeMessage *message;
	GMimeObject *subpart;
	GMimeFilter *filter1;
	GMimeFilter *filter2;
	char *buf = NULL; 
	char *id = NULL;
	FILE *fp1 = NULL;
	FILE *fp2 = NULL;
	int i = 0;
	int n = 0;
	unsigned char contentflag = FALSE;
	
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);

	/* 判断模式是否是多模式 */
		if (GMIME_IS_MULTIPART (part)) 
	{

	    log_printf (ZLOG_LEVEL_INFO, "%s__%d", __FILE__, __LINE__);
		multipart = (GMimeMultipart *)part;
		buf = g_alloca (strlen (spec) + 14);
		id = g_stpcpy (buf, spec);
		*id++ = '.';
		n = g_mime_multipart_get_count (multipart);
		for (i = 0; i < n; i++)
		{
			subpart = g_mime_multipart_get_part (multipart, i);
			sprintf (id, "%d", i + 1);
			write_part (subpart, nodelist, buf);
		}
	} 
	else if (GMIME_IS_MESSAGE_PART(part))  /* 嵌套的eml文件,即附件为eml文件 */
	{
	    char textname[512] = {};
	    //char nowpath[20] = {};
        char uuidnow[128] = {};
		/* 得到一个uuid串 */
	    uuid_t randNumber;
        uuid_generate(randNumber);
	    uuid_unparse(randNumber, uuidnow);
		log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
		
		/* 获取当前路径 */
		//getcwd (nowpath, 20);

		/* 如果发送的附件是eml格式的那么就在这里接收 */
		const char *filename = g_mime_object_get_content_disposition_parameter(part, "filename");

		log_printf (ZLOG_LEVEL_INFO, "%s-%d,filename=%s ", __FILE__, __LINE__, filename);

		buf = g_strdup_printf ("%s/%s.%smessage.eml", session_stream, spec, uuidnow);

		sprintf (textname, "%s.%smessage.eml", spec, uuidnow);
		/* 不是正文 */
		contentflag = FALSE;

		/* 添加文件名称到链表 */
        add_name_nodelist (nodelist, filename, textname, session_stream, contentflag);
		log_printf (ZLOG_LEVEL_INFO, "message,buf=%s ", buf);
		log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);
		fp1 = fopen (buf, "wt");
		if (fp1 == NULL)
		{
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
			log_printf (ZLOG_LEVEL_ERROR, "fopen");
		}
		g_free (buf);
		message = GMIME_MESSAGE_PART (part)->message;

		ostream = g_mime_stream_file_new (fp1);
		/* TRUE使得流关闭的时候，关闭文件描述符fclose,但是使用过程中检测内存泄漏，所以使用FALSE */
	    //g_mime_stream_file_set_owner ((GMimeStreamFile*) ostream, TRUE);
		
		/* FALSE流关闭的时候，使用fclose手动关闭文件指针 */
	    g_mime_stream_file_set_owner ((GMimeStreamFile*) ostream, FALSE);
		g_mime_object_write_to_stream (GMIME_OBJECT (message), ostream);
		g_object_unref (ostream);

		if (fp1)
		{
		    fclose(fp1);
			fp1 = NULL;
		}
	} 
	else if (GMIME_IS_PART (part))  /* 邮件的附件信息,提取附件，获取附件名称 */
	{
	    char textname[512] = {};
	    //char nowpath[20] = {};
        char uuidnow[128] = {};

		/* 得到一个uuid串 */
	    uuid_t randNumber;
        uuid_generate(randNumber);
	    uuid_unparse(randNumber, uuidnow);

		/* 获取当前路径 */
		//getcwd (nowpath, 20);

		const char *filename2 = g_mime_object_get_content_disposition_parameter (part, "filename");
		log_printf (ZLOG_LEVEL_INFO, "%s__%d,filename2=%s ", __FILE__, __LINE__, filename2);

		if (filename2 == NULL) /* 存储邮件正文 */
		{
			contentflag = TRUE;
		    buf = g_strdup_printf ("%s/%s.%scontent.txt", session_stream, spec, uuidnow);
		    log_printf (ZLOG_LEVEL_INFO, "content:%s__%d,buf=%s ", __FILE__, __LINE__, buf);
		    sprintf (textname, "%s.%scontent.txt", spec, uuidnow);
		}
		else                 /* 存储邮件附件 */
		{
			contentflag = FALSE;
		    buf = g_strdup_printf ("%s/%s.%spart.txt", session_stream, spec, uuidnow);
		    log_printf (ZLOG_LEVEL_INFO, "part:%s__%d,buf=%s ", __FILE__, __LINE__, buf);
		    sprintf (textname, "%s.%spart.txt", spec, uuidnow);
		}
		/* 添加文件名称到链表 */
        add_name_nodelist (nodelist, filename2, textname, session_stream, contentflag);

		log_printf (ZLOG_LEVEL_INFO, "%s__%d,buf%s ", __FILE__, __LINE__, buf);
		if ((fp2= fopen (buf, "wt")) == NULL)
		{
		    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
			log_printf (ZLOG_LEVEL_ERROR, "fopen");
		}
		g_free (buf);
	
		log_printf (ZLOG_LEVEL_DEBUG, "*************%s__%d*********** ", __FILE__, __LINE__);
	    /* open our output stream */
		ostream2 = g_mime_stream_file_new (fp2);

		/* istream不用释放 */
		istream = g_mime_data_wrapper_get_stream (GMIME_PART(part)->content);

		log_printf (ZLOG_LEVEL_DEBUG, "**************%s__%d*********** ", __FILE__, __LINE__);
		/* TRUE使得流关闭的时候，关闭文件描述符fclose */
	    //g_mime_stream_file_set_owner ((GMimeStreamFile*) ostream2, TRUE);
	    g_mime_stream_file_set_owner ((GMimeStreamFile*) ostream2, FALSE);
		log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ", __FILE__, __LINE__);

		/* fstream要释放 */
	    fstream = g_mime_stream_filter_new (ostream2);

	    /* attach an decode filter */
	    filter1 = g_mime_filter_basic_new (g_mime_part_get_content_encoding (GMIME_PART(part)), FALSE);
	    g_mime_stream_filter_add ((GMimeStreamFilter *) fstream, filter1);
	    g_object_unref (filter1);

/* 转码@20140818 */
#if 1
        /* 转码到utf-8 */
		log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ", __FILE__, __LINE__);
		GMimeObject *object = (GMimeObject *)part;
		/* 得到邮件使用的字符编码 */
        const char *myencode = g_mime_object_get_content_type_parameter (object, "charset");
	    log_printf (ZLOG_LEVEL_INFO, "myencode= %s ", myencode);
		log_printf (ZLOG_LEVEL_INFO, "%s_%d ", __FILE__, __LINE__);

		if (myencode)
		{
			/* 转换为UTF-8 */
	        filter2 = g_mime_filter_charset_new (myencode, "UTF-8");
	        g_mime_stream_filter_add((GMimeStreamFilter *)fstream, filter2);
	        g_object_unref (filter2);
		}
#endif
	    
		log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ", __FILE__, __LINE__);
		/* 生成附件 */
	    if (g_mime_stream_write_to_stream (istream, fstream) == -1) {
	    	g_object_unref (fstream);
	    	g_object_unref (ostream2);
	    	return ;
	    }

        if (fp2)
		{
            fclose(fp2);
			fp2 = NULL;
		}
       
		log_printf (ZLOG_LEVEL_DEBUG, "%s_%d ", __FILE__, __LINE__);
	    g_object_unref (fstream);
	    g_object_unref (ostream2);

	    return ;
	}
}

#if 0
/**
 * @brief 信头的接收发送，现在需求不需要信头里面的收件人和发件人
 *
 * @param message  信体信息
 * @param jsonsrc  源
 * @param jsondest 目的
 *
 * @return 成功返回0，失败返回-1 
 */
int set_mailbody_rcpt_from (GMimeMessage *message, char *jsonsrc, char **jsondest)
{
    if (jsonsrc == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		log_printf (ZLOG_LEVEL_ERROR, "json string is null");
		return -1;
	}
	json_t *object;
	json_error_t error;
	json_t *tolist;
	json_t *pAttr;
	int size = 0;
	int i = 0;
	char *result = NULL;
	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonsrc, strlen(jsonsrc), 0, &error);
	pAttr = json_object_get (object, "pAttr");

	/* 设置信封发件人 */
	char *sender = (char*)g_mime_message_get_sender (message);
	size = strlen(sender);
    char *mail_from = malloc(size+1);	
	if (mail_from == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (mail_from, 0, size+1);
    get_mail_addr (sender, &(mail_from));
	json_object_set_new (pAttr, "from", json_string(mail_from));
	if (mail_from)
	{
		free(mail_from);
		mail_from = NULL;
	}

    /*产生收件人的json数组*/
    char *to = NULL;
	char *mail_to = NULL;
    int listsize = 0; 
    tolist = json_object_get (pAttr, "to");
    InternetAddressList *recipient_list =  g_mime_message_get_all_recipients (message);
	listsize = internet_address_list_length (recipient_list);
	InternetAddress *recipient;
	log_printf (ZLOG_LEVEL_INFO, "listsize=%d ", listsize);
	log_printf (ZLOG_LEVEL_INFO, "%s_%d ", __FILE__, __LINE__);
	for(i = 0; i < listsize; i++)
	{
        recipient = internet_address_list_get_address (recipient_list, i);
        to  = internet_address_to_string(recipient, FALSE);
        //g_object_unref (recipient); 
		log_printf (ZLOG_LEVEL_INFO, "%s__%d- rcpt to is:%s ", __FILE__, __LINE__, to);
        size = strlen (to);
        mail_to = malloc(size + 1);	
	    if (mail_to == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    memset (mail_to, 0, size+1);
        get_mail_addr(to, &(mail_to));
	    
		json_array_append_new (tolist, json_string(mail_to));
        if (mail_to)
	    {
	    	free(mail_to);
	    	mail_to = NULL;
	    }
		if (to)
		{
			free(to);
			to = NULL;
		}
	}
    g_object_unref (recipient_list); 

    result = json_dumps(object, JSON_PRESERVE_ORDER);

	if (result)
	{
        size = strlen(result)+1;
	    *jsondest = malloc(size);
	    if (*jsondest == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    memset (*jsondest , 0, size);
	    strcpy(*jsondest, result);

	    free(result);
	    result = NULL;
	}

	json_decref(object);
	return 0;
}
#endif

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
int pro_smtp_jsonstr(GMimeMessage *message, char *emlpath, struct namenode* nodelist, char **projson)
{
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
	json_t *pAttr;
	json_t *objectmsg;
	json_t *smtp_json_str;
	json_t *flist;
	json_t *copy;
	json_t *mailto;

	pAttr = json_object();
    objectmsg = json_object();
	smtp_json_str = json_object();
	flist = json_array();
	mailto = json_array();
	
    char *result = NULL;
	int i = 0;
	char full_name[512] = {};

    struct namenode* flist_json =  nodelist;

    /* 加时间戳 */
    char nowtime[128] = {};                
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
	sprintf (nowtime, "%d-%02d-%02d %02d:%02d:%02d",
              1900+timeinfo->tm_year,
              1+timeinfo->tm_mon,
              timeinfo->tm_mday,
              timeinfo->tm_hour,
              timeinfo->tm_min,
              timeinfo->tm_sec
            );
	json_object_set_new (pAttr, "ts", json_string(nowtime));


    /*产生公共属性的json串*/
	/* 协议号 */
	json_object_set_new (smtp_json_str, "prt", json_string(""));
	/* 源IP */
	json_object_set_new (smtp_json_str, "sip", json_string(""));
	/* 目的IP */
	json_object_set_new (smtp_json_str, "dip", json_string(""));
	/* 源端口 */
	json_object_set_new (smtp_json_str, "spt", json_string(""));
	/* 目的端口 */
	json_object_set_new (smtp_json_str, "dpt", json_string(""));
	/* 文件大小 */
	json_object_set_new (smtp_json_str, "ts", json_string(nowtime));
     
	/* 协议属性 */
	json_object_set_new (smtp_json_str, "pAttr", pAttr);
	/* 文件列表 */
	json_object_set_new (smtp_json_str, "flist", flist);

	/* 不需要信头的收件人发件人 */
	/* 收件人 */
	json_object_set_new (pAttr, "from", json_string(""));
	/* 发件人 */
	json_object_set_new (pAttr, "to", mailto);

    /* 得到一个pAttr的json格式的字符串 */
	if (emlpath == NULL)
	{
         log_printf (ZLOG_LEVEL_ERROR, "emlpath is null");
         log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
	     json_decref(objectmsg);
         json_decref(smtp_json_str);
		 return -1;
	     //json_object_set_new (pAttr, "eml", json_string(""));
	}
	else
	{
	    json_object_set_new (pAttr, "eml", json_string(emlpath));
	}

	
	//json_object_set_new (pAttr, "ts", json_string(g_mime_message_get_date_as_string(message)));
	
	/* 邮件主题 */
	const char *sbj = g_mime_message_get_subject (message);
	if (sbj == NULL)
	{
	     json_object_set_new (pAttr, "sbj", json_string(""));
	}
	else
	{
	     json_object_set_new (pAttr, "sbj", json_string(sbj));
	}

	/* mail id */
	json_object_set_new (pAttr, "mid", json_string(g_mime_message_get_message_id(message)));

	/*产生附件属性的json串*/
	int content_exist_flag = 0;
	for(i = 0; flist_json->next != NULL; i++)
	{
		
        flist_json = flist_json->next;
	    /* 产生正文的文件路径名称 */ 
		if (flist_json->contentflag == TRUE)
		{
		     sprintf (full_name, "%s/%s", flist_json->filepath, flist_json->uuidname);
			 if (content_exist_flag == 0)
			 {
	             json_object_set_new (pAttr, "plain", json_string(full_name));
				 content_exist_flag = 1;
			 }
			 else
			 {
                 remove(full_name); 
			 }
			 continue;
		}

		sprintf (full_name, "%s/%s", flist_json->filepath, flist_json->uuidname);
		log_printf (ZLOG_LEVEL_INFO, "%s_%d,full_name= %s ", __FILE__, __LINE__, full_name);
		log_printf (ZLOG_LEVEL_INFO, "%s_%d,filepath= %s ", __FILE__, __LINE__, flist_json->filepath);
		log_printf (ZLOG_LEVEL_INFO, "%s_%d,uuidname= %s ", __FILE__, __LINE__, flist_json->uuidname);

        /* 文件大小 */
        unsigned long filesize = smtp_get_file_size (full_name);
        if (filesize == -1)
	    {
            log_printf (ZLOG_LEVEL_ERROR, "get_file_size");
		    log_printf (ZLOG_LEVEL_ERROR, "%s_%d ", __FILE__, __LINE__);
	    }

	    /* 以kb计算,不足1k的按照1k计算 */
	    double realfilesize = (double)filesize / 1024;
	    /* 四舍五入 */
	    filesize = (int)round(realfilesize);
	    if (filesize == 0)
	    {
            filesize = 1; 
	    }
	    log_printf (ZLOG_LEVEL_INFO, "%s__%d,filesize=%ld ", __FILE__, __LINE__, filesize);
		/* 无法保留小数位数为2位 */
        //json_object_set_new(objectmsg, "fs", json_real(fs));
		/* 文件大小 */
        json_object_set_new (objectmsg, "fs", json_integer(filesize));
		/* 文件类型 */
		json_object_set_new(objectmsg, "ft", json_string(""));
		//printf ("这里暂时是textname,代替filename，后面会怎加字段textname ");
		/* 文件名称 */
		if (flist_json->filename == NULL)
		{
            json_object_set_new(objectmsg, "fn", json_string(""));
		}
		else
		{
            json_object_set_new(objectmsg, "fn", json_string(flist_json->filename));
		}

		/* 作者 */
        json_object_set_new(objectmsg, "au", json_string(""));

		/* 解析出的文件，避免重复使用uuidname */
		char filepath[512] = {};
		sprintf (filepath, "%s/%s" , flist_json->filepath, flist_json->uuidname);
		/* 文件绝对路径 */
		if ((flist_json->filepath == NULL) && flist_json->uuidname)
		{
            json_object_set_new(objectmsg, "fp", json_string(""));
		}
		else
		{
            json_object_set_new(objectmsg, "fp", json_string(filepath));
		}

		/* 文件内容路径 */
        json_object_set_new(objectmsg, "ftp", json_string(""));

		copy = json_deep_copy(objectmsg);
		json_array_append(flist, copy);
        json_decref(copy);
	}

	/* mail content is null */
	if (content_exist_flag == 0) 
	{
	     json_object_set_new (pAttr, "plain", json_string(""));
	}

	/* 转换成字符串 */
    result = json_dumps(smtp_json_str, JSON_PRESERVE_ORDER);
	//log_printf (ZLOG_LEVEL_INFO, "result = %s ", result);
#if 1
	/* 不加头的收件人发件人 */
	if (result)
	{
	    int size = strlen (result);
        *projson = malloc (size+1);
	    if (*projson == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    memset (*projson, 0, size+1);
	    strcpy (*projson, result);
	}
	else
	{
	    log_printf (ZLOG_LEVEL_ERROR, "result json is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	}
#endif

#if 0
	/* 加头的收件人发件人 */
    set_mailbody_rcpt_from (message, result, projson);
#endif

	if (result)
	{
		free(result);
		result = NULL;
	}

	//计数器减一可以销毁数据, 创建的jansson对象最后调用
	json_decref(objectmsg);
    json_decref(smtp_json_str);
	return 0;	
}

/**
 * @brief 产生一个gmime的流对象
 *
 * @param fd 文件描述符
 *
 * @return  返回的gmime对象
 */
GMimeMessage *parse_message (int fd)
{
	GMimeMessage *message;
	GMimeParser *parser;
	GMimeStream *stream;
	
	/* create a stream to read from the file descriptor */
	stream = g_mime_stream_fs_new (fd);

	/* create a new parser object to parse the stream */
	parser = g_mime_parser_new_with_stream (stream);

	/* unref the stream (parser owns a ref, so this object does not actually get free'd until we destroy the parser) */
	g_object_unref (stream);
	
	/* parse the message from the stream */
	message = g_mime_parser_construct_message (parser);
	
	/* free the parser (and the stream) */
	g_object_unref (parser);
	
	return message;
}

/**
 * @brief 解析eml数据文件,产生smtp协议属性的json串
 *
 * @param emlpath  数据文件路径
 * @param projson  产生json串
 *
 * @return  成功返回0，失败返回-1
 */
int mysmtp (char *emlpath, char **projson)
{
	GMimeMessage *message;
	int fd = 0;
	
	//g_mime_init (0);

	log_printf (ZLOG_LEVEL_INFO, "%s__%d, emlpath = %s ", __FILE__, __LINE__,  emlpath);

    if ((fd = open (emlpath, O_RDONLY, 0)) == -1)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		log_printf (ZLOG_LEVEL_ERROR, "open");
		return -1;
	}

	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);

	/* 得到一个gmime信息结构 */
	message = parse_message (fd);

	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
    char nowtime[128] = {};                
    struct namenode *nodelist = init_name_nodelist();
	if (message) 
	{

		/* 信体信息 */
	    log_printf (ZLOG_LEVEL_INFO, "\033[5;33m ");
        log_printf (ZLOG_LEVEL_INFO, "	Sender:%s ", g_mime_message_get_sender (message));
        log_printf (ZLOG_LEVEL_INFO, "	Subject:%s ", g_mime_message_get_subject (message));
        log_printf (ZLOG_LEVEL_INFO, "	Recipients:%s ", internet_address_list_to_string(g_mime_message_get_all_recipients(message),FALSE));
        log_printf (ZLOG_LEVEL_INFO, "	Date:%s ", g_mime_message_get_date_as_string(message));
        log_printf (ZLOG_LEVEL_INFO, "	MessageID:%s ", g_mime_message_get_message_id(message));
	    log_printf (ZLOG_LEVEL_INFO, "  \033[0m  ");		

		if (message->message_id == NULL)
		{
	         uuid_t randNumber;
             uuid_generate (randNumber);
	         uuid_unparse (randNumber, nowtime);
		     log_printf (ZLOG_LEVEL_INFO, "nowtime= %s ", nowtime);
		}
		
		log_printf (ZLOG_LEVEL_INFO, "*************************************************** ");

		/* 解析数据文件 */
	    write_part (message->mime_part,  nodelist, "1");

		log_printf (ZLOG_LEVEL_INFO, "%s__%d, show file list message ", __FILE__, __LINE__);
		/* 显示附件链表信息 */
        show_name_nodelist (nodelist);

		log_printf (ZLOG_LEVEL_INFO, "produce json string......................... ");
		/* 产生smtp信息的json串 */
		pro_smtp_jsonstr (message, emlpath, nodelist, projson);

		log_printf (ZLOG_LEVEL_INFO, "%s, %d ", __FILE__, __LINE__);
		log_printf (ZLOG_LEVEL_INFO, "projson = %s ", *projson);
        log_printf (ZLOG_LEVEL_INFO, "&&&&&&&test:MessageID:%s ", g_mime_message_get_message_id(message));
        g_object_unref (message);

		log_printf (ZLOG_LEVEL_DEBUG, "%s, %d ", __FILE__, __LINE__);
	}
	else
	{
		log_printf (ZLOG_LEVEL_ERROR, "message is null");
	}

	close(fd);

	log_printf (ZLOG_LEVEL_DEBUG, "%s, %d ", __FILE__, __LINE__);
	/* 释放链表空间 */
    free_name_nodelist (nodelist);

#if 0
	/* 释放g_mime_init的资源*/
    log_printf (ZLOG_LEVEL_INFO, "%s, %d free g_mime_init()  ", __FILE__, __LINE__);
    g_mime_shutdown ();
#endif
	
	return 0;
}



