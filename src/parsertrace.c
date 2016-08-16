/* Based on src/http/ngx_http_parse.c from NGINX copyright Igor Sysoev
 *
 * Additional changes are licensed under the same terms as NGINX and
 * copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* Dump what the parser finds to stdout as it happen */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <pcre.h>
#include "beap_hash.h"
#include "http_parser.h"
#include "plog.h"

#if 0
#include "parsertrace.h"
#endif
extern char *  session_stream;
//typedef int (*http_sbuffputdata) (int iringbuffer_ID, SESSION_BUFFER_NODE ft);
//////////////////////////////////////////////////////////////////////////////////
extern  hash_table *ftp_session_tbl;
#define NON_NUM '0'

char *s_head_field[HEAD_FIELD_INDEX_MAX] =
{
	"Range",//许建东新增
 	"UploadtaskID",//许建东新增
 	"Host",
 	"Content-Type",
 	"Connection",
 	"Mail-Upload-name",
	"Mail-Upload-size",
	"Content-Length",
	"Accept",
	"Accept-Encoding",
	"Accept-Language",
	"Cookie",
	"Referer",
	"Origin",
	"X-QQMAIL-FILENAME",
	"contentSize",
	"User-Agent",
	"Mail-Upload-offset"
};

char Char2Num(char ch)
{   
	if(ch>='0' && ch<='9')return (char)(ch-'0');   
	if(ch>='a' && ch<='f')return (char)(ch-'a'+10);   
	if(ch>='A' && ch<='F')return (char)(ch-'A'+10);   
	return NON_NUM;
}



int http_mch(testhttp_string_and_len ** var, testhttp_string_and_len ** Sto,const char *pattern)
{
	pcre *re = NULL;
	const char *error = NULL;
	int erroffset = 0;
	int ovector[30];
	int rc = 0;

	re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	if (re == NULL) {
		log_printf(ZLOG_LEVEL_ERROR, "ptrace compilation failed at offset %d: %s\n", erroffset, error);
		return EXIT_FAILURE;
	}

	rc = pcre_exec(re, NULL, (*var)->string, (*var)->len, 0, 0, ovector, 30);
	if (rc < 0) {
		//if (rc == PCRE_ERROR_NOMATCH)
		//{
		   //log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace no match ...\n");
		//}
		//else
		//{
		   //log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace Matching error %d\n", rc);
		//}
		pcre_free(re);
		re = NULL;
		return EXIT_FAILURE;
	}
	char *substring_start = (*var)->string + ovector[0];
	int substring_length = ovector[1] - ovector[0];
	if (Sto != NULL) {
		(*Sto)->len = substring_length;
		(*Sto)->string = (char *)malloc(substring_length + 1);
		if ((*Sto)->string == NULL) {
		   log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in mch(),cant malloc something!");
			return EXIT_FAILURE;
		}
		memset((*Sto)->string, '\0', substring_length + 1);
		memcpy((*Sto)->string, substring_start, substring_length);
	}

	pcre_free(re);
	re = NULL;
	return EXIT_SUCCESS;
}


int URLDecode(const char* str, const int strSize, char* result, const int resultSize) {   
	char ch, ch1, ch2;   
	int i;   
	int j = 0; /* for result index */   
	  
	if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0)) {   
		return 0;   
	}   
	  
	for (i=0; (i<strSize) && (j<resultSize); i++) {   
		ch = str[i];   
		switch (ch) {   
		case '+':   
			result[j++] = ' ';
			break;   
		  
		case '%':   
			if (i+2 < strSize) {
				ch1 = Char2Num(str[i+1]);
				ch2 = Char2Num(str[i+2]);
				if ((ch1 != NON_NUM) && (ch2 != NON_NUM)) {
					result[j++] = (char)((ch1<<4) | ch2);
					i += 2;
					break;
				}
			}
			break;
		/* goto default */
		default:   
			result[j++] = ch;
			break;
		}
	}
	  
	result[j] = '\0';
	return j;
}

//////////////////////////////////////////////////////////////////////////////////
int on_message_begin(http_parser* _) {
  (void)_;
  return 0;
}

