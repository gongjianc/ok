#include "webmail_parser.h"
#include "type_define.h"
extern char * session_stream;
extern char * session_json;
extern unsigned int max_cfilesize;
hash_table *ftp_session_tbl;
//#include <heap-profiler.h>
extern hash_table *ftp_session_tbl;
//extern char *  session_stream;
pthread_mutex_t lockqq=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t locksohu=PTHREAD_MUTEX_INITIALIZER;
int ssbuffer_id = -1;
/*生成由四元组拼接成的字符串*/
int get_tuple_name(HttpSession*dlp_http , char tuple_name[128]  ){
    char tmp[128]= {};
    itoa((dlp_http->key).source , tmp ,16);
    strcat(tuple_name, tmp);
    itoa((dlp_http->key).dest , tmp , 16);
    strcat(tuple_name, tmp);
    itoa((dlp_http->key).saddr , tmp,16);
    strcat(tuple_name, tmp);
    itoa((dlp_http->key).daddr , tmp,16);
    strcat(tuple_name, tmp);
    return 0 ;
}

/**
 * @brief itoa 将整形数转换为字符
 *
 * @param num  待转换的数
 * @param str  转换后存放字符串的指针
 * @param radix数字进制
 *
 * @return
 */
char *itoa(int num,char*str,int radix)
{/*索引表*/
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned unum;/*中间变量*/
    int i=0,j,k;
    /*确定unum的值*/
    if(radix==10&&num<0)/*十进制负数*/
    {
        unum=(unsigned)-num;
        str[i++]='_';
    }
    else unum=(unsigned)num;/*其他情况*/
    /*转换*/
    do{
        str[i++]=index[unum%(unsigned)radix];
        unum/=radix;
    }while(unum);
    str[i]='\0';
    /*逆序*/
    if(str[0]=='-')k=1;/*十进制负数*/
    else k=0;
    char temp;
    for(j=k;j<=(i-1)/2;j++)
    {
        temp=str[j];
        str[j]=str[i-1+k-j];
        str[i-1+k-j]=temp;
    }
    return str;
}
/**
 * @brief to_json   将webmail->To中的邮箱地址进行分割，并放入到json串中
 *
 * @param tolist    存放发件人的json数组
 * @param webmail_to存放发件人的地址的字符串
 *
 * @return
 */
int to_json(json_t * tolist , char * webmail_to)
{
    log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n\n=========fuck webmail_to = %s=====================\n\n\n\n",webmail_to);
#if 1
	   int i=0;
		 int j=0;
		 int mailCounter=0;
	   char *to_mailName[1024];
		 memset(to_mailName,'\0',1024);
		 for(i=0;i<1024;i++)
		 {
			 to_mailName[i]="";
		 }
#endif
    /*如果不存在所要操作的对象，则退出*/
    if (webmail_to == NULL || tolist == NULL) {
        return -1;
    }
    if(strlen(webmail_to) == 0)
        return -1;

    /*变量初始化*/
    char sto[MAX_TO_LEN]= {};
    int Rloca = 0 ;

    /*循环判断，并以逗号作为分割符，将各个邮箱切开*/
    while((webmail_to[i])!='\0'&&  i<MAX_TO_LEN)
    {
			if ((webmail_to[i])==',') {
				memcpy(sto , webmail_to+Rloca ,i - Rloca );
				for(j=0;j<1024;j++)
				{
    log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n\n=== 1lun ==1====to reciver j = %d=====================\n\n\n\n",j);
//					if(to_mailName[j]!="")
	//				{
						if((strcmp(to_mailName[j],sto)==0))
							break;
		//			}
			//		else
				//	{
					//	break;
				//	}
				}
    log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n\n=========to reciver j = %d=====================\n\n\n\n",j);
				if(j<1024)
				{
				}
				else
				{
					json_array_append_new(tolist,json_string(sto) );
					to_mailName[mailCounter++]=sto;
				}
				memset(sto , '\0' , sizeof(sto));
				Rloca = i + 1;
			}
			i++;
		}
		/*考虑到多个邮箱的最后一个邮箱后面没有逗号或者只有一个邮箱的时候没有逗号的情况*/
		memcpy(sto , webmail_to+Rloca ,i - Rloca );
		for(j=0;j<1024;j++)
		{
    log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n\n=== 2lun ==1====to reciver j = %d=====================\n\n\n\n",j);
		//					if(to_mailName[j]!="")
			//				{
								if((strcmp(to_mailName[j],sto)==0))
									break;
				//			}
					//		else
						//	{
							//	break;
						//	}
		}
    log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n\n=== 2lun ==2====to reciver j = %d=====================\n\n\n\n",j);
		if(j<1024)
		{
		}
		else
		{
			json_array_append_new(tolist,json_string(sto) );
		}
		memset(sto , '\0' , sizeof(sto));
		return 0;

}

/**
 * @brief webmail_to_parser
 * 将经过解码后的包含多个发件人的字符串进行分析，提取出所有的发件人到一个字符串内
 *
 * @param var
 * 待提取的字符串
 * @param sto
 * 存储结果
 *
 * @return
 */
int webmail_to_parser(webmail_string_and_len *var , char *sto ){

    if(sto == NULL || var == NULL)return -1;
    /*正则所需变量的声明*/
    const char *pattern = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    pcre *re;
    const char *error = NULL;
    int erroffset;
    int ovector[OVECCOUNT];
    int rc = 0;
    int Rloca = 0;
    char *substring_start = NULL;
    int substring_length = 0;
    char sto1[MAX_TO_LEN]={};

    /*循环将所要操作的对象中的所有邮箱提取到sto中，并各个邮箱以逗号作为分割*/
    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (re == NULL) return -1;
    do{
        rc = 0;
        rc = pcre_exec(re, NULL, var->string, var->len, Rloca, 0, ovector, OVECCOUNT);
        if (rc < 0) break;
        if(Rloca>0||strlen(sto) !=0 ){
            strcat(sto , ",");
        }
        Rloca = ovector[1];
        substring_start = var->string + ovector[0];
        substring_length = ovector[1] - ovector[0];
        memcpy(sto1 , substring_start , substring_length);
        if(strlen(sto) + substring_length >MAX_TO_LEN){
            break;
        }
        strcat(sto , sto1);
        Rloca = ovector[1];
        memset(ovector , 0 , OVECCOUNT);
        memset(sto1 , 0 , MAX_TO_LEN);
    }
    while(rc>0) ;

    pcre_free(re);
    return 0;

}

/**
 * @brief 初始化结构体webmail_string_and_len
 *
 * @return*如果未成功申请空间，则返回为空，如果申请成功则返回所申请的var变量
 */
webmail_string_and_len *init_webmail_string_and_len()
{
    webmail_string_and_len *var ;
    c_malloc((void**)&var,sizeof(webmail_string_and_len));

    if(var != NULL)
    {
        var->len = 0;
        var->string = NULL;
        return var;
    }
    else{
        log_printf(ZLOG_LEVEL_ERROR, "init_webmail_string_and_len error ....");
        return NULL;
    }
}


/**
 * @brief system_time 获得系统时间并放置到nowtime中
 *
 * @param nowtime
 *
 * @return
 */

int system_time(char *nowtime)
{
    //	log_printf(ZLOG_LEVEL_DEBUG, "system_time start ....");
    time_t rawtime;
    struct tm * timeinfo;
    char yyyy[5];
    char mm[3];
    char dd[3];
    char hh[3];
    char mi[3];
    char ss[3];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(yyyy,"%d",1900+timeinfo->tm_year);
    timeinfo->tm_mon<9?sprintf(mm,"0%d",1+timeinfo->tm_mon):sprintf(mm,"%d",1+timeinfo->tm_mon);
    timeinfo->tm_mday<10?sprintf(dd,"0%d",timeinfo->tm_mday):sprintf(dd,"%d",timeinfo->tm_mday);
    timeinfo->tm_hour<10?sprintf(hh,"0%d",timeinfo->tm_hour):sprintf(hh,"%d",timeinfo->tm_hour);
    timeinfo->tm_min<10?sprintf(mi,"0%d",timeinfo->tm_min):sprintf(mi,"%d",timeinfo->tm_min);
    timeinfo->tm_sec<10?sprintf(ss,"0%d",timeinfo->tm_sec):sprintf(ss,"%d",timeinfo->tm_sec);
    sprintf (nowtime, "%s-%s-%s %s:%s:%s",yyyy,mm,dd,hh,mi,ss);/*时间的格式为yyyy-mm-dd hh:min:sec*/
    return 0;
}
/**
 * @brief time_convert  将时间戳转换为标准格式的时间
 *
 * @param time1         时间戳
 * @param s             放置转换后的时间
 *
 * @return
 */
int time_convert(char* time1,char*s)
{
    if(time1){
        char time[10]={};
        memcpy(time,time1,10);
        int t1;
        t1=atoi(time);
        time_t t;
        struct tm *p;
        t=(time_t)t1;
        p=localtime(&t);
        strftime(s, 20, "%Y-%m-%d %H:%M:%S", p);
        return 0;
    }
    return 0;
}
/**
 * @brief hex2num 十六进制转换到10进制
 *
 * @param c
 *
 * @return
 */

int hex2num(char c)
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='a' && c<='z') return c - 'a' + 10;
    if (c>='A' && c<='Z') return c - 'A' + 10;

    return NON_NUM;
}

/**
 * @brief URLDecode 解码 例如：%0A%0D转换为对应的ASCII字符回车换行 web_urldecode 是将UTF8进行转码 即将“%40” 转换为 “@”的形式
 *
 * @param str  被转换的字符字符串
 * @param strSize   被转换字符串对应长度
 * @param result decoded string 存放转换后的字符串
 * @param resultSize 存放转换后字符串长度
 *
 * @return
 */
int web_URLDecode(const char* str, const int strSize, char* result, const int resultSize)
{
    char ch,ch1,ch2;
    int i;
    int j = 0;//record result index

    if ((str==NULL) || (result==NULL) || (strSize<=0) || (resultSize<=0)) {
        return 0;
    }

    for ( i=0; (i<strSize) && (j<resultSize); ++i) {
        ch = str[i];
        switch (ch) {
            case '+':
                result[j++] = ' ';
                break;
            case '%':
                if (i+2<strSize) {
                    ch1 = hex2num(str[i+1]);
                    ch2 = hex2num(str[i+2]);
                    if ((ch1!=NON_NUM) && (ch2!=NON_NUM))
                        result[j++] = (char)((ch1<<4) | ch2);
                    i += 2;
                    break;
                } else {
                    break;
                }
            default:
                result[j++] = ch;
                break;
        }
    }

    result[j] = 0;
    return j;
}
/**
 * @brief mch       通过pcre正则引擎匹配字符串
 *
 * @param var       待检测的字符串
 * @param Sto2      将被匹配到的字符串放置到Sto里面
 * @param pattern   所需要的正则表达式
 *
 * @return
 */
int mch( webmail_string_and_len ** var, webmail_string_and_len ** Sto,const char *pattern)
{
    /*初始化正则匹配所需要的变量*/
    /*
     * re       放置编译后的正则串
     * error    放置出错信息
     * erroffset放置出错的位置
     * ovector  放置所匹配到的字符串的位置，ovetor【0】放置开始位置，ovector【1】放置结束位置
     * substring_start 放置匹配串在内存中的开始位置
     * substring_length 繁殖匹配串的长度
     */
    pcre *re;
    const char *error;
    int erroffset;
    int ovector[OVECCOUNT];
    int rc = 0;
    char *substring_start = NULL;
    int substring_length = 0;
    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (re == NULL) {
        return -1;
    }

    /*执行正则匹配，如果匹配出错返回小于0的值，如果成功则返回大于零的值*/
    rc = pcre_exec(re, NULL, (*var)->string, (*var)->len, 0, 0, ovector, OVECCOUNT);
    if (rc < 0) {
        if (rc == PCRE_ERROR_NOMATCH){
            goto mch_fail;
        }
        else{
            log_printf(ZLOG_LEVEL_ERROR, "Matching error %d\n", rc);
            goto mch_fail;
        }
    }

    substring_start = (*var)->string + ovector[0];
    substring_length = ovector[1] - ovector[0];

    /*将匹配串放置到Sto->string中,并将匹配串的长度放置在Sto1->len中*/
    if(Sto!=NULL){
        (*Sto)->len = substring_length;
        if(c_malloc((void**)&(*Sto)->string,substring_length+1) ==-1) goto mch_fail; //lj
        memset((*Sto)->string,'\0',substring_length+1);
        memcpy((*Sto)->string, substring_start, substring_length);
    }

    /*释放对应资源*/
    pcre_free(re);
    re=NULL;
    return EXIT_SUCCESS;
mch_fail:
    pcre_free(re);
    re=NULL;
    return EXIT_FAILURE;
}
/**
 * @brief mch       通过pcre正则引擎匹配字符串
 *
 * @param var       待检测的字符串
 * @param Sto2      将被匹配到的字符串放置到Sto里面
 * @param pattern   所需要的正则表达式
 *
 * @return
 */
int mch_t( webmail_string_and_len ** var, char Sto[MAX_REGEX_RESULT_LEN],const char *pattern)
{
    /*初始化正则匹配所需要的变量*/
    /*
     * re       放置编译后的正则串
     * error    放置出错信息
     * erroffset放置出错的位置
     * ovector  放置所匹配到的字符串的位置，ovetor【0】放置开始位置，ovector【1】放置结束位置
     * substring_start 放置匹配串在内存中的开始位置
     * substring_length 繁殖匹配串的长度
     */
    pcre *re;
    const char *error;
    int erroffset;
    int ovector[OVECCOUNT];
    int rc = 0;
    char *substring_start = NULL;
    int substring_length = 0;
    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (re == NULL) {
        return -1;
    }

    /*执行正则匹配，如果匹配出错返回小于0的值，如果成功则返回大于零的值*/
    rc = pcre_exec(re, NULL, (*var)->string, (*var)->len, 0, 0, ovector, OVECCOUNT);
    if (rc < 0) {
        if (rc == PCRE_ERROR_NOMATCH){
            goto mch_t_fail;
        }
        else{
            log_printf(ZLOG_LEVEL_ERROR, "Matching error %d\n", rc);
            goto mch_t_fail;
        }
    }

    substring_start = (*var)->string + ovector[0];
    substring_length = ovector[1] - ovector[0];

    /*将匹配串放置到Sto中*/
    if(substring_length >MAX_REGEX_RESULT_LEN)goto mch_t_fail;
    if(Sto!=NULL){
        memcpy(Sto, substring_start, substring_length);
    }

    /*释放对应资源*/
    pcre_free(re);
    re=NULL;
    return EXIT_SUCCESS;
mch_t_fail:
    pcre_free(re);
    re=NULL;
    return EXIT_FAILURE;
}

/**
 * @brief mch_sundy_research  快速在一个字符串中查找pattern1
 *
 * @param var
 * @param Sto1
 * @param Rloca
 * @param pattern1[200]
 *
 * @return  如果查找到，则返回所查找到的位置
 *          否则返回-1
 */
int mch_sundy_research(webmail_string_and_len * var, webmail_string_and_len ** Sto1, int Rloca, const char pattern1[PATTERN_LENGTH])
{
    //log_printf(ZLOG_LEVEL_INFO, "mch_sundy_research start ....");
    log_printf(ZLOG_LEVEL_DEBUG, "mch_sundy_research start ....");
    if ((pattern1 == NULL) || ((var->string + Rloca) == NULL)){
        log_printf(ZLOG_LEVEL_ERROR, "sundy END  in the begining!\n");
        return -1;
    }
    int m = strlen(pattern1);
    if(m>PATTERN_LENGTH)
    {
      m=PATTERN_LENGTH;    //add lining
    }
    int td[128];
    int c = 0;
    for (c = 0; c < 128; c++)
        td[c] = m + 1;
    const char* p;
    for (p = pattern1 ; *p ; p++)
        td[*p] = m - (p - pattern1);
    // start searching...
    const char *t, *tx = var->string+Rloca;
    // the main searching loop
    while (var->string + var->len - tx >= m) {
        for (p = pattern1, t = tx; *p; ++p, ++t) {
            if (*p != *t)
                break;
        }
        if (*p == '\0'){
            int file_length = (*Sto1)->len = tx - (var->string + Rloca);
            return file_length;
        }

        if(0<=tx[m]&&tx[m]<=128){
            tx += td[tx[m]];
        }
        else{
            tx+=m+1;
        }
    }
    log_printf(ZLOG_LEVEL_DEBUG, "mch_sundy_research end ....");
    return -1;
}

/**
 * @brief mch_att  循环提取出MIME 格式的文件，并加入到链表中
 *
 * @param var      待提取的消息体
 * @param webmail   存储邮件的相应信息
 *
 * @return
 */
