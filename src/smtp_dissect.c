/*
 * =====================================================================================
 *
 *       Filename:  smtp_dissect.c
 *
 *    Description:  处理smtp数据包，产生一个eml文件，发送信号给gmime解析函数进行解析 
 *
 *  function list:
 *           gmime_parse_mailbody()
 *                 使用gmime解析信体
 *           gmime_tika_send()
 *                 发送tika解析出的json,测试时使用
 *           seen_dot()
 *                 判断是否是数据传输完成的结尾的.
 *           dispose_smtp_cmd()
 *                 处理smtp命令
 *           dispose_STARTTLS_cmd()
 *                 处理STARTTLS加密传输
 *           dispose_smtp_response()
 *                 处理smtp响应
 *           dispose_smtp_mail_body_data()
 *                 处理smtp的DATA命令
 *           dispose_smtp_mail_body_bdat()
 *                 处理smtp的BDAT命令
 *           do_smtp()
 *                 处理smtp数据包，产生一个eml格式文件，发送信号给gmime解析函数
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
#include "smtp_dissect.h"
extern char *session_stream;
extern char *session_json;
/* 用于自测，可以抓取本机的报文 */
#if 0
#define TCPIPPACK
#endif

/**
 * @brief 使用gmime解析信体
 *
 * @param p_session smtp状态信息结构体
 * @param projson   返回eml的属性信息json串
 *
 * @return  成功返回0，失败返回-1
 */
int gmime_parse_mailbody (SmtpSession_t *p_session, char **projson)
{
    if (p_session == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "p_session is NULL");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	log_printf (ZLOG_LEVEL_INFO, "gmime parse mail body ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   
    char *file_path = NULL;
	char str[32] = {};
	char *result = NULL;
    json_t *object;
	json_error_t error;

	if ((p_session->mail_body_dir == NULL) || strlen(p_session->mail_body_dir) == 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "mail_body_dir is NULL");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	}
	else if ((p_session->mail_body_name == NULL) || strlen(p_session->mail_body_name) == 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "mail_body_name is NULL");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	}
			
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
    int size =  0;
	size = strlen(p_session->mail_body_dir) + strlen(p_session->mail_body_name);
	file_path = malloc (size+2);
	if (file_path == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (file_path, 0, size+2);
	sprintf (file_path, "%s/%s", p_session->mail_body_dir, p_session->mail_body_name);
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, file_path=%s ",__FILE__,  __LINE__, file_path);
    	
	/* 将gmime解析出的信体信息存放到p_session中 */
	char *jsonstr = NULL;
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, call gmime parse mail data, produce json string ", __FILE__,  __LINE__);

	/* 根据file_path给出的eml文件路径，调用gmime解析eml文件 */
    int res = mysmtp (file_path, &jsonstr);
	if (res == -1)
	{
		log_printf (ZLOG_LEVEL_ERROR, "jsonstr is NULL");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__,  __LINE__);
	    if (file_path)
	    {
	    	free(file_path);
	    	file_path = NULL;
	    }
        return -1;
	}
	if (file_path)
	{
		free(file_path);
		file_path = NULL;
	}

    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__,  __LINE__);

	if (jsonstr == NULL)
	{
		 log_printf (ZLOG_LEVEL_ERROR, "jsonstr is NULL");
         log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__,  __LINE__);
         return -1;
	}

	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonstr, strlen(jsonstr), 0, &error);

    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__,  __LINE__);
    /* 协议类型 */
    json_object_set_new(object, "prt", json_integer(3));

    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__,  __LINE__);
	/* 源ip */
	memset (str, 0, 32);
	int_to_ipstr (p_session->key.saddr, str);
    json_object_set_new (object, "sip", json_string(str));

    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ",__FILE__,  __LINE__);
	/* 目的ip */
	memset (str, 0, 32);
	int_to_ipstr(p_session->key.daddr, str);
    json_object_set_new (object, "dip", json_string(str));

    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ",__FILE__,  __LINE__);
	/* 源端口 */
    json_object_set_new (object, "spt", json_integer(p_session->key.source));

    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ",__FILE__,  __LINE__);
	/* 目的端口 */
    json_object_set_new (object, "dpt", json_integer(p_session->key.dest));

    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ",__FILE__,  __LINE__);
    result = json_dumps(object, JSON_PRESERVE_ORDER);

	if (result)
	{
#if 0
	    size = strlen(result);
	    *projson = malloc(size+1);
	    if (*projson == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    memset (*projson, 0, size+1);
	    strcpy(*projson, result);
#endif

		/* 根据桶中存放的信息设置信封的收件人发件人 */
        set_mailenvelope_rcpt_from (p_session, result, projson);
        if (result)
		{
	       free(result);
	       result = NULL;
		}
		
	}
    if (jsonstr)
	{
		free(jsonstr);
		jsonstr = NULL;
	}

	json_decref(object);
	return 0;
}

