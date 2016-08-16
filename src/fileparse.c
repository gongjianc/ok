/*
 * =====================================================================================
 *
 *       Filename:  fileparse.c
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

#include "fileparse.h"

/**
 * @brief  声明jvm Java Virtual Machine
 */
JavaVM *jvm;

/**
 * @brief  创建jvm
 */
void build_JVM()
{
//	log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n\n______________________ build_JVM begin ________________\n\n\n\n");
	 JavaVMOption options[4];
	 JNIEnv *env;	
	 JavaVMInitArgs vm_args;
	 long status;
	 options[3].optionString =  "-Djava.class.path=.:./tika-app-1.5.jar:./jackson-all-1.7.6.jar:./fileparser.jar";
	 options[2].optionString = "-Djava.library.path=/usr/java/jdk1.8.0_05/jre/lib/amd64/server/";
     options[1].optionString = "-Djava.library.path=/usr/java/jdk1.8.0_05/lib";
     options[0].optionString = "-Xmx32768M";
     //options[0].optionString = "-Xmx4096M";
	 memset(&vm_args, 0, sizeof(vm_args));
	 vm_args.version = JNI_VERSION_1_8;
	 vm_args.nOptions = 4;
	 vm_args.options = options;
	
	 status = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
//	 if(status!=JNI_ERR)
//	log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n\n______________________ BUILD_JVM OK jvm addr = %p ________________\n",jvm);
}

/**
 * @brief 销毁jvm
 */
void destory_JVM()
{
	 (*jvm)->DestroyJavaVM(jvm);
}

/**
 * @brief 解析文件返回文件属性json串
 *
 * @param filepath     需要解析的文件绝对路径
 * @param tofilepath   解析后文件内容存放的文本文件的绝对路径
 *
 * @return  返回json串    
 *          格式例如："{\"filetype\":\"pptx\",\"isSuccess\":\"true\",\"fileAuthor\":\"陈洪波\",\"isEncrypted\":\"true\"}";
 *          filetype    文件类型
 *          isSuccess   是否解析成功（true:加密，false:非加密）
 *          fileAuthor  文件作者
 *          isEncrypted 是否加密（true:加密，false:非加密，embededEncrypted:嵌入式加密）
 */
char* tika(char* filepath,char* tofilepath)
{
   	JNIEnv *env;
	int attach_ret = 0;
	jclass cls;
	jmethodID mid;
	jobject square;
	jstring jResult;
    char* str = NULL;
	
	//log_printf(ZLOG_LEVEL_INFO, "______________________ tika Enter step 1  addr = %p________________\n",jvm);
#if 1
	//attache current thread
    attach_ret = (*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL);
	if (attach_ret != 0)
	{
		log_printf(ZLOG_LEVEL_ERROR,"*************Tika AttachCurrentThread Failure\n");
		return str;
	}
#endif
#if 0
    if ((*jvm)->GetEnv(jvm,(void**) &env, JNI_VERSION_1_8) != JNI_OK) //将JNI的版本设置为1.4,并通过jvm获取JNIEnv结构体  
        return -1;  
    //if (registerNatives(env) != JNI_TRUE)                             //注册我们的本地方法到VM中.  
      //  return -1;  
#endif
	log_printf(ZLOG_LEVEL_INFO, "______________________ tika Enter step 2 ________________\n");
	
	//Find the class fileparser
    cls = (*env)->FindClass(env, "fileparser_1_5");

	if(cls == 0)
	{
		log_printf(ZLOG_LEVEL_ERROR,"*************Tika FindClass fileparser Failure\n");
		(*jvm)->DetachCurrentThread(jvm);
		return str;
	}
	log_printf(ZLOG_LEVEL_INFO, "______________________ tika Enter step 3 ________________\n");
	
	//GetStaticMethodID fileparser_jsonstring
   mid = (*env)->GetStaticMethodID(env, cls, "fileparser_jsonstring", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
   if (mid == 0)
   {
		log_printf(ZLOG_LEVEL_ERROR, "*************Tika GetStaticMethodID fileparser_jsonstring Failure\n");
		(*jvm)->DetachCurrentThread(jvm);
		return str;
   }
	log_printf(ZLOG_LEVEL_INFO, "______________________ tika Enter step 4 ________________\n");

	//transfer the filepth and to filepath to jstring
   jstring arg = (*env)->NewStringUTF(env, filepath);
   jstring arg1 = (*env)->NewStringUTF(env, tofilepath);

	log_printf(ZLOG_LEVEL_INFO, "*************Tika Call filepath = %s\n", filepath);

    square=(*env)->CallStaticObjectMethod(env, cls, mid, arg, arg1);

   jResult=(jstring)square;
   if(jResult == NULL)
   {
	    log_printf(ZLOG_LEVEL_ERROR, "*************Tika jResult == NULL\n");
		(*jvm)->DetachCurrentThread(jvm);
		return str;
   }
   str = (char*)(*env)->GetStringUTFChars(env, jResult, 0);
   (*jvm)->DetachCurrentThread(jvm);

	log_printf(ZLOG_LEVEL_INFO, "*************Tika OK Return Json str = %s\n", str);
   return str;
}
