/*
 *  Copyright (C) 2004-2008 Christos Tsantilas
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA.
 */

#include <unistd.h>
#include <zlog.h>
#include <plog.h>
#include "common.h"
#include "service.h"
#include "header.h"
#include "body.h"
#include "simple_api.h"
#include "debug.h"
#include <pcre.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <string.h>
#include <strings.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "webmail_parser.h"
#define   MSG_FILE "/tmp/msg_server"    
#define   PERM S_IRUSR | S_IWUSR   
//#include "test_ring_buffer.h"

#define BUFSIZE  512
#define MAX_TEXT 2048
#define WLTOCAP_KEY (key_t)0x28A5

#define WRITE_BLOCKED -1
#define NOT_BLOCKED 0
#define BLOCKED 1
int boundary_len = -1;
struct msg_st
{
    long int msg_type;
    char text[BUFSIZ];
};
struct file_msg
{
    long int mtype;
    char msg[BUFSIZE];
};

#define MAX_URL_SIZE  8192
#define MAX_METHOD_SIZE  16
#define MAX_CONTENT_TYPE 8190
#define MAX_BOUNDARY_SIZE 8190
#define MAX_IP_SIZE 20
#define SMALL_BUFF 1024

#define PATTERN_LENGTH 256
#define OVECCOUNT 30

#define WYMAIL_MY 1
#define UPLOADTEST 10

#define NOT_MAIL 0
#define IS_MAIL 1
#define MAIL_HAS_ATT 2
#define MAIL_HAS_MIME_ATT 3
#define UPLOAD_TEST 4

typedef struct mail_my{
    char *from;
    char *to;
    char *subject;
    char *content;
}webmail_info;

typedef struct http_info{
    int content_length;
    char *clientip;
    char *content_type;
    char *boundary;
    char method[MAX_METHOD_SIZE];
    char url[MAX_URL_SIZE];
#if 0
    char clientip[MAX_IP_SIZE];
    char content_type[MAX_CONTENT_TYPE];
    char boundary[MAX_BOUNDARY_SIZE];
#endif 
}http_info;

struct echo_req_data;
int echo_init_service(ci_service_xdata_t * srv_xdata,
        struct ci_server_conf *server_conf);
int echo_check_preview_handler(char *preview_data, int preview_data_len,
        ci_request_t *);
int echo_end_of_data_handler(ci_request_t * req);
void *echo_init_request_data(ci_request_t * req);
void echo_close_service();
void echo_release_request_data(void *data);
int echo_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof,ci_request_t * req);
void generate_redirect_page(char *, ci_request_t *, struct echo_req_data *);
int extract_http_info(ci_request_t *, ci_headers_list_t *, struct http_info *);

int wymail_my(webmail_string_and_len *, webmail_info *);
int jdmail_type_my(char *);
int jdmail_my(char *);

CI_DECLARE_MOD_DATA ci_service_module_t service = {
    "echo",                         /* mod_name, The module name */
    "Echo demo service",            /* mod_short_descr,  Module short description */
    ICAP_RESPMOD | ICAP_REQMOD,     /* mod_type, The service type is responce or request modification */
    echo_init_service,              /* mod_init_service. Service initialization */
    NULL,                           /* post_init_service. Service initialization after c-icap 
                                       configured. Not used here */
    echo_close_service,           /* mod_close_service. Called when service shutdowns. */
    echo_init_request_data,         /* mod_init_request_data */
    echo_release_request_data,      /* mod_release_request_data */
    echo_check_preview_handler,     /* mod_check_preview_handler */
    echo_end_of_data_handler,       /* mod_end_of_data_handler */
    echo_io,                        /* mod_service_io */
    NULL,
    NULL
};

/*
   The echo_req_data structure will store the data required to serve an ICAP request.
   */
struct echo_req_data {
    /*the body data*/
    //ci_ring_buf_t *body;
    /*flag for marking the eof*/
    int eof;
    ci_membuf_t *error_page;
    ci_simple_file_t *body;
    ci_request_t *req;
    http_info *httpinf;

    int blocked;
    int is_mail;
    enum{
        no_att,
        normal_att,
        mime_att,
        upload_test,
    }att;

    //int has_att;
    //int rediected;
};


/* This function will be called when the service loaded  */
int echo_init_service(ci_service_xdata_t * srv_xdata, struct ci_server_conf *server_conf)
{
    ci_debug_printf(5, "Initialization of echo module......\n");

    /*Tell to the icap clients that we can support up to 1024 size of preview data*/
    ci_service_set_preview(srv_xdata, 1024);

    /*Tell to the icap clients that we support 204 responses*/
    ci_service_enable_204(srv_xdata);

    /*Tell to the icap clients to send preview data for all files*/
    ci_service_set_transfer_preview(srv_xdata, "*");

    /*Tell to the icap clients that we want the X-Authenticated-User and X-Authenticated-Groups headers
      which contains the username and the groups in which belongs.  */
    ci_service_set_xopts(srv_xdata,  CI_XAUTHENTICATEDUSER|CI_XAUTHENTICATEDGROUPS);

    return CI_OK;
}

/* This function will be called when the service shutdown */
void echo_close_service() 
{
    ci_debug_printf(5,"Service shutdown!\n");
    /*Nothing to do*/
}

/*This function will be executed when a new request for echo service arrives. This function will
  initialize the required structures and data to serve the request.
  */