/**
 * @brief 发送tika解析出的json,测试时使用
 *
 * @param key          四元组，用于查找hash桶中的smtp的json串
 * @param max_filesize 最大的文件
 *
 * @return  成功返回0，失败返回-1
 */
int gmime_tika_send (struct tuple4 *key, unsigned long max_filesize)
{
    hash_bucket     *pBucket = NULL;	
	SmtpSession_t	*p_session = NULL;
	char *jsonstr = NULL;

    /* 查找桶中是否已经有项目 */
	log_printf (ZLOG_LEVEL_DEBUG, "beging gmime_tika_send ");
	log_printf (ZLOG_LEVEL_DEBUG, "find hash element ");
    if ((pBucket = find_hash(ftp_session_tbl, key, sizeof(struct tuple4))) == NULL)
    {
          log_printf (ZLOG_LEVEL_ERROR, " not find key, add new key ");
	      log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
		  return -1;
    }

	/* 取得hash桶中的smtp信息结构体 */
    p_session = (SmtpSession_t*)pBucket->content;
	if (p_session == NULL)
	{
          log_printf (ZLOG_LEVEL_ERROR, "p_session is null");
	      log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
		  return -1;
	}
	if (p_session->smtp_session_state.dot_seen == FALSE)
	{

         log_printf (ZLOG_LEVEL_WARN, " not find key, add new key ");
	     log_printf (ZLOG_LEVEL_WARN, "%s__%d ", __FILE__, __LINE__);   
         return -1; 
	}

    log_printf (ZLOG_LEVEL_DEBUG, "find hash key exist ");


    /* 每次都是在最新结点,相同的四员组时是链式结构 */
    while(p_session->next != NULL)	
	{
	    //log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		p_session = p_session->next;
	}

    /* 调用gmime解析信体eml文件，产生json串 */
	gmime_parse_mailbody (p_session, &jsonstr);
	if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "jsonstr is null");
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);
		return -1;
	}

	/* 产生的smtp的json串入桶 */
	p_session->result_json = jsonstr;
    log_printf (ZLOG_LEVEL_DEBUG, "gmime parse json string is: %s ", jsonstr);
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   

	/* 获取解析出的附件个数 */
	int file_num = 0;
    get_file_num (jsonstr, &file_num);
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, gmime parse file count is :=  %d ", __FILE__, __LINE__, file_num);
	/* 调用tika解析eml中的附件 */
    int res = file_extract_pthread (jsonstr, p_session->mail_body_dir, max_filesize);
	if (res == -1)
	{
	 	log_printf (ZLOG_LEVEL_ERROR, "call file_extract_pthread() is error ");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
		return -1;
	}

	log_printf (ZLOG_LEVEL_DEBUG, "call file parse function send to PolicyMatching is success ");
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
	if (jsonstr == NULL || strlen(jsonstr) == 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "json string is NULL");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
		return -1;
	}

    /* 销毁哈希桶 */
	log_printf (ZLOG_LEVEL_DEBUG, "free hash ");
	remove_hash (ftp_session_tbl, pBucket, sizeof(struct tuple4));

	/* 释放桶中资源 */
	free_smtp (pBucket->content);
	pBucket->content = NULL;
	free (pBucket);
	pBucket = NULL;
	log_printf (ZLOG_LEVEL_DEBUG, "p_session is NULL @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ");
	/* 将桶中的信息给返回变量 */
	//cp_pbucket_to_session(&re_struc, p_session);
	return 0;
}

