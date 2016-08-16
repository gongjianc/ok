/*
 * =====================================================================================
 *
 *       Filename:  smtp_dissect.h
 *
 *    Description:  处理smtp数据包，产生一个eml文件，发送信号给gmime解析函数进行解析 
 *
 *  function list:
 *           gmime_parse_mailbody()
 *                 使用gmime解析信体
 *           gmime_tika_send()
 *                 发送tika解析出的json 
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
#ifndef SMTP_DISSECT_H_
#define SMTP_DISSECT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <uuid/uuid.h>
#include <jansson.h>
#include "beap_hash.h"
#include "update_json.h"
#include "smtpfunc.h"
#include "extractmain.h"
#include "gmime_smtp.h"
#include "plog.h"

extern hash_table  *ftp_session_tbl;


/**
 * @brief 使用gmime解析信体
 *
 * @param p_session smtp状态信息结构体
 * @param projson   返回eml的属性信息json串
 *
 * @return  成功返回0，失败返回-1
 */
int gmime_parse_mailbody (SmtpSession_t *p_session, char **projson);

/**
 * @brief 发送tika解析出的json 
 *
 * @param key          四元组，用于查找hash桶中的smtp的json串
 * @param max_filesize 最大的文件
 *
 * @return  成功返回0，失败返回-1
 */
int gmime_tika_send(struct tuple4 *key, unsigned long max_filesize);


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
int seen_dot(SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen, smtp_sbuffputdata sbuff_putdata, int sbuffer_id);


/**
 * @brief 处理smtp命令
 *
 * @param p_session  结构体
 * @param packstr    命令
 * @param packlen    包长 
 *
 * @return  成功返回0，失败返回-1
 */
int dispose_smtp_cmd (SmtpSession_t	*p_sessionsrc, char *packstr, unsigned int packlen);
/**
 * @brief  处理STARTTLS加密传输
 *
 * @param p_session  结构体
 * @param packstr    命令
 * @param packlen    包长 
 *
 * @return  成功返回0，失败返回-1
 */
int dispose_STARTTLS_cmd (SmtpSession_t	*p_sessionsrc, char *packstr, unsigned int packlen);


/**
 * @brief 处理smtp响应
 *
 * @param p_session  结构体
 * @param packstr    响应 
 * @param packlen    包长度 
 *
 * @return  成功返回0，失败返回-1
 */
int dispose_smtp_response (SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen);


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
int dispose_smtp_mail_body_data (SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen, smtp_sbuffputdata sbuff_putdata, int sbuffer_id);



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
int dispose_smtp_mail_body_bdat (SmtpSession_t *p_sessionsrc, char *packstr, unsigned int packlen, smtp_sbuffputdata sbuff_putdata, int sbuffer_id);

#endif