int on_headers_complete(http_parser* _) {
  (void)_;
  return 0;
}

int on_message_complete(http_parser* _) {
  (void)_;
  return 0;
}

int on_url(http_parser*parser, const char* at, size_t length) {
    dlp_http_post_head *http_post_url= NULL;
    http_post_url=(dlp_http_post_head *)parser->http;
	if (NULL == at || length <= 0)
	{
		return -1;
	}

	if (length >= HTTP_FILE_URL_MAX)
	{
		length = HTTP_FILE_URL_MAX - 1;
	}
	strncpy(http_post_url->url, at ,length);
 
    return 0;
}

int on_header_field(http_parser* parser, const char* at, size_t length) {
    int head_index=0;
	if ((NULL == at)||(NULL == parser))
	{
		return -1;
	}
	for (head_index; head_index < HEAD_FIELD_INDEX_MAX; head_index++)
	{
	  if (!memcmp(at, s_head_field[head_index], length))
	  {
		parser->head_index = head_index;
		break;
 	  }
	}
	if (head_index >= HEAD_FIELD_INDEX_MAX)
	{
		parser->head_index = HEAD_FIELD_INDEX_MAX;
	}
    return 0;
}

testhttp_string_and_len *init_testhttp_string_and_len()
{
	testhttp_string_and_len *var = (testhttp_string_and_len *) malloc(sizeof(testhttp_string_and_len));
	if(var==NULL)
	{
	   log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in init_testhttp_string_and_len(),cant malloc something!");
	   return NULL;	
	}
	memset (var , 0, sizeof(testhttp_string_and_len));
	var->len = 0;
	var->string = NULL;
	return var;
}

