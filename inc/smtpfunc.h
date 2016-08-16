/*
 * =====================================================================================
 *
 *       Filename:  smtpfunc.h
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
#ifndef SMTPFUNC_H_
#define SMTPFUNC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <jansson.h>
//#include "session_buf.h"
#include "ring_buffer.h"
//key struct
#include "beap_hash.h"
#include "plog.h"


#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

#define NAME_LEN_MAX 128
#define MAIL_NAME_MAX   128
#define MAIL_RCPT_TO_MAX 40

typedef int (*smtp_sbuffputdata) (int iringbuffer_ID, SESSION_BUFFER_NODE ftt);

/*
 * smtp请求响应
 */
typedef enum zcsmtp_state
{
  SMTP_STATE_READING_RESPONSE, /* 响应 */
  SMTP_STATE_READING_CMDS,     /* 命令 */
  SMTP_STATE_READING_DATA,     /* DATA数据 */
  SMTP_STATE_READING_BDAT,     /* BDAT数据 */
  SMTP_STATE_READING_STARTTLS  /* STARTTLS */
} zcsmtp_state_t;

/* 信体传输状态的标志 */
typedef enum  zcsmtp_data_store_flag
{ 
  DATA_BDAT_WAIT,            /* 没有进行数据传输 */
  DATA_BDAT_START,           /* 开始数据传输 */
  DATA_BDAT_END              /* 数据传输结束 */
} zcsmtp_data_store_flag_t;  /* DATA数据传输的状态 */

/* 收件人链表 */
typedef struct rcpt_to
{
   char *rcpt_to_addr;                     /* 收件人 */
   struct rcpt_to *next;                   /* 指向下一个收件人邮箱地址 */
} rcpt_to_t;

/* 附件链表 */
typedef struct smtp_accessory
{
   char *smtp_accessory_name;               /* 附件名称 */
   char *smtp_accessory_dir;                /* 附件路径 */
   int smtp_accessory_size;                 /* 附件大小 */
   struct smtp_accessory *next;             /* 指向下一个附件 */
}smtp_accessory_t;

/* 信体信息 */
typedef struct mail_body_attribute
{
    char *mail_from;                   /* 信体发送人地址 */
    rcpt_to_t *rcpt_to;                /* 信体接收人地址 */
    int rcpt_count;                    /* 接收人个数 */
	char *mail_content;                /* 正文内容路径 */
	char *mail_subject;                /* 主题 */
	char *mail_time;                   /* 邮件发送的时间戳 */
	char *mail_id;                     /* 邮件id, 可能为空 */
    smtp_accessory_t *smtp_accessory;  /* 附件链表 */
	unsigned int finish;               /* 判断信体结束的标志,finish=1结束,0未结束*/ 
    struct mail_body_attribute *next;
} mail_body_attribute_t;

/* RSET命令前的mail信息 */
typedef struct smtp_reset 
{
    char *mail_from;                           /* 信封发送人地址 */
    rcpt_to_t *rcpt_to;                        /* 信封接收人地址 */
    int rcpt_count;                            /* 接收人个数 */
    mail_body_attribute_t *mail_body;          /* 信体信息链表 */
    struct smtp_reset *next;                   /* RSET的一下个mail信息 */
} smtp_reset_t;            

/* smtp传输状态信息 */
typedef struct zcsmtp_session_state
{
  unsigned char crlf_seen;           /* 看到一个CRLF在包尾 */
  unsigned char data_seen;           /* 已经看到一个DATA 命令 */
  unsigned char bdat_seen;           /* 已经看到一个BDAT 命令 */
  unsigned char dot_seen;            /* 看到一个DATA结束点 */
  unsigned char rset_seen;           /* 看到一个RSET */
  unsigned char quit_seen;           /* 看到一个RSET */
  unsigned char starttls_seen;       /* 看到STARTTLS */
  unsigned int bdat_total_len;       /* BDAT信息的总长度 */
  unsigned int bdat_read_len;        /* 目前读到BDAT信息的长度*/
  unsigned char bdat_last;           /* 是否是最后的BDAT数据块 */
} zcsmtp_session_state_t;



