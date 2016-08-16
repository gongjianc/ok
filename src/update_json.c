/*
 * =====================================================================================
 *
 *       Filename:  update_json.c
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
#include "update_json.h"


/**
 * @brief        网络的ip转换为点分十进制
 *
 * @param src    源ip
 * @param str    输出ip
 *
 * @return 成功返回0，失败返回-1
 */
static int int_to_ipstr(int src, char *str)
{
	if (int_ntoa (src))
	{
	   strcpy (str, int_ntoa (src));
	}
	return 0;
}

/**
 * @brief 从json串中提获取需要解析的文件绝对路径 
 *
 * @param jsonsrc         json 串
 * @param filefull_fname  文件绝对路径
 * @param file_flag       文件列表flist中的位置
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_path_absolute (char *jsonsrc, char *filefull_fname, int file_flag)
{
	char *path_key = "fp";
	int res = 0;

    /* 获取需要解析文件路径 */
    res = get_file_value(jsonsrc, path_key, filefull_fname, file_flag);
    if (res == -1)
    {
     	log_printf (ZLOG_LEVEL_ERROR, "get_file_value()");
     	log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
    }

	log_printf (ZLOG_LEVEL_INFO, "get file full fname = %s", filefull_fname);
    if (access(filefull_fname, F_OK) != 0)
	{
		log_printf (ZLOG_LEVEL_ERROR, "file not exist");
     	log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	log_printf (ZLOG_LEVEL_DEBUG, "file full name:%s", filefull_fname);

    return 0;
}

/**
 * @brief 删除文件链表中的项
 *
 * @param jsonstr     原json串
 * @param jsondest    处理后的json串
 * @param file_flag   flist文件索引值
 *
 * @return 成功返回0，失败返回-1
 */
int remove_flist (char *jsonstr, char **jsondest, int file_flag) 
{
	if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "remove_flist(),json string is null");
     	log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	int size;
	json_t *object;
	json_error_t error;
	json_t *jsonflist;
	char *result = NULL;

	/* 读取json串,为jansson格式 */
	//log_printf (ZLOG_LEVEL_INFO, "update_json.c-34,jsonstr=%s ", jsonstr);
	object = json_loadb (jsonstr, strlen(jsonstr), 0, &error);
	//log_printf (ZLOG_LEVEL_INFO, "update_json.c-36,jsonstr ");

	jsonflist = json_object_get (object, "flist");
	json_array_remove (jsonflist, file_flag);

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
	    memset (*jsondest, 0, size);

	    strcpy(*jsondest, result);

	    free(result);
	    result = NULL;
	}
	json_decref(object);
	return 0;
}

/**
 * @brief 显示从抓包收到的协议结构体信息
 *
 * @param pronode 协议属性信息结构体
 *
 * @return 成功返回0，失败返回-1
 */
int show_session(SESSION_BUFFER_NODE *pronode)
{
	log_printf (ZLOG_LEVEL_DEBUG, "show_session_ftp()");
    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d", __FILE__, __LINE__);
	if (pronode == NULL)
	{
	   log_printf (ZLOG_LEVEL_ERROR, "show_session_ftp, struct is null");
       log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
       return -1;
	}
    log_printf (ZLOG_LEVEL_DEBUG, "protocol: %d", pronode->session_five_attr.protocol);

	char str[32] = {};
	memset (str, 0, 32);
	int_to_ipstr (pronode->session_five_attr.ip_src, str);
	log_printf (ZLOG_LEVEL_DEBUG, "ip_src: %s", str);

	memset (str, 0, 32);
	int_to_ipstr(pronode->session_five_attr.ip_dst, str);
	log_printf (ZLOG_LEVEL_DEBUG, "ip_dst: %s", str);

	log_printf (ZLOG_LEVEL_DEBUG, "port_src: %d", pronode->session_five_attr.port_src);

	log_printf (ZLOG_LEVEL_DEBUG, "port_dst: %d", pronode->session_five_attr.port_dst);

	log_printf (ZLOG_LEVEL_DEBUG, "strname:%s", pronode->strname);

	log_printf (ZLOG_LEVEL_DEBUG, "strpath:%s", pronode->strpath);

	log_printf (ZLOG_LEVEL_DEBUG, "orgname:%s", pronode->orgname);
	return 0;
}