int on_header_value(http_parser* parser, const char* at, size_t length) {
    const char http_boundary_pattern[100] = "(?<=boundary=)\\V*";
    if((at == NULL)||(parser == NULL))
	{
	   log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value nothing to do!");
	   return 0;
	}
    char  len[32]={};
    dlp_http_post_head *http_post_head = NULL;
    http_post_head =(dlp_http_post_head *)parser->http;
    testhttp_string_and_len *mch_tmp1 = NULL;
    testhttp_string_and_len *mch_tmp2 = NULL;

    switch (parser->head_index)
	{
	   //许建东新增*********************************************************
	   case HEAD_FIELD_RANGE:
			if (length >= HTTP_FILE_RANGE)
			{
				length = HTTP_FILE_RANGE - 1;
			}
			strncpy(http_post_head->range, at, length);
			break;

		case HEAD_FIELD_UPLOAD_TASK_ID:
			if (length >= HTTP_FILE_UP_LOAD_TASK_ID)
			{
				length = HTTP_FILE_UP_LOAD_TASK_ID - 1;
			}
			strncpy(http_post_head->UploadtaskID, at, length);
			break;
		//*****************************************************************	
	   case HEAD_FIELD_HOST:
	     if (length >= HTTP_FILE_HOST_MAX)
	     {
		   length = HTTP_FILE_HOST_MAX - 1;
	     }
	     strncpy(http_post_head->host, at, length);
	   break;
				
	   case HEAD_FIELD_CONTENT_TYPE:
		 mch_tmp1 = init_testhttp_string_and_len();
	     if(mch_tmp1 == NULL)
	     {
	        log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value content type alloc1 error!");
	        return 0;
	     }
	     mch_tmp2 = init_testhttp_string_and_len();
	     if(mch_tmp2 == NULL)
	     {
	        log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value content type alloc2 error!");
	        return 0;
	     }
		 if (length >= HTTP_FILE_CONTENT_TYPE_MAX)
		 {
		     length = HTTP_FILE_CONTENT_TYPE_MAX -1;
		 }
		 strncpy(http_post_head->content_type, at, length);
		 mch_tmp1->len = length;
		 mch_tmp1->string = (char*)malloc(length + 1);
		 if(mch_tmp1->string == NULL)
		 {
		          if (mch_tmp1->string)
			{
				LS_MEM_FREE(mch_tmp1->string);
			}
			if (mch_tmp1)
			{
				LS_MEM_FREE(mch_tmp1);
			}
                        if (mch_tmp2)
			{
				LS_MEM_FREE(mch_tmp2);
			}
	              log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value content type alloc3 error!");
		      return 0;
		 }
		 memset(mch_tmp1->string, 0x00, mch_tmp1->len + 1);
		 memcpy(mch_tmp1->string, at, mch_tmp1->len);
	 	 if (http_mch(&mch_tmp1, &mch_tmp2, http_boundary_pattern) == EXIT_SUCCESS)
		 {
			if (mch_tmp1->string)
			{
				LS_MEM_FREE(mch_tmp1->string);
			}
			if(mch_tmp2->len < 100)
			{
			   strcpy(http_post_head->boundary, mch_tmp2->string);
			}
			else
			{
	                     log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value boundary is overflow!");
			}
			if (mch_tmp2->string) {
				LS_MEM_FREE(mch_tmp2->string);
			}
			if (mch_tmp1)
			{
				LS_MEM_FREE(mch_tmp1);
			}
                        if (mch_tmp2)
			{
				LS_MEM_FREE(mch_tmp2);
			}
		 }
		 else
		 {
		       if (mch_tmp1->string)
			{
				LS_MEM_FREE(mch_tmp1->string);
			}
		        if (mch_tmp2->string) 
		       {
				LS_MEM_FREE(mch_tmp2->string);
			}
			if (mch_tmp1)
			{
				LS_MEM_FREE(mch_tmp1);
			}
                        if (mch_tmp2)
			{
				LS_MEM_FREE(mch_tmp2);
			}
		 
		 }
	  break;
	  case HEAD_FIELD_CONNECTTION:
	     if(length>100)
		 {
			 length = 100;
			 //memset(http_post_head->connection, 0, 100);
	         log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value field connecttion is overflow!");
		 }
		strncpy(http_post_head->connection, at, length);
	  break;
	  case HEAD_FIELD_UPLOAD_NAME:
		http_post_head->file_name_length= URLDecode(at, length, http_post_head->file_name, 1024);
      break;
      case HEAD_FIELD_UPLOAD_SIZE:
		http_post_head->up_load_size = atoi(at);
	  break;
	  case HEAD_FIELD_CONTENT_LENGTH:
	    http_post_head->content_length = atoi(at);
	  break;
	  case HEAD_FIELD_ACCEPT:
	     if(length>512)
		 {
			length = 512;
			//memset(http_post_head->accept, 0, 100);
	        log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value field  accept is overflow!");
			return 0;
		 }
		strncpy(http_post_head->accept, at, length);
	  break;
	  case HEAD_FIELD_ACCEPT_ENCODING:
	     if(length>100)
		 {
			//memset(http_post_head->Accept_Encoding, 0, 100);
			length = 100;
	        log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value field  accept encodeing is overflow!");
			return 0;
		 }
		strncpy(http_post_head->Accept_Encoding, at, length);
	  break;
	  case HEAD_FIELD_ACCEPT_LANGUAGE:
	     if(length>100)
		 {
			//memset(http_post_head->Accept_Language, 0, 100);
			length = 100;
	        log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value field accept language is overflow!");
			return 0;
		 }
		strncpy(http_post_head->Accept_Language, at, length);
	  break;
	  case HEAD_FIELD_COOKIE:
		if (NULL == http_post_head->cookie)
		{
		   http_post_head->cookie = (char*)malloc(length+1);
		   if(http_post_head->cookie != NULL){
		     memset(http_post_head->cookie, '\0', length+1);
		     strncpy(http_post_head->cookie, at, length);
		   }
		}
	  break;
	  case HEAD_FIELD_REFERER:
	 	if (length >= HTTP_FILE_REFERER_MAX)
		{
		   length = HTTP_FILE_REFERER_MAX - 1;
		}
		strncpy(http_post_head->referer, at, length);
	  break;
	  case HEAD_FIELD_ORIGIN:
	    if (length >= HTTP_FILE_ORIGIN_MAX)
		{
		   length = HTTP_FILE_ORIGIN_MAX - 1;
		}
		strncpy(http_post_head->origin, at, length);
	  break;						
	  case HEAD_FIELD_USER_AGENT:
	    if (length > 512)
		{
		   length = 512;
	       log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in header value field user agent is overflow!");
		}
		strncpy(http_post_head->user_agent, at, length);
	  break;						
	  case HEAD_FIELD_X_QQMAIL_FILENAME:
		http_post_head->file_name_length = URLDecode(at, length, http_post_head->file_name, 1024);
	  break;						
	  case HEAD_FIELD_CONTENTSIZE:
		http_post_head->contentSize= atoi(at);
	  break;
	  case HEAD_FIELD_UPLOAD_OFFSET:
		http_post_head->upload_offset= atoi(at);
	  break;
	  default:
	  break;
	}
    return 0;
}