/**
 * @brief 判断是否是数据传输完成的结尾的.
 *
 * 0xaa   标记第一包是\r\n.的情况
 * 0x55   标记第一包是*\r\n的情况
 * 0x00   初始化标志位
 * @param p_sessionsrc smtp结构体信息
 * @param packstr      包信息
 *
 * @return  成功返回0，失败返回-1
 */
int seen_dot (SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen, smtp_sbuffputdata sbuff_putdata, int sbuffer_id)
{
	   if (p_sessionsrc == NULL)
	   {
			log_printf (ZLOG_LEVEL_ERROR, "smtp struct SmtpSession_t is NULL");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
			return -1;
	   }
       SmtpSession_t *p_session = p_sessionsrc;

	   char *packstr_tmp2 = NULL;
	   int size = 0;
	   int end_flag = 0;

        size = packlen;
		if(size >= 2)
		{
                /* 
				 *符合结束标志\r\n.\r\n
				 *
				 * */
				if (size >= 5)
				{
						log_printf (ZLOG_LEVEL_DEBUG, "size>=5 ");
						log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
						packstr_tmp2 = packstr + size - 5;
						if (0 == strncasecmp(packstr_tmp2, "\r\n.\r\n", 5))
						{
								end_flag = 1; 
								goto dot;
						}

				}

				/* 
				 *处理第一包的结尾是：\r\n.
				 *第二包前两个字符是\r\n的情况。这样的也符合结束标志\r\n.\r\n
				 *
				 * */
				if(size >= 3)
				{
						log_printf (ZLOG_LEVEL_DEBUG, "size>=3 ");
						log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
						packstr_tmp2 = packstr + size - 3;
						if (0 == strncasecmp (packstr_tmp2, "\r\n.", 3))
						{
								p_session->smtp_session_state.crlf_seen = 0xaa;
								end_flag = 0;
								goto dot;
						}
				}
				/*包长度大于等于2，就有可能有\r\n并且上一包结尾有\r\n.*/
				if((size >= 2)&&(p_session->smtp_session_state.crlf_seen == 0xaa))
				{
						log_printf (ZLOG_LEVEL_DEBUG, "size>=2 ");
						log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
						//packstr_tmp2 = packstr+size-2;
						packstr_tmp2 = packstr;
						/* 第二包前两个字符是\r\n,不是123\r\n这种 */
						if(0 == strncasecmp (packstr_tmp2, "\r\n", 2)){
								end_flag = 1;
								goto dot;
						}

				}

                /* 
				 *处理第一包的结尾是：\r\n
				 *第二包前三个个字符是.\r\n的情况。这样的也符合结束标志\r\n.\r\n
				 *
				 * */

				if(size >= 2) 
				{
						if(size >= 3)
						{
								packstr_tmp2 = packstr+size-3;
								if (0 == strncasecmp (packstr_tmp2, ".\r\n", 3) && (p_session->smtp_session_state.crlf_seen != 0x55))
								{
										p_session->smtp_session_state.crlf_seen = 0x55;
										end_flag = 0;
										goto dot;
								}
								else if(p_session->smtp_session_state.crlf_seen == 0x55)
								{
								        packstr_tmp2 = packstr;
										if (0 == strncasecmp (packstr_tmp2, ".\r\n", 3))
										{
												end_flag = 1;
												goto dot;
										}
								}

						}

						packstr_tmp2 = packstr+size-2;
						if(0 == strncasecmp (packstr_tmp2, "\r\n", 2)){
								p_session->smtp_session_state.crlf_seen = 0x55;
								end_flag = 0;
								goto dot;
						}
				}
		}
#if 1
		p_session->smtp_session_state.crlf_seen = 0x00;
		end_flag = 0;
	    return 0;	
#endif

dot:
	   if (end_flag == 1)
	   {
            p_session->smtp_session_state.bdat_seen = FALSE;
	        p_session->smtp_session_state.data_seen = FALSE;
	        p_session->smtp_session_state.dot_seen = TRUE;
	        p_session->data_store_flag = DATA_BDAT_END;
		    p_session->now_rcpt_count++;

	        SESSION_BUFFER_NODE    smtp_sft;
			smtp_sft.session_five_attr.ip_src = p_session->key.saddr;
			smtp_sft.session_five_attr.ip_dst = p_session->key.daddr;
			smtp_sft.session_five_attr.port_src = p_session->key.source;
			smtp_sft.session_five_attr.port_dst = p_session->key.dest;
			smtp_sft.session_five_attr.protocol = 3;
			sbuff_putdata(sbuffer_id, smtp_sft);
			log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
	   }
	   return 0;
}