int mch_att(webmail_string_and_len ** var,dlp_webmail_info *webmail)
{

    /*构造提取文件时所需要的正则表达式,存放到pat1，pat2中*/
    char __bd[100] = "--";
    char buf1[100] = "\\r\\n\\V*filename=\"[^\"][\\w\\W]*?(\\r\\n\\r\\n)";
    char buf2[5] = "--";
    char buf3[100] = "\r\n";
    char pat1[PATTERN_LENGTH] = {};
    char pat2[PATTERN_LENGTH] = {};
    memset(pat2, '\0', sizeof(pat2));
    memset(pat1, '\0', sizeof(pat1));
    strcat(__bd, webmail->boundary);
		log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao Test = %s\n",__bd);
    memcpy(pat1, __bd, 100);
		log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao2  pat1_a = %s\n",pat1);
    strcat(pat1, buf1);
		log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao2  pat1_b = %s\n",pat1);
    //pat1=--bd\\r\\n\\V*filename[^\\r]*
    memcpy(pat2, buf3, 3);
    strcat(pat2, __bd);
		log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao2  pat2 = %s\n\n",pat2);
    //pat2=\r\n--bd

    /*初始化正则匹配所需要的变量*/
    /*
     * re       放置编译后的正则串
     * error    放置出错信息
     * erroffset放置出错的位置
     * ovector  放置所匹配到的字符串的位置，ovetor【0】放置开始位置，ovector【1】放置结束位置
     * Rloca    记录已经匹配过的位置，并在下一次匹配时从此处继续匹配
     * Sto      用来存储正则匹配后的匹配结果
     * substring_start 放置匹配串在内存中的开始位置
     * substring_length 繁殖匹配串的长度
     */
    char *attfile=NULL;
    const char pat3[PATTERN_LENGTH] = "(?<=filename=\")[^\"]*";
    const char pat4[PATTERN_LENGTH] = "(?<=\"Filename\"\\r\\n\\r\\n)[\\W\\w]*?(?=\\r\\n)";
    pcre *re1 = NULL;
    int erroffset1 = 0, erroffset2 = 0;
    const char *error1 = NULL;
    const char *error2 = NULL;
    int ovector1[OVECCOUNT] = {0};
    int ovector2[OVECCOUNT] = {0};
    int rc1 = 0, rc2 = 0;
    int Rloca = 0;		/*to record the offset position each match */
    char *substring_start1 = NULL;
    int substring_length1 = 0;
    char *substring_start2 = NULL;
    int substring_length2 = 0;
    webmail_string_and_len *Sto1 = init_webmail_string_and_len();
    /*如果有没有初始化成功的结构体，则退出*/
    if(Sto1 ==NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into mch_att,alloc memory Sto1 failed!!\n");
    }
    webmail_string_and_len *Sto2 = init_webmail_string_and_len();
    if(Sto2 ==NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into mch_att,alloc memory Sto2 failed!!\n");
    }
    webmail_string_and_len *Sto3 = init_webmail_string_and_len();
    if(Sto3 ==NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into mch_att,alloc memory Sto3 failed!!\n");
    }
    if( Sto1==NULL || Sto2==NULL ||Sto3 == NULL) goto mch_att_fail;//lj

    /*对正则表达式进行编译*/
    re1 = pcre_compile(pat1, 0, &error1, &erroffset1, NULL);
    if (re1 == NULL ) {
        log_printf(ZLOG_LEVEL_ERROR, "PCRE compilation failed at offset %d: %s\n", erroffset1, error1);
        return -1;
    }

    /*开始循环提取流中的一个或者多个文件*/
    do {
        rc1 = 0;
        rc2 = 0;

        /*执行正则匹配，如果匹配出错返回小于0的值，如果成功则返回大于零的值，var-》string是待检测的字符串，var-》len是待检测的字符串长度，Rloca说明告诉从var-》string的第多少位开始检测*/
        rc1 = pcre_exec(re1, NULL, (*var)->string, (*var)->len, Rloca, 0, ovector1, 30);
        if (rc1 < 0) {
					log_printf(ZLOG_LEVEL_DEBUG, "---------------  Matching Failed ......\n");
            if (rc1 == PCRE_ERROR_NOMATCH){
                goto mch_att_fail;
            }
            else{
                goto mch_att_fail;
            }
        }

        else{
            /*将匹配结果进行记录*/
            Rloca = ovector1[1];
            substring_start1 = (*var)->string + ovector1[0];
            substring_length1 = ovector1[1] - ovector1[0];

            /*将匹配串放置到Sto->string中,并将匹配串的长度放置在Sto1->len中*/
            c_malloc((void**)&Sto1->string,substring_length1 + 1);
            if(Sto1->string==NULL){
                log_printf(ZLOG_LEVEL_ERROR, "in mch_att(),cant malloc something!");
                goto mch_att_fail;
            }
            Sto1->len = substring_length1;
            memcpy(Sto1->string, substring_start1, substring_length1);

						log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao3  pat3 = %s\n\n",pat3);
            /*提取附件的名字*/
            if(mch(&Sto1 ,&Sto3, pat3)==EXIT_SUCCESS){
                if(c_malloc((void**)&attfile,Sto3->len+1) == -1) goto mch_att_fail;//lj
                memcpy(attfile,Sto3->string,Sto3->len);
                if (Sto3)
                {
                    c_free(&Sto3->string);
                }
            }

						log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao4  Sto3->string = %s\n\n",Sto3->string);
            /*如果附件的名字为blob，考虑附件的名字在另一出地方保存，则继续寻找，并放入其中*/
            if(strstr(attfile,"blob")){
                if(mch(var,&Sto3,pat4)==EXIT_SUCCESS){
                    c_free(&attfile);
                    if(c_malloc((void**)&attfile,Sto3->len+1) == -1) goto mch_att_fail;//lj
                    memcpy(attfile,Sto3->string,Sto3->len);
                    if (Sto3)
                    {
                        c_free(&Sto3->string);
                    }
                }
            }

            /*释放用过的资源便于下次使用*/
            if (Sto1)
            {
                if (Sto1->string){
                    free(Sto1->string);
                    Sto1->string = NULL;
                }
            }

		log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao6  pat2 = %s,var->string = %s,(*Sto2)->string= %s\n\n",pat2,(*var)->string,Sto2->string);
            /*搜索文件的结束位置，并将位置返回到rc2中，如果小于零说明并未找到，退出*/
            rc2 = mch_sundy_research(*var, &Sto2, Rloca, pat2);
            if (rc2 < 0) {
                goto mch_att_fail;
            }

            /*如果rc2大于零，说明找到，则将文件直接落地，并用UUID作为文件名存储，之后将文件的原本名字、大小信息加入链表，便于之后生成jansson串*/
            else {
                char uuid_buf[UUID_LENGTH]={};
                char flodername[FILE_NAME_LENGTH]={};
                uuid_t randNumber;
                uuid_generate(randNumber);
                uuid_unparse(randNumber, uuid_buf);
                //strcat(flodername ,session_stream  );
                //strcat(flodername ,uuid_buf);
                sprintf(flodername , "%s%s%s", session_stream , "/" , uuid_buf);
								log_printf(ZLOG_LEVEL_DEBUG, "\n\n============== flodername = %s====================\n\n",flodername);
                FILE *fp = NULL;
                fp = fopen(flodername, "w+");
                if(fp == NULL){
                    log_printf(ZLOG_LEVEL_ERROR, "the mch_att cant open the file!");
                    goto mch_att_fail;
                }
                fwrite( (*var)->string+Rloca , Sto2->len , 1 , fp);
                fclose(fp);
                fp=NULL;
		log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao5  uuid_buf read name  = %s\n\n",uuid_buf);
                add_att_nodelist(webmail->filenode,Sto2->len,attfile,uuid_buf);
                Rloca +=  Sto2->len;/*记录已经搜索过得位置*/
                c_free(&attfile);
            }
        }
    } while (rc1 > 0);
    c_free(&attfile);
    if (Sto1){
        c_free(&Sto1->string);
        free(Sto1);
        Sto1 = NULL;
    }
    if (Sto2){
        c_free(&Sto2->string);
        free(Sto2);
        Sto2 = NULL;
    }
    if (Sto3){
        c_free(&Sto3->string);
        free(Sto3);
        Sto3 = NULL;
    }
    pcre_free(re1);
    re1 = NULL;
    log_printf(ZLOG_LEVEL_DEBUG, "mch_att end ....");
    return EXIT_SUCCESS;

mch_att_fail:
    c_free(&attfile);
    if (Sto1){
        c_free(&Sto1->string);
        free(Sto1);
        Sto1 = NULL;
    }
    if (Sto2){
        c_free(&Sto2->string);
        free(Sto2);
        Sto2 = NULL;
    }
    if (Sto3){
        c_free(&Sto3->string);
        free(Sto3);
        Sto3 = NULL;
    }
    pcre_free(re1);
    re1 = NULL;
    log_printf(ZLOG_LEVEL_DEBUG, "mch_att fail end ....");
    return EXIT_FAILURE;
}

/**
 * @brief mch_att_139
 * 类似mch_att函数，其主要作用情况在于处理利用多个POST请求发送一个文件，并且每一个POST请求均为MIME格式
 *
 * @param var       待提取的消息体
 * @param webmail   存放提取的文件信息
 * @param sto       存放所提取的内容
 * @param StoBY     MIME格式的boundary
 * @param           存放所提取的文件名
 *
 * @return
 */
int mch_att_139(webmail_string_and_len **var, webmail_string_and_len **sto, webmail_string_and_len *StoBY, dlp_webmail_info *webmail , char **filename)
{
    /*构造提取文件时所需要的正则表达式,存放到pat1，pat2中*/
    log_printf(ZLOG_LEVEL_DEBUG, "mch_att_139 start ....");
    char __bd[100] = "--";
    char buf1[100] = "\\r\\n\\V*filename=\"[^\"][\\w\\W]*?(\\r\\n\\r\\n)";
    char buf2[5] = "--";
    char buf3[100] = "\r\n";
    char pat1[PATTERN_LENGTH] = { };
    char pat2[PATTERN_LENGTH] = { };
    memset(pat2, '\0', sizeof(pat2));
    strcat(__bd, StoBY->string);
    memcpy(pat1, __bd, 100);
    strcat(pat1, buf1);
    //pat1=--bd\\r\\n\\V*filename[^\\r]*
    memcpy(pat2, buf3, 3);
    strcat(pat2, __bd);
    //pat2=\r\n--bd

    /*初始化正则匹配所需要的变量*/
    /*
     * re       放置编译后的正则串
     * error    放置出错信息
     * erroffset放置出错的位置
     * ovector  放置所匹配到的字符串的位置，ovetor【0】放置开始位置，ovector【1】放置结束位置
     * Rloca    记录已经匹配过的位置，并在下一次匹配时从此处继续匹配
     * Sto      用来存储正则匹配后的匹配结果
     * substring_start 放置匹配串在内存中的开始位置
     * substring_length 繁殖匹配串的长度
     */
    const char pat3[100] = "(?<=filename=\")[^\"]*";
    pcre *re1;
    int erroffset1, erroffset2;
    const char *error1;
    const char *error2;
    int ovector1[OVECCOUNT] = { 0 };
    int ovector2[OVECCOUNT] = { 0 };
    int rc1, rc2;
    int Rloca = 0;
    char *substring_start1 = NULL;
    int substring_length1 = 0;
    char *substring_start2 = NULL;
    int substring_length2 = 0;
    webmail_string_and_len *Sto1 = init_webmail_string_and_len();
    if(Sto1 == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into mch_att_139,alloc memory Sto1 failed!!\n");
    }
    webmail_string_and_len *Sto2 = init_webmail_string_and_len();
    if(Sto2 == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into mch_att_139,alloc memory Sto2 failed!!\n");
    }
    webmail_string_and_len *Sto3 = init_webmail_string_and_len();
    if(Sto3 == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into mch_att_139,alloc memory Sto3 failed!!\n");
    }
    if( Sto1==NULL || Sto2==NULL ||Sto3 == NULL) goto mch_139_file_fail;//lj

    re1 = pcre_compile(pat1, 0, &error1, &erroffset1, NULL);


    /*对正则表达式进行编译*/
    if (re1 == NULL  ) {
        log_printf(ZLOG_LEVEL_ERROR, "PCRE compilation failed at offset %d: %s\n", erroffset1, error1);
        goto mch_139_file_fail;
    }

    /*开始循环提取流中的一个或者多个文件*/
    do {
        rc1 = 0;
        rc2 = 0;

        /*执行正则匹配，如果匹配出错返回小于0的值，如果成功则返回大于零的值*/
        rc1 = pcre_exec(re1, NULL, (*var)->string, (*var)->len, Rloca, 0, ovector1, 30);
        if (rc1 < 0) {
            if (rc1 == PCRE_ERROR_NOMATCH){
                goto mch_139_file_fail;
            }
            else
                log_printf(ZLOG_LEVEL_ERROR, "Matching error %d\n", rc1);
            goto mch_139_file_fail;
        }

        else {
            /*将匹配结果进行记录*/
            Rloca = ovector1[1];
            substring_start1 = (*var)->string + ovector1[0];
            substring_length1 = ovector1[1] - ovector1[0];

            /*将匹配串放置到Sto->string中,并将匹配串的长度放置在Sto1->len中*/
            if(c_malloc((void**)&Sto1->string,substring_length1 + 1) == -1)
                goto mch_139_file_fail;//lj
            Sto1->len = substring_length1;
            memcpy(Sto1->string, substring_start1, substring_length1);

            /*提取文件的名字并放到filename中*/
            if(mch(&Sto1 ,&Sto3, pat3)==EXIT_SUCCESS){
                if(c_malloc((void**)filename,Sto3->len + 1) == -1)
                    goto mch_139_file_fail ;//lj
                memcpy(*filename , Sto3->string , Sto3->len);
                if (Sto3)
                    c_free(&Sto3->string);
            }

            /*释放用过的资源便于下次使用*/
            if (Sto1->string){
                free(Sto1->string);
                Sto1->string = NULL;
            }

            /*搜索文件的结束位置，并将位置返回到rc2中，如果小于零说明并未找到，退出*/
            rc2 = mch_sundy_research(*var, &Sto2, Rloca, pat2);
            if (rc2 < 0) {
                goto mch_139_file_fail;
            }

            else {
                if(c_malloc((void**)&((*sto)->string), Sto2->len + 1) == -1)
                    goto mch_139_file_fail ; //lj
                memcpy((*sto)->string, (*var)->string+Rloca, Sto2->len );
                (*sto)->len = Sto2->len;
                Rloca +=  Sto2->len;/*记录已经搜索过得位置*/
            }
        }
    } while (rc1 > 0);

    if (Sto1){
        c_free(&Sto1->string);
        free(Sto1);
        Sto1 = NULL;
    }
    if (Sto2){
        c_free(&Sto2->string);
        free(Sto2);
        Sto2 = NULL;
    }
    if (Sto3){
        c_free(&Sto3->string);
        free(Sto3);
        Sto3 = NULL;
    }
    pcre_free(re1);
    re1 = NULL;
    log_printf(ZLOG_LEVEL_DEBUG, "mch_att_139 end ....");
    return EXIT_SUCCESS;

mch_139_file_fail:
    if (Sto1){
        c_free(&Sto1->string);
        free(Sto1);
        Sto1 = NULL;
    }
    if (Sto2){
        c_free(&Sto2->string);
        free(Sto2);
        Sto2 = NULL;
    }
    if (Sto3){
        c_free(&Sto3->string);
        free(Sto3);
        Sto3 = NULL;
    }
    pcre_free(re1);
    re1 = NULL;
    log_printf(ZLOG_LEVEL_ERROR, "mch_att_139 fail end ....");
    return EXIT_FAILURE;
}

/**
 * @brief mch_att_139_multi_session 解析139多会话的形式
 *
 * @param StoMB
 * @param StoBY
 * @param webmail
 *
 * @return
 */
int mch_att_139_multi_session( webmail_string_and_len *StoMB , webmail_string_and_len *StoBY , dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "mch_att_139_multi_session start ....");
    if(StoMB==NULL || webmail ==NULL || StoBY == NULL)
        return -1;
    /*变量初始化*/



    /*构造所需要的正则表达式*/
    char __bd[128]={0};
    sprintf(__bd , "--%s" , StoBY->string );
    char filename_buf[128] = "\\r\\n\\V*filename=\"[^\"][\\w\\W]*?(\\r\\n\\r\\n)";

    /*range正则*/
    int begin = 0;
    char begin_t[MAX_REGEX_RESULT_LEN]={0};
    int end   = 0;
    char end_t[MAX_REGEX_RESULT_LEN]={0};
    char buf_range[128] = "\\r\\nContent-Disposition: form-data; name=\"range\"\\r\\n\\r\\n)[\\w\\W]*?(\\r\\n)";
    char sto_range[MAX_REGEX_RESULT_LEN]={0};
    char pattern_range[PATTERN_LENGTH]={0};
    sprintf(pattern_range , "%s%s%s" ,"(?<=", __bd , buf_range);
    if(mch_t(&StoMB , sto_range , pattern_range ) == EXIT_FAILURE)goto mch_att_139_multi_session_fail;
    sscanf(sto_range , "%[^-]" , begin_t );
    sscanf(sto_range , "%*[^-]-%[0-9]" , end_t);
    begin = atoi(begin_t);
    end= atoi(end_t);

    /*uuid正则*/
    char fileid[UUID_LENGTH]={0};//最终落地的uuid
    /*通过在对应位置加‘-’来构造出标准的uuid形式 */
    fileid[8]='-';
    fileid[13]='-';
    fileid[18]='-';
    fileid[23]='-';
    char buf_uuid[128]="\\r\\nContent-Disposition: form-data; name=\"filemd5\"\\r\\n\\r\\n)[\\w\\W]*?(\\r\\n)";
    char sto_uuid[MAX_REGEX_RESULT_LEN]={0};
    char pattern_uuid[PATTERN_LENGTH]={0};
    sprintf(pattern_uuid, "%s%s%s" ,"(?<=", __bd , buf_uuid);
    if(mch_t(&StoMB , sto_uuid, pattern_uuid) == EXIT_FAILURE)goto mch_att_139_multi_session_fail;
    memcpy(fileid,sto_uuid,8);
    memcpy(fileid+9,sto_uuid+8,4);
    memcpy(fileid+14,sto_uuid+12,4);
    memcpy(fileid+19,sto_uuid+16,4);
    memcpy(fileid+24,sto_uuid+20,12);

    /*filesize正则*/
    int filesize = 0 ;
    char buf_filesize[128]="\\r\\nContent-Disposition: form-data; name=\"filesize\"\\r\\n\\r\\n)[\\w\\W]*?(\\r\\n)";
    char sto_filesize[MAX_REGEX_RESULT_LEN]={0};
    char pattern_filesize[PATTERN_LENGTH]={0};
    sprintf(pattern_filesize, "%s%s%s" ,"(?<=", __bd , buf_filesize);
    if(mch_t(&StoMB , sto_filesize, pattern_filesize ) == EXIT_FAILURE)goto mch_att_139_multi_session_fail;
    filesize = atoi(sto_filesize);

    /*提取出文件和文件名*/
    webmail_string_and_len *sto1 = init_webmail_string_and_len();
    char *mail_upload_name=NULL;
    mch_att_139(&StoMB, &sto1, StoBY, webmail , &mail_upload_name) ;
    if(sto1->len == 0)
        goto mch_att_139_multi_session_fail;

    /*写入文件*/
    char filename[FILE_NAME_LENGTH]={0};
    char pathname[FILE_NAME_LENGTH]={0};
    sprintf(filename , "%s%s%s%s", session_stream , "/" , fileid, ".dlp");
    /*根据组号和相对位置，将该流写到对应的位置上，如果还没有该文件，则先建立该文件，如果已经存在该文件，则直接写入到对应位置*/
    FILE *fp = fopen(filename, "r+");
    if(fp == NULL)
    {
        fp = fopen(filename, "w+");
        if(fp == NULL){
            log_printf(ZLOG_LEVEL_ERROR, "cant open the fileid !!!!!\n");
            goto mch_att_139_multi_session_fail;
        }
    }
    fseek(fp ,  begin , SEEK_SET );
    fwrite(sto1->string , sto1->len , 1 ,fp);
    fclose(fp);
    fp=NULL;

    /*如果检测到最后一包文件则生成jansson串*/
    if( end+1== filesize){
        add_att_nodelist(webmail->filenode, filesize,mail_upload_name , NULL);
        sprintf(pathname, "%s%s%s", session_stream , "/" , fileid);
        rename(filename, pathname);
        webmail->state = mail_end;
        memcpy(webmail->uuid,fileid,UUID_LENGTH);
    }

    c_free(&mail_upload_name);
    if(sto1){
        c_free(&sto1->string);
        free(sto1);
        sto1 = NULL;
    }

    return 0;


mch_att_139_multi_session_fail :
    c_free(&mail_upload_name);
    if(sto1)
    {
        c_free(&sto1->string);
        free(sto1);
        sto1 = NULL;
    }
    return -1;

}
/**
 * @brief wymail    提取通过网易邮箱发送的主题、正文、发送者、接收者等信息
 *
 * @param StoMB  待分析的消息体
 * @param webmail   存储邮件的相应信息
 */
