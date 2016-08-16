#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "plog.h"

/*uuid长度*/
#ifndef UUID_LENGTH
#define UUID_LENGTH 512
#endif

/*附件名长度*/
#ifndef ATT_NAME_LENGTH
#define ATT_NAME_LENGTH 1024 
#endif

/*附件结构体*/
struct att_node
{
	int attlen;  /*文件长度*/
	char RawAttName[ATT_NAME_LENGTH];/* 文件名*/
	char att_uuid[UUID_LENGTH];/*附件存储名uuid*/
	struct att_node *next;/*结点指针*/
};

/*初始化附件链表*/
struct att_node *init_att_nodelist();

/*附件链表添加结点*/
int add_att_nodelist (struct att_node *nodelist,  int attlen,char RawAttName[ATT_NAME_LENGTH],char att_uuid[UUID_LENGTH]);

/*释放附件链表*/
int free_att_nodelist (struct att_node *nodelist);
