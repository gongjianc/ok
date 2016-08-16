/*
 * =====================================================================================
 *
 *       Filename:  smtpfunc.c
 *
 *    Description:  处理smtp的常用工具函数 
 *
 *  function list:
 *           get_mail_addr()
 *                  提取"<mail from>"中的"mail from"
 *           add_rcpt_to()
 *                  将收件人存放到hash桶
 *           numcapture()
 *                  从字符串中分离出数字 
 *           get_bdat_msg()
 *                  得到BDAT的信息的长度
 *           reset_smtp_session()
 *                  处理RSET命令
 *           free_accessory()
 *                  释放附件链表
 *           free_mail_body()
 *                  释放信体链表
 *           free_rcpt_to()
 *                  释放接收者链表
 *           free_smtp()
 *                  释放桶中的项
 *           show_smtp()
 *                  显示桶中的项
 *           while_show_smtp()
 *                  循环显示桶中的链表项
 *           store_smtp_data()
 *                  存储smtp数据生成eml文件 
 *           set_mailenvelope_rcpt_from()
 *                  设置信封发件人收件人
 *           smtp_get_file_size()
 *                  获取文件大小 
 *
 *        Version:  1.0
 *        Created:  08/30/2014 07:13:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  唐玉山, etangyushan@126.com
 *   organization:  北京中测安华科技有限责任公司 
 *
 * =====================================================================================
 */
#include "smtpfunc.h"


/**
 * @brief 截取<mail from>中的mail from
 *
 * @param mailstr   <mail from>
 * @param mailaddr  mail from 
 *
 * @return 成功返回0， 失败返回-1 
 */
int get_mail_addr(char *mailstr, char **mailaddr)
{
     char *p1 = strchr (mailstr, '<');	
	 char *p2 = strchr (mailstr, '>');
	 int size = p2 - p1 - 1;
	 if (size <= 0)
	 {
	     strcpy (*mailaddr, mailstr);
	 }
	 else
	 {
	     strncpy (*mailaddr, p1+1, size);
	 }
	 return 0;
}

/**
 * @brief 将收件人存放到hash桶
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 * @param packstr    smtp数据包
 * @param packlen    smtp数据包长度
 *
 * @return 成功返回0，失败返回-1 
 */