int wymail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "wymail start ....");
    if(StoMB==NULL || webmail ==NULL)
        return -1;

    /*初始化obj和sto
     * obj  存放解码后的数据体
     * sto  则存放提取后的信息
     * */
    webmail_string_and_len *obj=init_webmail_string_and_len();
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(obj == NULL||sto == NULL||sto1 == NULL)goto wymail_fail;
    unsigned int ln = StoMB->len;
    if(c_malloc((void**)&(obj->string),ln) == -1)
        goto wymail_fail;//lj
    /*web_urldecode 是将UTF8进行转码 即将“%40” 转换为 “@”的形式*/
    web_URLDecode(StoMB->string, ln, obj->string, ln);
    obj->len=StoMB->len;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern2_1[PATTERN_LENGTH] = "(?<=account\">)[\\w\\W]*?(?=</string>)";/*此正则表达式的意义时提取出以account、n>开头，以</string>结尾的中间部分的字符串*/
    const char pattern2_2[PATTERN_LENGTH] = "(?<=account\":\")[\\w\\W]*?(?=\",)";/*此正则表达式的意义时提取出以account、n>开头，以</string>结尾的中间部分的字符串*/
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL && (mch(&obj,&sto1, pattern2_1)==EXIT_SUCCESS) || (mch(&obj,&sto1, pattern2_2)==EXIT_SUCCESS)  ){/*首先判断webmail中没有东西，并且能够提取到账户，进行下一步，否则跳出 ， 经过mch（）函数后，已经将账户提取到了sto结构体中*/
        if(sto1->len){/*判断sto里面的东西长度大于0，表明确实提取到了*/
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) == -1) goto wymail_fail;//lj
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto->string));/*用过的结构体清空，便于下次使用*/
                c_free(&(sto1->string));
            }
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern3_1[PATTERN_LENGTH] = "(?<=\"to\"><string>)[\\w\\W]*?(?=</array>)";
    const char pattern3_2[PATTERN_LENGTH] = "(?<=\"to\":\")[\\w\\W]*?(?=\",)";
    if(mch(&obj,&sto, pattern3_1)==EXIT_SUCCESS || mch(&obj,&sto, pattern3_2)==EXIT_SUCCESS){
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
            webmail_to_parser(sto , webmail->To);
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

    /*提取主题，并对应存储*/
    const char pattern6_1[PATTERN_LENGTH] = "(?<=subject\">)[\\w\\W]*?(?=</string>)";
    const char pattern6_2[PATTERN_LENGTH] = "(?<=subject\":\")[\\w\\W]*?(?=\",)";
    if(webmail->Subject==NULL&&mch(&obj,&sto, pattern6_1)==EXIT_SUCCESS || mch(&obj,&sto, pattern6_2)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&webmail->Subject,sto->len+1) == -1 ) goto wymail_fail;//lj
            memcpy(webmail->Subject , sto->string,sto->len);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    const char pattern7_1[PATTERN_LENGTH] = "(?<=content\">)[\\w\\W]*?(?=</string>)";
    const char pattern7_2[PATTERN_LENGTH] = "(?<=content\":\")[\\w\\W]*?(?=\",)";
    if(webmail->Content==NULL&&mch(&obj,&sto, pattern7_1)==EXIT_SUCCESS  || mch(&obj,&sto, pattern7_2)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto wymail_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    if(sto1){
        free(sto1);
        sto1=NULL;
    }
    if(obj){
        c_free(&(obj->string));
        free(obj);
        obj=NULL;
    }
    log_printf(ZLOG_LEVEL_DEBUG, "wymail end ....");
    return 0;
wymail_fail:
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1=NULL;
    }
    if(obj){
        c_free(&(obj->string));
        free(obj);
        obj = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "wymail fail end ....");
    return -1;
}

/**
 * @brief wyattach      提取通过网易邮箱发送的文件
 *
 * @param dlp_http      经提取后的http头
 * @param StoPT         URL
 * @param StoMB         待分析数据体
 * @param webmail       存储邮件的相应信息
 */

int wyattach(HttpSession* dlp_http,webmail_string_and_len *StoPT,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "wyattach start ....");
    if(dlp_http==NULL || StoPT==NULL || StoMB==NULL || webmail ==NULL)
        return -1;

    /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
    dlp_http_post_head *http_session;
    http_session = (dlp_http_post_head *)dlp_http->http;

    /*通过四元组来生成hash值，来作为文件名来保存还未拼接完整的文件*/
    /*最终将拼接好的四元组放置在tuple_name中*/
    char tmp[128]= {};
    char tuple_name[128] = {};
    char filename[FILE_NAME_LENGTH]={0};
    char pathname[FILE_NAME_LENGTH]={0};
    get_tuple_name(dlp_http , tuple_name);
    sprintf(filename , "%s%s%s%s", session_stream , "/" , tuple_name , ".dlp");

    /*初始化所需要的变量*/
    /*
     *  now_length          现在已经拼接好的长度
     *  mail_upload_name    上传的文件的名字
     *  mail_upload_size    上传的文件的大小
     *  sto                 临时存储所提取信息
     */
    int now_length = 0;
    int mail_upload_size = 0;
    char mail_upload_name[FILE_NAME_LENGTH] = {};
    memcpy(mail_upload_name, http_session->file_name, FILE_NAME_LENGTH);
    mail_upload_size = http_session->up_load_size;
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL || sto1 == NULL) goto wyattach_fail; //lj

    /*如果已经存在有tuple_name命名的文件则打开这个文件，并在其之后继续拼接文件，如果不存在则建立这个文件*/
    FILE *fp=NULL;
    if(access(filename, F_OK) < 0){
        fp = fopen(filename, "w+");
        if(fp == NULL)
        {
            log_printf(ZLOG_LEVEL_ERROR, "wyattach file open is failing!!\n");
            return -1;
        }
        fclose(fp);
        fp=NULL;
    }
    fp = fopen(filename, "a+");
    if(fp == NULL)
    {
        log_printf(ZLOG_LEVEL_ERROR, "wyattach file open is failing!!\n");
        return -1;
    }

    /*获得现在该文件的大小*/
    fseek(fp, 0, SEEK_END );
    now_length = ftell(fp);
    /*将文件进行拼接*/
    fwrite(StoMB->string, http_session->content_length, 1,  fp);
    //printf("\n now the wyfile length is %d\n",now_length);

    /*如果现在文件的大小等于文件头部的文件大小，说明文件已经组合完整*/
    if (now_length+ http_session->content_length == mail_upload_size && mail_upload_size != 0) {
        webmail->state = mail_end;/*告知生成jansson串*/

        /*提取邮件的发件人，并对应存储便于之后生成jansson串*/
        const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
        const char pattern3[PATTERN_LENGTH] = "(?<=uid=)[\\w\\W]*?(?=&cid)";
        if(webmail->From==NULL&&mch(&StoPT,&sto1,pattern3)==EXIT_SUCCESS){
            if(sto1->len){
                if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                    if(c_malloc((void**)&(webmail->From),sto->len+1) == -1 ) goto wyattach_fail; //lj
                    memcpy(webmail->From,sto->string,sto->len);
                }
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }

        /*提取时间并将其转换成标准格式，其存在两种形式*/
        const char pattern5[PATTERN_LENGTH] = "(?<=cid=c:)[\\w\\W]*?(?=&)";
        const char pattern6[PATTERN_LENGTH] = "(?<=cid=)[\\w\\W]*?(?=&)";
        if(mch(&StoPT,&sto,pattern5)==EXIT_SUCCESS){
            if(sto->len){
                char s[MAX_TIME_LEN];
                memset(s,'\0',sizeof(s));
                time_convert(sto->string,s);
                memcpy(webmail->Time,s,strlen(s));
            }
            if (sto)
            {
                c_free(&(sto->string));
            }
        }
        else if (mch(&StoPT,&sto,pattern6)==EXIT_SUCCESS){
            if(sto->len){
                char s[MAX_TIME_LEN];
                memset(s,'\0',sizeof(s));
                time_convert(sto->string,s);
                memcpy(webmail->Time,s,strlen(s));
            }
            if (sto)
            {
                c_free(&(sto->string));
            }
        }

        /*用UUID作为文件名存储，之后将文件的原本名字、大小信息加入链表，便于之后生成jansson串*/
        add_att_nodelist(webmail->filenode, mail_upload_size, mail_upload_name, NULL);
        char uuid[UUID_LENGTH]={};
        uuid_t  wy_randNumber;
        uuid_generate(wy_randNumber);
        uuid_unparse(wy_randNumber, uuid);
        sprintf(pathname, "%s%s%s", session_stream , "/" , uuid);
        rename(filename , pathname);
        memcpy(webmail->uuid,uuid, UUID_LENGTH);
    }

    /*邮件还没有拼接完成*/
    else{
        webmail->state = mail_ing;/*告知不生成jansson串，邮件还为拼接完成*/
    }

    if(sto){
        free(sto);
        sto=NULL;
    }
    if(sto1){
        free(sto1);
        sto1=NULL;
    }
    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return 0;
wyattach_fail://lj
    if(sto)
    {
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1=NULL;
    }
    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "wyattach fail end ....");
    return -1;

}
/**
 * @brief wyattach1 提取通过网易网盘上传的文件
 *      此种是通过MIME格式传递文件的方式 则可以直接调用mch_att函数
 * @param StoBY
 * @param StoMB
 * @param webmail
 */
int wyattach1(webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "wyattach1 start ....");
    if(StoBY==NULL || StoMB==NULL || webmail ==NULL)
        return -1;
    memcpy(webmail->boundary,StoBY->string,MAX_BOUNDARY_LEN);
    mch_att(&StoMB,webmail);
    return 0;

}

/**
 * @brief extract the information of the webmail of qq except attach
 *
 * @param StoMB
 * @param webmail
 */
int QQmail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "QQmail start ....");
    if(StoMB==NULL || webmail ==NULL)
        return -1;

    /*初始化正则匹配所需要的变量*/
    /*
     * sto      用来临时存储正则匹配的结果
     * tomail   用来存放收件人
     * subject  用来存放主题
     * content  用来存放内容
     */
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL || sto1 == NULL) goto QQmail_fail;//lj
    char *subject=NULL;
    char*content=NULL;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern3[PATTERN_LENGTH] = "(?<=sendmailname=)[\\w\\W]*?(?=&)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern3)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) == -1) goto QQmail_fail;//lj
                memcpy(webmail->From,sto->string,sto->len);
            }
        }
        c_free(&(sto->string));
        c_free(&(sto1->string));
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern4[PATTERN_LENGTH] = "(?<=to=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern5[PATTERN_LENGTH] = "(?<=&cc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern6[PATTERN_LENGTH] = "(?<=bcc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取发送时间*/
    const char pattern10[PATTERN_LENGTH] = "(?<=comtm=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern10)==EXIT_SUCCESS){
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

    /*提取主题，并对应存储*/
    const char pattern7[PATTERN_LENGTH] = "(?<=subject=)[\\w\\W]*?(?=&)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern7)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&subject,sto->len+1) == -1 )goto QQmail_fail; //lj
            web_URLDecode(sto->string,sto->len,subject,sto->len );

            if(c_malloc((void**)&webmail->Subject,strlen(subject)+1) == -1) goto QQmail_fail; //lj
            memcpy(webmail->Subject ,subject,strlen(subject));
            c_free(&subject);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储，有两种发送内容的方式*/
    const char pattern8[PATTERN_LENGTH] = "(?<=content__html=)[\\w\\W]*?(?=&)";
    const char pattern9[PATTERN_LENGTH] = "(?<=content=)[\\w\\W]*?(?=&)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern8)==EXIT_SUCCESS){
        if(sto->len){
#if 0            /*转码*/
            if(c_malloc((void**)&content,sto->len+1) == -1 )goto QQmail_fail;//lj
            web_URLDecode(sto->string,sto->len,content,sto->len );
            if(c_malloc((void**)&webmail->Content,strlen(content)+1) == -1 ) goto QQmail_fail;//lj
            memcpy(webmail->Content,content,strlen(content));
            c_free(&content);
#endif
            /*不转码*/
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto QQmail_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    else if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern9)==EXIT_SUCCESS){
        if(sto->len){
#if 0       /*换码*/
            if(c_malloc((void**)&content,sto->len+1) == -1 )goto QQmail_fail;//lj
            web_URLDecode(sto->string,sto->len,content,sto->len );
            if(c_malloc((void**)&webmail->Content,strlen(content)+1) == -1 ) goto QQmail_fail;//lj
            memcpy(webmail->Content,content,strlen(content));
            c_free(&content);
#endif
            /*不转码*/
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto QQmail_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
        if(sto1){
            c_free(&(sto1->string));
        }
    }
    if(sto1){
        free(sto1);
        sto1=NULL;
    }
    if(sto){
        free(sto);
        sto = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "QQmail end ....");
    return 0;
QQmail_fail://lj
    c_free(&content);
    c_free(&subject);
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    };
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1=NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "QQmail fail end ....");
    return -1;
}
/**
 * @brief QQmail_qun    提取通过qq群的方式发送相应的信息
 *
 * @param StoCK         cookie头
 * @param StoMB         待提取的数据体
 * @param webmail       存储邮件的相应信息
 */
int QQmail_qun(webmail_string_and_len *StoCK,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "QQmail_qun start ....");
    if(StoCK==NULL  || StoMB==NULL || webmail ==NULL)
        return -1;
    /*初始化正则匹配所需要的变量*/
    /*
     * sto      用来临时存储正则匹配的结果
     * subject  用来存放主题
     * content  用来存放内容
     */
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL|| sto1 == NULL) goto QQmail_qun_fail;
    char *subject=NULL;//lj
    char *content=NULL;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串
     * 提取from有两种形式*/
    const char pattern5[PATTERN_LENGTH] = "(?<=sendmailname=)[\\w\\W]*?(?=&)";
    const char pattern1[PATTERN_LENGTH]="(?<=qqmail_alias=)[\\w\\W]*?(?=;)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern5)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto  QQmail_qun_fail ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }
    else if(webmail->From==NULL&&mch(&StoCK,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto QQmail_qun_fail   ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern2[PATTERN_LENGTH] = "(?<=qqgroupid=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if(sto){
            c_free(&(sto->string));
        }
    }


    /*提取主题，并对应存储*/
    const char pattern3[PATTERN_LENGTH] = "(?<=subject=)[\\w\\W]*?(?=&)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&subject,sto->len+1) == -1 )goto QQmail_qun_fail; //lj
            web_URLDecode(sto->string,sto->len,subject,sto->len );

            if(c_malloc((void**)&webmail->Subject,strlen(subject)+1) == -1) goto QQmail_qun_fail; //lj
            memcpy(webmail->Subject ,subject,strlen(subject));
            if (sto)
            {
                c_free(&subject);
            }
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }


    /*提取内容，并对应存储，有两种发送内容的方式*/
    const char pattern4[PATTERN_LENGTH] = "(?<=content__html=)[\\w\\W]*?(?=&)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        if(sto->len){
#if 0       /*换码*/
            if(c_malloc((void**)&content,sto->len+1) == -1 )goto QQmail_qun_fail;//lj
            web_URLDecode(sto->string,sto->len,content,sto->len );
            if(c_malloc((void**)&webmail->Content,strlen(content)+1) == -1 ) goto QQmail_qun_fail;//lj
            memcpy(webmail->Content,content,strlen(content));
            c_free(&content);
#endif
            /*不转码*/
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto QQmail_qun_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        free(sto);
        sto = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "QQmail_qun end ....");
    return 0; //lj
QQmail_qun_fail:
    c_free(&subject);
    c_free(&content);
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "QQmail_qun fail end ....");
    return -1;
}

/**
 * @brief QQattach      提取通过qq邮箱发送的文件
 *
 * @param dlp_http      经提取后的http头
 * @param StoPT         URL
 * @param StoMB         待分析数据体
 * @param webmail       存储邮件的相应信息
 * @param StoBY         MIME形式的boundary
 */
int QQattach(HttpSession* dlp_http,webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    //log_printf(ZLOG_LEVEL_INFO, "QQattach start ....");
    log_printf(ZLOG_LEVEL_DEBUG, "QQattach start ....");

    if(dlp_http==NULL || StoPT==NULL || StoBY==NULL || StoMB==NULL || webmail ==NULL)
        return -1;

    /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
    dlp_http_post_head *http_session;
    http_session = (dlp_http_post_head *)dlp_http->http;

    /*判断其时MIME形式传送的文件，还是通过多个会话传送文件*/
    if(strstr(http_session->content_type, "boundary")){
        /*MIME的处理方式*/
        memcpy(webmail->boundary,StoBY->string,StoBY->len);
        mch_att(&StoMB,webmail);
        webmail->state=build_jansson;
    }
    else{
        /*多会话的处理方式 */

        /*通过四元组来生成hash值，来作为文件名来保存还未拼接完整的文件*/
        char tmp[128]= {};
        char tuple_name[128] = {};
        char filename[FILE_NAME_LENGTH]={0};
        char pathname[FILE_NAME_LENGTH]={0};
        get_tuple_name(dlp_http , tuple_name);
        sprintf(filename , "%s%s%s%s", session_stream , "/" , tuple_name , ".dlp");

        /*初始化所需要的变量*/
        /*
         *  now_length          现在已经拼接好的长度
         *  uuid_name           拼接完成后通过uuid存储的文件的名字
         */
        int now_length = 0;
        char uuid_name[UUID_LENGTH] = {0};

        /*判断这是何种流，如果数据体的第354位不为0x02，并且之前未生成以此的hash值为名字的文件，则这个是小附件，并传输结束，生成jansson串*/
        char c[1]={};
        memcpy(c,(StoMB->string)+353,1);
        if(c[0] != 0x02&&access(filename, F_OK) < 0){
            char uuid_buf[UUID_LENGTH]={};
            char filename1[FILE_NAME_LENGTH]={0};
            uuid_t randNumber;
            uuid_generate(randNumber);
            uuid_unparse(randNumber, uuid_buf);
            sprintf(filename1 , "%s%s%s" , session_stream , "/" , uuid_buf);
            FILE *fp = NULL;
            fp = fopen(filename1, "w+");
            if(fp == NULL){
                log_printf(ZLOG_LEVEL_ERROR, "the QQattach cant open!\n");
                return -1;
            }
            fwrite(StoMB->string+364 , StoMB->len-364 , 1 , fp);
            fclose(fp);
            fp=NULL;
            add_att_nodelist(webmail->filenode,StoMB->len-364,"nofilename",uuid_buf);
            webmail->state = build_jansson;
        }

        /*分析通过多个包传递附件的情况，*/
        else{
            FILE *fp=NULL;

            /*判断这是何种流，如果数据体的第354位为0x02,并且不存在以此hash值命名的文件，则这是个大附件（通过多个包传输的附件称为大附件），且为第一包*/
            if(c[0] == 0x02&&access(filename, F_OK) < 0){
                fp = fopen(filename, "w+");
                if(fp == NULL)
                {
                    log_printf(ZLOG_LEVEL_ERROR, "in the QQattach folder the file cant open!\n");
                    return -1;
                }
                fclose(fp);
                fp=NULL;
            }
            fp = fopen(filename, "a+");
            if(fp == NULL){
                log_printf(ZLOG_LEVEL_ERROR, "in the QQattach folder the file cant open!\n");
                return -1;
            }
            fseek(fp, 0, SEEK_END );
            now_length = ftell(fp);
            /*具体的将数据体中实际的部分提取出来，并写入文件*/
            fwrite(StoMB->string+364,  http_session->content_length-364, 1 , fp);
            fclose(fp);
            fp=NULL;

            /*如果数据体的第354位为0x02,且存在以此hash值命名的文件，则这是个大附件，且为最后一包,生成jansson串*/
            if(c[0] != 0x02 && access(filename, F_OK) >= 0){
                //printf("the file length is %d \n",now_length+St)
                add_att_nodelist(webmail->filenode,now_length+(StoMB->len)-364,"nofilename",NULL);
                webmail->state = mail_end;
                char uuid[UUID_LENGTH] = {};
                uuid_t  qq_randNumber;
                uuid_generate(qq_randNumber);
                uuid_unparse(qq_randNumber, uuid);
                sprintf(pathname, "%s%s%s", session_stream , "/" , uuid);
                /*由于文件拼装完毕，则将文件名改为uuid的形式*/
                rename(filename, pathname);
                memcpy(webmail->uuid, uuid, UUID_LENGTH);
            }
        }
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "QQattach end ....");
    return 0;

}
/**
 * @brief QQattach2 分析通过多个会话，多个包传送附件，但并不加密
 *
 * @param dlp_http      经提取后的http头
 * @param StoPT         URL
 * @param StoMB         待分析数据体
 * @param webmail       存储分析邮件后的相应信息
 */
