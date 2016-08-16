#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ring_buffer.h"
//#include "session_buf.h"
#include "beap_hash.h"
#include "plog.h"
/***************************
Function: init_slock_for_sync()
Description: sbuff信号池同步初始化函数
Calls: 
Called By: sbuff_create  --------> main
Table Accessed: 
Table Updated: 
Input: 
      int iringbuffer_ID, 信号池ID
	  int iringbuffer_qlength 信号池中环形缓冲区长度
Output: 成功为0，失败为-1
Return: 成功为0，失败为-1
Others: 
***************************/

int init_slock_for_sync(int iringbuffer_ID, int iringbuffer_qlength)//initializing the semaphore lock_object
{
    if(sem_init(&sb_holder[iringbuffer_ID].empty_buffer,0,iringbuffer_qlength) == -1) 
    {
	  log_printf(ZLOG_LEVEL_FATAL, "init_slock_sync FATAL: the slock sync is failed!!");
	  goto lebel;
    }
	
    if(sem_init(&sb_holder[iringbuffer_ID].full_buffer,0,0) == -1) 
    {   
	  sem_destroy(&sb_holder[iringbuffer_ID].empty_buffer);
	  log_printf(ZLOG_LEVEL_FATAL, "init_slock_sync FATAL: the slock sync is failed!!");
  	  goto lebel;
    }
    return 0; 

    lebel : free(sb_holder[iringbuffer_ID].rb_base);//freeing alocated memory on initialisation failure...
    return -1;
}

/***************************
Function: check_for_first_free_sring_buffer()
Description: sring buff中查找空闲的节点
Calls: 
Called By: sbuff_create  --------> main
Table Accessed: 
Table Updated: 
Input: 无
Output: 
Return: 成功为空闲ID号，失败为-1
Others: 
***************************/

int check_for_first_free_sring_buffer()
{
    int i;
    for( i =0 ; i < MAX_SRING_BUFFER_COUNT; i++)
    {
        if(sb_holder[i].rb_base == NULL)
        {
            return i; // Gets the first free ring buffer
        }
    }
	log_printf(ZLOG_LEVEL_FATAL, "Check for free sring buff FATAL: there is no free sring buffer!!");
    return -1;
}

/***************************
Function: create_sring_buffer()
Description: 创建sring_buff环形缓冲区
Calls: 
Called By: sbuff_create  --------> main
Table Accessed: 
Table Updated: 
Input: 
      int iringbuffer_ID, 信号池ID
	  int iringbuffer_qlength 信号池环形缓冲区长度
Output: 
Return: 成功为空闲ID号，失败为-1
Others: 
***************************/

int  create_sring_buffer(int iringbuffer_ID, int iringbuffer_qlength)
{
    sb_holder[iringbuffer_ID].rb_base = (struct session_buffer_node_tag *)calloc(
                                        iringbuffer_qlength,
                                        sizeof(struct session_buffer_node_tag));
    if ( sb_holder[iringbuffer_ID].rb_base == NULL){
	  log_printf(ZLOG_LEVEL_FATAL, "create sring buff FATAL: the base mem alloc is failed!!");
      return -1;
	}
    pthread_mutex_init(&sb_holder[iringbuffer_ID].mutex,NULL);
    pthread_mutex_init(&sb_holder[iringbuffer_ID].pmutex,NULL);
    sb_holder[iringbuffer_ID].irb_qlength          = iringbuffer_qlength; 
    sb_holder[iringbuffer_ID].irb_head_index       = 0; 
    sb_holder[iringbuffer_ID].irb_tail_index       = 0;
    sb_holder[iringbuffer_ID].irb_buffer_end_index = iringbuffer_qlength-1; // need justification

    return iringbuffer_ID;//returning buffer header ID
}

/***************************
Function: sbuff_create()
Description: 创建sring_buff环形缓冲区
Calls: check_for_first_free_sring_buffer
       create_sring_buffer
	   init_slock_for_sync
Called By: main
Table Accessed: 
Table Updated: 
Input: 
	  int irb_qlength 信号池环形缓冲区长度
Output:
Return: 成功为空闲ID号，失败为-1
Others: 
***************************/

