/*************************************************
Copyright (C), 2014, Tech. Co., Ltd.
 File name: http_IM_parser.h
Author: 许建东
 Version: V 0.2
Date: 2014-12-15
Description: 读取并解析IM的配置文件。
	依赖文件：read_IM_config.h
Others:

Function List: // 主要函数列表，每条记录应包括函数名及功能简要说明
				1.	int IM_memcpy(char **p1, const char *p2)
					复制const char *p2到char **p1；
				2.	_IM_HOST_URL_REG  *init_IM_list()
					初始化链表
				3.	int add_IM_list(_IM_HOST_URL_REG *nodelist, const char *host, const char * url, const char *reg)
					为链表增加节点
				3.  int read_IM_config()
					读取并解析IM的配置文件。
 History:

1. Date:
 Author:
 Modification:
 2. ...
 *************************************************/
#include "read_IM_config.h"
#include "plog.h"

//定义全局的链表存放配置参数
_IM_HOST_URL_REG 		*IM_memcmp_match_NodeList;
_IM_HOST_URL_REG 		*IM_memcmp_match_head_Node;
_IM_HOST_URL_REG		*IM_pcre_match_NodeList;
_IM_HOST_URL_REG		*IM_pcre_match_head_Node;

/*************************************************
Function: int IM_memcpy(char **p1, const char *p2)
Description: 复制const char *p2到char **p1；
Calls: 
Called By: 
	1.int add_IM_list(_IM_HOST_URL_REG *nodelist, const char *host, const char * url, const char *reg)
Table Accessed: 
Table Updated: 
Input: 
Output: 
Return: 复制成功返回0，失败返回-1;
Others: // 其它说明
*************************************************/
int IM_memcpy(char **p1, const char *p2)
{
	int size = strlen(p2) + 1;
	*p1 = malloc(size);
	if(NULL == *p1)
	{
		log_printf(ZLOG_LEVEL_ERROR, "malloc failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	memset(*p1, '\0', size);
	memcpy(*p1, p2, size-1);
	return 0;
}
/*************************************************
Function: _IM_HOST_URL_REG  *init_IM_list()
Description: 初始化链表
Calls: 
Called By: 
	1. int read_IM_config()
	
Table Accessed: 
Table Updated: 
Input: 
Output: 
Return: 返回链表头指针
Others: // 其它说明
*************************************************/
_IM_HOST_URL_REG  *init_IM_list()
{
	_IM_HOST_URL_REG *headlist;
	int size = sizeof(_IM_HOST_URL_REG);
	headlist = (_IM_HOST_URL_REG *)malloc(size);
	if(NULL == headlist)
	{
		log_printf(ZLOG_LEVEL_ERROR, "malloc failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return NULL;
	}	
	memset (headlist, 0, size);
	headlist->host = NULL;
	headlist->url = NULL;
	headlist->reg = NULL;
	headlist->next = NULL;
	return headlist;
}
/*************************************************
Function: int add_IM_list(_IM_HOST_URL_REG *nodelist, const char *host, const char * url, const char *reg)
Description: 增加链表节点
Calls: 
	1.	int IM_memcpy(char **p1, const char *p2)
Called By: 
	1. int read_IM_config()
	
Table Accessed: 
Table Updated: 
Input: 
Output: 
Return: 增加成功返回0，否则返回-1
Others: // 其它说明
*************************************************/
int add_IM_list(_IM_HOST_URL_REG *nodelist, const char *host, const char * url, const char *reg)
{
	if (nodelist == NULL)
	{
		log_printf(ZLOG_LEVEL_ERROR, "init list fail : -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	int size = 0;
    size = sizeof(_IM_HOST_URL_REG);
    _IM_HOST_URL_REG *newnode = (_IM_HOST_URL_REG *)malloc(size);
    if(NULL == newnode)
    {
    	log_printf(ZLOG_LEVEL_ERROR, "malloc failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
    	return -1;
    }
    memset(newnode, 0, size);
    while (nodelist->next != NULL)
    {   
        nodelist = nodelist->next;
    }
	//为链表节点成员赋值
    
    if(0 != IM_memcpy(&newnode->host, host))
    {
    	log_printf(ZLOG_LEVEL_ERROR, "memcpy failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
    	return -1;
    }

    if(0 != IM_memcpy(&newnode->url, url))
    {
    	log_printf(ZLOG_LEVEL_ERROR, "memcpy failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
    	return -1;
    }

    if(0 != IM_memcpy(&newnode->reg, reg))
    {
    	log_printf(ZLOG_LEVEL_ERROR, "memcpy failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
    	return -1;
    }	
	newnode->next = NULL;
    nodelist->next = newnode;
    return 0;
}
/*************************************************
Function: int add_IM_list(_IM_HOST_URL_REG *nodelist, const char *host, const char * url, const char *reg)
Description: 读取并解析IM的配置文件。
Calls: 
	1.	_IM_HOST_URL_REG  *init_IM_list()
	2.	int add_IM_list(_IM_HOST_URL_REG *nodelist, const char *host, const char * url, const char *reg)
Called By: 
	1. int read_IM_config()
	
Table Accessed: 
Table Updated: 
Input: 
Output: 
Return: 增加成功返回0，否则返回-1
Others: // 其它说明
*************************************************/
int read_IM_config()
{
    int 			i,j;
	//定义迭代器指针
    void 			*memcmp_match_iter  = NULL;
    void 			*pcre_match_iter  = NULL;
	json_t 			*IM_json = NULL;
	json_t 			*memcmp_match_object = NULL;
	json_t 			*pcre_match_object = NULL;
	json_t 			*son_memcmp_match_object = NULL;
	json_t 			*son_pcre_match_object = NULL;
	json_error_t 	error;
	json_t 			*jhost = NULL;
	json_t 			*jurl = NULL;
	json_t 			*jreg = NULL;

	/* 读取json串,为jansson格式 */
	IM_json = json_load_file("conf/IM_config.json", 0, &error);
	if(NULL == IM_json)
	{
		log_printf(ZLOG_LEVEL_ERROR, "failed to load json file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	//获取JSON对象
	memcmp_match_object = json_object_get(IM_json, "memcmp_match");
	pcre_match_object = json_object_get(IM_json, "pcre_match");

	//产生jansson迭代器 ,把对象的key放到里面
    memcmp_match_iter = json_object_iter(memcmp_match_object);
    pcre_match_iter = json_object_iter(pcre_match_object);
    //初始化链表
    if(NULL == (IM_memcmp_match_NodeList = init_IM_list()))
	{
		log_printf(ZLOG_LEVEL_ERROR, "Initial IM list failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	//头节点指针赋值
	IM_memcmp_match_head_Node = IM_memcmp_match_NodeList;
	//初始化链表
	if(NULL == (IM_pcre_match_NodeList = init_IM_list()))
	{
		log_printf(ZLOG_LEVEL_ERROR, "Initial IM list failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
		return -1;
	}
	//头节点指针赋值
	IM_pcre_match_head_Node = IM_pcre_match_NodeList;
	
	while(memcmp_match_iter)
	{
		/* 获得value */
		son_memcmp_match_object = json_object_iter_value(memcmp_match_iter);
		jhost = json_object_get(son_memcmp_match_object, "host");
		jurl = json_object_get(son_memcmp_match_object, "url");
		jreg = json_object_get(son_memcmp_match_object, "reg");
		if(0 != add_IM_list(
					IM_memcmp_match_NodeList,
					json_string_value(jhost),
					json_string_value(jurl),
					json_string_value(jreg)))
		{
			log_printf(ZLOG_LEVEL_ERROR, "add IM list failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			return -1;
		}
		/* 下一个*/
		memcmp_match_iter = json_object_iter_next(memcmp_match_object, memcmp_match_iter);
		i++;
	}

	while(pcre_match_iter)
	{
		/* 获得value */
		son_pcre_match_object = json_object_iter_value(pcre_match_iter);
		jhost = json_object_get(son_pcre_match_object, "host");
		jurl = json_object_get(son_pcre_match_object, "url");
		jreg = json_object_get(son_pcre_match_object, "reg");
		if(0 != add_IM_list(
					IM_pcre_match_NodeList, 
					json_string_value(jhost),
					json_string_value(jurl),
					json_string_value(jreg)))
		{
			log_printf(ZLOG_LEVEL_ERROR, "add IM list failed: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
			return -1;
		}
		/* 下一个*/
		pcre_match_iter = json_object_iter_next(pcre_match_object, pcre_match_iter);
		j++;
	}
	//引用计数器减一可以销毁数据
	json_decref(jhost);
	json_decref(jurl);
	json_decref(jreg);
	json_decref(son_memcmp_match_object);
	json_decref(memcmp_match_object);
	json_decref(son_pcre_match_object);
	json_decref(pcre_match_object);
    json_decref(IM_json);
	return 0;
}