int QQattach2(HttpSession* dlp_http, webmail_string_and_len *StoPT, webmail_string_and_len *StoMB, dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "QQattach2 start ....");
    if(dlp_http==NULL || StoPT ==NULL||StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化正则匹配所需要的变量*/
    /*
     * sto               用来临时存储正则匹配的结果
     * pattern_filesize  表征所传递附件的大小
     * pattern_id        标识附件的uuid
     * pattern_mcnt      该流所在有流的排列中，位列第几组
     * pattern_moff      该流在所有流的排列中，位列一组中的相对位置
     * Groupsize         一个组的大小
     */
    webmail_string_and_len *sto=init_webmail_string_and_len();
    if(sto == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into QQattach2,alloc memory sto failed!!\n");
        return -1;
    }
    char pattern_filesize[PATTERN_LENGTH]="(?<=size=)[\\W\\w]*?(?=&)";
    char pattern_id[PATTERN_LENGTH]="(?<=storeid=)[\\W\\w]*?(?=&)";
    char pattern_mcnt[PATTERN_LENGTH]="(?<=mcnt=)[\\W\\w]*?(?=&)";
    char pattern_moff[PATTERN_LENGTH]="(?<=moff=)[\\W\\w]*?(?=&)";

    int  GROUPSIZE = 1048576;
    unsigned int  uploadsize=0;
    int  mcnt = -1;
    int  moff = -1;
    char fileid[UUID_LENGTH]={0};
    int now_length = 0;
    /*通过在对应位置加‘-’来构造出标准的uuid形式 */
    fileid[8]='-';
    fileid[13]='-';
    fileid[18]='-';
    fileid[23]='-';
    char nowlen[64]={0};

    /*提取出该流对应的组号和相对位置*/
    if(mch(&StoPT,&sto,pattern_mcnt)==EXIT_SUCCESS){
        mcnt = atoi(sto->string);
        if (sto)
        {
            c_free(&sto->string);
        }
    }
    if(mch(&StoPT,&sto,pattern_moff)==EXIT_SUCCESS){
        moff = atoi(sto->string);
        if (sto)
        {
            c_free(&sto->string);
        }
    }
    if(mcnt < 0 && moff < 0 ){
        log_printf(ZLOG_LEVEL_ERROR, "the mcnt or the moff error!\n");
        goto QQattach2_fail;//lj
    }

    /*提取出文件的大小*/
    if(mch(&StoPT,&sto,pattern_filesize)==EXIT_SUCCESS){
        uploadsize = atoi(sto->string);
        if (sto)
        {
            c_free(&sto->string);
        }
    }

    /*提取出文件的uuid，并转换为标准形式*/
    if(mch(&StoPT,&sto,pattern_id)==EXIT_SUCCESS){
        memcpy(fileid,sto->string,8);
        memcpy(fileid+9,sto->string+8,4);
        memcpy(fileid+14,sto->string+12,4);
        memcpy(fileid+19,sto->string+16,4);
        memcpy(fileid+24,sto->string+20,12);
        if (sto)
        {
            c_free(&sto->string);
        }
    }


    char filename[FILE_NAME_LENGTH]={0};
    char pathname[FILE_NAME_LENGTH]={0};
    sprintf(filename , "%s%s%s%s", session_stream , "/" , fileid, ".dlp");
    /*根据组号和相对位置，将该流写到对应的位置上，如果还没有该文件，则先建立该文件，如果已经存在该文件，则直接写入到对应位置*/
    FILE *fp = fopen(filename, "r+");
    if(fp == NULL)
    {
        fp = fopen(filename, "w+");
        if(fp == NULL){
            log_printf(ZLOG_LEVEL_ERROR, "cant open the fileid !!!!!\n");
            goto QQattach2_fail;
        }
        fseek(fp ,  (mcnt * 1048576) + moff , SEEK_SET );
        fwrite(StoMB->string , StoMB->len , 1 ,fp);
        fclose(fp);
        fp=NULL;
        if(sto){
            free(sto);
            sto = NULL;
        }
        return 0;
    }
    fseek(fp ,  (mcnt * 1048576) + moff , SEEK_SET );
    fwrite(StoMB->string , StoMB->len , 1 ,fp);
    fclose(fp);
    fp=NULL;
    /*如果检测到最后一包文件则生成jansson串*/
    if( (mcnt * 1048576) + moff + StoMB->len == uploadsize){
        //truncate(fileid , uploadsize);
        add_att_nodelist(webmail->filenode, uploadsize, "nofilename", NULL);
        sprintf(pathname, "%s%s%s", session_stream , "/" , fileid);
        rename(filename, pathname);
        webmail->state = mail_end;
        memcpy(webmail->uuid,fileid,UUID_LENGTH);
    }

    if(sto){
        free(sto);
        sto = NULL;
    }
    log_printf(ZLOG_LEVEL_DEBUG, "QQattach2 end ....");
    return 0;
QQattach2_fail://lj
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "QQattach2 fail end ....");
    return -1;//lj
}

/**
 * @brief QQattachF 分析通过qq邮箱发送附件，并且时基于mime格式
 *
 * @param dlp_http      经提取后的http头
 * @param StoCK         cookie信息
 * @param StoMB         待分析的数据体
 * @param webmail       存储邮件的相应信息
 */
int QQattachF(HttpSession* dlp_http,webmail_string_and_len *StoCK,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "QQattachF start ....");
    if(dlp_http ==NULL|| StoCK ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
    dlp_http_post_head *http_session;
    http_session = (dlp_http_post_head *)dlp_http->http;

    /*临时存储经过正则匹配，匹配到的信息*/
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL|| sto1 == NULL)  goto QQattachF_fail;

    /*提取发件人的信息*/
    const char pattern1[PATTERN_LENGTH]="(?<=qqmail_alias=)[\\w\\W]*?(?=;)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoCK,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto QQattachF_fail;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }

    /*以下两种情况均直接通过一个数据包发完附件*/
    /*如果有file_name项，则整个数据体都为附件内容,并将这个数据体存储为文件，并生成jansson串*/
    if(http_session->file_name[0]){
        char uuid_buf[UUID_LENGTH]={};
        uuid_t randNumber;
        uuid_generate(randNumber);
        uuid_unparse(randNumber, uuid_buf);
        char filename[FILE_NAME_LENGTH]={0};
        sprintf(filename , "%s%s%s", session_stream , "/" , uuid_buf);

        FILE *fp = NULL;
        fp = fopen(filename, "w+");
        if(fp == NULL){
            log_printf(ZLOG_LEVEL_ERROR, "When we enter into QQattachF,can not open the file!\n");
            goto QQattachF_fail;//lj
        }
        fwrite( StoMB->string , StoMB->len , 1 , fp);
        fclose(fp);
        fp=NULL;
        add_att_nodelist(webmail->filenode,StoMB->len,http_session->file_name,uuid_buf);
        webmail->state = build_jansson;
    }

    /*如果没有file_name项，则看其是否为mime格式的，如果是的话将附件提取出来，并生成jansson串*/
    else if(strstr(http_session->content_type, "boundary")){
        memcpy(webmail->boundary,http_session->boundary,MAX_BOUNDARY_LEN);
        mch_att(&StoMB,webmail);
        webmail->state = build_jansson;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&sto->string);//lj
        free(sto);
        sto = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "QQattachF end ....");
    return 0;
QQattachF_fail://lj
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "QQattachF end ....");
    return -1;//lj
}
/**
 * @brief sina_mail     分析通过新浪邮箱发送邮件
 *
 * @param StoPT
 * @param StoBY
 * @param StoMB
 * @param webmail
 *
 * @return
 */
int sina_mail(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "sina_mail start ....");
    if(StoPT==NULL || StoBY==NULL || StoMB==NULL || webmail ==NULL)
        return -1;
    /* sto      用来临时存储正则匹配的结果*/
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL || sto1 ==NULL)goto sina_mail_fail;//lj

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    char pattern1[PATTERN_LENGTH]="(?<=\"from\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto sina_mail_fail   ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    char pattern2[PATTERN_LENGTH]="(?<=\"to\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    char pattern3[PATTERN_LENGTH]="(?<=\"cc\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    char pattern4[PATTERN_LENGTH]="(?<=\"bcc\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取发送时间*/
    const char pattern7[PATTERN_LENGTH]="(?<=ts=)\\d*?(?= HTTP/)";
    if(mch(&StoPT,&sto,pattern7)==EXIT_SUCCESS){
        char s[MAX_TIME_LEN];
        memset(s,'\0',sizeof(s));
        /*将时间转换为规定的格式*/
        time_convert(sto->string,s);
        memcpy(webmail->Time,s,strlen(s));
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取主题，并对应存储*/
    char pattern5[PATTERN_LENGTH]="(?<=\"subj\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(c_malloc((void**)&webmail->Subject,sto->len+1) ==-1 ) goto sina_mail_fail;//lj
        memcpy(webmail->Subject ,sto->string,sto->len);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    char pattern6[PATTERN_LENGTH] = "(?<=\"msgtxt\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(c_malloc((void**)&webmail->Content,sto->len+1) ==-1 ) goto sina_mail_fail;//lj
        memcpy(webmail->Content,sto->string,sto->len);
        if(sto){
            c_free(&(sto->string));
        }
    }
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    mch_att(&StoMB,webmail);
    if(sto){
        free(sto);
        sto=NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "sina_mail end ....");
    return 0;//lj

sina_mail_fail://lj
    log_printf(ZLOG_LEVEL_ERROR, "sina_mail fail end ....");
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    return -1;//lj

}
/**
 * @brief sina_attach   提取通过新浪邮箱并且为MIME格式发出的附件
 *
 * @param StoPT         URL
 * @param StoMB         待提取的数据体
 * @param webmail       存储邮件的信息
 * @param StoBY         MIME格式的boundary
 *
 * @return
 */
int sina_attach(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "sina_attach start ....");
    if(StoPT ==NULL|| StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    /*临时存储正则匹配的结果*/
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL|| sto1==NULL) goto sina_attach_fail;//lj

    /*将附件提取出*/
    memcpy(webmail->boundary,StoBY->string,StoBY->len);/*首先从StoBY提取出boundary，便于之后mch_att函数进行处理MIME传输的附件*/
    mch_att(&StoMB,webmail);

    /*提取发件人，并存放到webmail_From中,有两种形式*/
    const char pattern1[PATTERN_LENGTH]="(?<=email=)[\\w\\W]*?(?=&token=)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoPT,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto  sina_attach_fail  ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }
    const char pattern2[PATTERN_LENGTH]="(?<=uid=)[\\w\\W]*?(?=&pid=)";
    if(webmail->From==NULL&&mch(&StoPT,&sto1,pattern2)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto  sina_attach_fail  ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "sina_attach end ....");
    return 0;
sina_attach_fail:
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&(sto->string));//lj
        free(sto);
        sto =NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "sina_attach fail end ....");
    return -1;

}

#if 1
/**
 * @brief sohu_mail 提取出通过搜狐邮箱发送的邮件
 *
 * @param StoMB     待检测的数据体
 * @param webmail   存储邮件的信息
 */
int sohu_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "sohu_mail start ....");
    if(StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化正则匹配所需要的变量*/
    /*
     * sto      用来临时存储正则匹配的结果
     * tomail   用来存放收件人
     * subject  用来存放主题
     * content  用来存放内容
     */
    webmail_string_and_len  *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL||sto1==NULL)goto sohu_mail_fail;//lj
    char *subject=NULL;
    char *content=NULL;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern1[PATTERN_LENGTH] = "(?<=from=)[\\w\\W]*?(?=&)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            web_URLDecode(sto1->string,sto1->len,sto1->string,sto1->len );
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) == -1) goto sohu_mail_fail;//lj
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto->string));
                c_free(&(sto1->string));
            }
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern2[PATTERN_LENGTH] = "(?<=to=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern3[PATTERN_LENGTH] = "(?<=&cc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern4[PATTERN_LENGTH] = "(?<=bcc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取主题，并对应存储*/
    const char pattern5[PATTERN_LENGTH] = "(?<=subject=)[\\w\\W]*?(?=&)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&subject,sto->len+1) ==-1 ) goto sohu_mail_fail;//lj
            web_URLDecode(sto->string,sto->len,subject,sto->len );

            if(c_malloc((void**)&(webmail->Subject),strlen(subject)+1) ==-1)goto sohu_mail_fail;//lj
            memcpy(webmail->Subject ,subject,strlen(subject));
            c_free(&subject);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    const char pattern6[PATTERN_LENGTH] = "(?<=text=)[\\w\\W]*?(?=&)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(sto->len){
#if 0       /*换码*/
            if(c_malloc((void**)&content,sto->len+1) ==-1 ) goto sohu_mail_fail;//lj
            web_URLDecode(sto->string,sto->len,content,sto->len );
            if(c_malloc((void**)&(webmail->Content),strlen(content)+1) ==-1)goto sohu_mail_fail;//lj
            memcpy(webmail->Content,content,strlen(content));
            c_free(&content);
#endif
            /*不转码*/
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto sohu_mail_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        free(sto);
        sto = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "sohu_mail end ....");
    return 0;//lj

sohu_mail_fail://lj
    log_printf(ZLOG_LEVEL_ERROR, "sohu_mail fail end ....");
    c_free(&content);
    c_free(&subject);
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    return -1;//lj
}
#endif
/**
 * @brief sohu_attach   通过搜狐邮箱发送附件
 *
 * @param StoPT         URL
 * @param StoBY         MIME格式的boundary
 * @param StoMB         待检测的数据体
 * @param webmail       存储邮件的信息
 */