/**
 * @brief 处理smtp命令
 *
 * @param p_session  结构体
 * @param packstr    命令
 * @param packlen    包长 
 *
 * @return  成功返回0，失败返回-1
 */
int dispose_smtp_cmd (SmtpSession_t	*p_sessionsrc, char *packstr, unsigned int packlen)
{
        SmtpSession_t *p_session = p_sessionsrc;
		if (p_session == NULL)
		{
			log_printf (ZLOG_LEVEL_ERROR, "p_session is NULL");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
			return -1;
		}
		int size = 0;
	    SmtpSession_t *p_session_tmp=NULL;
		/* TLS加密 */
	    if (0 == strncasecmp (packstr, "STARTTLS", 8))
		{
		   p_session->smtp_state = SMTP_STATE_READING_STARTTLS;
           p_session->smtp_session_state.starttls_seen = TRUE;
		}
#if 0
	    /* 与服务器链接,尝试扩展的SMTP链接 */
	    else if (0 == strncasecmp (packstr, "EHLO", 4))
	    {
		    log_printf (ZLOG_LEVEL_DEBUG, "hello smtp server ");
		}     
	    /* 与服务器链接,尝试普通SMTP链接 */
	    else if (0 == strncasecmp (packstr, "HELO", 4))
	    {
		    log_printf (ZLOG_LEVEL_DEBUG, "hello smtp server ");
		}     
#endif
		/* 与服务器断开连接 */
		else if (0 == strncasecmp (packstr, "QUIT", 4))
	    {
		    log_printf (ZLOG_LEVEL_INFO, "QUIT ");   
		    log_printf (ZLOG_LEVEL_INFO, "disconnect with server, %s__%d ", __FILE__, __LINE__);   
            //p_session->session_quit_flag = TRUE;
            p_session->smtp_session_state.quit_seen = TRUE;
		}
		/* 信封的发件人命令 */
		else if (0 == strncasecmp(packstr, "MAIL FROM", 9))
	    {
		    log_printf (ZLOG_LEVEL_DEBUG, "MAIL FROM size = %d ", packlen);
	        log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		    size = packlen;

		    /* 处理收件人 */
		    if (p_session->mail_from == NULL)
		    {
		        size = packlen;
	            log_printf (ZLOG_LEVEL_DEBUG, "size=%d ", size);
	        	log_printf (ZLOG_LEVEL_DEBUG, "mail_from is null,p_session->mail_from = %s ", p_session->mail_from);
	            log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		        p_session->mail_from = (char*)malloc(size + 1);
	            if (p_session->mail_from == NULL)
	            {
	                log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	                return -1;
	            }
		        memset (p_session->mail_from, 0, size + 1);
				/* 得到发件人信息 */
                get_mail_addr(packstr, &(p_session->mail_from));
		        log_printf (ZLOG_LEVEL_INFO, "sender1:%s ", p_session->mail_from);
	            log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   
		    }
            /* 处理多收件人,在邮件客户端设置分别发送 */
	        else if (p_session->mail_from != NULL)
	        {
	        	 log_printf (ZLOG_LEVEL_INFO, "%s__%d, mail_from not null, p_session->mail_from = %s ", __FILE__, __LINE__, p_session->mail_from);
	        	 while (p_session->mail_from != NULL)
	        	 {
	        		  if (p_session->next == NULL)
	        		  {
		  			      size = sizeof(SmtpSession_t);
	        	          p_session_tmp = malloc(size);
	                      if (p_session_tmp == NULL)
	                      {
	                          log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	                          log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	                          return -1;
	                      }
                          /* 创建新的信息结构体 */
	        	          memset(p_session_tmp, 0, size);
	        	          p_session_tmp->next = NULL;

	        	          p_session_tmp->key =  p_session->key;
		  			  
		                  size = packlen;
		                  p_session_tmp->mail_from = malloc(size + 1);
	                      if (p_session_tmp->mail_from == NULL)
	                      {
	                          log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	                          log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	                          return -1;
	                      }

		                  memset (p_session_tmp->mail_from, 0, size + 1);

						  /* 得到发件人信息 */
                          get_mail_addr(packstr, &(p_session_tmp->mail_from));
		                  log_printf (ZLOG_LEVEL_INFO, "sender2:%s ", p_session_tmp->mail_from);
	                      log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   
	        			  p_session->next = p_session_tmp;
	        			  return 0;
	        		  }
	        		  p_session = p_session->next;
	        	 }
	        }
			
		}
		/* 信封的收件人命令 */
		else if (0 == strncasecmp(packstr, "RCPT TO", 7))
	 	{
		    log_printf (ZLOG_LEVEL_DEBUG, "RCPT TO, %s__%d ", __FILE__, __LINE__);   
		    /* 添加收件人到链表 */
		    add_rcpt_to (p_session, packstr, packlen);
		    p_session->rcpt_count++;
		    log_printf (ZLOG_LEVEL_INFO, "rcpt_to list is: %s ", p_session->rcpt_to->rcpt_to_addr);
	        log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   
	 	}
		/* DATA是数据部分命令 */
		else if (0 == strncasecmp(packstr, "DATA", 4))
	    {
		    log_printf (ZLOG_LEVEL_INFO, "DATA, %s__%d ", __FILE__, __LINE__);   
		    p_session->smtp_session_state.data_seen = TRUE;
		    p_session->smtp_session_state.bdat_seen = FALSE;
			/*不能直接设置，这样会造成多存储了响应354*/
		    //p_session->data_store_flag = DATA_BDAT_START;
		    //log_printf (ZLOG_LEVEL_DEBUG, "178-----------DATA----------p_session->smtp_session_state.data_seen=%d ", p_session->smtp_session_state.data_seen);
		    log_printf (ZLOG_LEVEL_INFO, "start to transmit data DATA %s__%d ", __FILE__, __LINE__);   
	    }
		/* BDAT是数据部分命令 */
		else if (0 == strncasecmp(packstr, "BDAT", 4))
	    {
		    log_printf (ZLOG_LEVEL_INFO, "BDAT, %s__%d ", __FILE__, __LINE__);   
			/* 需要传送数据的大小 */
			if (strstr(packstr, "LAST") == NULL)
			{
			    int msg_len = (int)strtoul(packstr+5, NULL, 10);
			    if (msg_len == 0)
			    {
			    	/* 没有数据读 */
		            p_session->smtp_session_state.bdat_seen = FALSE;
		            p_session->smtp_session_state.data_seen = FALSE;
		            p_session->data_store_flag = DATA_BDAT_END;
			    }
			    else
			    {
		            p_session->smtp_session_state.bdat_seen = TRUE;
		            p_session->smtp_session_state.data_seen = FALSE;
		            p_session->data_store_flag = DATA_BDAT_START;
			    }
			}

			/* 最后的数据标志 */
            size = packlen;
		    if (0 == strncasecmp(packstr+size-4, "LAST", 4))
			{
		        log_printf (ZLOG_LEVEL_INFO, "BDAT LAST %s__%d ", __FILE__, __LINE__);   
		        p_session->smtp_session_state.bdat_last = TRUE;
			}
		    /* 得到bdat的信息 */
		    get_bdat_msg (p_session, packstr, packlen);
		    log_printf (ZLOG_LEVEL_INFO, "start to transmit data BDAT ");
	        log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   
	    }
        /* 重置命令 */
		else if (0 == strncasecmp (packstr, "RSET", 4))
		{
		    log_printf (ZLOG_LEVEL_INFO, "RSET, %s__%d ", __FILE__, __LINE__);   
		    /* 重置会话 */
		    reset_smtp_session(p_session);
		}
		return 0;
}

