/*
 * =====================================================================================
 *
 *       Filename:  ftp_extract.c
 *
 *    Description: ftp协议的json串的生成函数
 *                 http upload协议的json串的生成函数
 *
 *  function list:
 *           get_file_size()
 *                 获取文件大小
 *           int_to_ipstr()
 *                 将网络格式的ip地址转换为点分十进制的字符串 
 *           ftp_jsonstr()
 *                 产生ftp的json串 
 *           httpupload_pro_jsonstr()
 *                 产生http upload的json串 
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
#include <locale.h>
#include "ftp_extract.h"
#include <iconv.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "uchardet.h"
#include <gmime/gmime-iconv.h>
#define null NULL
#ifdef __cplusplus

extern "C"
{
  extern CHARDET_IMEXPORT int chardet_create (chardet_t * pdet);
  extern CHARDET_IMEXPORT int chardet_handle_data (chardet_t det,
						   const char *data,
						   unsigned int len);
  extern CHARDET_IMEXPORT int chardet_data_end (chardet_t det);
  extern int chardet_reset (chardet_t det);
  extern CHARDET_IMXPORT int chardet_get_charset (chardet_t det,
						  char *namebuf,
						  unsigned int buflen);
#endif

#ifdef __cplusplus

}
#endif
#if 1
#define CHARSET_MAX 1024
#if 1
#if 1
char *
detect (char *buffer, int len)
{
  uchardet_t handle = uchardet_new ();
  char *charset;
  // char        buffer[BUFFER_SIZE];
  int i;
  int retval = uchardet_handle_data (handle, buffer, len);
  if (retval != 0)
    {
      return NULL;
    }
  uchardet_data_end (handle);
  charset = strdup (uchardet_get_charset (handle));
  for (i = 0; charset[i]; i++)
    {
      /* Our test files are lowercase. */
      charset[i] = tolower (charset[i]);
    }
  uchardet_delete (handle);
  return charset;
}

#endif
int
charsetConvert (const char *from_charset, const char *to_charset,
		const char *src, const int srclen, char *save, int savelen)
{
  if (save == NULL || srclen == 0)
    {
      return -1;
    }
#if 1
  save[0] = 0;
  if (strcmp (from_charset, to_charset) == 0)
    {
      log_printf (ZLOG_LEVEL_INFO,
		  "\n\n  ======== charsetConvert ===== is ok =====\n\n");
      if (savelen <= srclen)
	strncat (save, src, savelen);
      else
	strncat (save, src, srclen);
      return savelen > srclen ? srclen : savelen;
    }
#endif
  //convert   
  iconv_t cd;
  int status = 0;		//result   
  char *outbuf = save;		//iconv outptr begin   
  char *inptr = src;
  char *outptr = outbuf;
  size_t insize = srclen;
  size_t outsize = savelen;

  cd = iconv_open (to_charset, from_charset);
  if ((iconv_t) (-1) == cd)
    {
      return -1;
    }
  iconv (cd, NULL, NULL, NULL, NULL);
  while (insize > 0)
    {
      size_t res = iconv (cd, (char **) &inptr, &insize, &outptr, &outsize);
      if (outptr != outbuf)
	{
	  outbuf = outptr;
	  *outbuf = 0;
	}
      if (res == (size_t) (-1))
	{
	  if (errno == EILSEQ)
	    {
	      int one = 1;
	      iconvctl (cd, ICONV_SET_DISCARD_ILSEQ, &one);
	      status = -3;
	    }
	  else if (errno == EINVAL)
	    {
	      if (srclen == 0)
		{
		  status = -4;
		  goto done;
		}
	      else
		{
		  break;
		}
	    }
	  else if (errno == E2BIG)
	    {
	      status = -5;
	      goto done;
	    }
	  else
	    {
	      status = -6;
	      goto done;
	    }
	}
    }
  status = strlen (save);	// ===  outbuf - save ;   
done:
  iconv_close (cd);
  return status;
}
#endif
int
encode_guess (char *s, int len, char *in_str, int str_len, char *out_str,
	      int out_str_len)
{
  int ret_return = 0;
  char *charset = detect (in_str, str_len);
  if (NULL != charset)
    {
      ret_return =
	charsetConvert ("gbk", "utf-8", in_str, str_len, out_str,
			out_str_len);
      log_printf (ZLOG_LEVEL_INFO,
		  "\n\n  ========  22 charset = %s====== \n\n", charset);
    }
  return 0;
}