void *echo_init_request_data(ci_request_t * req)
{

    struct echo_req_data *echo_data;

#if 0
    ci_debug_printf(5, "\033[0;32m  echo_init_request_data ......\n");
    ci_debug_printf(5, "############################################\n");
    ci_debug_printf(5, "\033[0m\n");
#endif
    /*Allocate memory fot the echo_data*/
    echo_data = malloc(sizeof(struct echo_req_data));
    if (!echo_data) {
        ci_debug_printf(1, "Memory allocation failed inside echo_init_request_data!\n");
        return NULL;
    }

    /*If the ICAP request encuspulates a HTTP objects which contains body data 
      and not only headers allocate a ci_cached_file_t object to store the body data.
      */
    //if (ci_req_hasbody(req))
    //     echo_data->body = ci_ring_buf_new(4096);
    //else

    echo_data->body = NULL;
    echo_data->error_page = NULL;
    echo_data->req = req;
    echo_data->blocked = 0;
    echo_data->att = no_att;
    echo_data->is_mail = 0;
    echo_data->httpinf = (http_info *)malloc(sizeof(http_info));
    //echo_data->redirected = 0;
    return echo_data;
}

/*This function will be executed after the request served to release allocated data*/
void echo_release_request_data(void *data)
{

#if 1
    ci_debug_printf(5, "\033[0;31m");
    ci_debug_printf(5, "_______echo_release_request_data____________ \r\n");
    ci_debug_printf(5, "\033[0m\n");
#endif

    /*The data points to the echo_req_data struct we allocated in function echo_init_service */
    struct echo_req_data *echo_data = (struct echo_req_data *)data;

    /*if we had body data, release the related allocated data*/
    //if(echo_data->body)
    //ci_ring_buf_destroy(echo_data->body);

    if (echo_data->body) {
        ci_simple_file_destroy(echo_data->body);
    }
    if (echo_data->error_page){
        ci_membuf_free(echo_data->error_page);
    }
    free(echo_data);
}

int ishttpsource(char *url)
{
    char *p = url;
    int i=0;
    int length = strlen(url);
    while(i < MAX_URL_SIZE){
        if(i>=length)
        {
            return 1;
        }
        if(strncasecmp(p,".gif",4)==0 || strncasecmp(p,".gif?",5)==0 || strncasecmp(p,".css",4)==0 || strncasecmp(p,".css?",5)==0 || strncasecmp(p,".js?",4)==0 || strncasecmp(p,".js",3)==0 || strncasecmp(p,".jpg",4)==0 || strncasecmp(p,".jpg?",5)==0 || strncasecmp(p,".bmp",4)==0 || strncasecmp(p,".bmp?",5)==0 || strncasecmp(p,".png",4)==0 || strncasecmp(p,".png?",5)==0 || strncasecmp(p,".swf",4)==0 || strncasecmp(p,".swf?",5)==0 || strncasecmp(p,".cab",4)==0 || strncasecmp(p,".cab?",5)==0)
        {
            return 0;
        }
        i++;
        p++;
    }
    return i;
}

//static int whattodo = 0;
int echo_check_preview_handler(char *preview_data, int preview_data_len,
        ci_request_t * req)
{
#if 0
    ci_debug_printf(5, "\033[0;34m  check_preview_handler ......\n");
    ci_debug_printf(5, "\033[0m\n");
#endif
    ci_headers_list_t *req_header;
    //struct http_info httpinf;
    ci_off_t content_len = 0;

    content_len = ci_http_content_length(req);
    ci_debug_printf(5, "\033[0;34m###################################################### ......\n");
    ci_debug_printf(9, "We expect to read :%" PRINTF_OFF_T " body data\n",(CAST_OFF_T) content_len);
    ci_debug_printf(5, "\033[0m\n");

    /*Get the echo_req_data we allocated using the  echo_init_service  function*/
    struct echo_req_data *echo_data = ci_service_data(req); //req->service_data

    /* Extract the HTTP header from the request */
    if ((req_header = ci_http_request_headers(req)) == NULL) {
        ci_debug_printf(0, "ERROR echo_check_preview_handler: bad http header, aborting.\n");
        return CI_ERROR;
    }
    if (!extract_http_info(req, req_header, echo_data->httpinf)) {
        /* Something wrong in the header or unknow method */
        ci_debug_printf(1, "DEBUG echo_check_preview_handler: bad http header, aborting.\n");
        return CI_MOD_ALLOW204;
    }
    //ci_debug_printf(2, "DEBUG echo_check_preview_handler: httpinf.url: %s\n", httpinf.url);

    if(!ci_req_hasbody(req))
        return CI_MOD_ALLOW204;

    ci_req_unlock_data(req);

    if (!preview_data_len){
        return CI_MOD_CONTINUE;
    }

    //if http.method is POST then go on
    if ((strncasecmp(echo_data->httpinf->method, "POST", 4) == 0 )) //&& (EXIT_SUCCESS == jdmail_my(httpinf.url))){
    {
        ci_debug_printf(5, "\033[0;34m \n");
        ci_debug_printf(5, "====================POST COMING====echo_data->httpinf->boundary=\n%s================= \r\n",echo_data->httpinf->boundary);
        boundary_len = strlen(echo_data->httpinf->boundary);
        ci_debug_printf(5, "\033[0m\n");
        //extract the httpinfo if att, get the name of upload file

        int res = jdmail_my(echo_data->httpinf->url);
        if(NOT_MAIL == res){
            ci_debug_printf(2, "\033[0;33m ++++++++++++++ not mail allow204 +++++++++++++++ \033[0m \n");
            return CI_MOD_ALLOW204;
        }else if(IS_MAIL == res){
            ci_debug_printf(2, "\033[0;33m ++++++++++++++mail without attach+++++++++++++++ \033[0m \n");
        }else if(MAIL_HAS_ATT == res){
            echo_data->att = normal_att;
            echo_data->blocked = WRITE_BLOCKED;
            ci_debug_printf(2, "\033[0;33m ++++++++++++++mail has attach+++++++++++++++ \033[0m \n");
        }else if(MAIL_HAS_MIME_ATT == res){
            echo_data->att = mime_att;
            echo_data->blocked = WRITE_BLOCKED;
            ci_debug_printf(2, "\033[0;33m ++++++++++++++mail has mime attach+++++++++++++++ \033[0m \n");
        }else if(UPLOAD_TEST == res){
            echo_data->att = upload_test;
            echo_data->blocked = WRITE_BLOCKED;
            ci_debug_printf(2, "\033[0;33m ++++++++++++++ upload test +++++++++++++++ \033[0m \n");
        }else{
            return CI_ERROR;
        }

        echo_data->body = ci_simple_file_new(content_len);
        ci_debug_printf(2,"_____________preview_data_len=%d__________\r\n",preview_data_len);
        if (!echo_data->body){
            ci_debug_printf(2, "________echo_data->body is null");
            return CI_ERROR;
        }
        if (preview_data_len) {
#if 0
            char *temp = NULL;
            temp = (char *)malloc(preview_data_len);
            memcpy(temp, preview_data, preview_data_len);
            FILE *fp;

            if ((fp=fopen("/home/jaygong/test/haha.txt","a+"))==NULL) //打开只写的文本文件
            {
                printf("_______________cannot open file!\n");
                return CI_ERROR;
            }
            fputs(temp,fp); //写入串
            fclose(fp); //关文件
            free(temp);
#endif 
            if (ci_simple_file_write(echo_data->body, preview_data, preview_data_len, ci_req_hasalldata(req)) == CI_ERROR){
                ci_debug_printf(2, "________ci_simple_file_write CI_ERROR");
                return CI_ERROR;
            }
        }
        ci_debug_printf(2,"\033[0;34m ----------CI_MOD_CONTINUE________________\033[0m \n");
        return CI_MOD_CONTINUE;
    }
    ci_debug_printf(5, "_____________CI_MOD_ALLOW204__________________________ \r\n");
    return CI_MOD_ALLOW204;
}
extern int sbuff_putdata(const int iringbuffer_ID, SESSION_BUFFER_NODE ft) ;
extern int http_buffer_id;