/**
 * @brief  处理STARTTLS加密传输
 *
 * @param p_session  结构体
 * @param packstr    命令
 * @param packlen    包长 
 *
 * @return  成功返回0，失败返回-1
 */
int dispose_STARTTLS_cmd (SmtpSession_t	*p_sessionsrc, char *packstr, unsigned int packlen)
{
        SmtpSession_t *p_session = p_sessionsrc;
		if (p_session == NULL)
		{
			log_printf (ZLOG_LEVEL_ERROR, "p_session is NULL");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
			return -1;
		}
		/* 与服务器断开连接命令 */
		if (0 == strncasecmp (packstr, "QUIT", 4))
	    {
		    log_printf (ZLOG_LEVEL_INFO, "QUIT ");   
		    log_printf (ZLOG_LEVEL_INFO, "disconnect with server, %s__%d ", __FILE__, __LINE__);   
            p_session->smtp_session_state.quit_seen = TRUE;
		}
        /* 与服务器断开连接响应 */
	    else if ((0 == strncasecmp (packstr, "221", 3)) && (p_session->smtp_session_state.quit_seen == TRUE))
	    {
	        log_printf (ZLOG_LEVEL_DEBUG, "goodbye, disconnect with server ");
	        log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
            p_session->session_quit_flag = TRUE;

            /* 销毁哈希桶 */
            hash_bucket     *pBucket = NULL;	
            if ((pBucket = find_hash(ftp_session_tbl, &p_session->key, sizeof(struct tuple4))) == NULL)
            {
                  log_printf (ZLOG_LEVEL_ERROR, " not find key, add new key ");
	              log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
	        	  return -1;
            }
	        log_printf (ZLOG_LEVEL_DEBUG, "free hash ");
	        remove_hash (ftp_session_tbl, pBucket, sizeof(struct tuple4));

	        /* 释放桶中资源 */
		    if (pBucket)
		    {
	            free_smtp (pBucket->content);
	            pBucket->content = NULL;
	            free (pBucket);
	            pBucket = NULL;
		    }
		}

		return 0;
}