#endif
int
GBK2UTF8 (char *pInBuf, int iInLen, char *pOutBuf, int iOutLen, char *charset)
{
  int iRet;
  iconv_t hIconv = iconv_open ("UTF-8", "GBK");
  if (-1 == (int) hIconv)
    {
      log_printf (ZLOG_LEVEL_INFO, "\n\n GBK error 1 \n\n");
      return -1;		//打开失败，可能不支持的字符集
    }
  //开始转换
  iRet =
    iconv (hIconv, (const char **) (&pInBuf), iInLen, (char **) (&pOutBuf),
	   iOutLen);
  //关闭字符集转换
  iconv_close (hIconv);
  return 0;			//iRet;
}

//包函 libiconv库头文件
/**
 * @brief 获得文件大小
 *
 * @param path 文件路径
 *
 * @return   文件大小
 */
static unsigned long
get_file_size (const char *path)
{

  if (access (path, F_OK) != 0)
    {
      log_printf (ZLOG_LEVEL_ERROR,
		  "file not exist, get_file_size() function error");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d, file path:%s ", __FILE__,
		  __LINE__, path);
      return -1;
    }
  unsigned long filesize = -1;
  struct stat statbuff;
  if (stat (path, &statbuff) < 0)
    {
      log_printf (ZLOG_LEVEL_DEBUG, "stat(), file not exist :filesize=%d ",
		  filesize);
      log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
      return filesize;
    }
  else
    {
      filesize = statbuff.st_size;
      log_printf (ZLOG_LEVEL_DEBUG, "stat(), filesize=%d ", filesize);
      log_printf (ZLOG_LEVEL_DEBUG, "%s__%d ", __FILE__, __LINE__);
      return filesize;
    }
  return 0;
}

/**
 * @brief        网络的ip转换为点分十进制
 *
 * @param src    源
 * @param str    输出
 *
 * @return  成功返回0，失败返回-1
 */
int
int_to_ipstr (int src, char *str)
{
  if (int_ntoa (src))
    {
      strcpy (str, int_ntoa (src));
    }
  return 0;
}

#if 1
int
isgbk (char *s, size_t ns)
{
  if (ns > 2 && (unsigned char) *s >= 0x81 && (unsigned char) *s <= 0xfe
      && (((unsigned char) *(s + 1) >= 0x80
	   && (unsigned char) *(s + 1) <= 0x7e)
	  || ((unsigned char) *(s + 1) >= 0xa1
	      && (unsigned char) *(s + 1) <= 0xfe)))
    {
      return 1;
    }
  return 0;
}

/****************************************************************************** 
 * function: gbk2utf8 
 * description: 实现由gbk编码到utf8编码的转换  
 *  
 * input: utfstr,转换后的字符串;  srcstr,待转换的字符串; maxutfstrlen, utfstr的最 
            大长度 
 * output: utfstr 
 * returns: -1,fail;>0,success 
 *  
 * modification history 
 * -------------------- 
 *  2011-nov-25, lvhongya written 
 * -------------------- 
 ******************************************************************************/
int
gbk2utf8 (char *utfstr, const char *srcstr, int maxutfstrlen)
{
  if (null == srcstr)
    {
      printf ("11 bad parameter\n");
      return -1;
    }
  log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO utfstr 11");
  //首先先将gbk编码转换为unicode编码  
  if (null == setlocale (LC_ALL, "zh_CN.gbk"))	//设置转换为unicode前的码,当前为gbk编码  
    {
      printf ("22 bad parameter\n");
      return -1;
    }
  log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO utfstr 22");
  int unicodelen = mbstowcs (null, srcstr, 0);	//计算转换后的长度  
  if (unicodelen <= 0)
    {
      printf ("can not transfer!!!\n");
      return -1;
    }
  log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO utfstr 33");
  wchar_t *unicodestr = (wchar_t *) calloc (sizeof (wchar_t), unicodelen + 1);
  mbstowcs (unicodestr, srcstr, strlen (srcstr));	//将gbk转换为unicode  

  log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO utfstr 44");
  //将unicode编码转换为utf8编码  
  if (null == setlocale (LC_ALL, "zh_CN.utf8"))	//设置unicode转换后的码,当前为utf8  
    {
      printf ("33 bad parameter\n");
      return -1;
    }
  log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO utfstr 55");
  int utflen = wcstombs (null, unicodestr, 0);	//计算转换后的长度  
  if (utflen <= 0)
    {
      printf ("can not transfer!!!\n");
      log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO can not transfer");
      return -1;
    }
  else if (utflen >= maxutfstrlen)	//判断空间是否足够  
    {
      printf ("dst str memory not enough\n");
      log_printf (ZLOG_LEVEL_INFO, "\n\ndst str memory not enough ");
      return -1;
    }
  log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO utfstr 66");
  wcstombs (utfstr, unicodestr, utflen);
  utfstr[utflen] = 0;		//添加结束符  
  log_printf (ZLOG_LEVEL_INFO, "\n\nWO KAO utfstr = %s\n\n", utfstr);
  free (unicodestr);
  return utflen;
}

