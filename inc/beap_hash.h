#ifndef __BEAP_HASH
#define __BEAP_HASH

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_HASH_SIZE	100001
#define UUID_MAX_LENGTH 512
//typedef signed char    SCHAR8;

typedef unsigned int uint32_t;
typedef unsigned long  uint64_t; 


typedef uint32_t(* t_hash_func)(void *, int) ;
typedef char(* t_comp_func)(void *, void *, int) ;

struct stru_hash_bucket;

typedef struct{
struct stru_hash_bucket * prev_hash;
struct stru_hash_bucket * next_hash;
}hash_ptr;
 
typedef struct stru_hash_bucket{
    void *		key;
    void *		content;
    hash_ptr	ptr;
}hash_bucket;
 
typedef struct{
    hash_bucket **hashtable;
    int 		nr_entries; //??¼??
    int		    tbl_size;//????С
    t_hash_func	hash_func;
    t_comp_func	comp_func;
}hash_table;

//hash_table  *ftp_session_tbl=NULL;
hash_table * create_hash(int , t_hash_func , t_comp_func );
void delete_hash(hash_table *);
void delete_hash_keep_content(hash_table *);
void insert_hash(hash_table * , hash_bucket * ,int);
void remove_hash(hash_table * , hash_bucket * ,int);
hash_bucket * find_hash(hash_table * , void* ,int);


///////////////////////////////////////////////////////////////
#define LS_MEM_MALLOC(Ptr,Type)      Ptr =  (Type *)malloc(sizeof(Type))
#define LS_MEM_FREE(p)\
    if(p){\
        free(p);\
        p = NULL;\
    }

#if 1
struct tuple4
{
   u_short source;
   u_short dest;
   u_int saddr;
   u_int daddr;
};
#endif

uint32_t session_hash(void *key, int);
unsigned char session_comp(void *key1, void *key2, int);
struct tuple5
{
	 struct tuple4 four_attr; //by niulw add 
  unsigned short source;
   unsigned short dest;
   unsigned int saddr;
   unsigned int daddr;
   unsigned int pro;
	 u_int32_t     protocol; //by niulw add
};

typedef struct {
    struct tuple5  key;
    int                  flag;
    char    uuid[UUID_MAX_LENGTH];
    char*   content;
}WebmailSession;

typedef struct https
{
     struct tuple4  key;
     int                  flag;
     int                  len;
     FILE*             old_file;
     char               old_name[128];
     char                uuid[512];
     void             *http;
}
HttpSession;



/*??????Ԫ?飬?ڷ????????ļ?????ʱ???ɹ?ϣ??ַ*/
struct http_IM_tuple5
{
	unsigned short IM_src;	//Դ?˿?
	unsigned short IM_dst;	//Ŀ?Ķ˿?
	unsigned int   IM_sad;	//ԴIP
	unsigned int   IM_dad;	//Ŀ??IP
	char           post_id[128];	//?Զ??????ַ???
};
/*?????????ļ?????ʱ??hashͰ????*/
typedef struct _http_IM_session
{
	unsigned short			block;	//?????ļ?ʱ?ķֿ???��
	unsigned long			range;	//Message Body???ļ??????????????ļ???λ??ƫ??��
	unsigned long			fs;		//?ļ???С
	char					ts[64];	//?ļ???ʼ??????ʱ??
	char					uid[70];//????ԭʼ?ļ???UUID
	struct http_IM_tuple5	key;	//??Ԫ??
	char					fn[256];//ԭʼ?ļ???
//	char					url[1024];
}http_IM_session;
struct http_feition_tuple2
{
    char s[64];
    char host[256];
};

typedef struct _http_feition_session
{
    struct http_feition_tuple2      key;
//  char                            url[1024];
    char                            fn[256];
    char                            uid[70];
    char                            ts[64];
    unsigned long                   fs;
}http_feition_session;

#if 0
typedef struct
{
   uint32_t sip;
   uint32_t dip;
   uint32_t port;
}
datasession;

typedef struct
{
   unsigned  char   flag;
   struct tuple4    key;
   unsigned char file_name[128];
}FtpdataSession;


typedef struct
{
	unsigned char flag;
	unsigned char flag2;
	struct tuple4 key;
    unsigned char file_name[128];
	uint32_t      port;
	uint32_t      ip;
}FtpSession;
#endif

/////////////SMTP///////////SMTP/////////SMTP//////////////SMTP////////////////////////

/*存放email数据包的链表*/
typedef struct SMTP_EML
{
	char			*p_eml;            //邮件数据包
	struct SMTP_EML	*next;
}SMTP_EML;

typedef struct EML_TO
{
    char    *p_to;
	struct  EML_TO  *next;
}EML_TO;
#if 0
/**********************************************************************************/
typedef struct
{
	SMTP_EML        *eml_list;		  //存放email 的链表
	EML_TO          *to_list;		  //存放email 的链表
	unsigned long   eml_to_len;		  //计算发件人所在数据包的大小
	unsigned long   eml_len;		  //用来标记邮件部分的状态
	unsigned short  eml_flag;		  //用来标记邮件部分的状态
	unsigned short  eml_end;		  //当它为1时，说明已经得到了，邮件原始数据
	unsigned short  crlf_seen;		  //
	unsigned short  smtp_state;		  //标记处理数据包的状态
	unsigned short  STARTTLS;		  //标记邮件是否加密的中间状态
	unsigned short  flag;			  //是否读桶数据的标记，为1时，需要读桶
	unsigned short  encypt;			  //邮件加密的标志，加密时为1，当读到它是1的时候需要关闭会话，释放hash
	char            *eml_from;		  //存放发件人
//	char            *eml_to;		  //存放收件人
//	char			*eml;			  //存放需要解析的EMAIL
	char            *smtp_json;		  //包含EML属性的JSON串 ，需要提取
	struct tuple4	key;
}SmtpSession;
/////////////////////////////////////////////////////////////////////////////////////////////////
#endif

#endif
// __BEAP_HASH