int receive_msg_t (struct file_msg *arg)
{
    int msgid;
    //memset(arg, 0, sizeof(struct file_msg));
    memset (arg, 0, strlen(arg) + 1); 
    // 获得消息队列ID
    msgid = msgget (WLTOCAP_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) 
    {   
        return -1; 
    }   
    arg->mtype = 10405;
    int res = msgrcv (msgid, arg, sizeof (struct file_msg), 0, 0); 
    // int res = msgrcv(msgid, arg, strlen(arg)+1, 0,0);
    if (-1 == res)
    {   
        return -1; 
    }   
    ci_debug_printf (2, 
            "####################### receive_msg content = %s ########### \n",
            arg->msg);
    return 1;
}
int receive_msg(struct file_msg  *arg) {
    int msgid;
    memset(arg, 0, sizeof(struct file_msg));
    //   memset(arg, 0, strlen(arg)+1);
    // 获得消息队列ID
    msgid=msgget(WLTOCAP_KEY,0666| IPC_CREAT);
    if (msgid == -1) {
        return -1; 
    }
    arg->mtype = 10405;
    while(1)
    {
        int res = msgrcv(msgid, arg, sizeof(struct file_msg), 0,0);
        ci_debug_printf(5, "===================== res receive msg = %d ======================= ",res);
        //int res = msgrcv(msgid, arg, strlen(arg)+1, 0,0);
        if (-1 == res){
            //   return -1; 
            continue;
        }
        else if(res==0)
            break;
        sleep(1);
    }
    return 1;
}
int recv_msg_from_policy(struct file_msg *arg)
{
    int msgid;
    key_t key;
    if((key = ftok(MSG_FILE, 'a')) == -1){
        perror("ftok");
        return -1;
    }   

    printf("Key:%d\n", key);
    if((msgid = msgget(key, PERM | IPC_CREAT)) == -1) 
    {   
        perror("msgget");
        return -2;
    }   
    int res = msgrcv (msgid, arg, sizeof (struct file_msg), arg->mtype, 0); 
    if(res<0)
        return -3;
    return 0;
}


/* This function will called if we returned CI_MOD_CONTINUE in  echo_check_preview_handler
   function, after we read all the data from the ICAP client*/
//static int kTest;
//extern int kTest2;
//extern file_content_t *pfc;