int sohu_attach(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "sohu_attach start ....");
    if(StoPT ==NULL|| StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    webmail_string_and_len *sto=init_webmail_string_and_len();
    if(sto == NULL) return -1;//lj

    /*提取出附件*/
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    mch_att(&StoMB,webmail);

    /*提取发送时间*/
    const char pattern1[PATTERN_LENGTH]="(?<=X-Progress-ID=)[0-9]{10}";
    if(mch(&StoPT,&sto,pattern1)==EXIT_SUCCESS){
        char s[MAX_TIME_LEN];
        memset(s,'\0',sizeof(s));
        time_convert(sto->string,s);
        memcpy(webmail->Time,s,strlen(s));
        if(sto){
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "sohu_attach end ....");
    return 0;
}
/**
 * @brief sohu_attach2
 * 通过搜狐邮箱发送附件，并经由搜狐优盘的方式,并且是通过多个会话多个包来发送附件
 *
 * @param dlp_http      经提取后的http头
 * @param StoPT         URL
 * @param StoMB         待分析数据体
 * @param webmail       存储邮件的相应信息
 */
int sohu_attach2(HttpSession* dlp_http,webmail_string_and_len *StoPT,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "sohu_attach2 start ....");
    if(dlp_http ==NULL|| StoPT ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化所需要的变量*/
    /*
     * sto          临时存放正则匹配的结果
     * uploadsize   上传附件的大小
     * fileid       拼接完成后通过uuid存储的文件的名字
     * filename     上传文件的名字
     * nowlength    现在已经拼接的文件长度
     * seek         偏移量
     */

    webmail_string_and_len *sto=init_webmail_string_and_len();
    if(sto == NULL) goto sohu_attach2_fail;//lj
    int uploadsize=0;
    char fileid[UUID_LENGTH]={};
    FILE *fl = NULL;
    fileid[8]='-';
    fileid[13]='-';
    fileid[18]='-';
    fileid[23]='-';
    char file_name[FILE_NAME_LENGTH]={0};
    int now_length = 0;
    int seek = 0;
    char pattern_filesize[PATTERN_LENGTH]="(?<=filesize=)[\\W\\w]*?(?=&)";
    char pattern_filename[PATTERN_LENGTH]="(?<=filename=)[\\W\\w]*?(?=&)";
    char pattern_id[PATTERN_LENGTH]="(?<=xid=)[\\W\\w]*?(?=&)";

    /*提取出文件的uuid并转换为标准的形式*/
    if(mch(&StoPT,&sto,pattern_id)==EXIT_SUCCESS){
        memcpy(fileid,sto->string,8);
        memcpy(fileid+9,sto->string+8,4);
        memcpy(fileid+14,sto->string+12,4);
        memcpy(fileid+19,sto->string+16,4);
        memcpy(fileid+24,sto->string+20,12);
        if(sto){
            c_free(&sto->string);
        }
    }

    /*构造出存储路径*/
    char filename[FILE_NAME_LENGTH]={0};
    char pathname[FILE_NAME_LENGTH]={0};
    sprintf(filename , "%s%s%s%s", session_stream , "/" , fileid, ".dlp");

    /*提取出偏移量*/
    char pattern_seek[PATTERN_LENGTH]="(?<=seek=)[\\W\\w]*?(?=&)";
    if(mch(&StoPT,&sto,pattern_seek)==EXIT_SUCCESS){
        seek=atoi(sto->string);
        if(sto){
            c_free(&sto->string);
        }
    }

    FILE *fp = fopen(filename, "r+");
    /*如果还没有存储过该文件，则创建该文件,并将该数据流写到对应的位置*/
    if(fp == NULL)
    {
        fp = fopen(filename, "w+");
        if(fp == NULL && fl == NULL){
            log_printf(ZLOG_LEVEL_ERROR, "cant open the fileid !!!!!\n");
            goto sohu_attach2_fail;//lj
        }
        fseek(fp ,seek , SEEK_SET );
        fwrite(StoMB->string , StoMB->len , 1 ,fp);
        fclose(fp);
        fp = NULL;
        if(sto){
            free(sto);
            sto = NULL;
        }
        return 0;
    }

    /*如果已经存在该文件，则将该数据流直接写到对应位置*/
    fseek(fp ,seek , SEEK_SET );
    fwrite(StoMB->string , StoMB->len , 1 ,fp);
    fclose(fp);
    fp = NULL;

    /*提取出文件的大小*/
    if(mch(&StoPT,&sto,pattern_filesize)==EXIT_SUCCESS){
        uploadsize = atoi(sto->string);
        FILE *fp = fopen(filename, "r+");
        if(fp == NULL){//lj
            goto sohu_attach2_fail;//lj
        }
        if(fp != NULL){
            fseek(fp, 0, SEEK_END);
            now_length = ftell(fp);
            fclose(fp);
            fp = NULL;
        }
        if(sto){
            c_free(&sto->string);
        }
    }

    /*如果现在文件的大小等于文件的大小，则上报jansson串*/
    if((now_length  == uploadsize)&&(now_length != 0)){
        if(mch(&StoPT,&sto,pattern_filename) == EXIT_SUCCESS){
            web_URLDecode(sto->string,sto->len,file_name,sto->len);
            if(sto){
                c_free(&sto->string);
            }
        }
        add_att_nodelist(webmail->filenode, uploadsize,file_name, NULL);
        webmail->state = mail_end;
        char uuid[UUID_LENGTH]={0};
        uuid_t  sohu_randNumber;
        uuid_generate(sohu_randNumber);
        uuid_unparse(sohu_randNumber, uuid);
        /*将名字改为标准uuid的形式*/
        sprintf(pathname, "%s%s%s", session_stream , "/" , uuid);
        rename(filename, pathname);
        memcpy(webmail->uuid,uuid,UUID_LENGTH);
    }

    if(sto){
        free(sto);
        sto = NULL;
    }
    return 0;
sohu_attach2_fail:
    log_printf(ZLOG_LEVEL_ERROR, "sohu_attach2 fail end ....");
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    return -1;
}
/**
 * @brief tom_mail extract  提取tom邮箱的邮件
 *
 * @param StoMB             待检测的数据体
 * @param webmail           存储邮件的信息
 */
int tom_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "tom_mail start ....");
    if(StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化正则匹配所需要的变量*/
    /*
     * sto      用来临时存储正则匹配的结果
     * tomail   用来存放收件人
     * subject  用来存放主题
     * content  用来存放内容
     */
    webmail_string_and_len  *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL || sto1==NULL)goto tom_mail_fail;//lj
    char *subject=NULL;
    char *content=NULL;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern1[PATTERN_LENGTH] = "(?<=from=)[\\w\\W]*?(?=&)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            web_URLDecode(sto1->string,sto1->len,sto1->string,sto1->len );
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) == -1) goto tom_mail_fail;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto->string));
                c_free(&(sto1->string));
            }
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern2[PATTERN_LENGTH] = "(?<=to=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern3[PATTERN_LENGTH] = "(?<=&cc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern4[PATTERN_LENGTH] = "(?<=bcc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取主题，并对应存储*/
    const char pattern5[PATTERN_LENGTH] = "(?<=subject=)[\\w\\W]*?(?=&)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&subject,sto->len+1) ==-1 ) goto tom_mail_fail;//lj
            web_URLDecode(sto->string,sto->len,subject,sto->len );
            if(c_malloc((void**)&(webmail->Subject),strlen(subject)+1) ==-1)goto tom_mail_fail;//lj
            memcpy(webmail->Subject ,subject,strlen(subject));
            c_free(&subject);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    const char pattern6[PATTERN_LENGTH] = "(?<=text=)[\\w\\W]*?(?=&)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(sto->len){
#if 0       /*换码*/
            if(c_malloc((void**)&content,sto->len+1) ==-1 ) goto tom_mail_fail;//lj
            web_URLDecode(sto->string,sto->len,content,sto->len );
            if(c_malloc((void**)&(webmail->Content),strlen(content)+1) ==-1)goto tom_mail_fail;//lj
            memcpy(webmail->Content,content,strlen(content));
            c_free(&content);
#endif
            /*不转码*/
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto tom_mail_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto = NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "tom_mail end ....");
    return 0;//lj

tom_mail_fail://lj
    c_free(&content);
    c_free(&subject);
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "tom_mail fail end ....");
    return -1;//lj

}
/**
 * @brief tom_attach extract the
 * 提取通过tom邮箱发送的附件,并且是通过MIME格式发送
 *
 * @param StoPT         URL
 * @param StoBY         MIME格式的boundary
 * @param StoMB         待检测的数据体
 * @param webmail       存储邮件的信息
 */
int tom_attach(webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "tom_attach start ....");
    if(StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    mch_att(&StoMB,webmail);
    return 0;
}
/**
 * @brief cn_mail       提取通过21cn邮箱发出的邮件
 *
 * @param StoMB         待检测的数据体
 * @param webmail       存储邮件的信息
 */
int cn_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "cn_mail start ....");
    if(StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化sto变量来存储正则匹配的结果*/
    webmail_string_and_len *sto=init_webmail_string_and_len();
    if(sto == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into cn_mail,alloc memory sto failed!!\n");
    }
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto1 == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into cn_mail,alloc memory sto1 failed!!\n");
    }
    webmail_string_and_len *sto2=init_webmail_string_and_len();
    if(sto2 == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into cn_mail,alloc memory sto2 failed!!\n");
    }
    webmail_string_and_len *sto3=init_webmail_string_and_len();
    if(sto3 == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into cn_mail,alloc memory sto3 failed!!\n");
    }
    if(sto==NULL || sto1==NULL || sto2==NULL || sto3 == NULL)goto cn_mail_fail;//lj

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    char pattern1[PATTERN_LENGTH]="(?<=\"sender\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 )goto cn_mail_fail;//lj
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    char pattern2[PATTERN_LENGTH]="(?<=\"to\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    char pattern3[PATTERN_LENGTH]="(?<=\"cc\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    char pattern4[PATTERN_LENGTH]="(?<=\"bcc\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则提取出发送的时间并转换为标准的格式*/
    char pattern7[PATTERN_LENGTH]="(?<=\"_year\"\\r\\n\\r\\n)\\d+";
    char pattern8[PATTERN_LENGTH]="(?<=\"_month\"\\r\\n\\r\\n)\\d+";
    char pattern9[PATTERN_LENGTH]="(?<=\"_day\"\\r\\n\\r\\n)\\d+";
    char pattern10[PATTERN_LENGTH]="(?<=\"_hour\"\\r\\n\\r\\n)\\d+";
    if(mch(&StoMB,&sto,pattern7)==EXIT_SUCCESS&&mch(&StoMB,&sto1,pattern8)==EXIT_SUCCESS&&mch(&StoMB,&sto2,pattern9)==EXIT_SUCCESS&&mch(&StoMB,&sto3,pattern10)==EXIT_SUCCESS){
        char namebuf[MAX_TIME_LEN];
        memset(namebuf,'\0',sizeof(namebuf));
        memcpy(namebuf,sto->string,sto->len);
        strcat(namebuf,"-");
        strcat(namebuf,sto1->string);
        strcat(namebuf,"-");
        strcat(namebuf,sto2->string);
        strcat(namebuf," ");
        strcat(namebuf,sto3->string);
        strcat(namebuf,":00:00");
        memcpy(webmail->Time,namebuf,20);

        if(sto){
            c_free(&(sto->string));
        }
        if(sto1){
            c_free(&(sto1->string));
        }
        if(sto2){
            c_free(&(sto2->string));
        }
        if(sto3){
            c_free(&(sto3->string));
        }
    }

    /*提取主题，并对应存储*/
    char pattern5[PATTERN_LENGTH]="(?<=\"subject\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(c_malloc((void**)&webmail->Subject,sto->len+1) ==-1) goto cn_mail_fail;//lj
        memcpy(webmail->Subject ,sto->string,sto->len);
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    char pattern6[PATTERN_LENGTH] = "(?<=\"content\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto cn_mail_fail;
        memcpy(webmail->Content,sto->string,sto->len);
        if(sto){
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    if(sto1){
        free(sto1);
        sto1=NULL;
    }
    if(sto2){
        free(sto2);
        sto2=NULL;
    }
    if(sto3){
        free(sto3);
        sto3=NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "cn_mail end ....");
    return 0;
cn_mail_fail:

    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto2){
        c_free(&(sto2->string));
        free(sto2);
        sto = NULL;
    }
    if(sto3){
        c_free(&(sto3->string));
        free(sto3);
        sto3 = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "cn_mail fail end ....");
    return -1;

}
/**
 * @brief cn_attach 提取通过21cn邮箱发送出的MIME格式的附件
 *
 * @param StoBY
 * @param StoMB
 * @param webmail
 */
int cn_attach(webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "cn_attach start ....");
    if(StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    mch_att(&StoMB,webmail);

    return 0;
}
/**
 * @brief mail_139      提取通过139邮箱发送出的邮件
 *
 * @param StoCK         cookie头
 * @param StoMB         待提取的数据体
 * @param webmail       存储邮件的相应信息
 */
int mail_139(webmail_string_and_len *StoCK,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "mail_139 start ....");
    if(StoCK ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化正则匹配所需要的变量*/
    /*
     * sto      用来临时存储正则匹配的结果
     */
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL || sto1==NULL) goto mail_139_fail;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern2[PATTERN_LENGTH] = "(?<=<string name=\"account\">)[\\w\\W]*?(?=</string>)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1, pattern2)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto  mail_139_fail  ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern3[PATTERN_LENGTH] = "(?<=<string name=\"to\")[\\w\\W]*?(?=/)";
    if(mch(&StoMB,&sto, pattern3)==EXIT_SUCCESS){
        if(sto->len){
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern4[PATTERN_LENGTH] = "(?<=<string name=\"cc\")[\\w\\W]*?(?=/)";
    if(mch(&StoMB,&sto, pattern4)==EXIT_SUCCESS){
        if(sto->len){
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }


    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern5[PATTERN_LENGTH] = "(?<=<string name=\"bcc\")[\\w\\W]*?(?=/)";
    if(mch(&StoMB,&sto, pattern5)==EXIT_SUCCESS){
        if(sto->len){
            webmail_to_parser(sto , webmail->To);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取发送时间*/
    const char pattern8[PATTERN_LENGTH]="(?<=lv=)[0-9]{10}";
    if(mch(&StoCK,&sto,pattern8)==EXIT_SUCCESS){
        char s[MAX_TIME_LEN];
        memset(s,'\0',sizeof(s));
        /*将时间转换为规定的格式*/
        time_convert(sto->string,s);
        memcpy(webmail->Time,s,strlen(s));
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取主题，并对应存储*/
    const char pattern6[PATTERN_LENGTH] = "(?<=<string name=\"subject\">)[\\w\\W]*?(?=</string>)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto, pattern6)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&webmail->Subject,sto->len+1) ==-1) goto mail_139_fail;//lj
            memcpy(webmail->Subject , sto->string,sto->len);
        }
        if(sto){
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    const char pattern7[PATTERN_LENGTH] = "(?<=<string name=\"content\">)[\\w\\W]*?(?=</string>)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto, pattern7)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&webmail->Content,sto->len+1) ==-1) goto mail_139_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }

        if(sto){
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "mail_139 end ....");
    return 0;//lj
mail_139_fail:
    log_printf(ZLOG_LEVEL_ERROR, "mail_139 fail end ....");
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    return -1;
}
/**
 * @brief attach_139 提取通过139云盘传输的附件
 *
 * @param StoBY         boundary
 * @param StoMB         待提取的数据体
 * @param webmail       存储邮件的信息
 */
int attach_139(webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "attach_139 start ....");
    if(StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    webmail_string_and_len *sto=init_webmail_string_and_len();
    if(sto == NULL) return -1;//lj

    /*提取附件*/
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    mch_att_139_multi_session(StoMB , StoBY , webmail);

    /*提取时间戳*/
    char pattern1[PATTERN_LENGTH]="(?<=\"timestamp\"\\r\\n\\r\\n)[0-9]{10}";
    if(mch(&StoMB,&sto,pattern1)==EXIT_SUCCESS){
        char s[MAX_TIME_LEN];
        memset(s,'\0',sizeof(s));
        time_convert(sto->string,s);
        memcpy(webmail->Time,s,strlen(s));
        if(sto){
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    return 0;
}
/**
 * @brief attach2_139 提取通过多个post请求来传递的附件
 *
 * @param dlp_http      经提取后的http头
 * @param StoPT         URL
 * @param StoMB         待分析数据体
 * @param webmail       存储分析邮件后的相应信息
 * @param StoBY         MIME格式的boundary
 */
int attach2_139(HttpSession* dlp_http, webmail_string_and_len *StoPT, webmail_string_and_len *StoMB, webmail_string_and_len *StoBY,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "attach2_139 start ....");
    if(dlp_http ==NULL|| StoPT ==NULL|| StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
    dlp_http_post_head *http_session;
    http_session = (dlp_http_post_head *)dlp_http->http;

    /*通过四元组来生成hash值，来作为文件名来保存还未拼接完整的文件*/
    char tmp[128]= {};
    char tuple_name[128] = {};
    char filename[FILE_NAME_LENGTH]={0};
    char pathname[FILE_NAME_LENGTH]={0};
    get_tuple_name(dlp_http , tuple_name);
    sprintf(filename , "%s%s%s%s", session_stream , "/" , tuple_name , ".dlp");

    /*初始化所需要的变量*/
    /*
     *  now_length          现在已经拼接好的长度
     *  mail_upload_name    上传的文件的名字
     *  mail_upload_size    上传的文件的大小
     *  sto                 临时存储所提取信息
     */
    int now_length = 0;
    int mail_upload_size = 0;
    char *mail_upload_name =NULL;
    webmail_string_and_len *sto = init_webmail_string_and_len();
    if(sto ==NULL)return -1;

    /*提取出文件大小*/
    char pattern_filesize[PATTERN_LENGTH] = "(?<=name=\"filesize\"\\r\\n\\r\\n)[^\\r]*?(?=\\r\\n--)";
    if(mch(&StoMB, &sto, pattern_filesize ) == EXIT_SUCCESS){
        mail_upload_size = atoi(sto->string);
        if(sto){
            c_free(&sto->string);
        }
    }
    else
    {
        if(sto){
            c_free(&sto->string);
            free(sto);
            sto = NULL;
        }
        goto attach2_139_fail;
    }

    FILE *fp=NULL;
    /*如果之前并未存储过该文件的流，则以此hash值创建该文件*/
    if(access(filename, F_OK) < 0){
        fp = fopen(filename, "w+");
        if(fp == NULL)
        {
            log_printf(ZLOG_LEVEL_ERROR, "the file cant open in the attach2_139!\n");
            goto attach2_139_fail;
        }
        fclose(fp);
        fp=NULL;
    }

    /*如果存在该文件，则继续向该文件拼接文件*/
    fp = fopen(filename, "a+");
    if(fp == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "the file cant open in the attach2_139!\n");
        goto attach2_139_fail;
    }
    fseek(fp, 0, SEEK_END );
    now_length = ftell(fp);
    /*提取文件*/
    mch_att_139(&StoMB, &sto, StoBY, webmail , &mail_upload_name);
    if(sto->len == 0)
        goto attach2_139_fail;
    if(sto->string){
        fwrite(sto->string,  sto->len, 1 , fp);
    }
    fclose(fp);
    fp=NULL;
    /*如果已经存储的文件大小等于附件的大小，则将其更名为标准的uuid形式，并上报jansson串*/
    if (now_length + sto->len == mail_upload_size && mail_upload_size != 0) {
        webmail->state = mail_end;
        add_att_nodelist(webmail->filenode, mail_upload_size, mail_upload_name, NULL);
        char uuid[UUID_LENGTH] = {};
        uuid_t  randNumber_139;
        uuid_generate(randNumber_139);
        uuid_unparse(randNumber_139, uuid);
        /*将名字改为标准uuid的形式*/
        sprintf(pathname, "%s%s%s", session_stream , "/" , uuid);
        rename(filename, pathname);
        memcpy(webmail->uuid,uuid, UUID_LENGTH);
    }
    /*否则仅拼接文件，不上报jansson串*/
    else {
        webmail->state = mail_ing;
    }
    sto->len = 0;
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    c_free(&mail_upload_name);
    //	log_printf(ZLOG_LEVEL_DEBUG, "attach2_139 end ....");
    return 0;
attach2_139_fail://lj
    log_printf(ZLOG_LEVEL_ERROR, "attach2_139 fail end ....");
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    c_free(&mail_upload_name);
    if(fp){
        fclose(fp);
        fp = NULL;
    }
    return -1;
}
/**
 * @brief attach3_139   解析通过139云盘上传附件，但不是mime格式
 *
 * @param dlp_http      经提取后的http头
 * @param StoMB         待分析数据体
 * @param webmail       存储邮件的相应信息
 *
 * @return
 */
int attach3_139(HttpSession* dlp_http,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "attach3_139 start ....");
    if(dlp_http ==NULL||StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化所需要的变量*/
    /*
     *  file_name            文件的名称
     *  sto                 临时存储所提取信息
     *  StoTY               临时存储所提取信息,内容类型
     */
    webmail_string_and_len *sto = init_webmail_string_and_len();
    webmail_string_and_len *StoTY = init_webmail_string_and_len();
    char *file_name=NULL;//lj
    FILE *fp=NULL;
    if(sto==NULL || StoTY == NULL) goto attach3_139_fail;//lj

    /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
    dlp_http_post_head *http_session;
    http_session = (dlp_http_post_head *)dlp_http->http;

    /*提取出内容的类型*/
    if(http_session->content_type){
        if(c_malloc((void**)&StoTY->string,strlen(http_session->content_type)+1) == -1) goto attach3_139_fail;
		memcpy(StoTY->string,http_session->content_type,strlen(http_session->content_type));
		StoTY->len=strlen(http_session->content_type);
    }

    /*提取文件名*/
    const char pattern_filename[PATTERN_LENGTH]="(?<=name=)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoTY,&sto,pattern_filename)==EXIT_SUCCESS){
        /*通过四元组来生成hash值，来作为文件名来保存还未拼接完整的文件*/
        char tmp[128]= {};
        char tuple_name[128] = {};
        char filename[FILE_NAME_LENGTH]={0};
        char pathname[FILE_NAME_LENGTH]={0};
        get_tuple_name(dlp_http , tuple_name);
        sprintf(filename , "%s%s%s%s", session_stream , "/" , tuple_name , ".dlp");

        /*初始化所需要的变量*/
        /*
         *  now_length          现在已经拼接好的长度
         *  mail_upload_name    上传的文件的名字
         *  mail_upload_size    上传的文件的大小
         *  sto                 临时存储所提取信息
         */
        int now_length = 0;
        int mail_upload_size = 0;
        char mail_upload_name[1024] = {};
        /*做对应提取，并将结果放到上述变量中*/
        if(c_malloc((void**)&file_name,sto->len+1) ==-1) goto attach3_139_fail;
        web_URLDecode(sto->string,sto->len,file_name,sto->len);
        memcpy(mail_upload_name ,file_name,strlen(file_name));
        c_free(&file_name);
        if(sto){
            c_free(&sto->string);
        }
        mail_upload_size = http_session->contentSize;

        FILE *fp;
        /*如果之前并未存储过该文件的流，则以此hash值创建该文件*/
        if(access(filename, F_OK) < 0){
            fp = fopen(filename, "w+");
            if(fp == NULL)
            {
                log_printf(ZLOG_LEVEL_ERROR, "the file cant open in the attach2_139!\n");
                goto attach3_139_fail;
            }
            fclose(fp);
            fp=NULL;
        }
        /*如果存在该文件，则继续向该文件拼接文件*/
        fp = fopen(filename, "a+");
        if(fp == NULL)
        {
            log_printf(ZLOG_LEVEL_ERROR, "the file cant open in the attach2_139!\n");
            goto attach3_139_fail;
        }
        fseek(fp, 0, SEEK_END );
        now_length = ftell(fp);
        fwrite(StoMB->string,  http_session->content_length, 1, fp);
        fclose(fp);
        fp=NULL;

        /*如果已经存储的文件大小等于附件的大小，则将其更名为标准的uuid形式，并上报jansson串*/
        if (now_length + http_session->content_length ==  mail_upload_size && mail_upload_size != 0) {
            webmail->state = mail_end;
            add_att_nodelist(webmail->filenode, mail_upload_size, mail_upload_name, NULL);
            char uuid[UUID_LENGTH] = {};
            uuid_t  randNumber_139;
            uuid_generate(randNumber_139);
            uuid_unparse(randNumber_139, uuid);
            /*将名字改为标准uuid的形式*/
            sprintf(pathname, "%s%s%s", session_stream , "/" , uuid);
            rename(filename, pathname);
            memcpy(webmail->uuid,uuid, UUID_LENGTH);
        }
        /*否则仅拼接文件，不上报jansson串*/
        else {
            webmail->state = mail_ing;
        }
        if (StoTY)
        {
            c_free(&StoTY->string);
        }
    }
    if(sto){
        free(sto);
        sto = NULL;
    }
    if(StoTY){
        free(StoTY);
        StoTY = NULL;
    }
    return 0;
attach3_139_fail:
    log_printf(ZLOG_LEVEL_ERROR, "attach3_139 fail end ....");
    c_free(&file_name);
    if(sto){
        c_free(&sto->string);
        free(sto);
        sto = NULL;
    }
    if(StoTY){
        c_free(&StoTY->string);
        free(StoTY);
        StoTY = NULL;
    }
    if(fp){
        fclose(fp);
        fp = NULL;
    }

    return -1;

}
/**
 * @brief mail_189      解析通过189邮箱发送邮件的流
 *
 * @param StoMB     待检测的数据体
 * @param webmail   存储邮件的信息
 */
int mail_189(webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "mail_189 start ....");
    if(StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化正则匹配所需要的变量*/
    /*
     * sto      用来临时存储正则匹配的结果
     * tomail   用来存放收件人
     * subject  用来存放主题
     * content  用来存放内容
     */
    webmail_string_and_len  *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if (sto == NULL||sto1==NULL) goto mail_189_fail;//lj
    char*content=NULL;
    char*subject=NULL;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern1[PATTERN_LENGTH] = "(?<=from=)[\\w\\W]*?(?=&)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            web_URLDecode(sto1->string,sto1->len,sto1->string,sto1->len );
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) == -1) goto mail_189_fail;//lj
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto->string));
                c_free(&(sto1->string));
            }
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern2[PATTERN_LENGTH] = "(?<=to=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern3[PATTERN_LENGTH] = "(?<=&cc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern4[PATTERN_LENGTH] = "(?<=bcc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取发送时间*/
    const char pattern7[PATTERN_LENGTH] = "(?<=attachmentList=)[0-9]{10}";
    if(mch(&StoMB,&sto,pattern7)==EXIT_SUCCESS){
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

    /*提取主题，并对应存储*/
    const char pattern5[PATTERN_LENGTH] = "(?<=subject=)[\\w\\W]*?(?=&)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&subject,sto->len+1) ==-1) goto mail_189_fail;
            web_URLDecode(sto->string,sto->len,subject,sto->len );

            if(c_malloc((void**)&webmail->Subject,sto->len+1) ==-1) goto mail_189_fail;
            memcpy(webmail->Subject ,subject,strlen(subject));
            c_free(&subject);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    const char pattern6[PATTERN_LENGTH] = "(?<=content=)[\\w\\W]*?(?=&)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(sto->len){
#if 0       /*换码*/
            if(c_malloc((void**)&content,sto->len+1) ==-1)goto mail_189_fail;//lj
            web_URLDecode(sto->string,sto->len,content,sto->len );
            if(c_malloc((void**)&webmail->Content,sto->len+1) ==-1) goto mail_189_fail;//lj
            memcpy(webmail->Content,content,strlen(content));
            c_free(&content);
#endif
            /*不转码*/
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto mail_189_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        free(sto);
        sto = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "mail_189 end ....");
    return 0;
mail_189_fail://lj
    log_printf(ZLOG_LEVEL_ERROR, "mail_189 fail end ....");
    c_free(&content);
    c_free(&subject);
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    return -1;

}
/**
 * @brief attach_189        解析通过189邮箱发送附件
 *
 * @param StoPT         URL
 * @param StoBY         MIME格式的boundary
 * @param StoMB         待检测的数据体
 * @param webmail       存储邮件的信息
 */
int attach_189(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "attach_189 start ....");
    if(StoPT ==NULL|| StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL|| sto1==NULL ) goto attach_189_fail;//lj

    /*提取附件*/
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    mch_att(&StoMB,webmail);

    /*提取发件人*/
    const char pattern1[PATTERN_LENGTH] = "(?<=189ACCOUNT=)[\\w\\W]*?(189.cn)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoPT,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto   attach_189_fail ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "attach_189 end ....");
    return 0;
attach_189_fail:
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "attach_189 fail end ....");
    return -1;
}
/**
 * @brief cnpc_mail     提取通过中国石油邮箱发送的邮件
 *
 * @param StoPT
 * @param StoMB
 * @param webmail
 */
int cnpc_mail(webmail_string_and_len *StoPT,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "cnpc_mail start ....");
    if(StoPT ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;

    /*初始化正则匹配所需要的变量*/
    /*
     * sto      用来临时存储正则匹配的结果
     * tomail   用来存放收件人
     * subject  用来存放主题
     * content  用来存放内容
     */
    webmail_string_and_len  *sto=init_webmail_string_and_len();
    if(sto == NULL ) goto cnpc_mail_fail;//lj
    char *from=NULL;
    char*content=NULL;
    char*subject=NULL;

    /*通过正则表达式提取发送人，并存入webmail->From中，便于之后产生jansson串*/
    const char pattern1[PATTERN_LENGTH] = "(?<=/exchange/)[\\W\\w]*?(?=/)";
    if(webmail->From==NULL&&mch(&StoPT,&sto,pattern1)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&from,sto->len+13) ==-1) goto cnpc_mail_fail;//lj
            memcpy(from,sto->string,sto->len);
            strcat(from,"@cnpc.com.cn");
            if(c_malloc((void**)&webmail->From,sto->len+13) ==-1 )goto cnpc_mail_fail;//lj
            memcpy(webmail->From,from,sto->len+12);
            c_free(&from);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*to收件人、cc抄送、bcc密送都认为时收件人并通过to_len来将三者拼接到一起*/
    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern2[PATTERN_LENGTH] = "(?<=MsgTo=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern3[PATTERN_LENGTH] = "(?<=&MsgCc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*通过正则表达式提取收件人，并存入webmail->to中，便于之后产生jansson串*/
    const char pattern4[PATTERN_LENGTH] = "(?<=MsgBcc=)[\\w\\W]*?(?=&)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        if(sto->len){
            web_URLDecode(sto->string,sto->len,sto->string,sto->len );
            webmail_to_parser(sto , webmail->To);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取主题，并对应存储*/
    const char pattern5[PATTERN_LENGTH] = "(?<=subject=)[\\w\\W]*?(?=&)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&subject,sto->len+1) == -1) goto cnpc_mail_fail;//lj
            web_URLDecode(sto->string,sto->len,subject,sto->len );

            if(c_malloc((void**)&(webmail->Subject),strlen(subject)+1) ==-1)goto cnpc_mail_fail;//lj
            memcpy(webmail->Subject ,subject,strlen(subject));
            c_free(&subject);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }

    /*提取内容，并对应存储*/
    const char pattern6[PATTERN_LENGTH] = "(?<=textdescription=)[\\w\\W]*?(?=&)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(sto->len){
#if 0       /*换码*/
            if(c_malloc((void**)&content,sto->len+1) == -1) goto cnpc_mail_fail;//lj
            web_URLDecode(sto->string,sto->len,content,sto->len );
            if(c_malloc((void**)&(webmail->Content),strlen(content)+1) ==-1)goto cnpc_mail_fail;//lj
            memcpy(webmail->Content,content,strlen(content));
            c_free(&content);
#endif
            /*不转码*/
            if(c_malloc((void**)&webmail->Content,sto->len+1) == -1 ) goto cnpc_mail_fail;//lj
            memcpy(webmail->Content,sto->string,sto->len);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "cnpc_mail end ....");
    return 0;