int sbuff_create (int irb_qlength)
{
    int irb_id = check_for_first_free_sring_buffer(); // Checks for the very first free 
                                                // ring buffer in the holder array
    if (irb_id == -1)
        return -1; // No Free ring buffers

    int ring_buffer_ID=create_sring_buffer(irb_id, irb_qlength); // Create new ring buffer in the holder with ID received
    if(ring_buffer_ID<0){
	  log_printf(ZLOG_LEVEL_FATAL, "Sbuff_create FATAL: there is no buffer_id for using!!");
      return -1; // Failed to create ring buffer
	}
    if((init_slock_for_sync(irb_id, irb_qlength))==-1){
	  log_printf(ZLOG_LEVEL_FATAL, "Sbuff_create FATAL: the slock sync is failed!!");
	  return -1;       
	}
    return ring_buffer_ID; // Returning the ring buffer ID(index) in success
    
}

/***************************
Function: sbuff_putdata()
Description: 将已经解析的五元组及其他信息放入sring_buff环形缓冲区中
Calls: 
Called By: session_proc_fun，(do_smtp，do_http_file)--------> tcp_callback，(webmail_mail)--------->http_file_thread，session_consumer_thread
Table Accessed: 
Table Updated: 
Input: 
	  const int iringbuffer_ID, 使用中的缓冲区ID号
	  SESSION_BUFFER_NODE ft，五元组及其他信息结构体
Output: 
Return: 成功为1，失败为-1
Others: 
***************************/

int sbuff_putdata(const int iringbuffer_ID, SESSION_BUFFER_NODE ft)
{
    if(sem_wait(&sb_holder[iringbuffer_ID].empty_buffer)==-1)//获得缓冲区信号量有效
    {
	   log_printf(ZLOG_LEVEL_ERROR, "sbuff sem_wait ERROR: sem_wait is failed to lock empty_buffer!!");
       return -1;
    }
    int temp_index;    	
    pthread_mutex_lock(&sb_holder[iringbuffer_ID].pmutex);//进入临界区操作环形缓冲区数据节点
    temp_index = sb_holder[iringbuffer_ID].irb_tail_index;
    if(sb_holder[iringbuffer_ID].irb_tail_index >= sb_holder[iringbuffer_ID].irb_buffer_end_index )
    {
        sb_holder[iringbuffer_ID].irb_tail_index = 0;
    }
    else
    {
        sb_holder[iringbuffer_ID].irb_tail_index++;
    }
    pthread_mutex_unlock(&sb_holder[iringbuffer_ID].pmutex);
	//相应的环形缓冲区数据赋值
    sb_holder[iringbuffer_ID].rb_base[temp_index].session_five_attr = ft.session_five_attr; //五元组信息赋值
    sb_holder[iringbuffer_ID].rb_base[temp_index].attr = ft.attr;//特殊属性赋值
    sb_holder[iringbuffer_ID].rb_base[temp_index].old_file = ft.old_file;//文件句柄赋值
	memcpy(sb_holder[iringbuffer_ID].rb_base[temp_index].orgname, ft.orgname, 128);//网络抓取原始文件名赋值
	memcpy(sb_holder[iringbuffer_ID].rb_base[temp_index].strname, ft.strname, 128);//网络抓取存储文件名赋值
    if(sem_post(&sb_holder[iringbuffer_ID].full_buffer)==-1)//释放已占用的信号资源，相应的信号量值减少
    {
	   log_printf(ZLOG_LEVEL_ERROR, "sbuff sem_post ERROR: failed to unlock in full_buff!!!!");
       return -1;
    }
	   log_printf(ZLOG_LEVEL_DEBUG, "sbuff sem_wait Send OK\n\n");
    return 1;
    
}