int add_rcpt_to (SmtpSession_t *p_session, char *packstr, unsigned int packlen)
{
	if (packstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d, packet is NULL", __FILE__, __LINE__);
		return -1;
	}
	else if (p_session == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "smtp struct message is NULL");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
		return -1;
	}

	//log_printf (ZLOG_LEVEL_DEBUG, "packet content is:%s ", packstr);
	//log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
	int size = packlen;
    char *mailaddr = NULL;
	mailaddr = malloc (size+1);
	if (mailaddr == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (mailaddr, 0, size+1);


	/* 获取地址 */
	log_printf (ZLOG_LEVEL_DEBUG, "get rcpt to from packet ");
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
    get_mail_addr (packstr, &mailaddr);
	log_printf (ZLOG_LEVEL_DEBUG, "mailaddr=%s ", mailaddr);
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   

	rcpt_to_t *tmp_add = NULL;
	
	if (p_session->rcpt_to == NULL)
	{
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		size = sizeof(rcpt_to_t);
        p_session->rcpt_to = malloc(size);
	    memset (p_session->rcpt_to, 0, size);

		size = strlen(mailaddr);
        p_session->rcpt_to->rcpt_to_addr = malloc(size+1);
	    if (p_session->rcpt_to->rcpt_to_addr == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
		memset (p_session->rcpt_to->rcpt_to_addr, 0, size+1);
		p_session->rcpt_to->next = NULL;
        strcpy (p_session->rcpt_to->rcpt_to_addr, mailaddr);
	}
	else
	{
        rcpt_to_t *rcpt_to_tmp = p_session->rcpt_to;
	    while (rcpt_to_tmp->next != NULL)
	    {
	    	rcpt_to_tmp = rcpt_to_tmp->next;
	    }

	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		size = sizeof(rcpt_to_t);
		tmp_add = malloc(size);
	    if (tmp_add == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    memset (tmp_add, 0, size);

		size = strlen(mailaddr);
        tmp_add->rcpt_to_addr = malloc(size+1);
	    if (tmp_add->rcpt_to_addr == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
		memset (tmp_add->rcpt_to_addr, 0, size+1);
        strcpy (tmp_add->rcpt_to_addr, mailaddr);
		tmp_add->next = NULL;

        rcpt_to_tmp->next = tmp_add;
	}

	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
	if (mailaddr)
	{
		free(mailaddr);
		mailaddr = NULL;
	}
	return 0;
}

/**
 * @brief 从字符串中分离出数字 
 *
 * @param str 源字符串
 * @param num 数字
 *
 * return 成功返回0，失败返回-1
 */
int numcapture (char *str, int *num)
{
     log_printf (ZLOG_LEVEL_DEBUG, "str=%s ", str);
     int sum = 0;
     int size = 0;
     int i = 0;
	 size = strlen(str);

     while (1) 
     {
         if (*str >= '0' && *str <= '9') 
         {
             sum = sum * 10 + (*str - '0');
             str++;
         }
         else
         {
             str++;
         }

         i++;
         if (i >= size) 
         {
			 break;
         }
     }
	 //log_printf (ZLOG_LEVEL_INFO, "sum=%d ", sum);
	 //log_printf (ZLOG_LEVEL_INFO, "str=%s ", str);
	 *num = sum;
	 return 0;
}

/**
 * @brief 得到bdat的信息的长度
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 * @param packstr    smtp数据包
 * @param packlen    smtp数据包长度
 *
 * return 成功返回0，失败返回-1
 */
int get_bdat_msg (SmtpSession_t *p_session, char *packstr, unsigned int packlen)
{
     int num = 0;

	 /* 字符串中提取数字 */
     numcapture (packstr, &num);
     p_session->smtp_session_state.bdat_total_len = num;
	 log_printf (ZLOG_LEVEL_DEBUG, "bdat len is:%d ", num);
	 return 0;
}

/**
 * @brief 处理rset命令
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int reset_smtp_session (SmtpSession_t *p_session)
{
	log_printf (ZLOG_LEVEL_INFO, "deal with rset command ");
	log_printf (ZLOG_LEVEL_INFO, "%s__%d ", __FILE__, __LINE__);   
	SmtpSession_t *p_session_tmp = p_session; 
	//SmtpSession_t *smtp_reset_msg_tmp =  p_session_tmp->smtp_reset_msg

	int size = sizeof (SmtpSession_t);
    p_session_tmp->smtp_reset_msg  = malloc (size+1);
	if (p_session_tmp->smtp_reset_msg == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (p_session_tmp->smtp_reset_msg, 0, size+1);

	p_session_tmp->smtp_reset_msg->mail_from = p_session_tmp->mail_from;
	p_session_tmp->smtp_reset_msg->rcpt_to = p_session_tmp->rcpt_to;
	p_session_tmp->smtp_reset_msg->rcpt_count = p_session_tmp->rcpt_count;
	p_session_tmp->smtp_reset_msg->now_rcpt_count = p_session_tmp->now_rcpt_count;
	p_session_tmp->smtp_reset_msg->mail_body = p_session_tmp->mail_body;
	p_session_tmp->smtp_reset_msg->mail_body_name = p_session_tmp->mail_body_name;
	p_session_tmp->smtp_reset_msg->result_json = p_session_tmp->result_json;
	p_session_tmp->smtp_reset_msg->mail_body_dir = p_session_tmp->mail_body_dir;
	p_session_tmp->smtp_reset_msg->smtp_state = p_session_tmp->smtp_state;
	p_session_tmp->smtp_reset_msg->data_store_flag = p_session_tmp->data_store_flag;
	p_session_tmp->smtp_reset_msg->smtp_session_state = p_session_tmp->smtp_session_state;
	p_session_tmp->smtp_reset_msg->session_quit_flag = p_session_tmp->session_quit_flag;
	p_session_tmp->smtp_reset_msg->key = p_session_tmp->key;
	p_session_tmp->smtp_reset_msg->smtp_reset_msg = p_session_tmp->smtp_reset_msg;
	p_session_tmp->smtp_reset_msg->next = p_session_tmp->next;

    p_session_tmp->mail_from = NULL; 
	p_session_tmp->rcpt_to = NULL;
	p_session_tmp->rcpt_count = 0;
	p_session_tmp->now_rcpt_count = 0;
	p_session_tmp->mail_body = NULL;
	p_session_tmp->mail_body_name = NULL; 
	p_session_tmp->result_json = NULL; 
	p_session_tmp->mail_body_dir = NULL;
	p_session_tmp->smtp_state = 0;
	p_session_tmp->data_store_flag = 0;
	memset (&(p_session_tmp->smtp_session_state), 0, sizeof(zcsmtp_session_state_t));
	p_session_tmp->session_quit_flag = 0;
	//p_session_tmp->key;  不改变
	p_session_tmp->smtp_reset_msg = NULL;
	p_session_tmp->next = NULL;

	return 0;
	/* 将前面的会话剪切到smtp_reset_t的链表 */
}

/**
 * @brief 释放附件链表
 *
 * @param smtp_accessory 附件链表
 *
 * return 成功返回0，失败返回-1
 */
int free_accessory(smtp_accessory_t *smtp_accessory)
{
	smtp_accessory_t *smtp_accessory_tmp = smtp_accessory;
	smtp_accessory_t *tmp = smtp_accessory;
	if (smtp_accessory_tmp == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "accessory file list is NULL");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__,  __LINE__);
        return -1;
	}

	while(smtp_accessory_tmp != NULL)
	{
        smtp_accessory_tmp = smtp_accessory_tmp->next;
		if (tmp->smtp_accessory_dir)
		{
            free(tmp->smtp_accessory_dir);
            tmp->smtp_accessory_dir = NULL;
		}
		if (tmp->smtp_accessory_name)
		{
            free(tmp->smtp_accessory_name);
            tmp->smtp_accessory_name = NULL;
		}
		if (tmp)
		{
			free(tmp);
			tmp = NULL;
		}
		tmp = smtp_accessory_tmp;
	}

	return 0;
}