int echo_end_of_data_handler(ci_request_t * req)
{
    ci_debug_printf(5, "\033[0;31m");
    ci_debug_printf(5, "===================== END OF DATA HANDLER START ======================= ");
    ci_debug_printf(5, "\033[0m \n");

    struct echo_req_data *echo_data = ci_service_data(req);
    /*mark the eof*/

    echo_data->eof = 1;
    ci_off_t content_len = 0;
    content_len = ci_http_content_length(req);
    ci_debug_printf(5, "\033[0;36m");
    ci_debug_printf(5, "================end of data handler : content_len is : %ld ", content_len);
    ci_debug_printf(5, "\033[0m\n");
    dlp_http_post_head stHttpPost;

    if(echo_data->att == no_att){
        echo_data->blocked = NOT_BLOCKED;
        return CI_MOD_DONE;
        /* ci_debug_printf(9, "there is no body, confusing ....\n"); */
        /* return CI_MOD_DONE; */
    }

    if(echo_data->att == mime_att || echo_data->att == upload_test || echo_data->att == normal_att){
        ci_debug_printf(9, "---------------------------new echo test------------------------------\n");
        memset(&stHttpPost,0x0,sizeof(dlp_http_post_head));
        stHttpPost.content_length = echo_data->httpinf->content_length;
        memcpy(stHttpPost.new_name, echo_data->body->filename, strlen(echo_data->body->filename) + 1);
        memcpy(stHttpPost.content_type, echo_data->httpinf->content_type, strlen(echo_data->httpinf->content_type) + 1);
        memcpy(stHttpPost.boundary, echo_data->httpinf->boundary, strlen(echo_data->httpinf->boundary) + 1);
        /* int thread_id =  */
        stHttpPost.current_thread_id = pthread_self();//这里为何？
        ci_debug_printf(2, "================  stHttpPost.boundary = \n%s\n",stHttpPost.boundary);
        ci_debug_printf(2, "================  stHttpPost.thread_id = %ld\n",stHttpPost.current_thread_id);
        SESSION_BUFFER_NODE ft; 
        memset(&ft,0x0,sizeof(SESSION_BUFFER_NODE));
        ft.session_five_attr.ip_dst = 12345;//dlp_http->key.daddr;
        ft.session_five_attr.ip_src = inet_addr(echo_data->httpinf->clientip);// dlp_http->key.saddr;
        ft.session_five_attr.port_src = 8080;//dlp_http->key.source;
        ft.session_five_attr.port_dst = 12345;//dlp_http->key.dest;
        ft.attr = &stHttpPost;
        ft.session_five_attr.protocol = 0;//webmail->pro_id;
        sbuff_putdata(0, ft);
        sleep(3);

        ci_debug_printf(5, "\033[0;36m========================= FINISH ===========================\n\033[0m");
    }
    
    /* echo_data->blocked = NOT_BLOCKED; */
    /* ci_debug_printf(9, "i am here\n"); */
    struct file_msg msg;
    memset(&msg,0x0,sizeof(struct file_msg));
    msg.mtype = stHttpPost.current_thread_id;
    ci_debug_printf(9, "msg.mtype = %ld\n",msg.mtype);
    int ret = -1;
    while(1)
    {
        //	 ret = receive_msg_t(&msg);
        ret = recv_msg_from_policy(&msg);
        ci_debug_printf(9, "ret = %d end of handler recv message msg.type = %ld, msg.msg = %s\n", ret, msg.mtype, msg.msg);
        if(ret==0)
            break;
        if(ret < 0)
            continue;
        if(ret == -3)
            break;
        usleep(100000);
    }
    //blocked the post
    if((strncasecmp(msg.msg,"n",1)==0)){
        ci_debug_printf(9,"\033[5;35m++++++++++ BLOCKED ++++++++++\033[0m\n");

#if 1
        char buf[256];
        ci_headers_list_t *req_header;
        if ((req_header = ci_http_request_headers(req)) == NULL) {
            ci_debug_printf(0, "ERROR echo_check_preview_handler: bad http header, aborting.\n");
            return CI_ERROR;
        }
        const char *host = ci_headers_value(req_header, "Host");
        ci_http_request_reset_headers(req);
        snprintf(buf, strlen("GET / HTTP/1.1"), "GET / HTTP/1.1");
        ci_http_request_add_header(req, buf);
        bzero(buf, strlen(buf));
        snprintf(buf, strlen(host), "Host: %s", host);
        ci_http_request_add_header(req, buf);
        ci_http_response_add_header(req, "Connection: keepalive");
        ci_http_response_add_header(req, "Content-Type: text/html");
        ci_http_response_add_header(req, "Content-Language: en");
        req->hasbody = 0;
        //ci_request_pack(req);
        
        ci_debug_printf(9, "send redirect FINISH!\n");
#endif
        echo_data->blocked = BLOCKED;

        /*
         * snprintf(buf, sizeof(buf), "Content-Length: %" PRINTF_OFF_T, (CAST_OFF_T)-1);
         * ci_http_request_remove_header(req, "Content-Length");
         * ci_http_request_add_header(req, buf);
         * bzero(buf, sizeof(buf));
         * snprintf(buf, sizeof(buf), "Connection: close");
         * if(req->keepalive)
         *  ci_debug_printf(9, "keepalive\n");
         * ci_http_request_remove_header(req, "Connection");
         * ci_http_request_add_header(req, buf);
         * req->keepalive = 0;
         */
        /* ci_simple_file_truncate(echo_data->body, content_len); */
#if 0

        char *temp = (char *)malloc(content_len);
        bzero(temp, content_len);
        /* memcpy(temp, ci_simple_file_to_const_string(echo_data->body), content_len); */

        FILE *fp;
        fp = fopen("/root/temp.txt", "r");
        if(fp == NULL){
            perror("fopen");
        }
        /* fgets(temp, content_len, fp); */
        fread(temp, content_len, 1, fp);
        ci_debug_printf(9, "temp is %s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n", temp);

        fclose(fp);
        /* bzero(temp ,content_len); */

        ci_simple_file_destroy(echo_data->body);
        echo_data->body = ci_simple_file_new(content_len);
        ret = ci_simple_file_write(echo_data->body, temp, content_len, 0);
        ci_debug_printf(9, "wirte len is %d\n", ret);
        free(temp);
#endif
    }else{
        ci_debug_printf(9,"\033[5;35m++++++++++ NOT BLOCKED ++++++++++\033[0m\n");
        echo_data->blocked = NOT_BLOCKED;
    }
    return CI_MOD_DONE;
}

int echo_read_from_net(char *buf, int len, int iseof, ci_request_t * req)
{
    struct echo_req_data *data = ci_service_data(req);
    //int allow_transfer;

    if (!data){
        return CI_ERROR;
    }
    if (!data->body){
        return len;
    }
    return ci_simple_file_write(data->body, buf, len, iseof);
}