#endif
#if 1

int
code_convert (char *from_charset, char *to_charset, char *inbuf, size_t inlen,
	      char *outbuf, size_t outlen)
{
  iconv_t cd;
  char **pin = &inbuf;
  char **pout = &outbuf;

  cd = iconv_open (to_charset, from_charset);
  if (cd == 0)
    return -1;
  memset (outbuf, 0, outlen);
  if (iconv (cd, pin, &inlen, pout, &outlen) == -1)
    return -1;
  iconv_close (cd);
  *pout = '\0';

  return 0;
}

int
u2g (char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
  return code_convert ("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
}

int
g2u (char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
  return code_convert ("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
}
#endif
/**
 * @brief  产生ftp的json串
 *
 * @param pronodesrc   ftp信息结构体
 * @param jsonstr      产生的json串
 *
 * @return 成功返回0，失败返回-1
 */
int
ftp_jsonstr (SESSION_BUFFER_NODE * pronodesrc, char **jsonstr)
{
  SESSION_BUFFER_NODE *pronode = pronodesrc;
  log_printf (ZLOG_LEVEL_INFO, "produce json string......");
  log_printf (ZLOG_LEVEL_INFO, "%s__%d", __FILE__, __LINE__);
  json_t *copy;
  json_t *myobject = json_object ();
  json_t *myfile = json_object ();
  json_t *flist = json_array ();
  json_t *objectmsg = json_object ();
  char temp[512] = { 0x0 };
  char *result;
  int size = 0;
  int site = 0;
  char *fullname = NULL;
  char str[32] = { };


  /* 协议类型 */
  json_object_set_new (objectmsg, "prt",
		       json_integer (pronode->session_five_attr.protocol));

  /* 源ip */
  memset (str, 0, 32);
  int_to_ipstr (pronode->session_five_attr.ip_src, str);
  json_object_set_new (objectmsg, "sip", json_string (str));

  /* 目的ip */
  memset (str, 0, 32);
  int_to_ipstr (pronode->session_five_attr.ip_dst, str);
  json_object_set_new (objectmsg, "dip", json_string (str));

  /* 源端口 */
  json_object_set_new (objectmsg, "spt",
		       json_integer (pronode->session_five_attr.port_src));

  /* 目的端口 */
  json_object_set_new (objectmsg, "dpt",
		       json_integer (pronode->session_five_attr.port_dst));

  /*生成当前时间的段 */
  time_t rawtime;
  struct tm *timeinfo;
  char nowtime[256] = { };
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  //sprintf (nowtime, "%d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",
  sprintf (nowtime, "%d-%02d-%02d %02d:%02d:%02d",
	   1900 + timeinfo->tm_year,
	   1 + timeinfo->tm_mon,
	   timeinfo->tm_mday,
	   timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  /* 时间 */
  json_object_set_new (objectmsg, "ts", json_string (nowtime));
  json_object_set_new (myobject, "Username", json_string (""));
  json_object_set_new (myobject, "Password", json_string (""));
  log_printf (ZLOG_LEVEL_INFO, "%s__%d, file path= %s", __FILE__, __LINE__,
	      pronode->strpath);
  log_printf (ZLOG_LEVEL_INFO, "%s__%d, file store name= %s", __FILE__,
	      __LINE__, pronode->strname);
  log_printf (ZLOG_LEVEL_INFO, "%s__%d, file original name= %s", __FILE__,
	      __LINE__, pronode->orgname);

  /* 文件类型 */
  json_object_set_new (myfile, "ft", json_string (""));

  /* 设置包含关系 */
  json_object_set_new (objectmsg, "pAttr", myobject);
  json_object_set_new (objectmsg, "flist", flist);

  /* 文件名称 */
  site = last_mark (pronode->orgname, '/');
  if ((pronode->orgname + site + 1) == NULL)
    {

      log_printf (ZLOG_LEVEL_INFO,
		  "\n\n NULL pronode->orgname+site+1 = %s,site = %d\n\n",
		  pronode->orgname + site + 1, site);
      json_object_set_new (myfile, "fn", json_string (""));
    }
  else
    {
   /*   int ret =
	isgbk (pronode->orgname + site + 1,
	       strlen (pronode->orgname + site + 1));
      encode_guess (pronode->orgname + site + 1,
		    strlen (pronode->orgname + site + 1),
		    pronode->orgname + site + 1,
		    strlen (pronode->orgname + site + 1), temp,
		    sizeof (temp));*/
      json_object_set_new (myfile, "fn",json_string(pronode->orgname+site+1));
      log_printf (ZLOG_LEVEL_INFO,
		  "\n\n Not NULLpronode->orgname+site+1 = %s,temp = %s,site = %d\n\n",
		  pronode->orgname + site + 1, temp, site);
    }

  /* 文件作者 */
  json_object_set_new (myfile, "au", json_string (""));
//              if ((pronode->orgname+site+1) != NULL)
  //      json_object_set_new_nocheck (myfile, "fn", json_string(pronode->orgname+site+1));

  /* 计算文件大小 */
  size = strlen (pronode->strname) + strlen (pronode->strpath);
  fullname = malloc (size + 4);
  memset (fullname, 0, size + 4);

  if (pronode->strpath)
    {
      site = last_mark (pronode->strname, '/');
      sprintf (fullname, "%s/%s", pronode->strpath,
	       pronode->strname + site + 1);
      log_printf (ZLOG_LEVEL_INFO, "full file name=%s", fullname);
      log_printf (ZLOG_LEVEL_INFO, "%s__%d", __FILE__, __LINE__);
    }
  else
    {
      log_printf (ZLOG_LEVEL_ERROR, "path is null");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
      json_decref (myfile);
      json_decref (objectmsg);
      return -1;
    }

  /* 文件路径 */
  if (fullname == NULL)
    {
      json_object_set_new (myfile, "fp", json_string (""));
    }
  else
    {
      json_object_set_new (myfile, "fp", json_string (fullname));
    }

#if 0
  /* 提取文件后缀 */
  char suffix[16] = { };
  char *orgname = pronode->orgname;
  int site = last_mark (orgname, '.');
  if (site > 0)
    {
      strcpy (suffix, orgname + site + 1);
    }
  else
    {
      strcpy (suffix, "txt");
    }
#endif

  //log_printf (ZLOG_LEVEL_INFO, "strpath=%s\n", pronode->strpath);

  /* 文件大小 */
  unsigned long filesize = get_file_size (fullname);
  if (filesize == -1)
    {
      log_printf (ZLOG_LEVEL_ERROR,
		  "ftp get_file_size(), get file size error");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d, file full path is: %s", __FILE__,
		  __LINE__, fullname);
      json_decref (myfile);
      json_decref (objectmsg);
      return -1;
    }

  /* 以kb计算,不足1k的按照1k计算 */
  double realfilesize = (double) filesize / 1024;

  /* 四舍五入 */
  filesize = (int) round (realfilesize);
  if (filesize == 0)
    {
      filesize = 1;
    }
  else if (filesize < 0)
    {
      filesize = 0;
      log_printf (ZLOG_LEVEL_ERROR, "file size errror");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
      json_decref (myfile);
      json_decref (objectmsg);
      return -1;
    }
  log_printf (ZLOG_LEVEL_INFO, "%s__%d-filesize=%ld", __FILE__, __LINE__,
	      filesize);
  //json_object_set_new (myfile, "fs", json_real(filesize));
  json_object_set_new (myfile, "fs", json_integer (filesize));

  /* 文件内容路径 */
  json_object_set_new (myfile, "ftp", json_string (""));

  /* 数组使用 */
  copy = json_deep_copy (myfile);
  json_array_append_new (flist, copy);


  /* 产生json格式字符串,结果使用完毕需要用free释放 */
  result = json_dumps (objectmsg, JSON_PRESERVE_ORDER);

  if (result)
    {
      /* 转换成字符串 */
      //log_printf (ZLOG_LEVEL_INFO, "result=%s\n", result);
      size = strlen (result) + 1;
      *jsonstr = malloc (size);
      memset (*jsonstr, 0, size);
      strcpy (*jsonstr, result);
      log_printf (ZLOG_LEVEL_INFO, "\n\n xi an debug *jsonstr = %s\n\n",
		  *jsonstr);

      free (result);
      result = NULL;
    }

  if (fullname)
    {
      free (fullname);
      fullname = NULL;
    }
  //计数器减一可以销毁数据, 创建的jansson对象最后调用
  json_decref (myfile);
  json_decref (objectmsg);

  return 0;
}

/**
 * @brief  产生http upload的json串
 *
 * @param pronodesrc   http upload信息结构体
 * @param jsonstr      产生的json串
 *
 * @return  成功返回0，失败返回-1
 */
int
httpupload_pro_jsonstr (SESSION_BUFFER_NODE * pronodesrc, char **jsonstr)
{
  SESSION_BUFFER_NODE *pronode = pronodesrc;
  log_printf (ZLOG_LEVEL_INFO, "produce http upload json string......");
  log_printf (ZLOG_LEVEL_INFO, "%s__%d", __FILE__, __LINE__);
  json_t *copy;
  json_t *myobject = json_object ();
  json_t *myfile = json_object ();
  json_t *flist = json_array ();
  json_t *objectmsg = json_object ();
  char *result;
  int size = 0;
  int site = 0;
  char *fullname = NULL;
  char str[32] = { };


  /* 协议类型 */
  json_object_set_new (objectmsg, "prt",
		       json_integer (pronode->session_five_attr.protocol));

  /* 源ip */
  memset (str, 0, 32);
  int_to_ipstr (pronode->session_five_attr.ip_src, str);
  json_object_set_new (objectmsg, "sip", json_string (str));

  /* 目的ip */
  memset (str, 0, 32);
  int_to_ipstr (pronode->session_five_attr.ip_dst, str);
  json_object_set_new (objectmsg, "dip", json_string (str));

  /* 源端口 */
  json_object_set_new (objectmsg, "spt",
		       json_integer (pronode->session_five_attr.port_src));

  /* 目的端口 */
  json_object_set_new (objectmsg, "dpt",
		       json_integer (pronode->session_five_attr.port_dst));

  /*生成当前时间的段 */
  time_t rawtime;
  struct tm *timeinfo;
  char nowtime[256] = { };
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  //sprintf (nowtime, "%d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",
  sprintf (nowtime, "%d-%02d-%02d %02d:%02d:%02d",
	   1900 + timeinfo->tm_year,
	   1 + timeinfo->tm_mon,
	   timeinfo->tm_mday,
	   timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  /* 时间 */
  json_object_set_new (objectmsg, "ts", json_string (nowtime));

  json_object_set_new (myobject, "ts", json_string (nowtime));
  json_object_set_new (myobject, "from", json_string (""));
  json_object_set_new (myobject, "to", json_string (""));
  json_object_set_new (myobject, "sbj", json_string (""));
  json_object_set_new (myobject, "plain", json_string (""));
  /* 设置包含关系 */
  json_object_set_new (objectmsg, "pAttr", myobject);
  json_object_set_new (objectmsg, "flist", flist);

#if 1
  dlp_http_post_head *http = (dlp_http_post_head *) pronode->attr;

  if (http == NULL)
    {
      log_printf (ZLOG_LEVEL_ERROR, "http upload is NULL");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
      json_decref (myfile);
      json_decref (objectmsg);
      return -1;
    }

  //size = strlen(http->host) + strlen(http->url);
  size = HTTP_FILE_HOST_MAX + HTTP_FILE_URL_MAX;
  log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, http upload host= %s", __FILE__,
	      __LINE__, http->host);
  log_printf (ZLOG_LEVEL_DEBUG, "%s__%d, http upload url= %s", __FILE__,
	      __LINE__, http->url);
  char *urlstr = malloc (size + 2);
  memset (urlstr, 0, size + 2);
  sprintf (urlstr, "%s%s", http->host, http->url);
  json_object_set_new (myobject, "url", json_string (urlstr));
#endif
  log_printf (ZLOG_LEVEL_INFO, "%s__%d, file path= %s", __FILE__, __LINE__,
	      pronode->strpath);
  log_printf (ZLOG_LEVEL_INFO, "%s__%d, file store name= %s", __FILE__,
	      __LINE__, pronode->strname);
  log_printf (ZLOG_LEVEL_INFO, "%s__%d, file original name= %s", __FILE__,
	      __LINE__, pronode->orgname);

  /* 文件类型 */
  json_object_set_new (myfile, "ft", json_string (""));

  /* 文件名称 */
  site = last_mark (pronode->orgname, '/');
  if ((pronode->orgname + site + 1) == NULL)
    {
      json_object_set_new (myfile, "fn", json_string (""));
    }
  else
    {
      json_object_set_new (myfile, "fn",
			   json_string (pronode->orgname + site + 1));
    }

  /* 文件作者 */
  json_object_set_new (myfile, "au", json_string (""));

  /* 计算文件大小 */
  size = strlen (pronode->strname) + strlen (pronode->strpath);
  fullname = malloc (size + 4);
  memset (fullname, 0, size + 4);
  if (pronode->strpath)
    {
      site = last_mark (pronode->strname, '/');
      sprintf (fullname, "%s/%s", pronode->strpath,
	       pronode->strname + site + 1);
      log_printf (ZLOG_LEVEL_INFO, "file full path=%s", fullname);
      log_printf (ZLOG_LEVEL_INFO, "%s__%d", __FILE__, __LINE__);
    }
  else
    {
      log_printf (ZLOG_LEVEL_ERROR, "file path is null");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
      json_decref (myfile);
      json_decref (objectmsg);
      return -1;
    }

  /* 文件路径 */
  if (fullname == NULL)
    {
      json_object_set_new (myfile, "fp", json_string (""));
    }
  else
    {
      json_object_set_new (myfile, "fp", json_string (fullname));
    }

#if 0
  /* 提取文件后缀 */
  char suffix[16] = { };
  char *orgname = pronode->orgname;
  int site = last_mark (orgname, '.');
  if (site > 0)
    {
      strcpy (suffix, orgname + site + 1);
    }
  else
    {
      strcpy (suffix, "txt");
    }
#endif

  //log_printf (ZLOG_LEVEL_INFO, "strpath=%s", pronode->strpath);

  /* 文件大小 */
  unsigned long filesize = get_file_size (fullname);
  if (filesize == -1)
    {
      log_printf (ZLOG_LEVEL_ERROR,
		  "httpupload get_file_size(), get file size error");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d, file full path is :%s", __FILE__,
		  __LINE__, fullname);
      json_decref (myfile);
      json_decref (objectmsg);
      return -1;
    }

  /* 以kb计算,不足1k的按照1k计算 */
  double realfilesize = (double) filesize / 1024;

  /* 四舍五入 */
  filesize = (int) round (realfilesize);
  if (filesize == 0)
    {
      filesize = 1;
    }
  else if (filesize < 0)
    {
      filesize = 0;
      log_printf (ZLOG_LEVEL_ERROR, "file size error");
      log_printf (ZLOG_LEVEL_ERROR, "%s__%d", __FILE__, __LINE__);
      json_decref (myfile);
      json_decref (objectmsg);
      return -1;
    }
  log_printf (ZLOG_LEVEL_INFO, "%s__%d-filesize=%ld", __FILE__, __LINE__,
	      filesize);
#if 0
  if (realfilesize < 0)
    {
      filesize = (unsigned long) (realfilesize * 1000);
      realfilesize = filesize / 1000;
    }
  log_printf (ZLOG_LEVEL_INFO,
	      "****************************************************143-realfilesize=%f\n",
	      realfilesize);
#endif
  //json_object_set_new (myfile, "fs", json_real(filesize));
  json_object_set_new (myfile, "fs", json_integer (filesize));

  /* 文件内容路径 */
  json_object_set_new (myfile, "ftp", json_string (""));

  copy = json_deep_copy (myfile);
  json_array_append_new (flist, copy);


  /* 产生json格式字符串,结果使用完毕需要用free释放 */
  result = json_dumps (objectmsg, JSON_PRESERVE_ORDER);

  if (result)
    {
      /* 转换成字符串 */
      //log_printf (ZLOG_LEVEL_INFO, "result=%s\n", result);
      size = strlen (result) + 1;
      *jsonstr = malloc (size);
      memset (*jsonstr, 0, size);
      strcpy (*jsonstr, result);

      free (result);
      result = NULL;
    }

  if (fullname)
    {
      free (fullname);
      fullname = NULL;
    }
#if 1
  if (urlstr)
    {
      free (urlstr);
      urlstr = NULL;
    }
#endif
  //计数器减一可以销毁数据, 创建的jansson对象最后调用
  json_decref (myfile);
  json_decref (objectmsg);

  return 0;
}
