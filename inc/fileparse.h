/*
 * =====================================================================================
 *
 *       Filename:  fileparse.h
 *
 *    Description: 使用jni技术调用tika解析，生成文件内容的文本文件，返回文件属性json串
 *
 *       function list:
 *                 build_JVM()
 *                       @brief  创建jvm
 *                 destory_JVM()
 *                       @brief 销毁jvm
 *                 tika()
 *                       @brief 解析文件返回文件属性json串
 *
 *        Version:  1.0
 *        Created:  08/30/2014 07:13:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  高奔
 *   Organization:  大庆油田信息技术公司
 *
 * =====================================================================================
 */

#ifndef FILEPARSE_H_
#define FILEPARSE_H_

#include <jni.h>
#ifdef _WIN32
#define PATH_SEPARATOR ';'
#else
#define PATH_SEPARATOR ':'
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "plog.h"

void build_JVM();

void destory_JVM();

char* tika(char* filepath,char* tofilepath);
void *thread1();
  
void *thread2();  

void thread_create(void);
  
void thread_wait(void); 

#endif