cnpc_mail_fail:
    log_printf(ZLOG_LEVEL_ERROR, "cnpc_mail fail end ....");
    c_free(&from);
    c_free(&content);
    c_free(&subject);
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    return -1;
}
/**
 * @brief cnpc_attach 提取通过中国石油邮箱发送的附件
 *
 * @param StoPT         URL
 * @param StoBY         MIME格式的boundary
 * @param StoMB         待检测的数据体
 * @param webmail       存储邮件的信息
 */
int cnpc_attach(webmail_string_and_len *StoPT,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "cnpc_attach start ....");
    if(StoPT ==NULL|| StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    webmail_string_and_len *sto=init_webmail_string_and_len();
    if(sto == NULL)goto cnpc_attach_fail;
    char * from=NULL;

    /*提取附件*/
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    if(mch_att(&StoMB,webmail) ==-1) goto cnpc_attach_fail;

    /*提取发件人*/
    const char pattern1[PATTERN_LENGTH] = "(?<=/exchange/)[\\W\\w]*?(?=/)";
    if(webmail->From==NULL&&mch(&StoPT,&sto,pattern1)==EXIT_SUCCESS){
        if(sto->len){
            if(c_malloc((void**)&from,sto->len+13) ==-1)goto cnpc_attach_fail;
            memcpy(from,sto->string,sto->len);
            strcat(from,"@cnpc.com.cn");
            if(c_malloc((void**)&(webmail->From),strlen(from)+1) ==-1 )goto cnpc_attach_fail;
            memcpy(webmail->From,from,strlen(from));
            c_free(&from);
        }
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "cnpc_attach end ....");
    return 0;
cnpc_attach_fail:
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto = NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "cnpc_attach fail end ....");
    return -1;
}
/**
 * @brief cntv_mail extract the cntvmail's content;
 *
 * @param StoMB
 * @param webmail
 */
int cntv_mail(webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    log_printf(ZLOG_LEVEL_DEBUG, "cntv_mail start ....");
    if(StoMB ==NULL|| webmail ==NULL)
        return -1;
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL || sto1 ==NULL)goto cntv_mail_fail;

    char pattern1[PATTERN_LENGTH]="(?<=\"usr\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoMB,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto   cntv_mail_fail ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }
    char pattern2[PATTERN_LENGTH]="(?<=\"to\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern2)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    char pattern3[PATTERN_LENGTH]="(?<=\"cc\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern3)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    char pattern4[PATTERN_LENGTH]="(?<=\"bcc\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern4)==EXIT_SUCCESS){
        webmail_to_parser(sto , webmail->To);
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    const char pattern7[PATTERN_LENGTH]="(?<=\"startTime\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(mch(&StoMB,&sto,pattern7)==EXIT_SUCCESS){
        char s[MAX_TIME_LEN];
        memset(s,'\0',sizeof(s));
        time_convert(sto->string,s);
        memcpy(webmail->Time,s,strlen(s));
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    char pattern5[PATTERN_LENGTH]="(?<=\"subject\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(webmail->Subject==NULL&&mch(&StoMB,&sto,pattern5)==EXIT_SUCCESS){
        if(c_malloc((void**)&webmail->Subject,sto->len+1)==-1)goto cntv_mail_fail;
        memcpy(webmail->Subject ,sto->string,sto->len);
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    char pattern6[PATTERN_LENGTH] = "(?<=\"textInner\"\\r\\n\\r\\n)[\\w\\W]*?(?=\\r\\n)";
    if(webmail->Content==NULL&&mch(&StoMB,&sto,pattern6)==EXIT_SUCCESS){
        if(c_malloc((void**)&webmail->Content,sto->len+1)==-1)goto cntv_mail_fail;
        memcpy(webmail->Content,sto->string,sto->len);
        if (sto)
        {
            c_free(&(sto->string));
        }
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "cntv_mail end ....");
    return 0;
cntv_mail_fail:
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto=NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "cntv_mail fail end ....");
    return -1;

}
/**
 * @brief cntv_attach extract the cntvmail's attach;
 *
 * @param StoCK
 * @param StoBY
 * @param StoMB
 * @param webmail
 */
int cntv_attach(webmail_string_and_len *StoCK,webmail_string_and_len *StoBY,webmail_string_and_len *StoMB,dlp_webmail_info *webmail)
{
    //log_printf(ZLOG_LEVEL_INFO, "cntv_attach start ....");
    //	log_printf(ZLOG_LEVEL_DEBUG, "cntv_attach start ....");
    if(StoCK ==NULL|| StoBY ==NULL|| StoMB ==NULL|| webmail ==NULL)
        return -1;
    webmail_string_and_len *sto=init_webmail_string_and_len();
    webmail_string_and_len *sto1=init_webmail_string_and_len();
    if(sto == NULL || sto1 ==NULL){
        goto cntv_attach_fail;
    }
    memcpy(webmail->boundary,StoBY->string,StoBY->len);
    if(mch_att(&StoMB,webmail) ==-1) goto cntv_attach_fail;
    const char pattern1[PATTERN_LENGTH] = "(?<=usr=)[\\W\\w]*?(cntv.cn)";
    const char pattern_mail[128] = "([a-zA-Z0-9_\\.\\-])+\\@(([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+";
    if(webmail->From==NULL&&mch(&StoCK,&sto1,pattern1)==EXIT_SUCCESS){
        if(sto1->len){
            if(mch(&sto1,&sto, pattern_mail)==EXIT_SUCCESS){
                if(c_malloc((void**)&(webmail->From),sto->len+1) ==-1 ) goto  cntv_attach_fail  ;
                memcpy(webmail->From,sto->string,sto->len);
                c_free(&(sto1->string));
                c_free(&(sto->string));
            }
        }
    }
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        free(sto);
        sto=NULL;
    }
    //	log_printf(ZLOG_LEVEL_DEBUG, "cntv_attach end ....");
    return 0;
cntv_attach_fail:
    if(sto1){
        c_free(&(sto1->string));
        free(sto1);
        sto1 = NULL;
    }
    if(sto){
        c_free(&(sto->string));
        free(sto);
        sto=NULL;
    }
    log_printf(ZLOG_LEVEL_ERROR, "cntv_attach fail end ....");
    return -1;
}
/**
 * @brief jdmail_att    判断具体为何种邮箱的何种流
 *
 * @param dlp_http      经提取后的http头
 * @param webmail       存储邮件的相应信息
 * @param host_ori_ref  包含host、origin、referer信息
 * @param webmailjson   存放生成的jansson串
 * @param sbuff_putdata
 * @param iringbuffer_ID
 *
 * @return
 */