/**
 * @brief json串中文件的个数
 *
 * @param jsonstr    json串
 * @param file_num   文件个数
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_num(char *jsonstr, int *file_num)
{
	if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "get_file_num(),json string is null");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	int size;
	json_t *object;
	json_error_t error;
	json_t *jsonflist;

	/* 读取json串,为jansson格式 */
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, get_file_num(),jsonstr=%s ", __FILE__, __LINE__, jsonstr);
	object = json_loadb (jsonstr, strlen(jsonstr), 0, &error);

	jsonflist = json_object_get (object, "flist");
    size = json_array_size(jsonflist);
    *file_num = size;

	json_decref(object);

	return 0;

}

/**
 * @brief           获取协议类型
 *
 * @param jsonstr   json串
 *
 * @return          协议号
 */
int get_protocol_type (char *jsonstr)
{
	if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "get_protocol_type(),json string is null");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	json_t *object;
	json_t *jsprotype;
	json_error_t error;
	int prt = 0;

	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, get_protocol_type(),jsonstr=%s ", __FILE__, __LINE__, jsonstr);
	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonstr, strlen(jsonstr), 0, &error);

	jsprotype = json_object_get (object, "prt");
	prt = json_integer_value (jsprotype);
	log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, jsprotype =%d ", __FILE__, __LINE__, prt);

	json_decref(object);
	return prt;

}



/**
 * @brief              获取tika产生的文件作者和文件类型
 *
 * @param jsonstr      tika产生的json串
 * @param tika_message tika解析出的文件信息结构体
 *
 * @return 成功返回0，失败返回-1
 */
int get_file_type_author (char *jsonstr, tika_message_s *tika_message)
{
	if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "get_file_type_author() json string is null");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	json_t *object;
	json_t *jsauthor;
   	json_t *jsfiletype;
   	json_t *jsissuccess;
   	json_t *jsisencrypted;
	json_error_t error;

	log_printf (ZLOG_LEVEL_DEBUG, "get_file_type_author(), jsonstr=%s ", jsonstr);
	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonstr, strlen(jsonstr), 0, &error);

	/* 文件作者 */
	jsauthor = json_object_get (object, "fileAuthor");
    if (json_string_value(jsauthor) != NULL)
	{	
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, jsauthor=%s ", __FILE__, __LINE__, json_string_value(jsauthor));
	    strcpy (tika_message->author, json_string_value(jsauthor));
	}

	/* 文件类型 */
	jsfiletype = json_object_get (object, "filetype");
    if (json_string_value(jsfiletype) != NULL)
	{
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, %s ", __FILE__, __LINE__, json_string_value(jsfiletype));
	    strcpy (tika_message->filetype, json_string_value(jsfiletype));
	}
  else
	{
	    strcpy (tika_message->filetype, "xxx");//by niulw for xxx
	    log_printf (ZLOG_LEVEL_INFO, "niulw for  add xxx file\n");
	}

	/* tika解析是否正确,是否成功 */
    jsissuccess = json_object_get (object, "isSuccess");
    if (json_string_value(jsissuccess) != NULL)
	{
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, %s ", __FILE__, __LINE__, json_string_value(jsissuccess));
	    strcpy (tika_message->issuccess, json_string_value(jsissuccess));
	}

	/* 得到是否加密 */
    jsisencrypted = json_object_get (object, "isEncrypted");
    if (json_string_value(jsisencrypted) != NULL)
	{
	    log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, %s ", __FILE__, __LINE__, json_string_value(jsisencrypted));
	    strcpy (tika_message->isEncrypted, json_string_value(jsisencrypted));
	}

	json_decref(object);
	return 0;

}