/**
 * @brief 释放信体链表
 *
 * @param mail_body smtp邮件信头信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int free_mail_body(mail_body_attribute_t *mail_body)
{
	mail_body_attribute_t *mail_body_tmp = mail_body;
	mail_body_attribute_t *tmp = mail_body;
	if (mail_body_tmp == NULL)
	{
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__,  __LINE__);
		log_printf (ZLOG_LEVEL_ERROR, "free mail body is NULL");
		return -1;
	}
	while (mail_body_tmp != NULL)
	{
		mail_body_tmp = mail_body_tmp->next;
		if (tmp->mail_from)
		{
			free(tmp->mail_from);
			tmp->mail_from = NULL;
		}

		if (tmp->rcpt_to)
		{
            free_rcpt_to (tmp->rcpt_to);
		}

		if (tmp->mail_content)
		{
			free(tmp->mail_content);
			tmp->mail_content = NULL;
		}

		if (tmp->mail_subject)
		{
			free(tmp->mail_subject);
			tmp->mail_subject = NULL;
		}
		if (tmp->mail_time)
		{
			free(tmp->mail_time);
			tmp->mail_time = NULL;
		}

		if (tmp->mail_id)
		{
			free(tmp->mail_id);
			tmp->mail_id = NULL;
		}

		if (tmp->smtp_accessory)
		{
            free_accessory(tmp->smtp_accessory);
		}
		if (tmp)
		{
			free(tmp);
			tmp = NULL;
		}
		tmp = mail_body_tmp;

	}

	return 0;
}

/**
 * @brief 释放接收者链表
 *
 * @param rcpt_to    结构者信息链表
 *
 * return 成功返回0，失败返回-1
 */
int free_rcpt_to(rcpt_to_t *rcpt_to)
{
	rcpt_to_t *rcpt_to_tmp = rcpt_to;
	rcpt_to_t *tmp = rcpt_to;
	if (rcpt_to_tmp == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d, rcpt to list is NULL", __FILE__, __LINE__);
		return -1;
	}
    while(rcpt_to_tmp != NULL)
	{
		rcpt_to_tmp = rcpt_to_tmp->next;

        if (tmp->rcpt_to_addr)
		{
			free(tmp->rcpt_to_addr);
			tmp->rcpt_to_addr = NULL;
		}
		if (tmp)
		{
			free(tmp);
			tmp = NULL;
		}
		tmp = rcpt_to_tmp;
	}

	return 0;
}