/***************************
Function: sbuff_getdata()
Description: 将从sring_buff环形缓冲区中提取SESSION_BUFFER_NODE结构体信息
Calls: 
Called By: http_file_thread，session_consumer_thread，tika_consumer_thread，
Table Accessed: 
Table Updated: 
Input: const int iringbuffer_ID, 使用中的缓冲区ID号
Output: SESSION_BUFFER_NODE ft，五元组及其他信息结构体
Return: 成功为1，失败为-1
Others: 
***************************/

int sbuff_getdata(const int iringbuffer_ID, SESSION_BUFFER_NODE *hdr)
{
//	   log_printf(ZLOG_LEVEL_DEBUG, "sbuff get sem_wait sbuff_getdata Enter into \n\n");
    if(sem_wait(&sb_holder[iringbuffer_ID].full_buffer)==-1)//等待信号有效
    {
	   log_printf(ZLOG_LEVEL_ERROR, "sbuff get sem_wait ERROR: sem_wait is failed to lock empty_buffer!!");
       //printf("sem_wait is failed to lock full_buffer!!\n");
       return -1;
    }
	   log_printf(ZLOG_LEVEL_DEBUG, "sbuff get ===============step 1===========================\n\n\n");

    if((sb_holder[iringbuffer_ID].rb_base)==NULL){ // Checks for Valid Ring Buffer
	   log_printf(ZLOG_LEVEL_ERROR, "sbuff get sem_wait ERROR: the rb_base is NULL!!");
       return -1;
	}

    pthread_mutex_lock(&sb_holder[iringbuffer_ID].mutex);
	//取出已存入结构体的信息数据
    hdr->session_five_attr = sb_holder[iringbuffer_ID].rb_base[sb_holder[iringbuffer_ID].irb_head_index].session_five_attr;
    hdr->old_file = sb_holder[iringbuffer_ID].rb_base[sb_holder[iringbuffer_ID].irb_head_index].old_file;
    hdr->attr = sb_holder[iringbuffer_ID].rb_base[sb_holder[iringbuffer_ID].irb_head_index].attr;
    memcpy(hdr->orgname, sb_holder[iringbuffer_ID].rb_base[sb_holder[iringbuffer_ID].irb_head_index].orgname, 128);
    memcpy(hdr->strname, sb_holder[iringbuffer_ID].rb_base[sb_holder[iringbuffer_ID].irb_head_index].strname, 128);
    if(sb_holder[iringbuffer_ID].irb_head_index >= sb_holder[iringbuffer_ID].irb_qlength -1)
    {
        sb_holder[iringbuffer_ID].irb_head_index=0;
    }
    else        
    {
        sb_holder[iringbuffer_ID].irb_head_index++;
    }
	
    pthread_mutex_unlock(&sb_holder[iringbuffer_ID].mutex);

    if(sem_post(&sb_holder[iringbuffer_ID].empty_buffer)==-1)
	{
	   log_printf(ZLOG_LEVEL_ERROR, "sbuff get sem_post ERROR: sem_post is failed to unlock full_buffer!!");
       return -1;
    }
	   log_printf(ZLOG_LEVEL_DEBUG, "sbuff get ==========================================\n\n\n");
    
    return 1; // return size  in terms of byte successfully Written to buffer
}

/***************************
Function: sbuff_destroy()
Description: 销毁sbuff中的数据链
Calls: 
Called By: 
Table Accessed: 
Table Updated: 
Input: const int iringbuffer_ID, 使用中的缓冲区ID号
Output: 
Return: 成功为1，失败为-1
Others: 
***************************/