/**
 * @brief 处理smtp响应
 *
 * @param p_session  结构体
 * @param packstr    响应 
 * @param packlen    包长度 
 *
 * @return  成功返回0，失败返回-1
 */
int dispose_smtp_response (SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen)
{
        SmtpSession_t *p_session = p_sessionsrc;
	    //log_printf (ZLOG_LEVEL_INFO, "packstr-211 = %s size=%d ", packstr, packlen);
		int size = 0;

	    /* 与服务器断开连接 */
	    if ((0 == strncasecmp (packstr, "221", 3)) && (p_session->smtp_session_state.quit_seen == TRUE))
	    {
	        log_printf (ZLOG_LEVEL_DEBUG, "goodbye, disconnect with server ");
	        log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
            p_session->session_quit_flag = TRUE;
	    }
		/* 命令成功 */
		else if (0 == strncasecmp (packstr, "250", 3))
	    {
	        log_printf (ZLOG_LEVEL_DEBUG, "smtp command ok ,%s__%d ", __FILE__, __LINE__);   
	    }
		/* 后面的数据是发送数据部分的命令 */
		else if(0 == strncasecmp(packstr, "354", 3))
	    {
		    log_printf (ZLOG_LEVEL_DEBUG, "mail body data beging -- DATA ");
		    log_printf (ZLOG_LEVEL_DEBUG, "p_session->smtp_session_state.data_seen=%d ", p_session->smtp_session_state.data_seen);
		    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);

		    if (p_session->smtp_session_state.data_seen == TRUE)
		    {
		         p_session->data_store_flag = DATA_BDAT_START;
			     
			     /* 显示结构体内容 */
#if 0
				 log_printf (ZLOG_LEVEL_DEBUG, "show smtp struct element  ");
	             log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		   	     while_show_smtp(p_session);
#endif
                 /* 产生uuid */
                 char buf[128] = {};
		   	     memset (buf, '\0', 128);
                 uuid_t randNumber;
                 uuid_generate (randNumber);
                 uuid_unparse (randNumber, buf);
                 log_printf (ZLOG_LEVEL_DEBUG, "UUID generate = %s ", buf);
	             log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		   	     //log_printf (ZLOG_LEVEL_DEBUG, "size=%d ", size);
		   	     //log_printf (ZLOG_LEVEL_DEBUG, "mail_from=%s ", p_session->mail_from);
		   	     //log_printf (ZLOG_LEVEL_DEBUG, "rcpt_to_addr=%s ",p_session->rcpt_to->rcpt_to_addr);
		   	     //log_printf (ZLOG_LEVEL_DEBUG, "333333333333333333333333 ");
		   	     size = strlen(buf) + strlen(".eml");
		   	     p_session->mail_body_name = (char*)malloc(size + 1);
	             if (p_session->mail_body_name == NULL)
	             {
	                 log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	                 log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	                 return -1;
	             }
		   	     memset (p_session->mail_body_name, '\0', size + 1);
		   	     sprintf (p_session->mail_body_name, "%s.eml", buf);
		   	     log_printf (ZLOG_LEVEL_INFO, "mail body name is: %s ", p_session->mail_body_name);
	             log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   

		   	     //char nowpath[20] = {};
		   	     //memset (nowpath, '\0', 20);
				 //getcwd (nowpath, 20);
		   	     //size = strlen(nowpath);
		   	     p_session->mail_body_dir = (char*)malloc (128);
	             if (p_session->mail_body_dir == NULL)
	             {
	                 log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	                 log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	                 return -1;
	             }
		   	     memset (p_session->mail_body_dir, '\0', 128);
		   	     strcpy (p_session->mail_body_dir, session_stream);
		   	     log_printf (ZLOG_LEVEL_INFO, "mail body path is: %s ", p_session->mail_body_dir);
	             log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   
		    }
	    }
        /* 出错的响应 */
		else if(0 == strncasecmp(packstr, "5", 1))
	    {
		    log_printf (ZLOG_LEVEL_WARN, "5xx ");
		    log_printf (ZLOG_LEVEL_WARN, "smtp transmit error, 5xx ");
	    }
		return 0;
}