/**
 * @brief 释放桶中的项
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int free_smtp (SmtpSession_t *p_session)
{
	SmtpSession_t *p_session_tmp = p_session; 
	SmtpSession_t *tmp = p_session; 
	if (p_session == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "free_smtp");
	}
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__,  __LINE__);
	while (p_session_tmp != NULL)
	{
        p_session_tmp = p_session_tmp->next;

	    if (tmp->mail_from)
		{
		     free (tmp->mail_from);	
		     tmp->mail_from = NULL;
		}

        log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__,  __LINE__);
	    if (tmp->rcpt_to)
		{
            free_rcpt_to(tmp->rcpt_to);
		}

	    if (tmp->mail_body)
		{
            free_mail_body(tmp->mail_body);
		}

	    if (tmp->mail_body_name)
		{
		    free(tmp->mail_body_name);
		    tmp->mail_body_name = NULL;
		}

        if (tmp->result_json)
		{
		    free(tmp->result_json);
		    tmp->result_json = NULL;
		}

        if (tmp->mail_body_dir)
		{
		    free(tmp->mail_body_dir);
		    tmp->mail_body_dir = NULL;
		}

	    if (tmp->smtp_reset_msg)
		{
		  	/* 递归释放 */
		  	free_smtp (tmp->smtp_reset_msg);
		}

		if (tmp)
		{
		  	free(tmp);
		  	tmp = NULL;
		}

		tmp = p_session_tmp;
	}
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__,  __LINE__);
	return 0;
}

/**
 * @brief 显示桶中的项
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int show_smtp (SmtpSession_t *p_session)
{
	if (p_session == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d, p_session is NULL", __FILE__, __LINE__);
	}
	log_printf (ZLOG_LEVEL_DEBUG, "mail_from=%s ", p_session->mail_from);

    rcpt_to_t *rcpt_to = p_session->rcpt_to;
       
	if (rcpt_to)
	{
		while(rcpt_to)
		{	
	        log_printf (ZLOG_LEVEL_DEBUG, "rcpt_to=%s ", rcpt_to->rcpt_to_addr);
			rcpt_to = rcpt_to->next;
		}
    }	
	log_printf (ZLOG_LEVEL_DEBUG, "rcpt_count=%d ", p_session->rcpt_count);
	log_printf (ZLOG_LEVEL_DEBUG, "now_rcpt_count=%d ", p_session->now_rcpt_count);
	log_printf (ZLOG_LEVEL_DEBUG, "mail_body... ");
	log_printf (ZLOG_LEVEL_DEBUG, "mail_body_name=%s ", p_session->mail_body_name);
	log_printf (ZLOG_LEVEL_DEBUG, "result_json=%s ", p_session->result_json);
	log_printf (ZLOG_LEVEL_DEBUG, "mail_body_dir=%s ", p_session->mail_body_dir);
	log_printf (ZLOG_LEVEL_DEBUG, "smtp_state=%d ", p_session->smtp_state);
	log_printf (ZLOG_LEVEL_DEBUG, "data_store_flag=%d ", p_session->data_store_flag);
	log_printf (ZLOG_LEVEL_DEBUG, "smtp_session_state->data_seen=%d ", p_session->smtp_session_state.data_seen);
	log_printf (ZLOG_LEVEL_DEBUG, "smtp_session_state->dot_seen=%d ", p_session->smtp_session_state.dot_seen);
	log_printf (ZLOG_LEVEL_DEBUG, "session_quit_flag=%d ", p_session->session_quit_flag);
	log_printf (ZLOG_LEVEL_DEBUG, "key.saddr=%d ", p_session->key.saddr);
	log_printf (ZLOG_LEVEL_DEBUG, "key.daddr=%d ", p_session->key.daddr);
	log_printf (ZLOG_LEVEL_DEBUG, "key.source=%d ", p_session->key.source);
	log_printf (ZLOG_LEVEL_DEBUG, "key.dest=%d ", p_session->key.dest);
	return 0;
}

/**
 * @brief 循环显示桶中的链表项
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int while_show_smtp(SmtpSession_t *p_session)
{
	while(p_session)
	{
       show_smtp (p_session);
	   p_session = p_session->next;
	}
	return 0;
}


/**
 * @brief   存储smtp数据生成eml文件 
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int store_smtp_data (SmtpSession_t *p_session, char *packstr, unsigned int packlen)
{
	log_printf (ZLOG_LEVEL_DEBUG, "store smtp data ");
	//log_printf (ZLOG_LEVEL_DEBUG, "packstr=%s ", packstr);
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__,  __LINE__);
	if (packstr == NULL)
	{
        log_printf (ZLOG_LEVEL_ERROR, "packet is NULL ");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__,  __LINE__);
	}
	int size =  strlen(p_session->mail_body_name) + strlen(p_session->mail_body_dir);
	char *filename = malloc(size + 2);
	if (filename == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (filename, 0, size + 2);
	/* eml名称的绝对路径 */
	sprintf (filename, "%s/%s", p_session->mail_body_dir, p_session->mail_body_name);
	
	log_printf (ZLOG_LEVEL_DEBUG, "filename=%s ", filename);