int sbuff_destroy(int iringbuffer_ID)
{
    int i;
    
    if((sb_holder[iringbuffer_ID].rb_base)==NULL){ // Checks for Valid Ring Buffer
	   log_printf(ZLOG_LEVEL_WARN, "sbuff destroy WARN: the rb_base is NULL!!");
       return -1;
	}
    for(i=0; i <sb_holder[iringbuffer_ID].irb_qlength ; i++)
    {     
       sb_holder[iringbuffer_ID].rb_base[i].session_five_attr.ip_src = 0;
       sb_holder[iringbuffer_ID].rb_base[i].session_five_attr.ip_dst = 0;
       sb_holder[iringbuffer_ID].rb_base[i].session_five_attr.port_src = 0;
       sb_holder[iringbuffer_ID].rb_base[i].session_five_attr.port_dst = 0;
       sb_holder[iringbuffer_ID].rb_base[i].session_five_attr.protocol = 0;
	   memset(sb_holder[iringbuffer_ID].rb_base[i].orgname,0,128);
	   memset(sb_holder[iringbuffer_ID].rb_base[i].strname,0,128);
    }
    
    sb_holder[iringbuffer_ID].rb_base        = NULL;
    sb_holder[iringbuffer_ID].irb_qlength    = 0; 
    sb_holder[iringbuffer_ID].irb_head_index = 0; 
    sb_holder[iringbuffer_ID].irb_tail_index = 0;
    sem_destroy(&sb_holder[iringbuffer_ID].empty_buffer);    
    sem_destroy(&sb_holder[iringbuffer_ID].full_buffer);    
    return 1; 
}

/***************************
Function: check_for_first_free_ring_buffer()
Description: ring buff中查找空闲的节点
Calls: 
Called By: rbuff_create  --------> main
Table Accessed: 
Table Updated: 
Input: 无
Output: 
Return: 成功为空闲ID号，失败为-1
Others: 
***************************/

int check_for_first_free_ring_buffer()
{
    int i;
    for( i =0 ; i < MAX_RING_BUFFER_COUNT; i++)
    {
        if(rb_holder[i].rb_base == NULL)
        {
            return i; // Gets the first free ring buffer
        }
    }
    return -1;
}

/***************************
Function: create_ring_buffer()
Description: 创建ring_buff环形缓冲区
Calls: 
Called By: rbuff_create  --------> main
Table Accessed: 
Table Updated: 
Input: 
      int iringbuffer_ID, 信号池ID
	  int iringbuffer_qlength 信号池环形缓冲区长度
Output: 
Return: 成功为空闲ID号，失败为-1
Others: 
***************************/

int  create_ring_buffer(int iringbuffer_ID, int iringbuffer_qlength)
{
    rb_holder[iringbuffer_ID].rb_base = (struct ring_buffer_node_tag*)calloc(
                                        iringbuffer_qlength,
                                        sizeof(struct ring_buffer_node_tag));
    if ( rb_holder[iringbuffer_ID].rb_base == NULL){
	   log_printf(ZLOG_LEVEL_FATAL, "ring buff create FATAL: the create process is failed!!");
       return -1;
	}
    pthread_mutex_init(&rb_holder[iringbuffer_ID].mutex,NULL);
    rb_holder[iringbuffer_ID].irb_qlength          = iringbuffer_qlength; 
    rb_holder[iringbuffer_ID].irb_head_index       = 0; 
    rb_holder[iringbuffer_ID].irb_tail_index       = 0;
    rb_holder[iringbuffer_ID].irb_buffer_end_index = iringbuffer_qlength-1; // need justification

    return iringbuffer_ID;//returning buffer header ID
}

/***************************
Function: init_lock_for_sync()
Description: rbuff信号池同步初始化函数
Calls: 
Called By: rbuff_create  --------> main
Table Accessed: 
Table Updated: 
Input: 
      int iringbuffer_ID, 信号池ID
	  int iringbuffer_qlength 信号池中环形缓冲区长度
Output: 
Return: 成功为0，失败为-1
Others: 
***************************/

int init_lock_for_sync(int iringbuffer_ID, int iringbuffer_qlength)//initializing the semaphore lock_object
{
    if(sem_init(&rb_holder[iringbuffer_ID].empty_buffer,0,iringbuffer_qlength) == -1) 
    {
	   log_printf(ZLOG_LEVEL_FATAL, "ring buff sync FATAL: the sync process is failed!!");
	   goto lebel;
    }
	
    if(sem_init(&rb_holder[iringbuffer_ID].full_buffer,0,0) == -1) 
    {   
	   sem_destroy(&rb_holder[iringbuffer_ID].empty_buffer);
	   log_printf(ZLOG_LEVEL_FATAL, "ring buff sem init FATAL: the sem init process is failed!!");
  	   goto lebel;
    }
    return 0; 
    lebel : free(rb_holder[iringbuffer_ID].rb_base);//freeing alocated memory on initialisation failure...
    return -1;
}