int on_body(http_parser* parser, const char* at, size_t length) {
  FILE *fp;
  dlp_http_post_head *head_file_node = NULL;
  head_file_node =(dlp_http_post_head *)parser->http;  
	log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n ===================== on_body ============= filename = %s\n\n\n\n",head_file_node->new_name);
  fp = fopen(head_file_node->new_name, "a+");
  if(fp != NULL)
  {
     if(fwrite(at, length, 1, fp) !=1)
     {
	    log_printf(ZLOG_LEVEL_ERROR, "Sorry, ptrace in body write file failed!");
		if (fp)
		{
	        fclose(fp);
		    fp = NULL;
		}
	return 0;
     }
     head_file_node->body_len+=length;
     if (fp)
	 {
	     fclose(fp);
	     fp = NULL;
	 }
  }
  return 0;
}

void usage(const char* name) {
  fprintf(stderr,
  "Usage: %s $type $filename\n"
  "  type: -x, where x is one of {r,b,q}\n"
  "  parses file as a Response, reQuest, or Both\n",
  name);
  log_printf(ZLOG_LEVEL_ERROR, "ptrace usage!");
}


int http( HttpSession *p_session)
{
  enum http_parser_type file_type;
  FILE *fp;
  if(((fp=fopen(p_session->old_name,"r"))==NULL))
  {
	 log_printf(ZLOG_LEVEL_ERROR, "Sorry, http open file failed!");
     goto fail;
  }
  fseek(fp,0,SEEK_END);
  file_type = HTTP_REQUEST;
  long file_length =ftell(fp);
  char* data = malloc(file_length+1);
  memset(data,0,file_length+1);
  fseek(fp,0,SEEK_SET);
  long a=fread(data,1, file_length,fp);
  if (fp)
  {
	fclose(fp);
	fp = NULL;
  }
  if (a != file_length)
  {
	 log_printf(ZLOG_LEVEL_ERROR, "Sorry, http read file failed!");
        LS_MEM_FREE(data);
	 goto fail;
  }  
  http_parser_settings settings;
  memset(&settings, 0, sizeof(settings));
  settings.on_message_begin = on_message_begin;
  settings.on_url = on_url;
  settings.on_header_field = on_header_field;
  settings.on_header_value = on_header_value;
  settings.on_headers_complete = on_headers_complete;
  settings.on_body = on_body;
  settings.on_message_complete = on_message_complete;
  http_parser parser;
  http_parser_init(&parser, file_type, p_session);
  size_t nparsed = http_parser_execute(&parser, &settings, data, file_length);
  if (data)
  {
	  LS_MEM_FREE(data);
  }
  if (nparsed != (size_t)file_length) {
	log_printf(ZLOG_LEVEL_ERROR, "Sorry, http Error: %s (%s)\n",
        http_errno_description(HTTP_PARSER_ERRNO(&parser)),
       http_errno_name(HTTP_PARSER_ERRNO(&parser)));
       goto fail;
  }

  return EXIT_SUCCESS;

fail:
  return EXIT_FAILURE;
}