#if 1
    FILE *pfile = fopen (filename, "a+");
	if (pfile == NULL)
	{
         if (filename)
	     {
	     	free(filename);
	     	filename = NULL;
	     }
		 return -1;
	}
	if (fwrite (packstr, packlen, 1, pfile) != 1)
	{
         if (filename)
	     {
	     	free(filename);
	     	filename = NULL;
	     }
         if (pfile)
	     {
	         fclose(pfile);
	         pfile = NULL;
	     }
		 return -1;
	}

	if (pfile)
	{
	    fclose(pfile);
	    pfile = NULL;
	}
#endif

	if (filename)
	{
		free(filename);
		filename = NULL;
	}

	return 0;
}

/**
 * @brief 设置信封发件人收件人
 *
 * @param p_sessionsrc  hash桶中的smtp状态属性信息结构体
 * @param jsonsrc       源json
 * @param jsondest      目的json
 *
 * return 成功返回0，失败返回-1
 */
int set_mailenvelope_rcpt_from (SmtpSession_t *p_sessionsrc, char *jsonsrc, char **jsondest)
{
    if (p_sessionsrc == NULL)
	{
	 	log_printf (ZLOG_LEVEL_ERROR, "smtp struct is NULL");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
	 	return -1;
	}
    SmtpSession_t *p_session = p_sessionsrc;

    if (jsonsrc == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "json string is NULL");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d ", __FILE__, __LINE__);   
		return -1;
	}

	json_t *object;
	json_error_t error;
	json_t *tolist;
	json_t *pAttr;
	tolist = json_array();;
	int size = 0;
	//int i = 0;
	char *result = NULL;

	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonsrc, strlen(jsonsrc), 0, &error);
	pAttr = json_object_get (object, "pAttr");

	/* 设置信封发件人 */
#if 0
	char *sender = "from";
	size = strlen (sender);
    char *mail_from = malloc(size + 1);	
	if (mail_from == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (mail_from, 0, size+1);
    get_mail_addr(sender, &(mail_from));
	if (mail_from)
	{
		free(mail_from);
		mail_from = NULL;
	}
#endif
	json_object_set_new (pAttr, "from", json_string(p_session->mail_from));

	/*产生收件人的json数组*/
	rcpt_to_t *rcpt_to = p_session->rcpt_to;
	int listsize = 0;
	log_printf (ZLOG_LEVEL_DEBUG, "listsize=%d ", listsize);
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
    //tolist = json_object_get (pAttr, "to");

    while (rcpt_to)	
	{
		log_printf (ZLOG_LEVEL_DEBUG, "set rcpt to json string array: ");
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);   
		json_array_append_new (tolist, json_string(rcpt_to->rcpt_to_addr));
		rcpt_to = rcpt_to->next;
	}
	json_object_set_new (pAttr, "to", tolist);

	result = json_dumps (object, JSON_PRESERVE_ORDER);
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

/**
 * @brief       获取文件大小 
 *
 * @param path  文件绝对路径
 *
 * return 成功返回0，失败返回-1
 */
unsigned long smtp_get_file_size (char *path)
{
	unsigned long filesize = -1;
	struct stat statbuff;
	if (stat (path, &statbuff) < 0)
	{
		return filesize;
	}
	else
	{
		filesize = statbuff.st_size;
		return filesize;
	}
}

#if 0
int main()
{
   SmtpSession_t *p_session = NULL;
   int size = sizeof (SmtpSession_t);
   p_session = malloc(size);
   if (p_session == NULL)
   {
       log_printf (ZLOG_LEVEL_ERROR, "malloc error");
       log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
       return -1;
   }
   memset (p_session, 0, size);
#if 0
   char *packstr = "MAIL FROM: <etangyushan@126.com>";
   add_rcpt_to (p_session, packstr);
   log_printf (ZLOG_LEVEL_INFO, "rcpt_to=%s ", p_session->rcpt_to->rcpt_to_addr);
   int num = 0;
   numcapture (packstr, &num);
   log_printf (ZLOG_LEVEL_INFO, "num=%d ", num);
   char *packstr = "BDAT 100403";
   get_bdat_msg (p_session, packstr);
   store_smtp_data (SmtpSession_t *p_session, char *packstr)
#endif
}
#endif