/***************************
Function: rbuff_create()
Description: 创建ring_buff环形缓冲区
Calls: check_for_first_free_ring_buffer
       create_ring_buffer
	   init_lock_for_sync
Called By: main
Table Accessed: 
Table Updated: 
Input: 
	  int irb_qlength 信号池环形缓冲区长度
Output:
Return: 成功为空闲ID号，失败为-1
Others: 
***************************/

int rbuff_create (int irb_qlength)
{
    int irb_id = check_for_first_free_ring_buffer(); // Checks for the very first free 
                                                // ring buffer in the holder array
    if (irb_id == -1){
	   log_printf(ZLOG_LEVEL_FATAL, "rbuff create FATAL: there is no id for using!!");
       return -1; // No Free ring buffers
	}

    int ring_buffer_ID=create_ring_buffer(irb_id, irb_qlength); // Create new ring buffer in the holder with ID received
	if(ring_buffer_ID<0){
	   log_printf(ZLOG_LEVEL_FATAL, "rbuff create FATAL: there is no buff id for create!!");
       return -1; // Failed to create ring buffer
	}
    if((init_lock_for_sync(irb_id, irb_qlength))==-1){
	   log_printf(ZLOG_LEVEL_FATAL, "rbuff create FATAL: the sync process is failed!!");
	   return -1;       
	}
    return ring_buffer_ID; // Returning the ring buffer ID(index) in success
    
}

/**Putting data on ring buffer**/
/***************************
Function: rbuff_putdata()
Description: 将已经解析的五元组及数据包信息放入ring_buff环形缓冲区中
Calls: 
Called By: dummyProcessPacket，packet_consumer_thread
Table Accessed: 
Table Updated: 
Input: 
	  const int iringbuffer_ID, 使用的缓冲区ID号
	  struct tuple5 *session_five_attr  五元组及其他信息结构体
	  struct pcap_pkthdr *hdr，数据包头部信息  
	  void *pData，采集的数据包
Output: 
Return: 成功为1，失败为-1
Others: 
***************************/