//int do_http_file(const char *packet_content, int len, struct tuple4 *key)
/* callback function */
int do_http_file(const char *packet_content, int len, struct tuple4 *key, http_sbuffputdata http_sbuff_putdata,const int iringbuffer_ID)
{
	 SESSION_BUFFER_NODE ft;
         hash_bucket         *pBucket=NULL;	
	 HttpSession	     *p_session=NULL;
	 HttpSession	     *sp_session=NULL;
	 dlp_http_post_head  *http_session=NULL;
	 uuid_t              randNumber, newNumber;
	  char flodername[128]={};
	  char flodername2[128]={};
	 struct tuple4       temp;
	 temp.saddr = key->saddr;
	 temp.daddr = key->daddr;
	 temp.source = key->source;
	 temp.dest = key->dest;
         if(ftp_session_tbl==NULL)
        {
	        log_printf(ZLOG_LEVEL_ERROR, "Sorry, table is NULL,no item to use!!!\n");
		return 0;
       }	
		
		if(len < 13){
			return 0;		
		}
        if(0 == memcmp(packet_content,"POST ",5))
      {  
         if ((pBucket = find_hash(ftp_session_tbl, &temp, sizeof(struct tuple4))) == NULL)
         {  
                    LS_MEM_MALLOC(pBucket, hash_bucket);
		    LS_MEM_MALLOC(p_session, HttpSession);
		    LS_MEM_MALLOC(http_session,dlp_http_post_head);
		    if(pBucket==NULL||p_session==NULL||http_session==NULL)
		    {
		          LS_MEM_FREE(pBucket);
			  LS_MEM_FREE(p_session);
			  LS_MEM_FREE(http_session);
			  return (-1);
		    }
		    memset(http_session,0,sizeof(dlp_http_post_head));
		    memset(p_session,0,sizeof(HttpSession));
		    memcpy(&p_session->key, key, sizeof(struct tuple4));
                    uuid_generate(randNumber);
                    uuid_unparse(randNumber, flodername);
		    //strcat(p_session->old_name ,session_stream);	
		    //strcat(p_session->old_name ,"/");		
		    //strcat(p_session->old_name ,flodername);	
                   sprintf(p_session->old_name, "%s/%s", session_stream,flodername);
			
			//printf("111111p_session->old_namep_session->old_namep_session->old_namep_session->old_name=%s\n",p_session->old_name);
                    uuid_generate(newNumber);		
                    uuid_unparse(newNumber,flodername2);
		    //strcat(http_session->new_name ,session_stream);	
		    //strcat(http_session->new_name ,"/");	
		    //strcat(http_session->new_name , flodername2);
		    sprintf(http_session->new_name, "%s/%s", session_stream,flodername2);
			//printf("1111111http_session->new_namehttp_session->new_namehttp_session->new_name=%s\n",http_session->new_name);
                    p_session->old_file=fopen(p_session->old_name,"a+");		
                    if(p_session->old_file==NULL)
		    {
	                  log_printf(ZLOG_LEVEL_ERROR, "ERROR, place file pointer to hash bucket,but file pointer is NULL!!!\n");
			  LS_MEM_FREE(pBucket);
			  LS_MEM_FREE(p_session);
			  LS_MEM_FREE(http_session);			
		            return (-1);		   
		    }
		    if(fwrite(packet_content,len,1,p_session->old_file)!=1)
		    {
	                      log_printf(ZLOG_LEVEL_ERROR, "ERROR, write file error!!!\n");
		              if(p_session->old_file);
			     {
		                   fclose(p_session->old_file);
		                   p_session->old_file = NULL;
			      }
			  LS_MEM_FREE(pBucket);
			  LS_MEM_FREE(p_session);
			  LS_MEM_FREE(http_session);		  
		             return (-1);		   
		    }	
                     if(p_session->old_file);
		    {
		        fclose(p_session->old_file);
		        p_session->old_file = NULL;
		     }
		     p_session->flag=1;	
		     p_session->http=(void *)http_session;
		     p_session->len=len;
		     pBucket->content = (void *)p_session;
		     pBucket->key = &(p_session->key);
                     insert_hash(ftp_session_tbl, pBucket, sizeof(struct tuple4)); 
		     return 0;
	  }
	             p_session = (HttpSession *)pBucket->content;
 	            if(p_session->flag == 0)
		  {
		         LS_MEM_MALLOC(http_session,dlp_http_post_head);
		         if(http_session==NULL)
		        {
			    LS_MEM_FREE(http_session);
			    return (-1);
		        } 
		          memset(http_session,0,sizeof(dlp_http_post_head));
                       	           uuid_generate(randNumber);
				   uuid_unparse(randNumber, flodername);
				   memset(p_session->old_name,0,128);
				   memset(http_session->new_name,0,128);
				   sprintf(p_session->old_name, "%s/%s", session_stream,flodername);	
			//printf("2222222222p_session->old_namep_session->old_namep_session->old_namep_session->old_name=%s\n",p_session->old_name);
         		  uuid_generate(newNumber);
			  uuid_unparse(newNumber,flodername2);
			  sprintf(http_session->new_name, "%s/%s", session_stream,flodername2);
				  //printf("2222222222http_session->new_namehttp_session->new_namehttp_session->new_name=%s\n",http_session->new_name);
                         p_session->old_file=fopen(p_session->old_name,"a+");	
                         if(p_session->old_file==NULL)
		        {
	                     log_printf(ZLOG_LEVEL_ERROR, "ERROR, place file pointer to hash bucket,but file pointer is NULL1!!!\n");
			     LS_MEM_FREE(http_session);			 
		             return (-1);		   
		        }
		        if(fwrite(packet_content,len,1,p_session->old_file)!=1)
		       {
	                       log_printf(ZLOG_LEVEL_ERROR, "ERROR, write file error1!!!\n");
                               if(p_session->old_file);
			      {
		                   fclose(p_session->old_file);
		                   p_session->old_file = NULL;
			       }
			      LS_MEM_FREE(http_session);		  			   
		              return (-1);		   
		      }	
                       if(p_session->old_file);
			{
		              fclose(p_session->old_file);
		              p_session->old_file = NULL;
			}
		    p_session->flag=1;	

		     if(p_session->http!=NULL)
		    {
		        dlp_http_post_head    *dlp_session=NULL;
		        dlp_session = (dlp_http_post_head *)p_session->http;
			 LS_MEM_FREE(dlp_session->cookie);
		        LS_MEM_FREE(p_session->http);
		    }


		    p_session->http=(void *)http_session;
		    p_session->len=len;
		    pBucket->content = (void *)p_session;      
		  }
	}
	else if((len>0)&&(0!= memcmp(packet_content,"HTTP/1.",7)))
	{		    
        if ((pBucket = find_hash(ftp_session_tbl, &temp, sizeof(struct tuple4))) == NULL)
        {
           return 0;
        }
	    p_session=(HttpSession *)pBucket->content;
		if(p_session->flag==1)
		{
          p_session->old_file=fopen(p_session->old_name,"a+");		
          if(p_session->old_file == NULL)
		  {
	               log_printf(ZLOG_LEVEL_ERROR, "ERROR, place file pointer to hash bucket,but file pointer is NULL1!!!\n");
		       return (-1);		   
		  }
		  fwrite(packet_content,len,1,p_session->old_file);
		  p_session->len+=len;

          if(p_session->old_file);
		  {
		      fclose(p_session->old_file);
		      p_session->old_file = NULL;
		  }
		}
	}	
    else  if(0 == memcmp(packet_content,"HTTP/1.",7))	
	{
		
		if(0 == memcmp(packet_content + 8," 100",4))
		{
			return 0;			
		}
       if ((pBucket = find_hash(ftp_session_tbl, &temp, sizeof(struct tuple4))) == NULL)
       {
          return 0;
       }
       p_session=(HttpSession *)pBucket->content;
 	   if(p_session->flag==1)
	   {
		  dlp_http_post_head *temp = NULL;
		  temp = (dlp_http_post_head *)p_session->http;
		  memcpy(ft.orgname, p_session->old_name, 128);
		  memcpy(ft.strname, temp->new_name, 128);
		  ft.attr = p_session->http;
		  ft.session_five_attr.ip_src = p_session->key.saddr;
		  ft.session_five_attr.ip_dst = p_session->key.daddr;
		  ft.session_five_attr.port_src = p_session->key.source;
		  ft.session_five_attr.port_dst = p_session->key.dest;
		  ft.session_five_attr.protocol = 250;
	      p_session->flag=0;
		  /* parsertrace.c callback function */
		  http_sbuff_putdata(iringbuffer_ID, ft);
	   } 
    }
} 