/**
 * @brief 释放并重新申请oldstr的内存，将newstr的字符串拷贝到oldstr中，释放newstr的内存资源
 *
 * @param oldstr  老的内存块
 * @param newstr  新的内存块
 *
 * @return 成功返回0，失败返回-1
 */
int freeold_copyfromnew_freenew (char **oldstr, char **newstr)
{
    if (*newstr == NULL) 	
	{
        log_printf (ZLOG_LEVEL_ERROR, "newstring is NULL");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}

	if (*oldstr)
	{
		free (*oldstr);
		*oldstr = NULL;
	}

	int size = strlen (*newstr);
	*oldstr = malloc (size + 1);
	if (*oldstr == NULL)
	{
        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	memset (*oldstr, 0, size + 1);

	strcpy (*oldstr, *newstr);

	free (*newstr);
	*newstr = NULL;

	return 0;
}

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
int set_tika_message (char *jsonsrc, char **jsondest, tika_message_s tika_message, char *file_content_path, int file_flag)
{
		int res = 0, size = 0;
		char *json_src = NULL;
		char *json_tmp = NULL;

		/* 作者 */
	    char *author_key = "au";
		/* 文件类型 */
        char *type_key = "ft";
		/* 文件内容路径 */
        char *content_key = "ftp";
		/* 加密标志 */
        char *isEncrypted_key = "ep";
		int encryptedflag = 0;


		/* 添加文件作者,到json串 */
		if (tika_message.author)
		{
            res = update_filelist_value (jsonsrc, &json_tmp, author_key, tika_message.author, file_flag);
	        if (res == -1)
	        {
                log_printf (ZLOG_LEVEL_ERROR, "update_filelist_value is error");
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
			    if (json_tmp)
			    {
			    	free(json_tmp);
			    	json_tmp = NULL;
			    }
				return -1;
	        }
            log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add author json string is：%s ", __FILE__, __LINE__, json_tmp);
		}

        res = freeold_copyfromnew_freenew (&json_src, &json_tmp);
		if (res == -1)
		{
            log_printf (ZLOG_LEVEL_ERROR, "copy error");
            log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
			if (json_tmp)
			{
				free(json_tmp);
				json_tmp = NULL;
			}
            if (json_src)
			{
				free(json_src);
				json_src = NULL;
			}
			return -1;
		}
		
        /* 添加文件类型,到json串 */
		if (tika_message.filetype)
		{
            res = update_filelist_value (json_src, &json_tmp, type_key, tika_message.filetype, file_flag);
	        if (res == -1)
	        {
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d, add file type error", __FILE__, __LINE__);
                if (json_src)
				{
					free(json_src);
					json_src = NULL;
				}
			    if (json_tmp)
			    {
			    	free(json_tmp);
			    	json_tmp = NULL;
			    }
				return -1;
	        }
            log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add author json string is：%s ", __FILE__, __LINE__, json_tmp);
		}

        res = freeold_copyfromnew_freenew (&json_src, &json_tmp);
		if (res == -1)
		{
            log_printf (ZLOG_LEVEL_ERROR, "copy error");
            log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
            if (json_src)
			{
				free(json_src);
				json_src = NULL;
			}
			if (json_tmp)
			{
				free(json_tmp);
				json_tmp = NULL;
			}
			return -1;
		}

	
        /* 添加加密判断,到json串 */
		if (tika_message.isEncrypted)
		{
			if (strcmp (tika_message.isEncrypted, "false") == 0 )
			{
                 encryptedflag = 0;
			}
			else if (strcmp (tika_message.isEncrypted, "true") == 0 )
			{
                 encryptedflag = 1;
			}
			else if (strcmp (tika_message.isEncrypted, "embededEncrypted") == 0 )
			{
                 encryptedflag = 2;
			}

            res = update_filelist_int_value (json_src, &json_tmp, isEncrypted_key,  encryptedflag, file_flag);
	        if (res == -1)
	        {
                log_printf (ZLOG_LEVEL_ERROR, "add encrypted error");
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
				if (json_src)
				{
					free(json_src);
					json_src = NULL;
				}
			    if (json_tmp)
			    {
			    	free(json_tmp);
			    	json_tmp = NULL;
			    }
				return -1;
	        }
            log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add author json string is：%s ", __FILE__, __LINE__, json_tmp);
		}
		
        res = freeold_copyfromnew_freenew (&json_src, &json_tmp);
		if (res == -1)
		{
            log_printf (ZLOG_LEVEL_ERROR, "copy error");
            log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
            if (json_src)
			{
				free(json_src);
				json_src = NULL;
			}
			if (json_tmp)
			{
				free(json_tmp);
				json_tmp = NULL;
			}
			return -1;
		}

        /* 添加文件内容路径到json串 */
		if (file_content_path)
		{
            res = update_filelist_value (json_src, &json_tmp, content_key, file_content_path, file_flag);
	        if (res == -1)
	        {
	            log_printf (ZLOG_LEVEL_ERROR, "add file content path error");
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
				if (json_src)
				{
					free(json_src);
					json_src = NULL;
				}
			    if (json_tmp)
			    {
			    	free(json_tmp);
			    	json_tmp = NULL;
			    }
				return -1;
	        }
		    else
		    {
                log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add author json string is：%s ", __FILE__, __LINE__, json_tmp);
		        size = strlen (json_tmp);
		        *jsondest = malloc (size+1);
	            if (*jsondest == NULL)
	            {
	                log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
				    if (json_src)
				    {
				    	free(json_src);
				    	json_src = NULL;
				    }
			        if (json_tmp)
			        {
			        	free(json_tmp);
			        	json_tmp = NULL;
			        }
	                return -1;
	            }
		        memset (*jsondest, 0, size+1);
		        strcpy (*jsondest, json_tmp);
                log_printf (ZLOG_LEVEL_INFO, "%s__%d, final result json string is：%s ", __FILE__, __LINE__, *jsondest);
		    }
		}

        if (json_tmp)
		{
			free(json_tmp);
			json_tmp = NULL;
		}

        if (json_src)
		{
			free(json_src);
			json_src = NULL;
		}
		return 0;
}


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
int set_tika_message_bak (char *jsonsrc, char **jsondest, tika_message_s tika_message, char *file_content_path, int file_flag)
{
		int res = 0, size = 0;
		char *json_author = NULL;
		char *json_filetype = NULL;
		char *json_content = NULL;
		char *json_encrypted = NULL;
		/* 作者 */
	    char *author_key = "au";
		/* 文件类型 */
        char *type_key = "ft";
		/* 文件内容路径 */
        char *content_key = "ftp";
		/* 加密标志 */
        char *isEncrypted_key = "ep";

		/* 添加文件作者,到json串 */
		if (tika_message.author)
		{
            res = update_filelist_value (jsonsrc, &json_author, author_key, tika_message.author, file_flag);
	        if (res == -1)
	        {
                log_printf (ZLOG_LEVEL_ERROR, "update_filelist_value is error");
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        }
            log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add author json string is：%s ", __FILE__, __LINE__, json_author);
		}

        /* 添加文件类型,到json串 */
		if (tika_message.filetype)
		{
            res = update_filelist_value (json_author, &json_filetype, type_key, tika_message.filetype, file_flag);
	        if (res == -1)
	        {
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d, add file type error", __FILE__, __LINE__);
	        }
            log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add file type json string is：%s",  __FILE__, __LINE__, json_filetype);
		}

        /* 添加加密判断,到json串 */
		if (tika_message.isEncrypted)
		{
            res = update_filelist_value (json_filetype, &json_encrypted, isEncrypted_key, tika_message.isEncrypted, file_flag);
	        if (res == -1)
	        {
                log_printf (ZLOG_LEVEL_ERROR, "add encrypted error");
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        }
            log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add judge encrypted json string is：%s",  __FILE__, __LINE__, json_encrypted);
		}

        /* 添加文件内容路径到json串 */
		if (file_content_path)
		{
            res = update_filelist_value (json_encrypted, &json_content, content_key, file_content_path, file_flag);
	        if (res == -1)
	        {
	            log_printf (ZLOG_LEVEL_ERROR, "add file content path error");
                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        }
		    else
		    {
                log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, add file content json string is：%s", __FILE__, __LINE__, json_content);
		        size = strlen (json_content);
		        *jsondest = malloc (size+1);
	            if (*jsondest == NULL)
	            {
	                log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	                log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	                return -1;
	            }
		        memset (*jsondest, 0, size+1);
		        strcpy (*jsondest, json_content);
                log_printf (ZLOG_LEVEL_INFO, "%s__%d, final result json string is：%s ", __FILE__, __LINE__, *jsondest);
		    }
		}

		/* 释放资源 */
		if (json_author)
		{
			free(json_author);
			json_author = NULL;
		}

		if (json_filetype)
		{
			free(json_filetype);
			json_filetype = NULL;
		}

		if (json_encrypted)
		{
			free(json_encrypted);
            json_encrypted = NULL;
		}

		if (json_content)
		{
			free(json_content);
			json_content = NULL;
		}
		return 0;
}


/**
* @brief 找到最后的slash(/)
*
* @param str  字符串
* @param mark 标记
*
* @return 成功返回0，失败返回-1
*/
int last_mark (char *str, char mark)
{
	log_printf (ZLOG_LEVEL_DEBUG, "str = %s", str);
	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d", __FILE__, __LINE__);
	if (str == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "last_mark(), string is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s____%d", __FILE__, __LINE__);
        return -1;
	}
    int site = 0;
    int count = 0;
    while (site <= strlen (str)) 
    {
        //log_printf (ZLOG_LEVEL_DEBUG, "laststr=%c ", str[site]);
        if (str[site++] == mark) 
        {
            count = site;
        }
    }
	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d", __FILE__, __LINE__);
	if ((count-1) < 0)
	{
		return -1;
	}
	else
	{
        return count-1;
	}
}

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
int get_file_path (char *jsonstr,  char *filepath, int file_flag)
{
	if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "json string is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
		return -1;
	}
	int size;
	json_t *object;
	json_t *jsonarr_obj;
	json_t *json_flist;
   	json_t *valuenew;
	json_error_t error;
    char *fpkey = "fp"; 
	//char file_value[NAMELEN] = {} ;
	char *file_value = NULL;
	int flag = 0;

	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonstr, strlen(jsonstr), 0, &error);
    json_flist = json_object_get (object, "flist");
    size = json_array_size(json_flist);
    if (file_flag > size)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d, get_file_path(), file list max", __FILE__, __LINE__);
        json_decref(object);
		return -1;
	}

	/* 得到文件列表 */
    jsonarr_obj = json_array_get (json_flist, file_flag);
    if (json_is_object(jsonarr_obj))
	{
		valuenew = json_object_get (jsonarr_obj, fpkey);
		const char *tmp = json_string_value(valuenew);
		size = strlen(tmp);
		file_value = malloc(size+1);
	    if (file_value == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
		memset (file_value, 0, size+1);
        if (tmp != NULL)
	    {
			 strcpy(file_value, tmp);
	    }
#if 0
        if (json_string_value(valuenew) != NULL)
	    {
			 strcpy(file_value, json_string_value(valuenew));
	    }
#endif
	}

    if(file_value)
	{
	    /* 找到最后的标记/,将文件路径切出来 */
        char mark = '/';
	    log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
        int site = last_mark (file_value, mark);
	    if (site > 0)
	    {
	       strncpy (filepath, file_value, site);
	    }
	    else
	    {
	       strcpy (filepath, ".");
	    }
	}
	else
	{
		log_printf (ZLOG_LEVEL_ERROR, "file path is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s____%d", __FILE__, __LINE__);
		flag = -1;
	}

	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d ", __FILE__, __LINE__);
	if (file_value)
	{
		free(file_value);
		file_value = NULL;
	}
	log_printf (ZLOG_LEVEL_DEBUG, "%s____%d", __FILE__, __LINE__);
    json_decref(object);
	return flag;
}



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
int get_file_value(char *jsonstr, char *key, char *file_value, int file_flag)
{
	if (jsonstr == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "json string is null");
	    log_printf (ZLOG_LEVEL_ERROR, "%s____%d", __FILE__, __LINE__);
		return -1;
	}
	int size;
	json_t *object;
	json_t *jsonarr_obj;
	json_t *json_flist;
   	json_t *valuenew;
	json_error_t error;

	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonstr, strlen(jsonstr), 0, &error);

	json_flist = json_object_get (object, "flist");
    size = json_array_size(json_flist);
	if (file_flag > size)
	{
		return -1;
	}
    jsonarr_obj = json_array_get (json_flist, file_flag);
    if (json_is_object(jsonarr_obj))
	{
		valuenew = json_object_get (jsonarr_obj, key);
		if (json_string_value(valuenew) != NULL)
		{
		    strcpy(file_value, json_string_value(valuenew));
		}
	}

	json_decref(object);
	return 0;
}

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
int update_filelist_int_value (char *jsonsrc, char **jsondest, char *key, int value, int file_flag)
{
	if (jsonsrc == NULL || key == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d,json string is null", __FILE__, __LINE__);
		return -1;
	}
    assert(jsonsrc != NULL && key != NULL);
	json_t *object;
	json_error_t error;
	char *result = NULL;
	int size = 0;
	json_t *json_flist;
	json_t *jsonarr_obj;

	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonsrc, strlen(jsonsrc), 0, &error);
	
	json_flist = json_object_get (object, "flist");
    size = json_array_size(json_flist);
    if (file_flag > size)
	{
		log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, file list max ", __FILE__, __LINE__);
	    json_decref(object);
		return -1;
	}
    jsonarr_obj = json_array_get (json_flist, file_flag);
    if (json_is_object(jsonarr_obj))
	{
		json_object_set_new (jsonarr_obj, key, json_integer(value));
	}

    result = json_dumps(object, JSON_PRESERVE_ORDER);

    log_printf (ZLOG_LEVEL_DEBUG, "%s-%d,result=%s ",__FILE__, __LINE__, result);
    log_printf (ZLOG_LEVEL_DEBUG, "%s-%d,size=%d ",__FILE__, __LINE__, size);
	log_printf (ZLOG_LEVEL_DEBUG, "%s-%d,*jsondest = %s ",__FILE__, __LINE__, *jsondest);
	if (result)
	{
#if 1
        size = strlen(result)+1;
	    *jsondest = malloc(size);
	    if (*jsondest == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    memset (*jsondest , 0, size);
#endif
	    strcpy(*jsondest, result);

	    free(result);
	    result = NULL;
	}

	json_decref(object);
	return 0;

}

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
int update_filelist_value (char *jsonsrc, char **jsondest, char *key, char *value, int file_flag)
{
	if (jsonsrc == NULL || key == NULL || value == NULL)
	{
		log_printf (ZLOG_LEVEL_ERROR, "%s__%d,json string is null", __FILE__, __LINE__);
		return -1;
	}
    assert(jsonsrc != NULL && key != NULL && value != NULL);
	int size;
	json_t *object;
	json_error_t error;
	char *result;
	json_t *json_flist;
	json_t *jsonarr_obj;

	/* 读取json串,为jansson格式 */
	object = json_loadb (jsonsrc, strlen(jsonsrc), 0, &error);
	
	json_flist = json_object_get (object, "flist");
    size = json_array_size(json_flist);
    if (file_flag > size)
	{
		log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, file list max ", __FILE__, __LINE__);
	    json_decref(object);
		return -1;
	}

    jsonarr_obj = json_array_get (json_flist, file_flag);
    if (json_is_object(jsonarr_obj))
	{
		json_object_set_new (jsonarr_obj, key, json_string(value));
	}

    result = json_dumps(object, JSON_PRESERVE_ORDER);

    log_printf (ZLOG_LEVEL_DEBUG, "%s-%d,result=%s ",__FILE__, __LINE__, result);
    log_printf (ZLOG_LEVEL_DEBUG, "%s-%d,size=%d ",__FILE__, __LINE__, size);
	log_printf (ZLOG_LEVEL_DEBUG, "%s-%d,*jsondest = %s ",__FILE__, __LINE__, *jsondest);
	if (result)
	{
#if 1
        size = strlen(result)+1;
	    *jsondest = malloc(size);
	    if (*jsondest == NULL)
	    {
	        log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	        log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	        return -1;
	    }
	    memset (*jsondest , 0, size);
#endif
	    strcpy(*jsondest, result);

	    free(result);
	    result = NULL;
	}

	json_decref(object);
	return 0;

}