int echo_write_to_net(char *buf, int len, ci_request_t * req)
{
    int bytes;
    struct echo_req_data *data = ci_service_data(req);

    if (!data)
        return CI_ERROR;

    if(data->blocked == WRITE_BLOCKED)
        return 0;

    ci_req_unlock_data(req);
    ci_simple_file_unlock_all(data->body);

    if (data->body){
        ci_debug_printf(9, "\033[0;34m ____________________________Body data size=%" PRINTF_OFF_T " \033[0m \n ",
                (CAST_OFF_T) data->body->bytes_in);

        bytes = ci_simple_file_read(data->body, buf, len);
        //char *temp = NULL;
        //temp = (char *)malloc(*rlen);
        // memcpy(temp,rbuf,*rlen);
#if 0
        FILE *fp;
        if ((fp=fopen("/home/jaygong/test/test.txt","a"))==NULL) //打开只写的文本文件
        {
            printf("cannot open file!");
            exit(0);
        }
        int i=0;
        for(i=0;i<len; i++){
            if((buf[i] >= 0x31 && buf[i] <0x40)
                    || (buf[i] >= 0x41 && buf[i] <0x5a)
                    || (buf[i] >= 0x61 && buf[i] <0x7a)){
                fputc(*(buf+i),fp); //写入串
            }
            else{
                fputc('.',fp);
            }
        }
        fclose(fp); //关文件
#endif
    }
    else{
        bytes =0;
    }
    return bytes;
}

/* This function will called if we returned CI_MOD_CONTINUE in  echo_check_preview_handler
   function, when new data arrived from the ICAP client and when the ICAP client is 
   ready to get data.
   */
int echo_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof, ci_request_t * req)
{
    struct echo_req_data *data = ci_service_data(req);
    ci_debug_printf(9, "<=============================ECHO_IO START ===================================> \r\n");

#if 1
    if(data->blocked == BLOCKED)
    {
        ci_debug_printf(9, "*wlen is EOF================>\n");
        *wlen = CI_EOF;
        return CI_OK;
    }
#endif
#if 0
    char *temp = NULL;
    temp = (char *)malloc(*rlen);
    memcpy(temp,rbuf,*rlen);
    FILE *fp;
    if ((fp=fopen("/home/jaygong/test/haha.txt","a"))==NULL) //打开只写的文本文件
    {
        printf("cannot open file!");
        return CI_ERROR;
    }
    fputs(temp,fp); //写入串
    fclose(fp); //关文件
    free(temp);
#endif
    if(rlen){
        ci_debug_printf(9, "1>>>>>>>>>>>>>>>>>>>>> rlen =%d .. \r\n",*rlen);
    }else{
        ci_debug_printf(9, "1>>>>>>>>>>>>>>>>>>>>> rlen is null .. \r\n");
    }
    //int ret = CI_OK;

    if (rbuf && rlen) {
        *rlen = echo_read_from_net(rbuf, *rlen, iseof, req);
        ci_debug_printf(9, "2>>>>>>>>>>>>>>>>>>>>> rlen =%d .. \r\n",*rlen);
        if (*rlen == CI_ERROR)
            return CI_ERROR;
    } else if (iseof) {
        ci_debug_printf(9, "3!!!!!>>>>>>>>>>>>>>>>>>>>> iseof =%d .. \r\n",iseof);
        if (echo_read_from_net(NULL,0, iseof, req) == CI_ERROR)
            return CI_ERROR;
    }
    ci_debug_printf(9, "4>>>>>>>>>>>>>>>>>>>>>>>>> wlen =%d .. \r\n",*wlen);
    if (wbuf && wlen) {
        *wlen = echo_write_to_net(wbuf, *wlen, req);
        ci_debug_printf(9, "5>>>>>>>>>>>>>>>>>>>>> wlen =%d .. \r\n",*wlen);
    }
    if(*wlen==0 && data->eof==1){
        *wlen = CI_EOF;
    }
    return CI_OK;
}

static const char *blocked_header_message =
"<html>\n"
"<body>\n"
"<p>\n"
"You will be redirected in few seconds, if not use this <a href=\"";

static const char *blocked_footer_message =
"\">direct link</a>.\n"
"</p>\n"
"</body>\n"
"</html>\n";

void generate_redirect_page(char * redirect, ci_request_t * req, struct echo_req_data * data)
{
    int new_size = 0;
    char buf[MAX_URL_SIZE];
    ci_membuf_t *error_page;

    new_size = strlen(blocked_header_message) + strlen(redirect) + strlen(blocked_footer_message) + 10;

    if ( ci_http_response_headers(req)){
        printf("======ci_http_response_headers(req) != NULL =========\n");
        ci_http_response_reset_headers(req);
    }
    else{
        printf("======ci_http_response_headers(req) == NULL =========\n");
        ci_http_response_create(req, 1, 1);
    }

    ci_debug_printf(2, "DEBUG generate_redirect_page: creating redirection page\n");

    snprintf(buf, MAX_URL_SIZE, "Location: %s", redirect);

    ci_debug_printf(3, "DEBUG generate_redirect_page: %s\n", buf);

    ci_http_response_add_header(req, "HTTP/1.0 301 Moved Permanently");
    ci_http_response_add_header(req, buf);
    ci_http_response_add_header(req, "Server: C-ICAP");
    ci_http_response_add_header(req, "Connection: close");
    /*ci_http_response_add_header(req, "Content-Type: text/html;");*/
    ci_http_response_add_header(req, "Content-Type: text/html");
    ci_http_response_add_header(req, "Content-Language: en");
    //ci_http_response_add_header(req, "Host: www.g.cn");
#if 0
    if (data->blocked == 1) {
        error_page = ci_membuf_new_sized(new_size);
        ((struct echo_req_data *) data)->error_page = error_page;
        ci_membuf_write(error_page, (char *) blocked_header_message, strlen(blocked_header_message), 0);
        ci_membuf_write(error_page, (char *) redirect, strlen(redirect), 0);
        ci_membuf_write(error_page, (char *) blocked_footer_message, strlen(blocked_footer_message), 1);
    }
#endif
    ci_debug_printf(3, "DEBUG generate_redirect_page: done\n");
}