int jdmail_att(HttpSession* dlp_http, dlp_webmail_info *webmail, char* host_ori_ref, char**webmailjson, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
{
    //log_printf(ZLOG_LEVEL_DEBUG, "jdmail_att start ....");
    /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
    dlp_http_post_head *http_session = NULL;
    http_session = (dlp_http_post_head *)dlp_http->http;


    /*初始化所需要的变量*/
    /*
     * StoPT        URL
     * StoBY        MIME格式中的boundary
     * StoCK        cookie信息
     * StoMB        待检测的数据体
     */
    webmail_string_and_len *StoPT = NULL;
    webmail_string_and_len *StoBY = NULL;
    webmail_string_and_len *StoCK = NULL;
    webmail_string_and_len *StoMB = NULL;

    /*将从上层提取的信息放置到上面申请的变量中*/
    FILE *pr = fopen(http_session->new_name,"r");
    if(pr == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When jdmail_att is processed,the open file is failed!!\n\n");
        //goto jdmail_att_fail;
        return -1;
    }

    StoPT= init_webmail_string_and_len();
    if(StoPT == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into jdmail_att,alloc memory StoPT failed!!\n");
        goto jdmail_att_fail;
    }

    StoBY = init_webmail_string_and_len();
    if(StoBY == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into jdmail_att,alloc memory StoBY failed!!\n");
        goto jdmail_att_fail;
    }

    StoCK = init_webmail_string_and_len();
    if(StoCK == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into jdmail_att,alloc memory StoCK failed!!\n");
        goto jdmail_att_fail;
    }

    StoMB = init_webmail_string_and_len();
    if(StoMB == NULL){
        log_printf(ZLOG_LEVEL_ERROR, "When we enter into jdmail_att,alloc memory StoMB failed!!\n");
        goto jdmail_att_fail;
    }

    StoMB->len=http_session->content_length;
    if(c_malloc((void**)&StoMB->string,StoMB->len+1) ==-1)
    {
        goto jdmail_att_fail;
    }
    fseek(pr,0,SEEK_SET);
    fread(StoMB->string,1,StoMB->len,pr);
    if (pr)
    {
        fclose(pr);
        pr = NULL;
    }

    //		if(remove(http_session->new_name))
    //				printf("the file can't delte %s \n",http_session->new_name );

    if(http_session->url){
        if(c_malloc((void**)&StoPT->string,HTTP_FILE_URL_MAX)==-1)goto jdmail_att_fail;
        memcpy(StoPT->string,http_session->url,HTTP_FILE_URL_MAX);
        StoPT->len=HTTP_FILE_URL_MAX;
    }
    if(strstr(http_session->content_type, "boundary")){
        if(c_malloc((void**)&StoBY->string,MAX_BOUNDARY_LEN)==-1)goto jdmail_att_fail;
        memcpy(StoBY->string,http_session->boundary,MAX_BOUNDARY_LEN);
        StoBY->len=MAX_BOUNDARY_LEN;
    }
    if(http_session->cookie){
        if(c_malloc((void**)&StoCK->string,strlen(http_session->cookie)+1) == -1) goto jdmail_att_fail;
        memcpy(StoCK->string,http_session->cookie,strlen(http_session->cookie));
        StoCK->len=strlen(http_session->cookie);
    }

    /*开始通过正则表达式，与字符串匹配的方法匹配url、origin、referer的信息来判断具体为何种邮箱的何种流，并进入到对应的函数中进行处理*/
    switch(jdmail_type(host_ori_ref)){
        case WYMAIL:
            {
                char *webmail_wy_mail_pattern1 = "&func=mbox:compose";
                char *webmail_wy_mail_pattern2 = "action=deliver";
                char *webmail_wy_mail_pattern3 = "compose/compose.do?action=DELIVER";
                char *webmail_wy_att_pattern = "/upxmail/upload";
                char *webmail_wy_att_pattern1 = "/compose/upload";
                //if (strstr(StoPT->string, webmail_wy_mail_pattern1)) {
                ///*下面判断的意思是：在URL 中如果找到了“&func=mbox:compose”,
                //并且同时找到了"action=deliver"则进入wymail（）函数，如果进入到wymail函数中出错了，则goto
                //释放资源，做出错处理*/
                if ((strstr(StoPT->string, webmail_wy_mail_pattern1) && strstr(StoPT->string, webmail_wy_mail_pattern2)) || strstr(StoPT->string , webmail_wy_mail_pattern3)) {
                    if(wymail(StoMB,webmail) ==-1) goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                /*如果URL中没有找到相应的字符串，则开始判断是否里面包含"/upxmail/upload",同时头部项中有up_load_size项，则进入到wyattach函数进行处理*/
                else if (strstr(StoPT->string, webmail_wy_att_pattern)  && http_session->up_load_size) {
                    if(wyattach(dlp_http, StoPT, StoMB, webmail)==-1)goto jdmail_att_fail;
                }
                /*如果上面两项都不满足，继续判断里面是否有"/upxmail/upload",同时头部项中的content—type为MIME形式的，则进入到wyattach1函数进行处理*/
                else if ((strstr(StoPT->string, webmail_wy_att_pattern)||strstr(StoPT->string, webmail_wy_att_pattern1) ) && strstr(http_session->content_type, "boundary")) {
                    if(wyattach1(StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                break;
            }
                case QMAIL:
            {
                char *webmail_qq_mail_pattern1 = "/cgi-bin/compose_send";
                char *webmail_qq_mail_pattern2 = "/cgi-bin/groupmail_send";//qq发送群邮件
                char *webmail_qq_att_pattern1 = "/ftn_handler";
                char *webmail_qq_att_pattern2 = "/cgi-bin/upload";
                if (strstr(StoPT->string, webmail_qq_mail_pattern1)) {
                    if(QQmail(StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if (strstr(StoPT->string, webmail_qq_mail_pattern2)) {
                    if(QQmail_qun(StoCK,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if (strstr(StoPT->string, webmail_qq_att_pattern1)) {
                    if(QQattach(dlp_http, StoPT, StoBY, StoMB, webmail)==-1)goto jdmail_att_fail;
                }
                else if(strstr(StoPT->string,webmail_qq_att_pattern2)){
                    char *webmail_qq_att_pattern3 = "/cgi-bin/uploadfile";//分会话分post无乱码情况
                    char pattern_filesize[PATTERN_LENGTH]="(?<=size=)[\\W\\w]*?(?=&)";
                    char pattern_id[PATTERN_LENGTH]="(?<=storeid=)[\\W\\w]*?(?=&)";
                    /*下面这个式子的意思时首先检查URL中是否有"/cgi-bin/uploadfile"，并且能够在URL中正则匹配到pattern——filesize"(?<=size=)[\\W\\w]*?(?=&)"，再同时匹配到"(?<=storeid=)[\\W\\w]*?(?=&)",则进入QQattach2进行处理*/
                    if(strstr(StoPT->string,webmail_qq_att_pattern3) && mch(&StoPT,NULL,pattern_filesize)==EXIT_SUCCESS
                            &&mch(&StoPT,NULL,pattern_id)==EXIT_SUCCESS){
                        //printf("We will enter into QQattach2 process function!!\n\n");
                        if( QQattach2(dlp_http,StoPT,StoMB,webmail)==-1)goto jdmail_att_fail;
                    }
                    else
                        if( QQattachF(dlp_http,StoCK,StoMB,webmail)==-1)goto jdmail_att_fail;
                }
                break;
            }
                case CNPCMAIL:
            {
                char *webmail_cnpc_mail_pattern="/exchange.*";
                if(mch(&StoPT,NULL,webmail_cnpc_mail_pattern)==EXIT_SUCCESS){
                    char *webmail_cnpc_att_pattern="/exchange.*Cmd=addattach";
                    if(mch(&StoPT,NULL,webmail_cnpc_att_pattern)==EXIT_SUCCESS && strstr(http_session->content_type, "boundary")){
                        if(cnpc_attach(StoPT,StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                        webmail->state = build_jansson;
                    }
                    else if(mch(&StoPT,NULL,webmail_cnpc_att_pattern)==EXIT_FAILURE){
                        if(cnpc_mail(StoPT,StoMB,webmail)==-1)goto jdmail_att_fail;
                        webmail->state = build_jansson;
                    }
                }
                break;
            }
                case SINAMAIL:
            {
                char *webmail_sina_mail_pattern = "/classic/send";
                char *webmail_sina_att_pattern1 = "/classic/uploadatt";
                //                char *webmail_sina_att_pattern1 = "upload";
                //				char *webmail_sina_att_pattern2 = "(?<=Host: |Host:)(\\w+\\.){0,2}upload-vdisk\\.\\w+\\.(com|cn|net){0,2}";
                char *webmail_sina_att_pattern2 = "/\\d*/files/basic/";
                char *webmail_sina_att_pattern3 = "diskupload";//linux网盘

                if(strstr(StoPT->string,webmail_sina_mail_pattern)&& strstr(http_session->content_type, "boundary")){
                    if(sina_mail(StoPT,StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if(strstr(StoPT->string,webmail_sina_att_pattern1)||mch(&StoPT,NULL,webmail_sina_att_pattern2)==EXIT_SUCCESS||strstr(StoPT->string,webmail_sina_att_pattern3)){
                    if(sina_attach(StoPT,StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                break;
            }
                case SOHUMAIL:
            {
                char *webmail_sohu_mail_pattern="/bapp/\\d*/mail";
                char *webmail_sohu_att_pattern="/bapp/\\d*/mail/att";
                char *webmail_sohu_att_pattern1="/flashUploadAction.action";//批量上传到优盘
                const char pattern_id[PATTERN_LENGTH]="(?<=xid)[\\W\\w]*?(?=&)";
                //printf("This is sohu function!!!!!!!!!!!\n");
                if(mch(&StoPT,NULL,webmail_sohu_mail_pattern)==EXIT_SUCCESS){
                    if(mch(&StoPT,NULL,webmail_sohu_att_pattern)==EXIT_SUCCESS && strstr(http_session->content_type, "boundary")){
                        if(sohu_attach(StoPT,StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                        webmail->state = build_jansson;
                    }
                    else if(mch(&StoPT,NULL,webmail_sohu_att_pattern)==EXIT_FAILURE){
                        if(sohu_mail(StoMB,webmail)==-1)goto jdmail_att_fail;
                        webmail->state = build_jansson;
                    }
                }
                else if(strstr(StoPT->string,webmail_sohu_att_pattern1) && mch(&StoPT,NULL,pattern_id)==EXIT_SUCCESS){
                    //printf("This is sohu_attach2 function!!!!!!!!!!!\n");
                    if(sohu_attach2(dlp_http, StoPT, StoMB, webmail)==-1)goto jdmail_att_fail;
                }
                break;
            }
                case TOMMAIL:
            {
                char *webmail_tom_mail_pattern="/webmail/writemail/sendmail";
                char *webmail_tom_att_pattern="/webmail/writemail/.*uploadAttachment";
                if(strstr(StoPT->string,webmail_tom_mail_pattern)){
                    if(tom_mail(StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if(mch(&StoPT,NULL,webmail_tom_att_pattern)==EXIT_SUCCESS && strstr(http_session->content_type, "boundary")){
                    if(tom_attach(StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                break;
            }
                case CNMAIL:
            {
                char *webmail_21cn_mail_pattern="/webmail/sendMail";
                char *webmail_21cn_att_pattern="/webmail/upload";
                if(strstr(StoPT->string,webmail_21cn_mail_pattern)){
                    if(cn_mail(StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if(strstr(StoPT->string,webmail_21cn_att_pattern)&& strstr(http_session->content_type, "boundary")){
                    if(cn_attach(StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                break;
            }
                case MAIL139:
            {
                char *webmail_139_mail_pattern=".*func=mbox:compose";
                char *webmail_139_att_pattern1=".*func=attach:upload";
                char *webmail_139_att_pattern2="/new-fcgi-bin/";
                char *webmail_139_att_pattern3="/storageWeb/servlet/pcUploadFile";//网盘附件上传
                if(mch(&StoPT,NULL,webmail_139_mail_pattern)==EXIT_SUCCESS){
                    if(mail_139(StoCK,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if(mch(&StoPT,NULL,webmail_139_att_pattern2)==EXIT_SUCCESS){
                    if(attach_139(StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                }
                else if(mch(&StoPT,NULL,webmail_139_att_pattern1)==EXIT_SUCCESS && strstr(http_session->content_type, "boundary")){
                    if(attach2_139(dlp_http, StoPT, StoMB, StoBY, webmail)==-1)goto jdmail_att_fail;
                }
                else if(mch(&StoPT,NULL,webmail_139_att_pattern3)==EXIT_SUCCESS && http_session->contentSize){
                    if(attach3_139(dlp_http,StoMB,webmail)==-1)goto jdmail_att_fail;
                }
                break;
            }
                case MAIL189:
            {
                char *webmail_189_mail_pattern=".*/mail/sendMail";
                char *webmail_189_att_pattern1=".*/mail/upload";
                char *webmail_189_att_pattern2=".*WebUploadSmallFileAction";
                if(mch(&StoPT,NULL,webmail_189_mail_pattern)==EXIT_SUCCESS){
                    if(mail_189(StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if(mch(&StoPT,NULL,webmail_189_att_pattern1)==EXIT_SUCCESS||mch(&StoPT,NULL,webmail_189_att_pattern2)==EXIT_SUCCESS && strstr(http_session->content_type, "boundary")){
                    if(attach_189(StoPT,StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                break;
            }
                case CNTVMAIL:
            {
                char *webmail_cntv_mail_pattern="/mail/mailOperate/";
                char *webmail_cntv_att_pattern="/FileUploadServlet";
                if(strstr(StoPT->string,webmail_cntv_mail_pattern)){
                    if(cntv_mail(StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                else if(strstr(StoPT->string,webmail_cntv_att_pattern) && strstr(http_session->content_type, "boundary")){
                    if(cntv_attach(StoCK,StoBY,StoMB,webmail)==-1)goto jdmail_att_fail;
                    webmail->state = build_jansson;
                }
                break;
            }
                default:webmail->state = no_jansson;
                        break;
            }
            if(webmail->state == build_jansson || webmail->state == mail_end){
                webmail_pro_jsonstr(dlp_http, webmail, webmailjson, sbuff_putdata, iringbuffer_ID);
            }

            /*资源释放*/
            if(StoPT){
                c_free(&StoPT->string);
                free(StoPT);
                StoPT = NULL;
            }
            if(StoMB){
                c_free(&StoMB->string);
                free(StoMB);
                StoMB = NULL;
            }
            if(StoBY){
                c_free(&StoBY->string);
                free(StoBY);
                StoBY = NULL;
            }
            if(StoCK){
                c_free(&StoCK->string);
                free(StoCK);
                StoCK = NULL;
            }
            //	log_printf(ZLOG_LEVEL_DEBUG, "jdmail_att end ....");
            return 0;
jdmail_att_fail:
            log_printf(ZLOG_LEVEL_ERROR, "jdmail_att fail end ....");
            if(pr){
                fclose(pr);
                pr = NULL;
            }
            if(StoPT){
                c_free(&StoPT->string);
                free(StoPT);
                StoPT = NULL;
            }
            if(StoMB){
                c_free(&StoMB->string);
                free(StoMB);
                StoMB = NULL;
            }
            if(StoBY){
                c_free(&StoBY->string);
                free(StoBY);
                StoBY = NULL;
            }
            if(StoCK){
                c_free(&StoCK->string);
                free(StoCK);
                StoCK = NULL;
            }
            return -1;
    }

    /**
     * @brief jdmail_type   判断是何种邮箱
     *
     * @param host_ori_ref  包含host、origin、referer信息
     *
     * @return
     */
    int jdmail_type(char*host_ori_ref)
    {
        log_printf(ZLOG_LEVEL_DEBUG, "jdmail_type start ....");
        char*wypat1="163.com";
        char*wypat2="126.com";
        char*wypat3="yeah.net";
        char*qqpat="qq.com";
        char*cnpcpat="cnpc";
        char*sinapat1="sina.com";
        char*sinapat2="sina.cn";
        char*sohu_sougopat1="sohu.com";
        char*sohu_sougopat2="sogou.com";
        char*sohu_sougopat3="chinaren.com";
        char*tompat="tom.com";
        char*cnpat="21cn.com";
        char*pat139="10086.cn";
        char*pat189="189.cn";
        char*cntvpat="cntv.cn";
        if(strstr(host_ori_ref,wypat1)||strstr(host_ori_ref,wypat2)||strstr(host_ori_ref,wypat3)){
            return WYMAIL;
        }
        else if(strstr(host_ori_ref,qqpat)){
            return QMAIL;
        }
        else if(strstr(host_ori_ref,cnpcpat)){
            return CNPCMAIL;
        }
        else if(strstr(host_ori_ref,sinapat1)||strstr(host_ori_ref,sinapat2)){
            return SINAMAIL;
        }
        else if(strstr(host_ori_ref,sohu_sougopat1)||strstr(host_ori_ref,sohu_sougopat2)||strstr(host_ori_ref,sohu_sougopat3)){
            return SOHUMAIL;
        }
        else if(strstr(host_ori_ref,tompat)){
            return TOMMAIL;
        }
        else if(strstr(host_ori_ref,cnpat)){
            return CNMAIL;
        }
        else if(strstr(host_ori_ref,pat139)){
            return MAIL139;
        }
        else if(strstr(host_ori_ref,pat189)){
            return MAIL189;
        }
        else if(strstr(host_ori_ref,cntvpat)){
            return CNTVMAIL;
        }
        //	log_printf(ZLOG_LEVEL_DEBUG, "jdmail_type end ....");
        return 0;
    }

    /**
     * @brief jdmail 判断是否为webmail
     *
     * @param host_ori_ref  host + origin + referer;
     *
     * @return
     */
    int jdmail(char* host_ori_ref)
    {
        if (strstr(host_ori_ref, "mail")){
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }

    /**
     * @brief c_malloc get the room to store the data dynamiclly
     *
     * @param obj
     * @param len
     */
    int c_malloc(void **obj,int len)
    {
        //	log_printf(ZLOG_LEVEL_DEBUG, "c_malloc start ....");
        *obj=(char*)malloc(len);
        if(*obj==NULL){
            return -1;
        }
        memset(*obj,'\0',len);
        //	log_printf(ZLOG_LEVEL_DEBUG, "c_malloc end ....");
        return 0;
    }

    /**
     * @brief c_free free the space which has been apply;
     *
     * @param obj
     */
    void c_free(char**obj)
    {
        //	log_printf(ZLOG_LEVEL_DEBUG, "c_free start ....");
        if(*obj!=NULL){
            free (*obj);
            *obj=NULL;
        }
        //	log_printf(ZLOG_LEVEL_DEBUG, "c_free end ....");
    }

    /**
     * @brief webmail_          生成jansson串的函数
     *
     * @param dlp_http      经提取后的http头
     * @param webmail       存储邮件的相应信息
     * @param webmailjson   存放生成的jansson串
     * @param sbuff_putdata
     * @param iringbuffer_ID
     *
     * @return
     */
    int webmail_pro_jsonstr(HttpSession* dlp_http,dlp_webmail_info *webmail, char **webmailjson, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
    {
        log_printf(ZLOG_LEVEL_DEBUG, " webmail_pro_jsonstr start ....");

        /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
        dlp_http_post_head* http_session = NULL;
        http_session = (dlp_http_post_head *)dlp_http->http;

        /*初始化所需要的变量*/
        /*
         * fp           文件指针
         * nowtime      系统运行时间
         * result       存放jansson串的结果
         * tmp          存放临时节点
         * url          存放拼接后的完整url
         */
        FILE *fp = NULL;//lj
        char *nowtime = NULL;
        if(c_malloc((void**)&nowtime,256) == -1)  goto webmail_pro_jsonstr_fail;
        system_time(nowtime);
        json_t *objectmsg = NULL;
        json_t *objectall = NULL;
        json_t *myfile = NULL;
        json_t *filelist = NULL;
        json_t *tolist = NULL;
        char *result = NULL;
        struct att_node *tmp=webmail->filenode;
        SESSION_BUFFER_NODE ft;
        char url[HTTP_FILE_URL_MAX] = {};
        strcat(url, http_session->host);
        strcat(url, http_session->url);

        objectmsg = json_object();
        objectall = json_object();
        filelist= json_array();
        tolist= json_array();

        /*将源地址ip转换为标准格式，存入jansson串*/
        char * ipaddr1=NULL;
        char addr1[20]= {};
        struct in_addr inaddr1;
        inaddr1.s_addr=dlp_http->key.saddr;
        ipaddr1 = inet_ntoa(inaddr1);
        strcpy(addr1,ipaddr1);
        json_object_set_new (objectall, "sip", json_string(addr1));

        /*将目的地址ip转换为标准格式，存入jansson串*/
        char * ipaddr2=NULL;
        char addr2[20]={};
        struct in_addr inaddr2;
        inaddr2.s_addr=dlp_http->key.daddr;
        ipaddr2 = inet_ntoa(inaddr2);
        strcpy(addr2,ipaddr2);
        json_object_set_new (objectall, "dip", json_string(addr2));

        /*如果不存在收件人则协议号改为7*/
        if(strlen(webmail->To)==0)
            webmail->pro_id=7;
        log_printf(ZLOG_LEVEL_DEBUG, "\n==================== http_session->current_thread_id=%d\n ",http_session->current_thread_id);
        json_object_set_new (objectall, "id", json_integer(http_session->current_thread_id));
        json_object_set_new (objectall, "prt", json_integer(webmail->pro_id));
        json_object_set_new (objectall, "spt", json_integer(dlp_http->key.source));
        json_object_set_new (objectall, "dpt", json_integer(dlp_http->key.dest));
        json_object_set_new (objectall, "ts", json_string(nowtime));
        json_object_set_new (objectmsg, "url", json_string(url));
        json_object_set_new (objectmsg, "ts", json_string(webmail->Time));

        /*生成发件人，收件人，主题等信息*/
        if(webmail->From)
            json_object_set_new (objectmsg, "from", json_string(webmail->From));
        else
            json_object_set_new (objectmsg, "from", json_string(""));

        if(strlen(webmail->To)!=0){
            to_json(tolist , webmail->To);
        }
        json_object_set_new (objectmsg, "to", tolist);

        if(webmail->Subject){
            json_object_set_new (objectmsg, "sbj", json_string(webmail->Subject));
        }
        else{
            json_object_set_new (objectmsg, "sbj", json_string(""));
        }

        /*创建正文存储的路径*/
        char foldername[FILE_NAME_LENGTH]={};
#if 0 //当前路径
        getcwd(foldername, FILE_NAME_LENGTH);
        strcat(foldername,"/");
#endif
        sprintf(foldername , "%s%s", session_stream , "/" );
        char uuid_buf[UUID_LENGTH] = {};

        /*正文存储，并相应生成jansson串*/
        if(webmail->Content){
            /*创建正文的存储名，并存储到当前目录*/
            char filename[FILE_NAME_LENGTH]={};/*存放路径+文件名*/
            char filename_tika[FILE_NAME_LENGTH]={};/*存放路径+文件名*/
            uuid_t randNumber;
            uuid_generate(randNumber);
            uuid_unparse(randNumber, uuid_buf);
            strcpy(filename,foldername);
            strcat(filename,uuid_buf);
         //   strcat(filename,".txt");
						sprintf(filename_tika,"%s.txt",filename);
            //json_object_set_new (objectmsg, "plain", json_string(filename));
            json_object_set_new (objectmsg, "plain", json_string(filename_tika));
            /*存储正文到当前目录*/
            fp = fopen(filename , "w+");
            if(fp == NULL){
                log_printf(ZLOG_LEVEL_ERROR, " the pro_jansson cant open the file!");
                goto webmail_pro_jsonstr_fail;
            }
            fwrite( webmail->Content , strlen(webmail->Content) , 1 , fp);
            fclose(fp);
            fp=NULL;
#if 1
						char *ret = tika(filename,filename_tika);
						if(ret==NULL)
            json_object_set_new (objectmsg, "plain", json_string(""));

#endif
        }
        /*如果没有正文则输出为空*/
        else
            json_object_set_new (objectmsg, "plain", json_string(""));

        json_object_set_new (objectall, "pAttr", objectmsg);

        /*生成附件相关jansson串*/
        json_t *copy = NULL;
        /*从附件链表中循环取出一个或多个附件*/
        while(tmp->next!=NULL){
            myfile = json_object();
            tmp=tmp->next;
            json_object_set_new (myfile, "ft", json_string(""));
	     if(tmp->RawAttName==NULL) json_object_set_new (myfile, "fn", json_string("file_no_name"));//0115
            else json_object_set_new (myfile, "fn", json_string(tmp->RawAttName));
            json_object_set_new (myfile, "fs", json_integer(tmp->attlen/1024));
            json_object_set_new (myfile, "au", json_string(""));
            char uuid_buf1[UUID_LENGTH]={};
            char filename2[FILE_NAME_LENGTH]={};/*放置路径+文件名*/
            if(webmail->state == build_jansson){
                memcpy(uuid_buf1,tmp->att_uuid,UUID_LENGTH);
            }
            if(webmail->state == mail_end){
                memcpy(uuid_buf1,webmail->uuid,UUID_LENGTH);
            }
            strcpy(filename2,foldername);
            strcat(filename2,uuid_buf1);
            json_object_set_new (myfile, "fp", json_string(filename2));
            json_object_set_new (myfile, "ftp", json_string(""));
            copy = json_deep_copy(myfile);
            json_decref(myfile);
            myfile=NULL;
            json_array_append_new(filelist, copy);
        }
        json_object_set_new (objectall, "flist", filelist);

        /* 转换成字符串 */
        result = json_dumps(objectall, JSON_PRESERVE_ORDER);
        if(c_malloc((void**)webmailjson,strlen(result)+1) ==-1) goto webmail_pro_jsonstr_fail;
        memcpy(*webmailjson, result,strlen(result));

				log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n ===xiaogou====== result = %s,strlen(result)= %d\n\n\n",result,strlen(result));
        /* 释放json_dumps调用后的结果 */
        c_free(&result);

        /*释放对应资源*/
        json_decref(objectall);
        c_free(&nowtime);
        if((&(**webmailjson)) != NULL)
        {
            ft.session_five_attr.ip_dst = dlp_http->key.daddr;
            ft.session_five_attr.ip_src = dlp_http->key.saddr;
            ft.session_five_attr.port_src = dlp_http->key.source;
            ft.session_five_attr.port_dst = dlp_http->key.dest;
            ft.attr = &(**webmailjson);
            ft.session_five_attr.protocol = webmail->pro_id;
		//		log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n == Sem post webmail iringbuffer_ID = %d\n\n\n",iringbuffer_ID);
    //        sbuff_putdata(iringbuffer_ID, ft);
#if 1
	 int res = 0;
     strcpy(ft.strpath, session_stream); 
	  if((ft.session_five_attr.protocol==157)||(ft.session_five_attr.protocol==7))//webmail和http协议
	 {
        if(ft.attr != NULL){
          int res = 0;//sshdr->strpath
		  res = file_extract_pthread ((char *)ft.attr, session_json, max_cfilesize);//拾取文件处理函数
          if (res == -1)
          {    
	        log_printf(ZLOG_LEVEL_ERROR, "file analysis process ERROR: webmail process failed, jason is NULL!!");
          }
          free(ft.attr);    
        }else
	      log_printf(ZLOG_LEVEL_ERROR, "file analysis process ERROR: webmail process failed, parameter transfered is NULL!!");
	 }
				log_printf(ZLOG_LEVEL_DEBUG, "\n\n\n ======================== TIKA END ===============================\n\n\n");
#endif
        }
        if(fp){
            fclose(fp);
            fp =NULL;
        }
        return 0;
webmail_pro_jsonstr_fail:
        json_decref(objectall);
        c_free(&nowtime);
        c_free(&result);
        if(fp){
            fclose(fp);
            fp =NULL;
        }
        log_printf(ZLOG_LEVEL_ERROR, " webmail_pro_jsonstr fail end ....");
        return -1;

    }
    /**
     * @brief webmail_free_HttpSession  free struct of "HttpSession"
     *
     * @param dlp_http
     *
     * @return
     */
    int webmail_free_HttpSession (HttpSession* dlp_http){
       if(dlp_http ==NULL)return -1;
        dlp_http_post_head *http_session=NULL;
        http_session = (dlp_http_post_head*)dlp_http->http;
        if (http_session == NULL)
        {
            log_printf(ZLOG_LEVEL_ERROR, "http_session is null, %s__%d", __FILE__, __LINE__);
            return -1;
        }
        if (http_session->cookie)
        {
            c_free(&(http_session->cookie));
        }

        if(http_session != NULL){
            //free(http_session);//by niulw add
            http_session = NULL;
        }
        return 0;
    }

    /**
     * @brief webmail_mail  the entrance of the  main fuction and initiate the
     * related data ;
     *
     * @param dlp_http
     * @param webmailjson
     * @param sbuff_putdata
     * @param iringbuffer_ID
     *
     * @return
     */
    int webmail_mail(HttpSession* dlp_http,char **webmailjson, http_sbuffputdata sbuff_putdata, const int iringbuffer_ID)
    {
			log_printf(ZLOG_LEVEL_DEBUG, " \n\n\n           webmail_mail  function -------------------\n\n");
        /*通过dlp_http 中的dlp_http_post_head类型的dlp_http来提取相应http的头信息*/
        dlp_http_post_head *http_session = NULL;
        http_session = (dlp_http_post_head* )dlp_http->http;
		log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao1  http_session->boundary = %s\n\n",http_session->boundary);
        webmail_string_and_len *StoMB = init_webmail_string_and_len();
        if(StoMB == NULL)goto webmail_mail_fail;//lj
        FILE *pr=NULL;
        /*初始化存放解析后的邮件信息*/
        dlp_webmail_info *webmail = NULL;
        if(c_malloc((void **)&webmail,sizeof(dlp_webmail_info)) ==-1) goto webmail_mail_fail;//lj
        webmail->filenode = init_att_nodelist();
        memset(webmail->Time, '\0', sizeof(webmail->Time));
        memset(webmail->boundary,'\0',sizeof (webmail->boundary));
        memset(webmail->uuid,'\0', sizeof(webmail->uuid));
        webmail->From=NULL;
        webmail->Subject=NULL;
        webmail->Content=NULL;
        webmail->state = no_jansson;
        if(c_malloc((void**)&(webmail->To),MAX_TO_LEN) == -1 ) goto webmail_mail_fail;//lj

        /*存放host、origin、regerer信息，便于之后判断时否为webmail以及时何种邮箱流*/
        char host_ori_ref[HTTP_FILE_HOST_MAX+HTTP_FILE_ORIGIN_MAX+HTTP_FILE_REFERER_MAX] = {0};//存放提取前五个post的host、origine、referer放到一个指针里
        if(http_session == NULL){
            log_printf(ZLOG_LEVEL_ERROR, "%s__%d\n", __FILE__, __LINE__);
            goto webmail_mail_fail;
        }
        strcat(host_ori_ref, http_session->host);
        strcat(host_ori_ref, http_session->origin);
        strcat(host_ori_ref, http_session->referer);
				log_printf(ZLOG_LEVEL_DEBUG, "\n\nhttp_session->host = %s\n", http_session->host);
				log_printf(ZLOG_LEVEL_DEBUG, "\n\nhttp_session->origin = %s\n", http_session->origin);
				log_printf(ZLOG_LEVEL_DEBUG, "\n\nhttp_session->referer = %s\n", http_session->referer);
				log_printf(ZLOG_LEVEL_DEBUG, "\n\nhttp_session->new_name = %s\n", http_session->new_name);

        /*判断是否为webmail，如果是在判断是何种邮箱发出的何种流（附件或者单纯的邮件）*/
        if(jdmail(host_ori_ref)==EXIT_SUCCESS){
            if(http_session->content_length != 0){
                webmail->pro_id=157;
                if(jdmail_att(dlp_http, webmail, host_ori_ref, webmailjson, sbuff_putdata, iringbuffer_ID)==-1){
                    log_printf(ZLOG_LEVEL_ERROR, "%s__%d\n", __FILE__, __LINE__);
                    goto webmail_mail_fail;
                }
            }

        }
        /*如果均不是，则判断是否为MIME格式的http*/
        else if(strstr(http_session->content_type,"boundary")){
            char new_name[FILE_NAME_LENGTH];
						log_printf(ZLOG_LEVEL_DEBUG, "\n\n============== newname = %s====================\n\n",http_session->new_name);
            strcpy(new_name,http_session->new_name);
						log_printf(ZLOG_LEVEL_DEBUG, "\n\n============== newname = %s====================\n\n",new_name);
            //printf("the new name is %s\n",new_name);
            FILE *pr = fopen(new_name,"r");
            if(pr == NULL){
                log_printf(ZLOG_LEVEL_ERROR, "When webmail_mail is processed,the open file is failed!!\n\n");
                goto webmail_mail_fail;
            }
            //fseek(pr,0,SEEK_END);
            StoMB->len = http_session->content_length;
						log_printf(ZLOG_LEVEL_DEBUG, "--- StoMB->len = %d----\n\n",StoMB->len);
            if(c_malloc((void**)&StoMB->string,StoMB->len+1) == -1){
                goto webmail_mail_fail;//lj
            }
            fseek(pr,0,SEEK_SET);
            fread(StoMB->string,1,StoMB->len,pr);
            fclose(pr);
            pr=NULL;
            webmail->pro_id=7;
            memcpy(webmail->boundary,http_session->boundary,MAX_BOUNDARY_LEN);
						log_printf(ZLOG_LEVEL_DEBUG, "--------------- zu duan ni hao1  webmail->boundary = %s\n\n",webmail->boundary);
            /*提取MIME格式传输的附件*/
            mch_att(&StoMB,webmail);
            if((webmail->filenode)->next!=NULL){
                webmail->state = build_jansson;
                webmail_pro_jsonstr(dlp_http,webmail, webmailjson,sbuff_putdata, iringbuffer_ID);
            }
        }
        else
        {
#if 1
            http_IM(dlp_http, webmailjson, sbuff_putdata, iringbuffer_ID);
            goto webmail_mail_fail;//lj
#endif
        }
        if(StoMB){
            c_free(&StoMB->string);
            free(StoMB);
            StoMB = NULL;
        }
        if(webmail != NULL){
            free_att_nodelist (webmail->filenode);
            c_free(&webmail->Subject);
            c_free(&webmail->From);
            c_free(&webmail->To);
            c_free(&webmail->Content);
            free(webmail);
            webmail = NULL;
        }
        webmail_free_HttpSession(dlp_http);
        	log_printf(ZLOG_LEVEL_DEBUG, " Fuck  you down webmail_mail end ....");

        return 0;
webmail_mail_fail:
        if(StoMB){
            c_free(&StoMB->string);
            free(StoMB);
            StoMB = NULL;
        }
        if(webmail){
            free_att_nodelist (webmail->filenode);
            c_free(&webmail->Subject);
            c_free(&webmail->From);
            c_free(&webmail->To);
            c_free(&webmail->Content);
            free(webmail);
            webmail = NULL;
        }
        if(pr){
            fclose(pr);
            pr =NULL;
        }
        webmail_free_HttpSession(dlp_http);
        //log_printf(ZLOG_LEVEL_ERROR, " webmail_mail fail end ....");
        return -1;
    }
#if 1
void* tika_consumer_thread(void* _id) 
{
	int i=0;
#if 1
  u_int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
  long thread_id = (long)_id;
  u_long core_id = thread_id % numCPU;

  if(numCPU > 1) {
    if(bind2core(core_id) == 0)
	  log_printf(ZLOG_LEVEL_INFO, "file analysis consumer INFO: Set thread %lu on core %lu/%u !!", thread_id, core_id, numCPU);
  }
#endif

  SESSION_BUFFER_NODE *sshdr;
  sshdr = malloc(sizeof(SESSION_BUFFER_NODE));
  if(sshdr==NULL){
	log_printf(ZLOG_LEVEL_FATAL, "file analysis process FATAL: malloc sshdr is failed!!");
	return(NULL);
  }

  while(1){
#if 1
			log_printf(ZLOG_LEVEL_DEBUG, "\ntika_consumer_thread  processing -----ssbuffer_id  = %d--------------------\n",ssbuffer_id);
#if 1
    if(sbuff_getdata(ssbuffer_id, sshdr)>0){//获得信号量进行后续处理
			log_printf(ZLOG_LEVEL_DEBUG, "\ntika_consumer_thread  processing ----- i = %d--------------------\n",i++);
	 int res = 0;
     strcpy(sshdr->strpath, session_stream); 
	  if((sshdr->session_five_attr.protocol==157)||(sshdr->session_five_attr.protocol==7))//webmail和http协议
	 {
        if(sshdr->attr != NULL){
          int res = 0;//sshdr->strpath
		  res = file_extract_pthread ((char *)sshdr->attr, session_json, max_cfilesize);//拾取文件处理函数
          if (res == -1)
          {    
	        log_printf(ZLOG_LEVEL_ERROR, "file analysis process ERROR: webmail process failed, jason is NULL!!");
          }
          free(sshdr->attr);    
        }else
	      log_printf(ZLOG_LEVEL_ERROR, "file analysis process ERROR: webmail process failed, parameter transfered is NULL!!");
	 }
		}
#endif
#endif
	}
		return NULL;
}
#endif
extern int sbuff_putdata(const int iringbuffer_ID, SESSION_BUFFER_NODE ft) ;
#if 0
		int main()
		{
			if (zlog_init("conf/session_cap.conf")) {//开启日志功能
				printf("zlog init failed");
				return (-1);
			}
			//设置日志等级
			ftl_log = zlog_get_category(LOG4C_CATEGORY_FTL);
			err_log = zlog_get_category(LOG4C_CATEGORY_ERR);
			wrn_log = zlog_get_category(LOG4C_CATEGORY_WRN);
			dbg_log = zlog_get_category(LOG4C_CATEGORY_DBG);
			inf_log = zlog_get_category(LOG4C_CATEGORY_INF);
			ntc_log = zlog_get_category(LOG4C_CATEGORY_NTC);
			if(0 != read_IM_config())
			{
				log_printf(ZLOG_LEVEL_ERROR, "failed to read IM config file: -----%s------%s------%d\n",__FILE__, __func__, __LINE__);
				return -1;
			}
			/* init kmem interface */
			ikmem_init(NULL);//内存管理器初始化函数

			build_JVM();//java虚拟机初始化函数 打开JVM //by niulw
			ssbuffer_id =sbuff_create(100000);
			if(ssbuffer_id<0)
				return 0;
#if 1
			int thd_id =0;
			int tika_pthreads=500;
			pthread_t tika_thread[1000];
	for(thd_id=2; thd_id<tika_pthreads; thd_id++)
	  pthread_create(&tika_thread[thd_id], NULL, tika_consumer_thread, &thd_id);//tika文件内容及属性提取处理线程池创建
#endif
      char *webmailjson=NULL;
			dlp_http_post_head stHttpPost;
			memset(&stHttpPost,0x0,sizeof(dlp_http_post_head));
			stHttpPost.content_length = 5628013;
			memcpy(stHttpPost.new_name,"test.eml",strlen("test.eml")+1);
			memcpy(stHttpPost.content_type,"-------------- = multipart/form-data; boundary=----------------------------bd5fa42409b5",strlen("-------------- = multipart/form-data; boundary=----------------------------bd5fa42409b5")+1);
			memcpy(stHttpPost.boundary,"----------------------------bd5fa42409b5",strlen("----------------------------bd5fa42409b5")+1);
      HttpSession        sp_session;
      sp_session.http = &stHttpPost;
      sp_session.key.saddr = 12345;//sshdr->session_five_attr.ip_src;
      sp_session.key.daddr = 8080;//sshdr->session_five_attr.ip_dst;
      sp_session.key.source = 12345;//sshdr->session_five_attr.port_src;
      sp_session.key.dest = 8080;//sshdr->session_five_attr.port_dst;
      webmail_mail(&sp_session, &webmailjson, sbuff_putdata, ssbuffer_id);//http内容处理函数，负责所有http相关的处理
			while(1)
			{
				sleep(1);
			}
			return 0;
		}
#endif