int rbuff_putdata(const int iringbuffer_ID, void *pData, struct pcap_pkthdr *hdr, struct tuple5 *session_five_attr)
{
    if(sem_wait(&rb_holder[iringbuffer_ID].empty_buffer)==-1)//等待信号量有效
    {
	   log_printf(ZLOG_LEVEL_ERROR, "ring buff put ERROR: the put data process pointer is NULL!!");
       return -1;
    }
	struct tuple5  five_attr;
	struct pcap_pkthdr  temp_hdr;
    temp_hdr.len = hdr->len;
    temp_hdr.caplen = hdr->caplen;
    temp_hdr.ts.tv_sec = hdr->ts.tv_sec;
    temp_hdr.ts.tv_usec = hdr->ts.tv_usec;
	if(session_five_attr != NULL){//五元组信息有效性赋值
	  five_attr.four_attr.saddr = session_five_attr->four_attr.saddr;
	  five_attr.four_attr.daddr = session_five_attr->four_attr.daddr;
      five_attr.four_attr.source = session_five_attr->four_attr.source;
	  five_attr.four_attr.dest= session_five_attr->four_attr.dest;
	  five_attr.protocol = session_five_attr->protocol;
	  //five_attr.flow_flag = session_five_attr->flow_flag;
	  //five_attr.flow = session_five_attr->flow;
	}

    if((temp_hdr.caplen>0)&&(temp_hdr.caplen<=65536)){//判断采集数据包是否有效
       rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pdata_buffer = ikmem_malloc(temp_hdr.caplen);// Allocating memory
       if(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pdata_buffer == NULL){ // Checks Memory  Allocated
	      log_printf(ZLOG_LEVEL_ERROR, "ring buff put ERROR: the put packet data pointer is NULL!!");
          return -1;
	   }else{
          if(memcpy(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pdata_buffer,pData,temp_hdr.caplen)==NULL)//转存数据
          {   
             ikmem_free(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pdata_buffer);//释放之前内存
             rb_holder[iringbuffer_ID].rb_base=NULL;
	         log_printf(ZLOG_LEVEL_ERROR, "ring buff put ERROR: the packet copy is failed!!");
             return -1;
          }
	   }
	}
    else{
	   log_printf(ZLOG_LEVEL_ERROR, "ring buff put ERROR: the put packet len is over flow, the packet len is %d!!", temp_hdr.len);
       return -1;    
	}            
    //赋值数据包头信息
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pkthdr.len = temp_hdr.len;//数据包长度
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pkthdr.caplen = temp_hdr.caplen;//采集数据包长度
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pkthdr.ts.tv_sec = temp_hdr.ts.tv_sec;//采集时间(秒)
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].pkthdr.ts.tv_usec = temp_hdr.ts.tv_usec;//采集时间(微妙)
	if(session_five_attr != NULL)//五元组信息有效性赋值
	{
		rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].session_five_attr.ip_src = five_attr.four_attr.saddr;
		rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].session_five_attr.ip_dst = five_attr.four_attr.daddr;
		rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].session_five_attr.port_src= five_attr.four_attr.source;
		rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].session_five_attr.port_dst= five_attr.four_attr.dest;
		rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].session_five_attr.protocol = five_attr.protocol;
		//rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].session_five_attr.flow_flag = five_attr.flow_flag;
		//rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_tail_index].session_five_attr.flow = five_attr.flow;
	}
    //修改环形缓冲区地址
    if(rb_holder[iringbuffer_ID].irb_tail_index >= rb_holder[iringbuffer_ID].irb_buffer_end_index )
    {
        rb_holder[iringbuffer_ID].irb_tail_index = 0;
    }
    else
    {
        rb_holder[iringbuffer_ID].irb_tail_index++;
    }
	
    if(sem_post(&rb_holder[iringbuffer_ID].full_buffer)==-1)//释放占用的信号量
    {
	   log_printf(ZLOG_LEVEL_ERROR, "ring buff put ERROR: sem_post is failed to unlock in full_buffer!!");
       return -1;
    }
    return 1;
}

/**Getting data from ring buffer**/
/***************************
Function: rbuff_getdata()
Description: 将从ring_buff环形缓冲区中提取数据包以及相应的头信息和五元组信息
Calls: 
Called By: session_process_thread，packet_consumer_thread
Table Accessed: 
Table Updated: 
Input: const int iringbuffer_ID, 使用的缓冲区ID号
Output: void *pData  缓冲区中的数据
        struct pcap_pkthdr *hdr  数据包头部信息
		struct tuple5 *session_five_attr  五元组信息
Return: 成功为1，失败为-1
Others: 
***************************/

