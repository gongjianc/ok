/**
 * @file webmail_attach.c
 * @brief webmail程序提取附件，将附件长度、附件名、附件的存储名字uuid放到链表里
 * @author xiebeibei liji
 * @version 2014.10.24
 * @date 2014-10-24
 */
#include "webmail_attach.h"

/**
 * @brief init_att_nodelist 初始化附件链表
 *
 * @return 返回附件链表
 */
struct att_node *init_att_nodelist()
{
	struct att_node *headlist;
	int size = sizeof(struct att_node);
	headlist = malloc(size);
	if (headlist == NULL){
		log_printf(ZLOG_LEVEL_ERROR, "headlist is null,%s__%d", __FILE__, __LINE__);
		return NULL;
	}
	memset (headlist, 0, size);

	headlist->attlen=0;
	memset(headlist->RawAttName,'\0',ATT_NAME_LENGTH);
	memset(headlist->att_uuid,'\0',UUID_LENGTH);
	headlist->next = NULL;

	return headlist;
}
/**
 * @brief add_att_nodelist 增加附件链表结点 
 *
 * @param nodelist 要增加结点的附件链表
 * @param attlen 附件长度
 * @param RawAttName[ATT_NAME_LENGTH] 附件名字
 * @param att_uuid[UUID_LENGTH] 附件存储名字uuid
 *
 * @return 
 */
int add_att_nodelist (struct att_node *nodelist,  int attlen,char *RawAttName,char *att_uuid)
{
  	
	log_printf(ZLOG_LEVEL_DEBUG, "call add_att_nodelist, %s__%d", __FILE__, __LINE__);
	if (nodelist == NULL){
		log_printf(ZLOG_LEVEL_ERROR, "nodelist is null,%s__%d", __FILE__, __LINE__);
		return -1;
	}
	int size = 0;
	size = sizeof(struct att_node);
	struct att_node *newnode = malloc(size);
	if (newnode == NULL)
	{
		log_printf(ZLOG_LEVEL_ERROR, "malloc error %s__%d", __FILE__, __LINE__);
		return -1;
	}
	memset (newnode, 0, size);

	while (nodelist->next != NULL){
		nodelist = nodelist->next;
	}

	newnode->attlen=attlen;

    if(RawAttName)
	{
	  memcpy(newnode->RawAttName,RawAttName,strlen(RawAttName));
	}
	if(att_uuid)
	{
      memcpy(newnode->att_uuid,att_uuid,strlen(att_uuid));
	}

	newnode->next = NULL;
	nodelist->next = newnode;
	return 0;
}
/**
 * @brief free_att_nodelist 释放附件链表空间
 *
 * @param nodelist 附件链表
 *
 * @return 
 */
int free_att_nodelist (struct att_node *nodelist)
{
	if (nodelist == NULL){
		return -1;
	}

	struct att_node *tmp = nodelist;
	struct att_node *now = nodelist;

	while(now->next != NULL){
		tmp = now->next;
		now->next = tmp->next;

		free(tmp);
		tmp = NULL;
	}

	free(nodelist);
	nodelist = NULL;
	return 0;
}