/* mail基本信息结构体，放入桶 */
typedef struct SmtpSession
{   
    char *mail_from;                           /* 信封发送人地址 */
    rcpt_to_t *rcpt_to;                        /* 信封接收人地址 */
    int rcpt_count;                            /* 接收人个数 */
    int now_rcpt_count;                        /* 当前接收人个数 */
    mail_body_attribute_t *mail_body;          /* 信体信息链表 */
    char *mail_body_name;                      /* 截取的信体名称 */
    char *mail_body_dir;                       /* 信体存放路径 */
    zcsmtp_state_t smtp_state;                 /* smtp请求响应,枚举 */
    zcsmtp_data_store_flag_t data_store_flag;  /* 信体传输标志,枚举 */
    zcsmtp_session_state_t smtp_session_state; /* smtp传输状态信息 */
	unsigned int session_quit_flag;            /* 标识会话结束发出QUIT命令 */
	struct tuple4 key;                         /* 会话id */
	char *result_json;                         /* 最终产生的json串 */
    struct SmtpSession *smtp_reset_msg;        /* RSET前的信息链表 */
    struct SmtpSession *next;
} SmtpSession_t;

/* 返回的结果结构体 */
typedef struct SMTPID
{   
    char *mail_from;                     /* 信封发件人地址 */
    rcpt_to_t *rcpt_to;                  /* 信封接收人地址 */
    int rcpt_count;                      /* 接收人个数 */
    mail_body_attribute_t *mail_body;    /* 信体信息链表 */
    struct SmtpSession *smtp_reset_msg;  /* RSET前的信息链表 */
	struct tuple4 key;                   /* 会话id */
    struct SMTPID *next;
} SMTPID_t;



/**
 * @brief 截取<mail from>中的mail from
 *
 * @param mailstr   <mail from>
 * @param mailaddr  mail from 
 *
 * @return 成功返回0， 失败返回-1 
 */
int get_mail_addr(char *mailstr, char **mailaddr);


/**
 * @brief 将收件人存放到hash桶
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 * @param packstr    smtp数据包
 * @param packlen    smtp数据包长度
 *
 * @return 成功返回0，失败返回-1 
 */
int add_rcpt_to (SmtpSession_t *p_session, char *packstr, unsigned int packlen);


/**
 * @brief 从字符串中分离出数字 
 *
 * @param str 源字符串
 * @param num 数字
 *
 * return 成功返回0，失败返回-1
 */
int numcapture (char *str, int *num);

/**
 * @brief 得到BDAT的信息的长度
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 * @param packstr    smtp数据包
 * @param packlen    smtp数据包长度
 *
 * return 成功返回0，失败返回-1
 */
int get_bdat_msg (SmtpSession_t *p_session, char *packstr, unsigned int packlen);


/**
 * @brief 处理RSET命令
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int reset_smtp_session (SmtpSession_t *p_session);


/**
 * @brief 释放附件链表
 *
 * @param smtp_accessory 附件链表
 *
 * return 成功返回0，失败返回-1
 */
int free_accessory(smtp_accessory_t *smtp_accessory);

/**
 * @brief 释放信体链表
 *
 * @param mail_body smtp邮件信头信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int free_mail_body(mail_body_attribute_t *mail_body);

/**
 * @brief 释放接收者链表
 *
 * @param rcpt_to    结构者信息链表
 *
 * return 成功返回0，失败返回-1
 */
int free_rcpt_to(rcpt_to_t *rcpt_to);

/**
 * @brief 释放桶中的项
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int free_smtp (SmtpSession_t *p_session);

/**
 * @brief 显示桶中的项
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int show_smtp (SmtpSession_t *p_session);

/**
 * @brief 循环显示桶中的链表项
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int while_show_smtp(SmtpSession_t *p_session);


/**
 * @brief   存储smtp数据生成eml文件 
 *
 * @param p_session  hash桶中的smtp状态属性信息结构体
 *
 * return 成功返回0，失败返回-1
 */
int store_smtp_data (SmtpSession_t *p_session, char *packstr, unsigned int packlen);


/**
 * @brief 设置信封发件人收件人
 *
 * @param p_sessionsrc  hash桶中的smtp状态属性信息结构体
 * @param jsonsrc       源json
 * @param jsondest      目的json
 *
 * return 成功返回0，失败返回-1
 */
int set_mailenvelope_rcpt_from (SmtpSession_t *p_sessionsrc, char *jsonsrc, char **jsondest);

/**
 * @brief       获取文件大小 
 *
 * @param path  文件绝对路径
 *
 * return 成功返回0，失败返回-1
 */
unsigned long smtp_get_file_size (char *path);

#endif