#if 0
int extract_http_info(ci_request_t * req, ci_headers_list_t * req_header,
        struct http_info *httpinf)
{
    char *temp=NULL;
    if(httpinf == NULL){
        ci_debug_printf(2, "emtpy httpinf");
        return 0;
    }
    struct echo_req_data *echo_data = ci_service_data(req); //req->service_data
    char *str;
    int i = 0;
    /* Format of the HTTP header we want to parse:
       GET http://www.squid-cache.org/Doc/config/icap_service HTTP/1.1
       */
    httpinf->url[0]='\0';
    httpinf->method[0] = '\0';
    httpinf->content_type = (char *)malloc(MAX_CONTENT_TYPE);
    httpinf->boundary = (char *)malloc(MAX_BOUNDARY_SIZE);
    httpinf->clientip = (char *)malloc(MAX_IP_SIZE);
    str = req_header->headers[0];
    ci_debug_printf(3, "DEBUG extract_http_info: come in \n");
    memset(httpinf,'\0',sizeof(struct http_info));

    /* if we can't find a space character, there's somethings wrong */
    if (strchr(str, ' ') == NULL) {
        return 0;
    }

    /* extract the HTTP method */
    while (*str != ' ' && i < MAX_METHOD_SIZE) {
        httpinf->method[i] = *str;
        str++;
        i++;
    }
    httpinf->method[i] = '\0';
    ci_debug_printf(3, "DEBUG extract_http_info: method %s\n", httpinf->method);

    /* Extract the URL part of the header */
    while (*str == ' ') str++;
    i = 0;
    while (*str != ' ' && i < MAX_URL_SIZE) {
        httpinf->url[i] = *str;
        i++;
        str++;
    }
    httpinf->url[i] = '\0';
    ci_debug_printf(3, "DEBUG extract_http_info: url %s\n", httpinf->url);
    if (*str != ' ') {
        return 0;
    }
    /* we must find the HTTP version after all */
    while (*str == ' ')
        str++;
    if (*str != 'H' || *(str + 4) != '/') {
        return 0;
    }
    if ((httpinf->clientip = ci_headers_value(req->request_header, "X-Client-IP")) != NULL) {
        ci_debug_printf(2, "DEBUG echo_check_preview_handler: X-Client-IP: %s\n", httpinf->clientip);
    }
    if((/*httpinf->content_type*/temp = ci_headers_value(req_header, "Content-type")) != NULL){
        memcpy(httpinf->content_type, temp, MAX_CONTENT_TYPE);
        ci_debug_printf(2, "DEBUG echo_check_preview_handler: \r\nContent-Type : %s\n", httpinf->content_type);
        //  httpinf->boundary = strstr(httpinf->content_type, "boundary");
        temp = NULL;
        temp = strstr(httpinf->content_type, "boundary");
        memcpy(httpinf->boundary, temp, MAX_BOUNDARY_SIZE);

        if(httpinf->boundary != NULL){
            httpinf->boundary += strlen("boundary");
            if(*httpinf->boundary == '=')
                httpinf->boundary++;
            ci_debug_printf(2, "boundary is :%s \n", httpinf->boundary);
        }else{
            ci_debug_printf(2, "content type without boundary\n");
        }
        temp=NULL;//
    }
    httpinf->content_length = ci_http_content_length(req);
    ci_debug_printf(2, "DEBUG extract_http_info: Content-length : %d\n", httpinf->content_length);
    return 1;
}
#endif
int extract_http_info(ci_request_t * req, ci_headers_list_t * req_header,
        struct http_info *httpinf)
{
    if(httpinf == NULL){
        ci_debug_printf(2, "emtpy httpinf");
        return 0;
    }
    //struct echo_req_data *echo_data = ci_service_data(req); //req->service_data
    char *str;
    int i = 0;
    /* Format of the HTTP header we want to parse:
       GET http://www.squid-cache.org/Doc/config/icap_service HTTP/1.1
       */
    httpinf->url[0]='\0';
    httpinf->method[0] = '\0';
    httpinf->content_type = (char *)malloc(MAX_CONTENT_TYPE);
    memset(httpinf->content_type, 0, MAX_CONTENT_TYPE);
    httpinf->boundary = (char *)malloc(MAX_BOUNDARY_SIZE);
    memset(httpinf->boundary, 0, MAX_BOUNDARY_SIZE);
    httpinf->clientip = (char *)malloc(MAX_IP_SIZE);
    memset(httpinf->clientip, 0, MAX_IP_SIZE);
    str = req_header->headers[0];
    /* ci_debug_printf(3, "DEBUG extract_http_info: come in \n"); */

    /* if we can't find a space character, there's somethings wrong */
    if (strchr(str, ' ') == NULL) {
        return 0;
    }

    /* extract the HTTP method */
    while (*str != ' ' && i < MAX_METHOD_SIZE) {
        httpinf->method[i] = *str;
        str++;
        i++;
    }
    httpinf->method[i] = '\0';
    ci_debug_printf(3, "DEBUG extract_http_info: method %s\n", httpinf->method);