#if 0
int main(int argc, char *argv[])
{
	char *jsonstr = "{\"prt\": \"0\", \"sip\": \"NULL\", \"dip\": \"NULL\", \"spt\": \"0\", \"dpt\": \"0\", \"pAttr\": {\"Username\": \"daigang\", \"Password\": \"123456\"}, \"flist\": [{\"ft\": \"NULL\", \"fn\": \"安装说明.docx\", \"au\": \"NULL\", \"fs\": 48399, \"fp\": \"安装说明.docx\", \"ftp\": \"NULL\"}]}";
	log_printf (ZLOG_LEVEL_INFO, "jsonstr=%s ", jsonstr);


	char *jsondest = malloc(strlen(jsonstr)+256);
	if (jsondest == NULL)
	{
	    log_printf (ZLOG_LEVEL_ERROR, "malloc error");
	    log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
	    return -1;
	}
	memset (jsondest, 0, strlen(jsonstr)+256);
    struct pronodebuf pronode;
	pronode.proto_type = 7;
	pronode.session_five_attr.source = 12345;
	pronode.session_five_attr.dest = 54321;
	pronode.session_five_attr.ssession_five_attr = 3333333;
	pronode.session_five_attr.dsession_five_attr = 4444444;
	int res = add_five_member(jsonstr, &jsondest, pronode);
	log_printf (ZLOG_LEVEL_INFO, "jsondest=%s ", jsondest);

#if 0
	int size = 256;
	char *file_value = malloc(size);
	memset (file_value, 0, size);

	int file_flag = 0;
	char *key1 = "fp";
	/* 获取源文件路径 */
	get_file_value(jsonstr, key1, &file_value, file_flag);
	log_printf (ZLOG_LEVEL_INFO, "file_value=%s ", file_value);

	/* 添加文件类型 */
	char *key2 = "ft";
	char *file_type2 = "docx";
	file_flag = 0;
	size = 1024;
	char *jsondest = malloc(size);
	memset (jsondest, 0, size);
	update_filelist_value(jsonstr, &jsondest, key2, file_type2, file_flag);
	log_printf (ZLOG_LEVEL_INFO, "jsondest=%s ", jsondest);

	char *key3 = "ftp";
	char *file_type3 = "content.docx";
	update_filelist_value(jsondest, &jsondest, key3, file_type3, file_flag);
	log_printf (ZLOG_LEVEL_INFO, "jsondest=%s ", jsondest);
#endif
	
	return 0;
}
#endif