int rbuff_getdata(const int iringbuffer_ID, void *pData, struct pcap_pkthdr *hdr, struct tuple5 *session_five_attr)
{
	if(sem_wait(&rb_holder[iringbuffer_ID].full_buffer)==-1)//等待系统信号量
    {
       if(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer != NULL)//如果等待失效，释放资源
       {
         ikmem_free(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer);
	   }
	   log_printf(ZLOG_LEVEL_ERROR, "ring buff get ERROR: sem_wait is failed to lock in full_buffer!!");
       return -1;
    }

    if((rb_holder[iringbuffer_ID].rb_base)==NULL){ // Checks for Valid Ring Buffer
	   log_printf(ZLOG_LEVEL_ERROR, "ring buff get ERROR: the rb_base pointer is NULL!!");
       return -1;
	}

    // 判断数据包数据的有效性
    if((rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.caplen>0)&&(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.caplen<=65536))
    {
       if(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer != NULL)
       {
          if(NULL == memcpy(pData,rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer,
                rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.caplen)){//转存数据包
             ikmem_free(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer);
             rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer = NULL;
	         log_printf(ZLOG_LEVEL_ERROR, "ring buff get ERROR: copy data is failed!!");
             return -1;
		  }
       }
       else{
	      log_printf(ZLOG_LEVEL_ERROR, "ring buff get ERROR: the pdata buff pointer is NULL!!");
          return -1;
	   }
    }    
    else{
       if(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer != NULL)
       {
         ikmem_free(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer);
	   }
	   log_printf(ZLOG_LEVEL_ERROR, "ring buff get ERROR: the packet caplen is %d!!",rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.caplen);
       return -1;
	}
	//转存数据包头信息
    hdr->len = rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.len;
    hdr->caplen = rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.caplen;
    hdr->ts.tv_sec = rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.ts.tv_sec;
    hdr->ts.tv_usec = rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.ts.tv_usec;
	//转存数据包五元组信息
	if(session_five_attr != NULL)
	{
		session_five_attr->four_attr.saddr= rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].session_five_attr.ip_src;
		session_five_attr->four_attr.dest= rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].session_five_attr.port_dst;
		session_five_attr->four_attr.source= rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].session_five_attr.port_src;
		session_five_attr->four_attr.daddr= rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].session_five_attr.ip_dst;
		session_five_attr->protocol = rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].session_five_attr.protocol;
		//session_five_attr->flow_flag = rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].session_five_attr.flow_flag;
		//session_five_attr->flow = rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].session_five_attr.flow;
	}
	//释放之前资源
    ikmem_free(rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer);
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pdata_buffer = NULL;
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.len = 0;
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.caplen = 0;
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.ts.tv_sec = 0;
    rb_holder[iringbuffer_ID].rb_base[rb_holder[iringbuffer_ID].irb_head_index].pkthdr.ts.tv_usec = 0;
    //updating head pointer
    if(rb_holder[iringbuffer_ID].irb_head_index >= rb_holder[iringbuffer_ID].irb_buffer_end_index)//修改环形缓冲区地址指针
    {
        rb_holder[iringbuffer_ID].irb_head_index=0;
    }
    else        
    {
        rb_holder[iringbuffer_ID].irb_head_index++;
    }

    if(sem_post(&rb_holder[iringbuffer_ID].empty_buffer)==-1)//释放系统信号量
    {
	   log_printf(ZLOG_LEVEL_ERROR, "ring buff get ERROR: sem_post is failed to unlock in empty_buffer!!");
       return -1;
    }
    return 1; // return size  in terms of byte successfully Written to buffer
}

/**Destroying Ring Buffer**/
/***************************
Function: rbuff_destroy()
Description: 销毁buff中的数据链
Calls: 
Called By: 
Table Accessed: 
Table Updated: 
Input: const int iringbuffer_ID, 使用的缓冲区ID号
Output: 
Return: 成功为1，失败为-1
Others: 
***************************/

int rbuff_destroy(int iringbuffer_ID)
{
    int i;
    
    if((rb_holder[iringbuffer_ID].rb_base)==NULL){ // Checks for Valid Ring Buffer
	   log_printf(ZLOG_LEVEL_WARN, "ring buff destroy WARN: the rb_base pointer is NULL!!");
       return -1;
	}
        
    for(i=0; i <rb_holder[iringbuffer_ID].irb_qlength ; i++)
    {
        if(rb_holder[iringbuffer_ID].rb_base[i].pdata_buffer != NULL)
        {
            ikmem_free(rb_holder[iringbuffer_ID].rb_base[i].pdata_buffer);
        }
    }
    
    rb_holder[iringbuffer_ID].rb_base        = NULL;
    rb_holder[iringbuffer_ID].irb_qlength    = 0; 
    rb_holder[iringbuffer_ID].irb_head_index = 0; 
    rb_holder[iringbuffer_ID].irb_tail_index = 0;
    sem_destroy(&rb_holder[iringbuffer_ID].empty_buffer);    
    sem_destroy(&rb_holder[iringbuffer_ID].full_buffer);    
    return 1; 
}