    /* Extract the URL part of the header */
    while (*str == ' ') str++;
    i = 0;
    while (*str != ' ' && i < MAX_URL_SIZE) {
        httpinf->url[i] = *str;
        i++;
        str++;
    }
    httpinf->url[i] = '\0';
    ci_debug_printf(3, "DEBUG extract_http_info: url %s\n", httpinf->url);
    if (*str != ' ') {
        return 0;
    }
    /* we must find the HTTP version after all */
    while (*str == ' ')
        str++;
    if (*str != 'H' || *(str + 4) != '/') {
        return 0;
    }
    const char *temp = NULL;
    if ((temp = ci_headers_value(req->request_header, "X-Client-IP")) != NULL) {
        memcpy(httpinf->clientip, temp, MAX_IP_SIZE);
        ci_debug_printf(2, "DEBUG echo_check_preview_handler: X-Client-IP: %s\n", httpinf->clientip);
        temp = NULL;
    }

    if((temp = ci_headers_value(req_header, "Content-type")) != NULL){
        memcpy(httpinf->content_type, temp, MAX_CONTENT_TYPE);
        ci_debug_printf(2, "DEBUG : \r\nContent-Type : %s\n", httpinf->content_type);
        temp = NULL;

        if((temp = strstr(httpinf->content_type, "boundary")) != NULL){
            memcpy(httpinf->boundary, temp, MAX_BOUNDARY_SIZE);
            if(httpinf->boundary != NULL){
                httpinf->boundary += strlen("boundary");
                if(*httpinf->boundary == '=')
                    httpinf->boundary++;
                //strcat(httpinf->boundary, '\0');
                ci_debug_printf(2, "boundary is :%s \n", httpinf->boundary);
            }else{
                ci_debug_printf(2, "content type without boundary\n");
            }
            temp = NULL;
        }
    }

    httpinf->content_length = ci_http_content_length(req);
    ci_debug_printf(2, "DEBUG extract_http_info: Content-length : %d\n", httpinf->content_length);
    return 1;
}
#if 0
char *http_content_type(ci_request_t * req)
{
    ci_headers_list_t *heads;
    char *val;
    if (!(heads =  ci_http_response_headers(req))) {
        /* Then maybe is a reqmod request, try to get request headers */
        if (!(heads = ci_http_request_headers(req)))
            return NULL;
    }
    if (!(val = ci_headers_value(heads, "Content-Type")))
        return NULL;

    return val;
}
#endif

int jdmail_type_my(char *host_ori_ref)
{
    ci_debug_printf(9, "jdmail_type_my start ----->\n");
    char *wypat1 = "163.com";
    char *wypat2 = "126.com";
    char *wypat3 = "yeah.net";
    char *uploadtest = "8080";

    if(strstr(host_ori_ref, wypat1) || strstr(host_ori_ref, wypat2) || strstr(host_ori_ref, wypat3)){
        return WYMAIL_MY;
    }else if(strstr(host_ori_ref, uploadtest)){
        return UPLOADTEST;
    }else
        return 0;
}

int jdmail_my(char *host_ori_ref)
{
    /* ci_debug_printf(9, "in jdmail_my \n"); */
    if(strstr(host_ori_ref, "mail") || strstr(host_ori_ref, "8080")){
        switch(jdmail_type_my(host_ori_ref)){
            case WYMAIL_MY:
                {
                    char *wy_mail_pattern1 = "&func=mbox:compose";
                    char *wy_mail_pattern2 = "action=deliver";
                    char *wy_mail_pattern3 = "compose/compose.do?action=DELIVER";
                    char *wy_att_pattern1 = "/upxmail/upload";
                    char *wy_att_pattern2 = "/compose/upload";
                    if((strstr(host_ori_ref, wy_mail_pattern1) && strstr(host_ori_ref, wy_mail_pattern2))
                            || strstr(host_ori_ref, wy_mail_pattern3)){
                        return IS_MAIL;
                    }else if(strstr(host_ori_ref, wy_att_pattern1)){
                        return MAIL_HAS_ATT;
                    }else if(strstr(host_ori_ref, wy_att_pattern2)){
                        return MAIL_HAS_MIME_ATT;
                    }else{
                        return NOT_MAIL;
                    }
                }
            case UPLOADTEST:
                {
                    char *upload_pattern = "/upload";
                    ci_debug_printf(9, "i am here\n");
                    if(strstr(host_ori_ref, upload_pattern))
                        return UPLOAD_TEST;
                    else
                        return NOT_MAIL;
                }
            default:
                return NOT_MAIL;
        }
    }
    return NOT_MAIL;
}


#if 0
int wymail_my(webmail_string_and_len *StoMB, webmail_info *webmail)
{
    ci_debug_printf(9, "in wymail_my parser\n");
    if(StoMB == NULL || webmail == NULL)
        return -1;
    webmail_string_and_len *obj = init_webmail_string_and_len();
    webmail_string_and_len *sto = init_webmail_string_and_len();
    webmail_string_and_len *sto1 = init_webmail_string_and_len();

    if(NULL == obj || NULL == sto || NULL == sto1){
        ci_debug_printf(9, "init_webmail_string_and_len objects error ...\n");
        return -1;
    }
    unsigned int ln = StoMB->len;
    if(NULL == (obj->string = (char *)malloc(ln))){
        ci_debug_printf(5, "error malloc obj->string\n");
        return -1;
    }
    web_URLDecode(StoMB->string, ln, obj->string, ln);
    obj->len = StoMB->len;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern2_1[PATTERN_LENGTH] = "(?<=account\">)[\\w\\W]*?(?=</string>)";/*此正则表达式的意义时提取出以account、n>开头，以</string>结尾的中间部分的字符串*/
    const char pattern2_2[PATTERN_LENGTH] = "(?<=account\":\")[\\w\\W]*?(?=\",)";/*此正则表达式的意义时提取出以account、n>开头，以</string>结尾的中间部分的字符串*/
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->from==NULL && (mch(&obj,&sto1, pattern2_1)==EXIT_SUCCESS || mch(&obj,&sto1, pattern2_2)==EXIT_SUCCESS ) ){
        /*首先判断webmail中没有东西，并且能够提取到账户，进行下一步，否则跳出 ， 经过mch（）函数后，已经将账户提取到了sto结构体中*/
        if(sto1->len){/*判断sto里面的东西长度大于0，表明确实提取到了*/
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                //if(c_malloc((void**)&(webmail->from),sto->len+1) == -1) goto wymail_my_fail;//lj
                if((webmail->from = (char *)malloc(sto->len + 1)) == NULL){
                    ci_debug_printf(5, "malloc webmail->from error...\n");
                    return -1;
                }
                memcpy(webmail->from,sto->string,sto->len);
                //strcat(webmail->from, "/0");
                free(sto->string);
                free(sto1->string);
            }
        }
    }

    ci_debug_printf(9, "finish from parser\n");