/**
 * @brief 处理smtp信体部分 DATA命令
 *
 * @param p_sessionsrc    smtp协议结构体
 * @param packstr         数据包内容
 * @param packlen         数据包长度
 * @param sbuff_putdata   回调函数
 * @param sbuffer_id      信号id
 *
 * int sbuff_putdata(int iringbuffer_ID, SESSION_BUFFER_NODE ftt);
 * typedef int (sbuff_putdata) (int iringbuffer_ID, SESSION_BUFFER_NODE ftt)
 * @return  成功返回0，失败返回-1
 */
int dispose_smtp_mail_body_data (SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen, smtp_sbuffputdata sbuff_putdata, int sbuffer_id)
{
	    if (p_sessionsrc == NULL)
	    {
	     	log_printf (ZLOG_LEVEL_ERROR, "smtp struct is null");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
	     	return -1;
	    }
        SmtpSession_t *p_session = p_sessionsrc;
		//char *packstr_tmp1 = NULL;
		//char *packstr_tmp2 = NULL;
		//char *jsonstr = NULL;
		//int size = 0;

		log_printf (ZLOG_LEVEL_DEBUG, "&&&&&&&&&&&&&&&&smtp data transmit-start-&&&&&&&&&&&&&&&&&&&& ");
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   

		/* 先存储数据再判断是否到结尾，会有点存下来 */
		store_smtp_data(p_session, packstr, packlen);

		log_printf (ZLOG_LEVEL_DEBUG, "&&&&&&&&&&&&&&&&smtp data trasmit-end-&&&&&&&&&&&&&&&&&&&& ");
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   

		//log_printf (ZLOG_LEVEL_DEBUG, "packet concent is: %s ", packstr);
	    //log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
        /* 判断是否是数据传输完成的结尾的. 得到p_session->smtp_session_state.dot_seen 的信息 */
        seen_dot (p_session, packstr, packlen, sbuff_putdata, sbuffer_id);

		/* 数据传输结束 */
		if (p_session->smtp_session_state.dot_seen == TRUE) 
		{
		    p_session->data_store_flag = DATA_BDAT_END;
#if 0
	        SESSION_BUFFER_NODE    smtp_sft;
			smtp_sft.session_five_attr.ip_src = p_session->key.saddr;
			smtp_sft.session_five_attr.ip_dst = p_session->key.daddr;
			smtp_sft.session_five_attr.port_src = p_session->key.source;
			smtp_sft.session_five_attr.port_dst = p_session->key.dest;
			smtp_sft.session_five_attr.protocol = 3;
			sbuff_putdata(sbuffer_id, smtp_sft);
#endif

#if 0
		     /* 调用gmime解析信体 */
		     gmime_parse_mailbody (p_session, &jsonstr);

			 /* json串入桶 */
		     p_session->result_json = jsonstr;
             log_printf (ZLOG_LEVEL_INFO, "smtp_dissect.c-384,gmime parsed json string=%s ", jsonstr);
			 int file_num = 0;
             get_file_num(jsonstr, &file_num);
             log_printf (ZLOG_LEVEL_INFO, "******************smtp_dissect.c-465,gmime parsed file count=  %d ", file_num);
			 /* 调用tika进行解析 */
             int res = file_extract_pthread (jsonstr, p_session->mail_body_dir);
	         if (res == -1)
	         {
	          	log_printf (ZLOG_LEVEL_ERROR, "smtp_dissect.c-465, file_extract_pthread() call file parse error ");
	         	return -1;
	         }

	         log_printf (ZLOG_LEVEL_INFO, "smtp_dissect.c-470, file_extract_pthread() call file parse success ");
	         if (jsonstr == NULL || strlen(jsonstr) == 0)
	         {
	         	log_printf (ZLOG_LEVEL_ERROR, "smtp_dissect.c-474,josn string is null");
	         	return -1;
	         }
#endif
		}

	    //log_printf (ZLOG_LEVEL_INFO, "packstr-298 = %s size=%d ", packstr, packlen);
		return 0;
}