#if 0

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern3_1[PATTERN_LENGTH] = "(?<=\"to\"><string>)[\\w\\W]*?(?=</array>)";
    const char pattern3_2[PATTERN_LENGTH] = "(?<=\"to\":\")[\\w\\W]*?(?=\",)";
    if(mch(&obj,&sto, pattern3_1)==EXIT_SUCCESS || mch(&obj,&sto, pattern3_2)==EXIT_SUCCESS){
        if(sto->len){
            webmail_to_parser(sto , webmail->to);
        }
        if (sto)
        {
            free(sto->string);
        }
    }

    //同上
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern4_1[PATTERN_LENGTH] = "(?<=\"cc\"><string>)[\\w\\W]*?(?=<array)";
    const char pattern4_2[PATTERN_LENGTH] = "(?<=\"cc\":\")[\\w\\W]*?(?=\",)";
    if(mch(&obj,&sto, pattern4_1)==EXIT_SUCCESS || mch(&obj,&sto, pattern4_2)==EXIT_SUCCESS){
        if(sto->len){
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    //同上
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern5_1[PATTERN_LENGTH] = "(?<=\"bcc\"><string>)[\\w\\W]*?(?=<string name=\"subject\">)";
    const char pattern5_2[PATTERN_LENGTH] = "(?<=\"bcc\":\")[\\w\\W]*?(?=\",)";
    if(mch(&obj,&sto, pattern5_1)==EXIT_SUCCESS || mch(&obj,&sto, pattern5_2)==EXIT_SUCCESS){
        if(sto->len){
            webmail_to_parser(sto , webmail->to);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取发送时间*/
    const char pattern8[PATTERN_LENGTH] = "(?<=\"id\">c:)[\\w\\W]*?(?=</string>)";
    if(mch(&obj,&sto, pattern8)==EXIT_SUCCESS){
        if(sto->len){
            char s[MAX_TIME_LEN];
            memset(s,'\0',sizeof(s));
            /*将时间转换为规定的格式*/
            time_convert(sto->string,s);
            memcpy(webmail->Time,s,strlen(s));
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
#endif

    /*提取主题，并对应存储*/
    const char pattern6_1[PATTERN_LENGTH] = "(?<=subject\">)[\\w\\W]*?(?=</string>)";
    const char pattern6_2[PATTERN_LENGTH] = "(?<=subject\":\")[\\w\\W]*?(?=\",)";
    if(webmail->subject==NULL&& (mch(&obj,&sto, pattern6_1)==EXIT_SUCCESS || mch(&obj,&sto, pattern6_2)==EXIT_SUCCESS)){
        if(sto->len){
            if(c_malloc((void**)&webmail->subject,sto->len+1) == -1 ) goto wymail_my_fail;//lj
            memcpy(webmail->subject , sto->string,sto->len);
        }
        if (sto)
        {
            free(sto->string);
        }
    }
    ci_debug_printf(9, "finish subject parser\n");

    /*提取内容，并对应存储*/
    const char pattern7_1[PATTERN_LENGTH] = "(?<=content\">)[\\w\\W]*?(?=</string>)";
    const char pattern7_2[PATTERN_LENGTH] = "(?<=content\":\")[\\w\\W]*?(?=\",)";
    if(webmail->content==NULL&& (mch(&obj,&sto, pattern7_1)==EXIT_SUCCESS  || mch(&obj,&sto, pattern7_2)==EXIT_SUCCESS)){
        if(sto->len){
            if(c_malloc((void**)&webmail->content,sto->len+1) == -1 ) goto wymail_my_fail;//lj
            memcpy(webmail->content,sto->string,sto->len);
        }
        if (sto)
        {
            free(sto->string);
        }
    }

    ci_debug_printf(9, "finish content parser\n");

    if(sto){
        free(sto);
        sto=NULL;
    }
    if(sto1){
        free(sto1);
        sto1=NULL;
    }
    if(obj){
        free(obj->string);
        free(obj);
        obj=NULL;
    }
    //log_printf(ZLOG_LEVEL_DEBUG, "wymail_my end ....");
    ci_debug_printf(9, "wymail_my end ...\n");
    return 0;

wymail_my_fail:
    ci_debug_printf(5, "wymail_my_fail bad .... \n");

    if(sto){
        free(sto->string);
        free(sto);
        sto = NULL;
    }
    if(sto1){
        free(sto1->string);
        free(sto1);
        sto1=NULL;
    }
    if(obj){
        free(obj->string);
        free(obj);
        obj = NULL;
    }
    //log_printf(ZLOG_LEVEL_ERROR, "wymail_my fail end ....");
    return -1;
}
#endif