/**
 * @brief 处理smtp信体部分 DATA命令
 *
 * @param p_sessionsrc    smtp协议结构体
 * @param packstr         数据包内容
 * @param packlen         数据包长度
 * @param sbuff_putdata   回调函数
 * @param sbuffer_id      信号id
 *
 * int sbuff_putdata(int iringbuffer_ID, SESSION_BUFFER_NODE ftt);
 * typedef int (sbuff_putdata) (int iringbuffer_ID, SESSION_BUFFER_NODE ftt)
 *
 * @return  成功返回0，失败返回-1
 */
int dispose_smtp_mail_body_bdat (SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen, smtp_sbuffputdata sbuff_putdata, int sbuffer_id)
{
        SmtpSession_t *p_session = p_sessionsrc;
		//char *packstr_tmp1 = NULL;
		//char *packstr_tmp2 = NULL;
		//char *jsonstr = NULL;
		int size = 0;
		
        size = packlen;
		/* 将接受到到包长度累积 */
		p_session->smtp_session_state.bdat_read_len += size;

		log_printf (ZLOG_LEVEL_DEBUG, "&&&&&&&&&&&&&&&&smtp BDAT trasmit-start-&&&&&&&&&&&&&&&&&&&& ");
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   

		/* 存储数据 */
		store_smtp_data (p_session, packstr, packlen);

		log_printf (ZLOG_LEVEL_DEBUG, "&&&&&&&&&&&&&&&&smtp BDAT trasmit-end-&&&&&&&&&&&&&&&&&&&& ");
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   

        /* 数据传输结束 */
		if ((p_session->smtp_session_state.bdat_total_len == p_session->smtp_session_state.bdat_read_len))
		{
			
		     p_session->data_store_flag = DATA_BDAT_END;
		     p_session->smtp_session_state.bdat_read_len = 0;
             p_session->smtp_session_state.bdat_total_len = 0;
			 log_printf (ZLOG_LEVEL_DEBUG, "this is bdat ");
	         log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   

		}
		else if ((p_session->smtp_session_state.bdat_last == TRUE) &&  (p_session->smtp_session_state.bdat_total_len == p_session->smtp_session_state.bdat_read_len)) 
		{
            p_session->smtp_session_state.bdat_seen = FALSE;
	        p_session->smtp_session_state.data_seen = FALSE;
	        p_session->smtp_session_state.dot_seen = TRUE;
	        p_session->data_store_flag = DATA_BDAT_END;

		    p_session->now_rcpt_count++;

	        SESSION_BUFFER_NODE    smtp_sft;
			smtp_sft.session_five_attr.ip_src = p_session->key.saddr;
			smtp_sft.session_five_attr.ip_dst = p_session->key.daddr;
			smtp_sft.session_five_attr.port_src = p_session->key.source;
			smtp_sft.session_five_attr.port_dst = p_session->key.dest;
			smtp_sft.session_five_attr.protocol = 3;
			sbuff_putdata(sbuffer_id, smtp_sft);
			log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
		}
		return 0;
}

