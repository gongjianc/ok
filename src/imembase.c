/**********************************************************************
 *
 * imembase.c - basic interface of memory operation
 *
 * - application layer slab allocator implementation
 * - unit interval time cost: almost speed up 500% - 1200% vs malloc
 * - optional page supplier: with the "GFP-Tree" algorithm
 * - memory recycle: automatic give memory back to os to avoid wasting
 * - platform independence
 *
 * for the basic information about slab algorithm, please see:
 *   The Slab Allocator: An Object Caching Kernel 
 *   Memory Allocator (Jeff Bonwick, Sun Microsystems, 1994)
 * with the URL below:
 *   http://citeseer.ist.psu.edu/bonwick94slab.html
 *
 * history of this file:
 * - May. 14 2007 skywind  created this file
 * - May. 15 2007 skywind  implemented basic algorithms (imnode, ...)
 * - May. 21 2007 skywind  page supplier supported
 * - May. 22 2007 skywind  basic slab struct algorithms added
 * - May. 28 2007 skywind  page supplier optimized: the GFP-TREE
 * - May. 29 2007 skywind  first completed interface version
 * - Jun. 02 2007 skywind  optimized size-location with a lookup table
 *   
 *
 **********************************************************************/

#include "imembase.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define IMEMSLAB_NEXT(ptr) (((void**)(ptr))[0])

/*====================================================================*/
/* SLAB INTERFACE                                                     */
/*====================================================================*/

/* init slab structure with given memory block and coloroff */
long imslab_init(struct IMEMSLAB *slab, void*mem, long len, int dlen, int off)
{
	char *start = ((char*)mem) + off;
	char *endup = ((char*)mem) + len - dlen;
	long retval = 0;
	char *tail = NULL;

	assert(slab && mem);
	assert((unsigned long)dlen >= sizeof(void*));

	iqueue_init(&slab->queue);
	slab->membase = mem;
	slab->memsize = len;
	slab->coloroff = off;
	slab->inuse = 0;
	slab->extra = NULL;
	slab->bufctl = NULL;

	for (tail = NULL; start <= endup; start += dlen) {
		IMEMSLAB_NEXT(start) = NULL;
		if (tail == NULL) slab->bufctl = start;
		else IMEMSLAB_NEXT(tail) = start;
		tail = start;
		retval++;
	}

	return retval;
}


/* alloc data from slab */ 
void *imslab_alloc(struct IMEMSLAB *slab)
{
	void *ptr = NULL;

	if (slab->bufctl == 0) return 0;
	ptr = slab->bufctl;
	slab->bufctl = IMEMSLAB_NEXT(slab->bufctl);
	slab->inuse++;

	return ptr;
}


/* free data into slab */
void imslab_free(struct IMEMSLAB *slab, void *ptr)
{
	char *start = ((char*)slab->membase) + slab->coloroff;
	char *endup = ((char*)slab->membase) + slab->memsize;
	char *p = (char*)ptr;

	assert(slab->inuse > 0);
	assert(p >= start && p < endup);

	if (p >= start && p < endup) {
		IMEMSLAB_NEXT(p) = slab->bufctl;
		slab->bufctl = p;
	}

	slab->inuse--;
}

/*====================================================================*/
/* EXTERNAL ALLOCATOR                                                 */
/*====================================================================*/
void *(*imv_malloc_ptr)(unsigned long size) = 0;
void (*imv_free_ptr)(void *mem) = 0;


void* imv_malloc(struct IALLOCATOR *allocator, unsigned long size)
{
	if (allocator) {
		return allocator->alloc(allocator, size);
	}
	if (imv_malloc_ptr) {
		return imv_malloc_ptr(size);
	}
	return malloc(size);
}

void imv_free(struct IALLOCATOR *allocator, void *ptr)
{
	if (allocator) {
		allocator->free(allocator, ptr);
		return;
	}
	if (imv_free_ptr) {
		imv_free_ptr(ptr);
		return;
	}
	free(ptr);
}


/*====================================================================*/
/* IVECTOR                                                           */
/*====================================================================*/
void iv_init(struct IVECTOR *v, struct IALLOCATOR *allocator)
{
	if (v == 0) return;
	v->data = 0;
	v->length = 0;
	v->block = 0;
	v->allocator = allocator;
}

void iv_destroy(struct IVECTOR *v)
{
	if (v == NULL) return;
	if (v->data) {
		imv_free(v->allocator, v->data);
	}
	v->data = NULL;
	v->length = 0;
	v->block = 0;
}

int iv_resize(struct IVECTOR *v, unsigned long newsize)
{
	unsigned char*lptr = NULL;
	unsigned long block = 0, min = 0;
	unsigned long nblock = 0;

	if (v == NULL) return -1;
	if (newsize > v->length && newsize <= v->block) { 
		v->length = newsize; 
		return 0; 
	}

	for (nblock = 1; nblock < newsize; ) nblock <<= 1;
	block = nblock;

	if (block == v->block) { 
		v->length = newsize; 
		return 0; 
	}

	if (v->block == 0 || v->data == NULL) {
		v->data = (unsigned char*)imv_malloc(v->allocator, block);
		if (v->data == NULL) return -1;
		v->length = newsize;
		v->block = block;
	}    else {
		lptr = (unsigned char*)imv_malloc(v->allocator, block);
		if (lptr == NULL) return -1;

		min = (v->length <= newsize)? v->length : newsize;
		memcpy(lptr, v->data, (size_t)min);
		imv_free(v->allocator, v->data);

		v->data = lptr;
		v->length = newsize;
		v->block = block;
	}

	return 0;
}



/*====================================================================*/
/* IMEMFIXED                                                          */
/*====================================================================*/
void imnode_init(struct IMEMNODE *mn, long nodesize, struct IALLOCATOR *ac)
{
	struct IMEMNODE *mnode = mn;
	unsigned long newsize = 0, shift = 0;

	assert(mnode != NULL);
	mnode->allocator = ac;

	iv_init(&mnode->vprev, ac);
	iv_init(&mnode->vnext, ac);
	iv_init(&mnode->vnode, ac);
	iv_init(&mnode->vdata, ac);
	iv_init(&mnode->vmem, ac);
	iv_init(&mnode->vmode, ac);

	for (shift = 1; (1ul << shift) < (unsigned long)nodesize; ) shift++;

	newsize = (nodesize < IMROUNDSIZE)? IMROUNDSIZE : nodesize;
	newsize = IMROUNDUP(newsize);

	mnode->node_size = newsize;
	mnode->node_shift = (int)shift;
	mnode->node_free = 0;
	mnode->node_max = 0;
	mnode->mem_max = 0;
	mnode->mem_count = 0;
	mnode->list_open = -1;
	mnode->list_close = -1;
	mnode->total_mem = 0;
	mnode->grow_limit = 0;
	mnode->extra = NULL;
}

void imnode_destroy(struct IMEMNODE *mnode)
{
    int i;

	assert(mnode != NULL);
	if (mnode->mem_count > 0) {
		for (i = 0; i < mnode->mem_count && mnode->mmem; i++) {
			if (mnode->mmem[i]) {
				imv_free(mnode->allocator, mnode->mmem[i]);
			}
			mnode->mmem[i] = NULL;
		}
		mnode->mem_count = 0;
		mnode->mem_max = 0;
		iv_destroy(&mnode->vmem);
		mnode->mmem = NULL;
	}

	iv_destroy(&mnode->vprev);
	iv_destroy(&mnode->vnext);
	iv_destroy(&mnode->vnode);
	iv_destroy(&mnode->vdata);
	iv_destroy(&mnode->vmode);

	mnode->mprev = NULL;
	mnode->mnext = NULL;
	mnode->mnode = NULL;
	mnode->mdata = NULL;
	mnode->mmode = NULL;

	mnode->node_free = 0;
	mnode->node_max = 0;
	mnode->list_open = -1;
	mnode->list_close= -1;
	mnode->total_mem = 0;
}

static int imnode_node_resize(struct IMEMNODE *mnode, long size)
{
	unsigned long size1, size2;

	size1 = (unsigned long)(size * (long)sizeof(long));
	size2 = (unsigned long)(size * (long)sizeof(void*));

	if (iv_resize(&mnode->vprev, size1)) return -1;
	if (iv_resize(&mnode->vnext, size1)) return -2;
	if (iv_resize(&mnode->vnode, size1)) return -3;
	if (iv_resize(&mnode->vdata, size2)) return -5;
	if (iv_resize(&mnode->vmode, size1)) return -6;

	mnode->mprev = (long*)((void*)mnode->vprev.data);
	mnode->mnext = (long*)((void*)mnode->vnext.data);
	mnode->mnode = (long*)((void*)mnode->vnode.data);
	mnode->mdata =(void**)((void*)mnode->vdata.data);
	mnode->mmode = (long*)((void*)mnode->vmode.data);
	mnode->node_max = size;

	return 0;
}

static int imnode_mem_add(struct IMEMNODE *mnode, int node_count, void **mem)
{
	unsigned long newsize;
	char *mptr;

	if (mnode->mem_count >= mnode->mem_max) {
		newsize = (mnode->mem_max <= 0)? IMROUNDSIZE : mnode->mem_max * 2;
		if (iv_resize(&mnode->vmem, newsize * sizeof(void*))) 
			return -1;
		mnode->mem_max = newsize;
		mnode->mmem = (char**)((void*)mnode->vmem.data);
	}
	newsize = node_count * mnode->node_size + IMROUNDSIZE;
	mptr = (char*)imv_malloc(mnode->allocator, newsize);
	if (mptr == NULL) return -2;

	mnode->mmem[mnode->mem_count++] = mptr;
	mnode->total_mem += newsize;
	mptr = (char*)IMROUNDUP(((unsigned long)mptr));

	if (mem) *mem = mptr;

	return 0;
}


static long imnode_grow(struct IMEMNODE *mnode)
{
	int size_start = mnode->node_max;
	int size_endup;
	int retval, count, i, j;
	void *mptr;
	char *p;

	count = (mnode->node_max <= 0)? IMROUNDSIZE : mnode->node_max;
	if (mnode->grow_limit > 0) {
		if (count > mnode->grow_limit) count = mnode->grow_limit;
	}
	size_endup = size_start + count;

	retval = imnode_mem_add(mnode, count, &mptr);
	if (retval) return -10 + retval;

	retval = imnode_node_resize(mnode, size_endup);
	if (retval) return -20 + retval;

	p = (char*)mptr;
	for (i = mnode->node_max - 1, j = 0; j < count; i--, j++) {
		IMNODE_NODE(mnode, i) = 0;
		IMNODE_MODE(mnode, i) = 0;
		IMNODE_DATA(mnode, i) = p;
		IMNODE_PREV(mnode, i) = -1;
		IMNODE_NEXT(mnode, i) = mnode->list_open;
		if (mnode->list_open >= 0) IMNODE_PREV(mnode, mnode->list_open) = i;
		mnode->list_open = i;
		mnode->node_free++;
		p += mnode->node_size;
	}

	return 0;
}


long imnode_new(struct IMEMNODE *mnode)
{
	long node, next;

	assert(mnode);
	if (mnode->list_open < 0) {
		if (imnode_grow(mnode)) return -2;
	}

	if (mnode->list_open < 0 || mnode->node_free <= 0) return -3;

	node = mnode->list_open;
	next = IMNODE_NEXT(mnode, node);
	if (next >= 0) IMNODE_PREV(mnode, next) = -1;
	mnode->list_open = next;

	IMNODE_PREV(mnode, node) = -1;
	IMNODE_NEXT(mnode, node) = mnode->list_close;

	if (mnode->list_close >= 0) IMNODE_PREV(mnode, mnode->list_close) = node;
	mnode->list_close = node;
	IMNODE_MODE(mnode, node) = 1;

	mnode->node_free--;

	return node;
}

void imnode_del(struct IMEMNODE *mnode, long index)
{
	long prev, next;

	assert(mnode);
	assert((index >= 0) && (index < mnode->node_max));
	assert(IMNODE_MODE(mnode, index));

	next = IMNODE_NEXT(mnode, index);
	prev = IMNODE_PREV(mnode, index);

	if (next >= 0) IMNODE_PREV(mnode, next) = prev;
	if (prev >= 0) IMNODE_NEXT(mnode, prev) = next;
	else mnode->list_close = next;

	IMNODE_PREV(mnode, index) = -1;
	IMNODE_NEXT(mnode, index) = mnode->list_open;

	if (mnode->list_open >= 0) IMNODE_PREV(mnode, mnode->list_open) = index;
	mnode->list_open = index;

	IMNODE_MODE(mnode, index) = 0;
	mnode->node_free++;
}

long imnode_head(struct IMEMNODE *mnode)
{
	return (mnode)? mnode->list_close : -1;
}

long imnode_next(struct IMEMNODE *mnode, long index)
{
	return (mnode)? IMNODE_NEXT(mnode, index) : -1;
}

void*imnode_data(struct IMEMNODE *mnode, long index)
{
	return (char*)IMNODE_DATA(mnode, index);
}


/*====================================================================*/
/* IMUTEXHOOK                                                         */
/*====================================================================*/
static struct IMEMNODE imutex_node;
static struct IMEMNODE imspin_node;
static struct IMUTEXHOOK imutex_hook;
static long imutex_inited = 0;
static long imutex_master = -1;
static long*imutex_core = NULL;
static long imutex_neglected = 0;

void imutex_setup(struct IMUTEXHOOK *hook)
{
	if (imutex_inited) return;
	memset(&imutex_hook, 0, sizeof(imutex_hook));
	imutex_hook.mutex_size = 4;
	imutex_hook.mspin_size = 4;
	if (hook) memcpy(&imutex_hook, hook, sizeof(imutex_hook));
	if (imutex_hook.mutex_size < 4) imutex_hook.mutex_size = 4;
	if (imutex_hook.mspin_size < 4) imutex_hook.mspin_size = 4;
	imnode_init(&imutex_node, imutex_hook.mutex_size + sizeof(long), 0);
	imnode_init(&imspin_node, imutex_hook.mspin_size + sizeof(long), 0);
	imutex_master = imnode_new(&imutex_node);
	assert(imutex_master >= 0);
	imutex_core = (long*)imnode_data(&imutex_node, imutex_master);
	*imutex_core++ = imutex_master;
	if (imutex_hook.mutex_init) 
		imutex_hook.mutex_init(imutex_core);
	imutex_neglected = 0;
	imutex_inited = 1;
}

void imutex_cleanup(void)
{
	long index;
	char*ptr;
	if (imutex_inited == 0) return;
	imutex_lock(imutex_core);
	for (index = imnode_head(&imutex_node); index >= 0; ) {
		if (index == imutex_master) continue;
		ptr = (char*)imnode_data(&imutex_node, index);
		ptr = ptr + sizeof(long);
		if (imutex_hook.mutex_destroy) 
			imutex_hook.mutex_destroy(ptr);
		index = imnode_next(&imutex_node, index);
	}
	for (index = imnode_head(&imspin_node); index >= 0; ) {
		index = imnode_next(&imspin_node, index);
	}
	imutex_unlock(imutex_core);
	if (imutex_hook.mutex_destroy) 
		imutex_hook.mutex_destroy(imutex_core);
	imnode_destroy(&imutex_node);
	imnode_destroy(&imspin_node);
	imutex_master = -1;
	imutex_core = NULL;
	imutex_inited = 0;
}

void imutex_neglect(int isneglect)
{
	imutex_neglected = isneglect;
}

imutex_t imutex_new(void)
{
	long index;
	char *ptr;
	if (imutex_inited == 0) return NULL;
	imutex_lock(imutex_core);
	index = imnode_new(&imutex_node);
	assert(index >= 0);
	ptr = ((char*)imnode_data(&imutex_node, index));
	*(long*)ptr = index;
	ptr += sizeof(long);
	if (imutex_hook.mutex_init) {
		if (imutex_hook.mutex_init(ptr)) {
			imnode_del(&imutex_node, index);
			imutex_unlock(imutex_core);
			return NULL;
		}
	}
	imutex_unlock(imutex_core);
	return (imutex_t)ptr;
}

void imutex_del(imutex_t mutex)
{
	long index;
	char *ptr;
	if (imutex_inited == 0 || mutex == 0) return;
	imutex_lock(imutex_core);
	ptr = (char*)mutex;
	index = *(long*)(ptr - sizeof(long));
	assert(index >= 0 && index < imutex_node.node_max);
	assert(IMNODE_MODE(&imutex_node, index));
	if (imutex_hook.mutex_destroy) 
		imutex_hook.mutex_destroy(ptr);
	imnode_del(&imutex_node, index);
	imutex_unlock(imutex_core);
}

void imutex_lock(imutex_t mutex)
{
	if (imutex_hook.mutex_lock && imutex_neglected == 0) 
		imutex_hook.mutex_lock(mutex);
}

void imutex_unlock(imutex_t mutex)
{
	if (imutex_hook.mutex_unlock && imutex_neglected == 0) 
		imutex_hook.mutex_unlock(mutex);
}

int imutex_currentcpu(void)
{
	int retval = 0;
	if (imutex_hook.current_cpu)
		retval = imutex_hook.current_cpu();
	return retval & (IMUTEX_MAXCPU - 1);
}


#ifndef IMUTEX_TYPE

#if (defined(WIN32) || defined(_WIN32))
#include <windows.h>
#define IMUTEX_TYPE			CRITICAL_SECTION
#define IMUTEX_INIT(m)		InitializeCriticalSection((CRITICAL_SECTION*)(m))
#define IMUTEX_DESTROY(m)	DeleteCriticalSection((CRITICAL_SECTION*)(m))
#define IMUTEX_LOCK(m)		EnterCriticalSection((CRITICAL_SECTION*)(m))
#define IMUTEX_UNLOCK(m)	LeaveCriticalSection((CRITICAL_SECTION*)(m))

#elif defined(__unix)
#include <unistd.h>
#include <pthread.h>
#define IMUTEX_TYPE			pthread_mutex_t
#define IMUTEX_INIT(m)		pthread_mutex_init((pthread_mutex_t*)(m), 0)
#define IMUTEX_DESTROY(m)	pthread_mutex_destroy((pthread_mutex_t*)(m))
#define IMUTEX_LOCK(m)		pthread_mutex_lock((pthread_mutex_t*)(m))
#define IMUTEX_UNLOCK(m)	pthread_mutex_unlock((pthread_mutex_t*)(m))
#endif

#endif

#ifdef IMUTEX_TYPE

static int imutex_default_init(void *mutex) {
	IMUTEX_INIT(mutex);
	return 0;
}

static void imutex_default_destroy(void *mutex) {
	IMUTEX_DESTROY(mutex);
}

static void imutex_default_lock(void *mutex) {
	IMUTEX_LOCK(mutex);
}

static void imutex_default_unlock(void *mutex) {
	IMUTEX_UNLOCK(mutex);
}

static struct IMUTEXHOOK imutex_win32 = {
	sizeof(IMUTEX_TYPE), 4, 
	imutex_default_init,
	imutex_default_destroy,
	imutex_default_lock,
	imutex_default_unlock,
	NULL
};

void imutex_default_setup(void)
{
	imutex_setup(&imutex_win32);
}

#endif


/*====================================================================*/
/* IMLOGGING                                                          */
/*====================================================================*/
static imlogging_t imlog_writer = NULL;
static imutex_t imlog_mutex = NULL;
static int imlog_inited = 0;
static int imlog_mask = 0;

void imlog_setmask(int logmask)
{
	imlog_mask = logmask;
}

void imlog_setup(imlogging_t handler)
{
	imlog_writer = handler;
	if (imlog_inited) return;
	if (imutex_inited == 0) {
		#ifdef IMUTEX_TYPE
			imutex_default_setup();
		#else
			imutex_setup(NULL);
		#endif
	}
	imlog_mutex = imutex_new();
	imlog_inited = 1;
}

void imlog_destroy(void)
{
	if (imlog_inited == 0) return;
	imutex_lock(imlog_mutex);
	imutex_unlock(imlog_mutex);
	imutex_del(imlog_mutex);
	imlog_mutex = NULL;
	imlog_inited = 0;
}

#ifdef IMLOG_ENABLE
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static int imlog_mode = 0;
static char imlog_directory[] = "./";

static void imlogging_text_handler(const char *text) 
{
	static char fname[4097] = { 0 };
	static time_t tt_save = 0, tt_now = 0;
	static struct tm tm_time, *tmx = &tm_time;
	static char timetxt[32];
	static char mtext[4097];
	static char xtext[4097];
	static int daylog = 0;
	static FILE *fp = NULL;
	int mtime, i;

	tt_now = time(NULL);
	mtime = clock() % 1000;
	mtime = mtime;

	if (tt_now != tt_save) {
		tt_save = tt_now;
		memcpy(&tm_time, localtime(&tt_now), sizeof(tm_time));
		sprintf(timetxt, "%4d-%2d-%2d#%2d:%2d:%2d", tmx->tm_year + 1900, 
			tmx->tm_mon + 1, tmx->tm_mday, tmx->tm_hour, 
			tmx->tm_min, tmx->tm_sec);
		for (i = 0; timetxt[i]; i++) {
			if (timetxt[i] == ' ') timetxt[i] = '0';
			else if (timetxt[i] == '#') timetxt[i] = ' ';
		}
		if (daylog != tmx->tm_mday) {
			fname[0] = 0;
			daylog = tmx->tm_mday;
			if (fp) fclose(fp);
			fp = NULL;
		}
	}
	if (fname[0] == 0) {
		sprintf(mtext, "m%4d%2d%2d.log", tmx->tm_year + 1900, 
			tmx->tm_mon + 1, tmx->tm_mday);
		for (i = 0; mtext[i]; i++) 
			mtext[i] = (mtext[i] == ' ')? '0' : mtext[i];
		sprintf(fname, "%s%s", imlog_directory, mtext);
	}
	if (fp == NULL) fp = fopen(fname, "a");
	if (fp == NULL) return;
	for (i = 0; text[i] && i < 4096; i++) 
		mtext[i] = (text[i] >= 0x20)? text[i] : '.';
	mtext[i] = 0;

	fprintf(fp, "[%s] %s\n", timetxt, mtext);
	fflush(fp);
	if (imlog_mode & 1) sprintf(xtext, "[%s] %s\n", timetxt, mtext);
	if (imlog_mode & 2) printf("[%s] %s\n", timetxt, mtext);
}

void imlog_setup_debug(int logmode)
{
	imlog_mode = logmode;
	imlog_setup(imlogging_text_handler);
}

#endif


void imlog_write(int level, const char *fmt, ...)
{
	#ifdef IMLOG_ENABLE
	static char text[4097];
	va_list argptr;
	if ((level & imlog_mask) == 0) return;
	if (imlog_inited == 0) return;
	imutex_lock(imlog_mutex);
	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);
	if (imlog_writer) imlog_writer(text);
	imutex_unlock(imlog_mutex);
	#endif
}


/*====================================================================*/
/* IMPAGE                                                             */
/*====================================================================*/
static struct IMEMNODE impage_node;
static imutex_t impage_mutex = NULL;
static int impage_inited = 0;
unsigned long impage_shift = 12;
unsigned long impage_psize = 0;
static unsigned long impage_inuse = 0;
static unsigned long impage_in = 0;
static unsigned long impage_out = 0;


void impage_setup(long pageshift)
{
	if (impage_inited) return;
	pageshift = pageshift < 12 ? 12 : pageshift;
	if (imutex_inited == 0) {
		#ifdef IMUTEX_TYPE
			imutex_default_setup();
		#else
			imutex_setup(NULL);
		#endif
	}
	impage_mutex = imutex_new();
	impage_shift = pageshift;
	impage_psize = (1ul << impage_shift) + 32 * IMROUNDUP(sizeof(void*));
	impage_psize = IMROUNDUP(impage_psize);
	imnode_init(&impage_node, impage_psize + IMROUNDUP(sizeof(long)), 0);
	impage_node.grow_limit = 32;
	impage_inuse = 0;
	impage_in = 0;
	impage_out = 0;
	impage_inited = 1;
	imlog_write(IMLOG_PAGE, "[PAGE] init page-shift=%d page-size=%d",
		impage_shift, impage_psize);
}

void impage_destroy(void)
{
	if (impage_inited == 0) return;
	imutex_lock(impage_mutex);
	imnode_destroy(&impage_node);
	imutex_unlock(impage_mutex);
	imutex_del(impage_mutex);
	impage_mutex = 0;
	impage_inuse = 0;
	impage_in = 0;
	impage_out = 0;
	impage_inited = 0;
	imlog_write(IMLOG_PAGE, "[PAGE] destroy");
}

void *impage_alloc(void)
{
	long index;
	char *lptr;
	if (impage_inited == 0) 
		impage_setup(IMPAGE_DEFAULT);
	imutex_lock(impage_mutex);
	index = imnode_new(&impage_node);
	assert(index >= 0);
	lptr = (char*)imnode_data(&impage_node, index);
	impage_inuse++;
	impage_in++;
	imutex_unlock(impage_mutex);
	*(long*)lptr = index;
	lptr += IMROUNDUP(sizeof(long));
	assert(IMROUNDUP((unsigned long)lptr) == (unsigned long)lptr);
	if (imlog_mask & IMLOG_PAGE)
		imlog_write(IMLOG_PAGE, "[PAGE] pagenew=%lXh inuse=%d", 
			lptr, impage_inuse);
	return lptr;
}

void impage_free(void *ptr)
{
	long index;
	char *lptr = (char*)ptr;
	assert(impage_inited);
	if (imlog_mask & IMLOG_PAGE) {
		imlog_write(IMLOG_PAGE, "[PAGE] pagedel=%lxh inuse=%d", 
				ptr, impage_inuse);
	}
	lptr -= IMROUNDUP(sizeof(long));
	index = *(long*)lptr;
	imutex_lock(impage_mutex);
	assert(index >= 0 && index < impage_node.node_max);
	assert(IMNODE_MODE(&impage_node, index));
	assert(lptr == (char*)imnode_data(&impage_node, index));
	imnode_del(&impage_node, index);
	impage_inuse--;
	impage_out++;
	imutex_unlock(impage_mutex);
}

void impage_state(long *inuse, long *in, long *out)
{
	if (inuse) inuse[0] = (long)impage_inuse;
	if (in) in[0] = (long)impage_in;
	if (out) out[0] = (long)impage_out;
}

void impage_info(long *pageshift, long *pagesize)
{
	if (pageshift) pageshift[0] = impage_shift;
	if (pagesize) pagesize[0] = impage_psize;
}


/*====================================================================*/
/* IMSLABSET                                                          */
/*====================================================================*/
static struct IMEMNODE imslabset_node;
static imutex_t imslabset_mutex = NULL;
static int imslabset_inited = 0;

void imslabset_init(void)
{
	unsigned long size;
	if (imslabset_inited) return;
	if (impage_inited == 0) impage_setup(IMPAGE_DEFAULT);
	imslabset_mutex = imutex_new();
	size = IMROUNDUP(sizeof(long));
	size = IMROUNDUP(size + sizeof(struct IMEMSLAB));
	imnode_init(&imslabset_node, size, 0);
	imslabset_inited = 1;
}

void imslabset_destroy(void)
{
	if (imslabset_inited == 0) return;
	imutex_lock(imslabset_mutex);
	imnode_destroy(&imslabset_node);
	imutex_unlock(imslabset_mutex);
	imutex_del(imslabset_mutex);
	imslabset_mutex = 0;
	imslabset_inited = 0;
}

struct IMEMSLAB *imslabset_new(void)
{
	long index;
	char*ptr;
	if (imslabset_inited == 0) imslabset_init();
	imutex_lock(imslabset_mutex);
	index = imnode_new(&imslabset_node);
	assert(index >= 0);
	ptr = (char*)imnode_data(&imslabset_node, index);
	imutex_unlock(imslabset_mutex);
	*(long*)ptr = index;
	ptr += IMROUNDUP(sizeof(long));
	assert(IMROUNDUP((unsigned long)ptr) == (unsigned long)ptr);
	if (imlog_mask & IMLOG_SLABDBG)
		imlog_write(IMLOG_SLABDBG, "[SLAB] slabset new: %lxh\n", ptr);
	return (struct IMEMSLAB*)ptr;
}

void imslabset_del(struct IMEMSLAB *slab)
{
	long index;
	index = *(long*)(((char*)slab) - IMROUNDUP(sizeof(long)));
	if (imlog_mask & IMLOG_SLAB)
		imlog_write(IMLOG_SLABDBG, "[SLAB] slabset del: %lxh\n", slab);
	assert(imslabset_inited);
	imutex_lock(imslabset_mutex);
	assert(index >= 0 && index < imslabset_node.node_max);
	assert(IMNODE_MODE(&imslabset_node, index));
	imnode_del(&imslabset_node, index);
	imutex_unlock(imslabset_mutex);
}


/*====================================================================*/
/* IMEMGFP                                                            */
/*====================================================================*/
void* imgfp_alloc_page(struct IMEMGFP *gfp)
{
	void *ptr;
	if (imslabset_inited == 0) imslabset_init();
	if (gfp == NULL) ptr = impage_alloc();
	else {
		ptr = gfp->alloc_page(gfp);
		gfp->pages_inuse++;
		gfp->pages_in++;
	}
	if (imlog_mask & IMLOG_GFP) 
		imlog_write(IMLOG_GFP, "[GFP*] newpage=%lxh gfp=%lxh", ptr, gfp);
	return ptr;
}

void imgfp_free_page(struct IMEMGFP *gfp, void *ptr)
{
	if (imslabset_inited == 0) imslabset_init();
	if (imlog_mask & IMLOG_GFP) 
		imlog_write(IMLOG_GFP, "[GFP*] delpage=%lxh gfp=%lxh", ptr, gfp);
	if (gfp == NULL) impage_free(ptr);
	else {
		gfp->free_page(gfp, ptr);
		gfp->pages_inuse--;
		gfp->pages_out++;
	}
}


/*====================================================================*/
/* IMCACHE_LIST3                                                      */
/*====================================================================*/
static void imcache_list_init(struct IMCACHE_LIST3 *mlist, 
			unsigned long obj_size, int flags, struct IMEMGFP *gfp)
{
	unsigned long pagesize;
	if (imslabset_inited == 0) imslabset_init();
	iqueue_init(&mlist->slab_partial);
	iqueue_init(&mlist->slab_full);
	iqueue_init(&mlist->slab_free);
	if (gfp == NULL) mlist->page_size = impage_psize;
	else mlist->page_size = gfp->pagesize;
	mlist->gfp = gfp;
	mlist->obj_size = obj_size;
	mlist->unit_size = IMROUNDUP(mlist->obj_size + sizeof(void*));
	mlist->count_free = 0;
	mlist->count_full = 0;
	mlist->count_partial = 0;
	mlist->free_limit = 0;
	mlist->color_next = 0;
	pagesize = mlist->page_size;
	mlist->color_max = pagesize - sizeof(struct IMEMSLAB) * 2;
	if (mlist->color_max > IMROUNDUP(obj_size + sizeof(void*)))
		mlist->color_max = IMROUNDUP(obj_size + sizeof(void*));
	mlist->list_lock = imutex_new();
	mlist->flags = flags;
	mlist->extra = NULL;
	mlist->free_objects = 0;
	mlist->pages_in = 0;
	mlist->pages_out = 0;
	imlog_write(IMLOG_LIST3, "[LIST] init list3: %lxh obj_size=%d gfp=%lx", 
		mlist, obj_size, mlist->gfp);
}

#define IMCACHE_FLAG_OFFSLAB	1
#define IMCACHE_FLAG_NODRAIN	2
#define IMCACHE_FLAG_NOLOCK		4
#define IMCACHE_FLAG_SYSTEM		8
#define IMCACHE_FLAG_ONQUEUE	16

#define IMCACHE_OFFSLAB(mlist) ((mlist)->flags & IMCACHE_FLAG_OFFSLAB)
#define IMCACHE_NODRAIN(mlist) ((mlist)->flags & IMCACHE_FLAG_NODRAIN)
#define IMCACHE_NOLOCK(mlist)  ((mlist)->flags & IMCACHE_FLAG_NOLOCK)
#define IMCACHE_SYSTEM(mlist)  ((mlist)->flags & IMCACHE_FLAG_SYSTEM)
#define IMCACHE_ONQUEUE(mlist) ((mlist)->flags & IMCACHE_FLAG_ONQUEUE)


static
void imcache_slab_destry(struct IMCACHE_LIST3 *mlist, struct IMEMSLAB *slab)
{
	if (imlog_mask & IMLOG_SLAB) 
		imlog_write(IMLOG_SLAB, "[SLAB] release: %lx gfp=%lx page=%lx off=%d",
			slab, mlist->gfp, slab->membase, IMCACHE_OFFSLAB(mlist));
	imgfp_free_page(mlist->gfp, slab->membase);
	mlist->pages_out++;
	if (IMCACHE_OFFSLAB(mlist)) {
		imslabset_del(slab);
	}
}

static
struct IMEMSLAB *imcache_slab_create(struct IMCACHE_LIST3 *mlist, long *cnt)
{
	struct IMEMSLAB *slab;
	unsigned long coloroff;
	unsigned long obj_size;
	long count;
	char *page;

	obj_size = mlist->unit_size;
	page = (char*)imgfp_alloc_page(mlist->gfp);
	assert(page);
	coloroff = mlist->color_next;
	if (IMCACHE_OFFSLAB(mlist)) {
		slab = imslabset_new();
		assert(slab);
	}	else {
		coloroff = IMROUNDUP((unsigned long)(page + coloroff));
		coloroff -= (unsigned long)page;
		slab = (struct IMEMSLAB*)(page + coloroff);
		coloroff += sizeof(struct IMEMSLAB);
	}
	count = imslab_init(slab, page, mlist->page_size, obj_size, coloroff);
	assert(count > 0);
	mlist->color_next += IMROUNDSIZE;
	if (mlist->color_next >= mlist->color_max)
		mlist->color_next = 0;
	slab->extra = mlist;
	mlist->pages_in++;
	if (cnt) *cnt = count;
	if (imlog_mask & IMLOG_SLAB)
		imlog_write(IMLOG_SLAB, "[SLAB] create: %lx gfp=%lx page=%lx off=%d",
			slab, mlist->gfp, slab->membase, IMCACHE_OFFSLAB(mlist));
	return slab;
}

static
int imcache_drain_list(struct IMCACHE_LIST3 *mlist, int tofree, int id)
{
	struct IMEMSLAB *slab;
	iqueue_head *head, *p;
	int free_count = 0;

	if (id == 0) head = &mlist->slab_free;
	else if (id == 1) head = &mlist->slab_full;
	else if (id == 2) head = &mlist->slab_partial;
	else return -1;

	while (!iqueue_is_empty(head)) {
		if (tofree >= 0 && free_count >= tofree) break;
		imutex_lock(mlist->list_lock);
		p = head->prev;
		if (p == head) {
			imutex_unlock(mlist->list_lock);
			break;
		}
		slab = iqueue_entry(p, struct IMEMSLAB, queue);
		iqueue_del(p);
		imutex_unlock(mlist->list_lock);
		if (id == 0) {
			while (!IMEMSLAB_ISFULL(slab)) imslab_alloc(slab);
			mlist->free_objects -= slab->inuse;
		}
		imcache_slab_destry(mlist, slab);
		free_count++;
	}
	if (id == 0) mlist->count_free -= free_count;
	else if (id == 1) mlist->count_full -= free_count;
	else mlist->count_partial -= free_count;
	if (imlog_mask & IMLOG_LIST3)
		imlog_write(IMLOG_LIST3, "[LIST] drain: id=%d tofree=%d nfree=%d",
			id, tofree, free_count);
	return free_count;
}

static void imcache_list_destroy(struct IMCACHE_LIST3 *mlist)
{
	imcache_drain_list(mlist, -1, 0);
	imcache_drain_list(mlist, -1, 1);
	imcache_drain_list(mlist, -1, 2);
	imlog_write(IMLOG_LIST3, "[LIST] destroy list3: %lxh obj_size=%d gfp=%lx",
		mlist, mlist->obj_size, mlist->gfp);
	imutex_lock(mlist->list_lock);
	mlist->count_free = 0;
	mlist->count_full = 0;
	mlist->count_partial = 0;
	mlist->free_limit = 0;
	mlist->color_next = 0;
	mlist->free_objects = 0;
	imutex_unlock(mlist->list_lock);
	imutex_del(mlist->list_lock);
	mlist->list_lock = NULL;
}

#define IMCACHE_CHECK_MAGIC		0x05

static void* imcache_list_alloc(struct IMCACHE_LIST3 *mlist)
{
	struct IMEMSLAB *slab;
	iqueue_head *p;
	long slab_obj_num;
	char *lptr = NULL;

	if (IMCACHE_NOLOCK(mlist) == 0)
		imutex_lock(mlist->list_lock);
	p = mlist->slab_partial.next;
	if (p == &mlist->slab_partial) {
		p = mlist->slab_free.next;
		if (p == &mlist->slab_free) {
			slab = imcache_slab_create(mlist, &slab_obj_num);
			p = &slab->queue;
			mlist->free_objects += slab_obj_num;
			slab = iqueue_entry(p, struct IMEMSLAB, queue);
			if (imlog_mask & IMLOG_LISTDBG)
				imlog_write(IMLOG_LISTDBG, 
					"[LIST] slab_partial add: %lxh with new", slab);
		}	else {
			iqueue_del(p);
			iqueue_init(p);
			mlist->count_free--;
			slab = iqueue_entry(p, struct IMEMSLAB, queue);
			if (imlog_mask & IMLOG_LISTDBG)
				imlog_write(IMLOG_LISTDBG,
					"[LIST] slab_partial add: %lxh from free", slab);
		}
		iqueue_add(p, &mlist->slab_partial);
		mlist->count_partial++;
	}
	slab = iqueue_entry(p, struct IMEMSLAB, queue);
	assert(IMEMSLAB_ISFULL(slab) == 0);
	lptr = (char*)imslab_alloc(slab);
	assert(lptr);
	if (mlist->free_objects) mlist->free_objects--;
	if (IMEMSLAB_ISFULL(slab)) {
		iqueue_del(p);
		iqueue_init(p);
		iqueue_add(p, &mlist->slab_full);
		mlist->count_partial--;
		mlist->count_full++;
		if (imlog_mask & IMLOG_LISTDBG)
			imlog_write(IMLOG_LISTDBG,
				"[LIST] slab_full add: %lx from partial", slab);
	}
	if (IMCACHE_NOLOCK(mlist) == 0)
		imutex_unlock(mlist->list_lock);
	*(struct IMEMSLAB**)lptr = slab;
	if (imlog_mask & IMLOG_LISTDBG)
		imlog_write(IMLOG_LISTDBG, "[LIST] newnode: slab=%lx ptr=%lx\n", 
			slab, lptr);
	lptr += sizeof(struct IMEMSLAB*);
	return lptr;
}

static void imcache_list_free(struct IMCACHE_LIST3 *mlist, void *ptr)
{
	struct IMEMSLAB *slab;
	iqueue_head *p;
	char *lptr = (char*)ptr;
	char *membase;
	int result;

	lptr -= sizeof(struct IMEMSLAB*);
	slab = *(struct IMEMSLAB**)lptr;
	assert(slab);

	membase = (char*)slab->membase;
	result = (lptr >= membase && lptr < membase + slab->memsize);
	if (imlog_mask & IMLOG_LISTDBG) {
		imlog_write(IMLOG_LISTDBG, "[LIST] delnode: slab=%lx ptr=%lx\n",
			slab, lptr);
		if (result == 0) {
			imlog_write(IMLOG_LIST3, "[LIST] error free: slab=%lx ptr=%lx",
				slab, lptr);
		}
	}
	assert(result);
	if (result == 0) return;
	mlist = (struct IMCACHE_LIST3*)slab->extra;
	p = &slab->queue;
	if (IMCACHE_NOLOCK(mlist) == 0)
		imutex_lock(mlist->list_lock);
	if (IMEMSLAB_ISFULL(slab)) {
		assert(mlist->count_full);
		iqueue_del(p);
		iqueue_init(p);
		iqueue_add_tail(p, &mlist->slab_partial);
		mlist->count_full--;
		mlist->count_partial++;
	}
	imslab_free(slab, lptr);
	mlist->free_objects++;
	if (IMEMSLAB_ISEMPTY(slab)) {
		iqueue_del(p);
		iqueue_init(p);
		iqueue_add(p, &mlist->slab_free);
		mlist->count_partial--;
		mlist->count_free++;
	}
	if (IMCACHE_NOLOCK(mlist) == 0)
		imutex_unlock(mlist->list_lock);
	if (IMCACHE_NODRAIN(mlist) == 0) {
		if (mlist->free_objects >= mlist->free_limit) {
			result = mlist->count_free >> 1;
			if (result > 0)
				result = imcache_drain_list(mlist, result, 0);
		}
	}
}


/*====================================================================*/
/* IMEMECACHE                                                         */
/*====================================================================*/
static struct IMEMNODE imcache_node;
static int imcache_inited = 0;
static imutex_t imcache_mutex = NULL;

static void imcache_init(void)
{
	if (imcache_inited) return;

	if (imslabset_inited == 0) imslabset_init();

	imnode_init(&imcache_node, sizeof(struct IMEMCACHE), 0);

	imcache_mutex = imutex_new();

	imcache_inited = 1;
}

static void imcache_calculate(struct IMEMCACHE *cache);
static void imcache_destroy(struct IMEMCACHE *cache);
static void imcache_gfp_init(struct IMEMCACHE *cache);

static void imcache_quit(void)
{
	struct IMEMCACHE *cache;
	long index, next;

	if (imcache_inited == 0) return;

	imutex_lock(imcache_mutex);

	for (; imnode_head(&imcache_node) >= 0; ) {
		for (index = imnode_head(&imcache_node); index >= 0; index = next) {
			next = imnode_next(&imcache_node, index);
			cache = (struct IMEMCACHE*)imnode_data(&imcache_node, index);
			if (cache->page_supply.refcnt == 0) {
				imcache_destroy(cache);
				imnode_del(&imcache_node, index);
			}
		}
	}

	imnode_destroy(&imcache_node);

	imutex_unlock(imcache_mutex);
	imutex_del(imcache_mutex);

	imcache_inited = 0;
}

static void imcache_release(struct IMEMCACHE *cache)
{
	struct IMEMCACHE *p;
	long index;
	index = cache->index;
	assert(imcache_inited);
	assert(index >= 0 && index < imcache_node.node_max);
	imcache_destroy(cache);
	imutex_lock(imcache_mutex);
	assert(IMNODE_MODE(&imcache_node, index));
	p = (struct IMEMCACHE*)imnode_data(&imcache_node, index);
	assert(cache == p);
	imnode_del(&imcache_node, index);
	imutex_unlock(imcache_mutex);
}

static struct IMEMCACHE *imcache_create(const char *name, 
				unsigned long obj_size, struct IMEMGFP *gfp)
{
	struct IMEMCACHE *cache;
	unsigned long page_size;
	long index, limit, i;

	if (imcache_inited == 0) imcache_init();
	page_size = (gfp == NULL)? impage_psize : gfp->pagesize;

	assert(obj_size > 0 && obj_size <= page_size - IMROUNDSIZE);

	imutex_lock(imcache_mutex);
	index = imnode_new(&imcache_node);
	assert(index >= 0);
	cache = (struct IMEMCACHE*)imnode_data(&imcache_node, index);
	imutex_unlock(imcache_mutex);

	assert(cache);

	cache->index = index;
	cache->gfp = gfp;
	cache->obj_size = obj_size;
	cache->slab_size = sizeof(struct IMEMSLAB);
	cache->page_size = page_size;
	cache->lock = imutex_new();
	cache->flags = 0;
	cache->user = 0;
	name = name? name : "NONAME";
	strncpy(cache->name, name, IMCACHE_NAMESIZE);
	if (cache->gfp) cache->gfp->refcnt++;

	imcache_calculate(cache);

	limit = (1) * cache->batchcount + cache->num;
	imcache_list_init(&cache->list, obj_size, cache->flags, gfp);
	cache->list.color_max = cache->color_limit;
	cache->list.extra = cache;
	cache->list.free_limit = limit;
	cache->list.flags |= IMCACHE_FLAG_NODRAIN | IMCACHE_FLAG_NOLOCK;

	limit = limit >= IMCACHE_ARRAYLIMIT? IMCACHE_ARRAYLIMIT : limit;
	limit = limit < 2? 2 : limit;
	for (i = 0; i < IMUTEX_MAXCPU; i++) {
		cache->array[i].avial = 0;
		cache->array[i].batchcount = (limit / 2);
		cache->array[i].limit = limit;
		cache->array[i].lock = imutex_new();
	}
	cache->pages_hiwater = 0;
	cache->pages_inuse = 0;
	iqueue_init(&cache->queue);
	imcache_gfp_init(cache);

	return cache;
}


static void imcache_calculate(struct IMEMCACHE *cache)
{
	unsigned long limit = 8;
	unsigned long size = 0;
	unsigned long objnum = 0;
	unsigned long objsize = 0;
	unsigned long color = 0;
	int mustonslab = 0;

	objsize = IMROUNDUP(cache->obj_size + sizeof(void*));
	if (objsize > IMROUNDUP(sizeof(struct IMEMSLAB))) {
		objnum = cache->page_size / objsize;
		size = cache->page_size - objnum * objsize;
		if (size >= IMROUNDUP(sizeof(struct IMEMSLAB) + 8)) mustonslab = 1;
	}

	if (cache->obj_size >= (cache->page_size >> 3) && mustonslab == 0)
		cache->flags |= IMCACHE_FLAG_OFFSLAB;

	if (objsize >= 131072) limit = 1;
	else if (objsize >= cache->page_size) limit = 8;
	else if (objsize >= 1024) limit = 24;
	else if (objsize >= 256) limit = 56;
	else limit = 120;
	#ifdef IMEM_DEBUG
	if (limit >= 8) limit = 8;
	#endif

	cache->limit = limit;
	cache->batchcount = (limit + 1) >> 1;
	
	if (IMCACHE_OFFSLAB(cache)) {
		objnum = cache->page_size / objsize;
		color = cache->page_size - objnum * objsize;
	}	else {
		objnum = (cache->page_size - IMROUNDUP(sizeof(struct IMEMSLAB)) -
			IMROUNDSIZE) / objsize;
		color = (cache->page_size - objnum * objsize) - 
			IMROUNDUP(sizeof(struct IMEMSLAB)) - IMROUNDSIZE;
	}
	color = color > objsize ? objsize : color;
	color = color < 0? 0 : color;
	cache->num = objnum;
	cache->color_limit = color;
}

static void imcache_destroy(struct IMEMCACHE *cache)
{
	struct IMCACHE_ARRAY *array;
	void *entry[IMCACHE_ARRAYLIMIT];
	long n, i, j;
	assert(imcache_inited);
	imutex_lock(cache->lock);

	for (i = 0; i < IMUTEX_MAXCPU; i++) {
		array = &cache->array[i];

		imutex_lock(array->lock);
		for (j = 0, n = 0; j < array->avial; j++, n++) 
			entry[j] = array->entry[j];
		array->avial = 0;
		imutex_unlock(array->lock);

		imutex_del(array->lock);

		if (IMCACHE_NOLOCK(&cache->list))
			imutex_lock(cache->list.list_lock);

		for (j = 0; j < n; j++) 
			imcache_list_free(NULL, entry[j]);

		if (IMCACHE_NOLOCK(&cache->list))
			imutex_unlock(cache->list.list_lock);
	}

	imcache_list_destroy(&cache->list);

	if (cache->gfp) 
		cache->gfp->refcnt--;

	imutex_unlock(cache->lock);
	imutex_del(cache->lock);
	cache->lock = NULL;
}

static int imcache_fill_batch(struct IMEMCACHE *cache, int array_index)
{
	struct IMCACHE_ARRAY *array = &cache->array[array_index];
	struct IMCACHE_LIST3 *list = &cache->list;
	unsigned long pages;
	int count = 0;
	void *ptr;

	if (IMCACHE_NOLOCK(list))
		imutex_lock(list->list_lock);

	for (count = 0; array->avial < array->batchcount; count++) {
		ptr = imcache_list_alloc(list);
		assert(ptr);
		array->entry[array->avial++] = ptr;
	}

	pages = list->count_free + list->count_full + list->count_partial;

	if (IMCACHE_NOLOCK(list))
		imutex_unlock(list->list_lock);

	cache->pages_inuse = pages;

	if (cache->pages_inuse > cache->pages_hiwater)
		cache->pages_hiwater = cache->pages_inuse;

	return count;
}

#define IMCACHE_CHECK_MAGIC		0x05

static void imcache_set_magic(void *ptr)
{
	void **head = (void**)((char*)ptr - sizeof(void*));
	unsigned long linear = (unsigned long)head[0];
	head[0] = (void*)(linear | IMCACHE_CHECK_MAGIC);
}

static int imcache_check_magic(void *ptr)
{
	void **head = (void**)((char*)ptr - sizeof(void*));
	unsigned long linear = (unsigned long)head[0];
	if ((linear & IMCACHE_CHECK_MAGIC) != IMCACHE_CHECK_MAGIC) return -1;
	head[0] = (void*)(linear & ~(IMROUNDSIZE - 1));
	return 0;
}

static void *imcache_alloc(struct IMEMCACHE *cache)
{
	struct IMCACHE_ARRAY *array;
	int array_index = 0, count = 0;
	void *ptr = NULL;

	array_index = imutex_currentcpu();
	if (array_index >= IMUTEX_MAXCPU) array_index = 0;
	array = &cache->array[array_index];

	imutex_lock(array->lock);
	if (array->avial == 0)
		count = imcache_fill_batch(cache, array_index);
	count = count;
	ptr = array->entry[--array->avial];
	imutex_unlock(array->lock);

	assert(ptr);
	imcache_set_magic(ptr);

	return ptr;
}

static void *imcache_free(struct IMEMCACHE *cache, void *ptr)
{
	struct IMCACHE_ARRAY *array;
	struct IMCACHE_LIST3 *list;
	struct IMEMSLAB *slab;
	unsigned long pages;
	char *lptr = (char*)ptr;
	int array_index = 0;
	int invalidptr, count;

	array_index = imutex_currentcpu();
	if (array_index >= IMUTEX_MAXCPU) array_index = 0;

	invalidptr = imcache_check_magic(ptr);
	assert(invalidptr == 0);

	lptr -= sizeof(struct IMEMSLAB*);
	slab = *(struct IMEMSLAB**)lptr;
	list = (struct IMCACHE_LIST3*)slab->extra;

	if (cache) {
		assert(cache == (struct IMEMCACHE*)list->extra);
	}

	cache = (struct IMEMCACHE*)list->extra;
	array = &cache->array[array_index];

	imutex_lock(array->lock);

	if (array->avial >= array->limit) {

		if (IMCACHE_NOLOCK(list)) 
			imutex_lock(list->list_lock);

		for (count = 0; array->avial > array->batchcount; count++)
			imcache_list_free(NULL, array->entry[--array->avial]);

		imcache_list_free(NULL, ptr);

		if (IMCACHE_NOLOCK(list)) 
			imutex_unlock(list->list_lock);

		if (list->free_objects >= list->free_limit) {
			if (list->count_free > 1) 
				imcache_drain_list(list, list->count_free >> 1, 0);
		}

		pages = list->count_free + list->count_full + list->count_partial;
		cache->pages_inuse = pages;

	}	else {

		array->entry[array->avial++] = ptr;
	}

	imutex_unlock(array->lock);

	return cache;
}

static void *imcache_gfp_alloc(struct IMEMGFP *gfp)
{
	struct IMEMCACHE *cache = (struct IMEMCACHE*)gfp->extra;
	char *lptr;
	lptr = (char*)imcache_alloc(cache);
	assert(lptr);
	return lptr;
}

static void imcache_gfp_free(struct IMEMGFP *gfp, void *ptr)
{
	struct IMEMCACHE *cache = (struct IMEMCACHE*)gfp->extra;
	char *lptr = (char*)ptr;;
	imcache_free(cache, lptr);
}

static void imcache_gfp_init(struct IMEMCACHE *cache)
{
	struct IMEMGFP *gfp = &cache->page_supply;
	gfp->pagesize = cache->obj_size;
	gfp->refcnt = 0;
	gfp->alloc_page = imcache_gfp_alloc;
	gfp->free_page = imcache_gfp_free;
	gfp->extra = cache;
	gfp->pages_inuse = 0;
	gfp->pages_in = 0;
	gfp->pages_out = 0;
}



/*====================================================================*/
/* IKMEM INTERFACE                                                    */
/*====================================================================*/
static struct IVECTOR ikmem_vector;
static struct IVECTOR ikmem_record;
static struct IMEMCACHE **ikmem_array = NULL;
static struct IMEMCACHE **ikmem_lookup = NULL;
static struct IMEMCACHE *ikmem_root = NULL;
static void *ikmem_sizemap_l1[257];
static void *ikmem_sizemap_l2[257];
static int ikmem_inited = 0;
static int ikmem_count = 0;
static imutex_t ikmem_lock = NULL;
static struct IMEMNODE ikmem_node;
static unsigned long ikmem_inuse;
static struct IQUEUEHEAD ikmem_head;

static unsigned long ikmem_range_high = 0;
static unsigned long ikmem_range_low = ~0ul;
static unsigned long ikmem_water_mark = 0;


static int ikmem_insert(unsigned long size, struct IMEMGFP *gfp)
{
	struct IMEMCACHE *cache;
	char name[64];
	int retval;
	sprintf(name, "KMEM_%ld", size);
	cache = imcache_create(name, size, gfp);
	assert(cache);
	cache->flags |= IMCACHE_FLAG_SYSTEM;
	retval = iv_resize(&ikmem_vector, sizeof(struct IMEMCACHE) * 
		(ikmem_count + 2));
	assert(retval == 0);
	retval = iv_resize(&ikmem_record, sizeof(struct IMEMCACHE) *
		(ikmem_count + 2));
	ikmem_array = (struct IMEMCACHE**)ikmem_vector.data;
	ikmem_lookup = (struct IMEMCACHE**)ikmem_record.data;
	retval = ikmem_count;
	ikmem_array[ikmem_count] = cache;
	ikmem_lookup[ikmem_count++] = cache;
	imlog_write(IMLOG_KMEM, "[KMEM] %s: size=%lu flags=%d num=%d",
		cache->name, cache->obj_size, cache->flags, cache->num);
	return retval;
}

static void ikmem_setup_caches(unsigned long *sizelist);

void ikmem_init(unsigned long *sizelist)
{
	struct IMEMCACHE *cache;
	unsigned long psize;
	int index, shift;
	int limit;
	if (ikmem_inited) return;
	if (imcache_inited == 0) imcache_init();
	iv_init(&ikmem_vector, NULL);
	iv_init(&ikmem_record, NULL);
	ikmem_lookup = NULL;
	ikmem_array = NULL;
	ikmem_count = 0;
	ikmem_inuse = 0;
	psize = impage_psize - IMROUNDUP(sizeof(void*));
	index = ikmem_insert(psize, NULL);
	ikmem_root = ikmem_array[index];
	ikmem_root->user = 1;
	limit = impage_shift - 4;
	limit = limit < 10? 10 : limit;
	for (shift = impage_shift - 1; shift >= limit; shift--) {
		index = ikmem_insert((1ul << shift) + sizeof(void*), 0);
		cache = ikmem_array[index];
		cache->user = 1;
	}
	imnode_init(&ikmem_node, sizeof(void*), NULL);
	ikmem_setup_caches(sizelist);
	ikmem_lock = imutex_new();
	iqueue_init(&ikmem_head);
	ikmem_range_high = 0;
	ikmem_range_low = ~0ul;
	ikmem_water_mark = 0;
	ikmem_inited = 1;
}

void ikmem_quit(void)
{
	struct IMEMCACHE *cache;
	struct IQUEUEHEAD *p;
	long index;
	if (ikmem_inited == 0) return;
	imutex_lock(ikmem_lock);
	for (p = ikmem_head.next; p != &ikmem_head; ) {
		cache = IQUEUE_ENTRY(p, struct IMEMCACHE, queue);
		p = p->next;
		iqueue_del(&cache->queue);
		imcache_release(cache);
	}
	imutex_unlock(ikmem_lock);
	for (index = ikmem_count - 1; index >= 0; index--) {
		cache = ikmem_array[index];
		imcache_release(cache);
	}
	iv_destroy(&ikmem_vector);
	iv_destroy(&ikmem_record);
	ikmem_lookup = NULL;
	ikmem_array = NULL;
	ikmem_count = 0;
	imutex_lock(ikmem_lock);
	for (index = imnode_head(&ikmem_node); index >= 0; ) {
		imv_free(NULL, IMNODE_DATA(&ikmem_node, index));
		index = imnode_next(&ikmem_node, index);
	}
	imnode_destroy(&ikmem_node);
	imutex_unlock(ikmem_lock);
	imutex_del(ikmem_lock);
	ikmem_lock = NULL;
	ikmem_inited = 0;
	imcache_quit();
}


static unsigned long ikmem_waste(unsigned long objsize, unsigned long psize)
{
	struct IMEMCACHE cache;
	memset (&cache, 0, sizeof(struct IMEMCACHE));
	unsigned long size = 0, k = 0;
	cache.obj_size = (unsigned long)objsize;
	cache.page_size = (unsigned long)psize;
	imcache_calculate(&cache);
	size = IMROUNDUP(cache.obj_size + sizeof(void*)) * cache.num;
	size = psize - size;
	k = IMROUNDUP(sizeof(struct IMEMSLAB));
	if (IMCACHE_OFFSLAB(&cache)) size += k;
	return size;
}

static unsigned long ikmem_cost(unsigned long objsize, struct IMEMGFP *gfp)
{
	struct IMEMCACHE *cache = NULL;
	unsigned long waste = 0;
	if (gfp == NULL) {
		return ikmem_waste(objsize, impage_psize);
	}
	cache = (struct IMEMCACHE*)gfp->extra;
	waste = ikmem_waste(objsize, cache->obj_size) * cache->num;
	return waste + ikmem_cost(cache->obj_size, cache->gfp);
}

static struct IMEMGFP *ikmem_choose_gfp(unsigned long objsize, long *w)
{
	unsigned long hiwater = IMROUNDUP(objsize + sizeof(void*)) * 64;
	unsigned long lowater = IMROUNDUP(objsize + sizeof(void*)) * 8;
	struct IMEMCACHE *cache = NULL;
	unsigned long min, waste;
	int index, i = -1;
	min = impage_psize;
	for (index = 0; index < ikmem_count; index++) {
		cache = ikmem_array[index];
		if (cache->obj_size < lowater || cache->obj_size > hiwater)
			continue;
		waste = ikmem_cost(objsize, &ikmem_array[index]->page_supply);
		if (waste < min) min = waste, i = index;
	}
	if (i < 0 || i >= ikmem_count) {
		if (w) w[0] = (long)ikmem_cost(objsize, NULL);
		return NULL;
	}
	if (w) w[0] = (long)min;
	return &ikmem_array[i]->page_supply;
}

static void ikmem_append(unsigned long objsize, int approxy)
{
	struct IMEMCACHE *cache = NULL;
	struct IMEMGFP *gfp = NULL;
	unsigned long optimize = 0;
	long index = 0, waste = 0, k = 0;

	for (index = 0; index < ikmem_count; index++) {
		optimize = ikmem_array[index]->obj_size;
		if (optimize < objsize) continue;
		if (optimize == objsize) break;
		if (optimize - objsize <= (objsize >> 3) && approxy) break;
	}
	if (index < ikmem_count) {
		imlog_write(IMLOG_KMEM, "[KMEM] append: objsize=%lu reuse=%lu - reused", 
			objsize, ikmem_array[index]->obj_size);
		return;
	}
	gfp = ikmem_choose_gfp(objsize, &waste);
	imlog_write(IMLOG_KMEM, "[KMEM] append: objsize=%lu pagesize=%lu - w=%lu",
			objsize, gfp? gfp->pagesize : impage_psize, waste);
	ikmem_insert(objsize, gfp);
	for (index = ikmem_count - 1; index > 1; index = k) {
		k = index - 1;
		if (ikmem_lookup[index]->obj_size < ikmem_lookup[k]->obj_size)
			break;
		cache = ikmem_lookup[index];
		ikmem_lookup[index] = ikmem_lookup[k];
		ikmem_lookup[k] = cache;
	}
}

static struct IMEMCACHE *ikmem_choose_size(unsigned long size)
{
	int index = 0;
	for (index = ikmem_count - 1; index >= 0; index--) {
		if (ikmem_lookup[index]->obj_size >= size) 
			return ikmem_lookup[index];
	}
	return NULL;
}

static void ikmem_setup_caches(unsigned long *sizelist)
{
	unsigned long fib1 = 8, fib2 = 16, f;
	struct IVECTOR listv;
	unsigned long *list = NULL, k = 0;
	int shift = 0, count = 0, i = 0, j = 0;

	iv_init(&listv, 0);
	for (shift = impage_shift, count = 0; shift >= 2; shift--) {
		i = iv_resize(&listv, (count + 2) * sizeof(long));
		list = (unsigned long*)listv.data;
		assert(i == 0);
		list[count++] = 1ul << shift;
	}
	for (; fib2 < (impage_psize >> 3); ) {
		f = fib1 + fib2;
		fib1 = fib2;
		fib2 = f;
		i = iv_resize(&listv, (count + 2) * sizeof(long));
		list = (unsigned long*)listv.data;
		assert(i == 0);
		#ifdef IKMEM_USEFIB
		list[count++] = f;
		#endif
	}

	for (i = 0; i < count - 1; i++) {
		for (j = i + 1; j < count; j++) {
			if (list[i] < list[j]) 
				k = list[i], list[i] = list[j], list[j] = k;
		}
	}

	for (i = 0; i < count; i++) ikmem_append(list[i], 1);
	iv_destroy(&listv);

	if (sizelist) {
		for (; sizelist[0]; sizelist++) ikmem_append(*sizelist, 0);
	}
	
	for (f = 4; f <= 1024; f += 4) 
		ikmem_sizemap_l1[f >> 2] = ikmem_choose_size(f);

	for (f = 1024; f <= (256 << 10); f += 1024) 
		ikmem_sizemap_l2[f >> 10] = ikmem_choose_size(f);

	ikmem_sizemap_l1[0] = NULL;
	ikmem_sizemap_l2[0] = NULL;
}

void* ikmem_malloc(unsigned long size)
{
	struct IMEMCACHE *cache = NULL;
	unsigned long round = 0;
	long index = 0;
	char *lptr = NULL;

	if (ikmem_inited == 0) ikmem_init(NULL);
	assert(size >= 4);
	round = (size + 3) & ~3ul;

	if (round <= 1024) {
		cache = (struct IMEMCACHE*)ikmem_sizemap_l1[round >> 2];
	}	else {
		round = (size + 1023) & ~1023ul;
		if (round < (256 << 10)) 
			cache = (struct IMEMCACHE*)ikmem_sizemap_l2[round >> 10];
	}

	if (cache == NULL)
		cache = ikmem_choose_size(size);

	if (ikmem_water_mark > 0 && size > ikmem_water_mark)
		cache = NULL;

	if (cache == NULL) {
		lptr = (char*)imv_malloc(0, size + sizeof(void*) + sizeof(long));
		if (lptr == NULL) return NULL;
		imutex_lock(ikmem_lock);
		index = imnode_new(&ikmem_node);
		assert(index >= 0);
		IMNODE_DATA(&ikmem_node, index) = lptr;
		IMNODE_NODE(&ikmem_node, index) = size;
		imutex_unlock(ikmem_lock);
		*(long*)lptr = index;
		lptr += sizeof(long);
		*(void**)lptr = NULL;
		lptr += sizeof(void*);
	}	else {
		lptr = (char*)imcache_alloc(cache);
		ikmem_inuse += cache->obj_size;
	}

	if (ikmem_range_high < (unsigned long)lptr)
		ikmem_range_high = (unsigned long)lptr;
	if (ikmem_range_low > (unsigned long)lptr)
		ikmem_range_low = (unsigned long)lptr;

	return lptr;
}

void ikmem_free(void *ptr)
{
	struct IMEMCACHE *cache = NULL;
	char *lptr = (char*)ptr - sizeof(void*);
	long index = 0;
	if(*(void**)lptr == NULL) {
		lptr -= sizeof(long);
		index = *(long*)lptr;
		imutex_lock(ikmem_lock);
		assert(index >= 0 && index < ikmem_node.node_max);
		assert(IMNODE_MODE(&ikmem_node, index));
		assert((char*)IMNODE_DATA(&ikmem_node, index) == lptr);
		imnode_del(&ikmem_node, index);
		imutex_unlock(ikmem_lock);
		imv_free(0, lptr);
	}	else {
		cache = (struct IMEMCACHE*)imcache_free(NULL, ptr);
		ikmem_inuse -= cache->obj_size;
	}
}


unsigned long ikmem_ptr_size(void *ptr)
{
	struct IMEMCACHE *cache = NULL;
	struct IMEMSLAB *slab = NULL;
	char *lptr = (char*)ptr - sizeof(void*);
	unsigned long size = 0;
	unsigned long linear = 0;
	long index = 0;

	if ((unsigned long)ptr < ikmem_range_low ||
		(unsigned long)ptr > ikmem_range_high)
		return 0;

	if (*(void**)lptr == NULL) {
		lptr -= sizeof(long);
		index = *(long*)lptr;
		if (index < 0 || index >= ikmem_node.node_max) 
			return 0;
		if (IMNODE_MODE(&ikmem_node, index) == 0 ||
			lptr != (char*)IMNODE_DATA(&ikmem_node, index))
			return 0;
		size = IMNODE_NODE(&ikmem_node, index);
	}	else {
		linear = (unsigned long)(*(void**)lptr);
		if ((linear & IMCACHE_CHECK_MAGIC) != IMCACHE_CHECK_MAGIC)
			return 0;
		slab = (struct IMEMSLAB*)(linear & ~(IMROUNDSIZE - 1));
		if ((char*)slab->membase + slab->coloroff > lptr ||
			(char*)slab->membase + slab->memsize < lptr)
			return 0;
		if (slab->extra == NULL) return 0;
		cache = (struct IMEMCACHE*)(((struct IMCACHE_LIST3*)slab->extra)->extra);
		size = cache->obj_size;
	}
	return size;
}

void* ikmem_realloc(void *ptr, unsigned long size)
{
	unsigned long oldsize = 0;
	void *newptr = NULL;

	if (ptr == NULL) return ikmem_malloc(size);
	oldsize = ikmem_ptr_size(ptr);

	assert(oldsize > 0);
	
	if (oldsize >= size) {
		if (oldsize * 3 < size * 4) 
			return ptr;
	}

	newptr = ikmem_malloc(size);
	assert(newptr);

	memcpy(newptr, ptr, oldsize < size? oldsize : size);
	ikmem_free(ptr);

	return newptr;
}

void ikmem_info(unsigned long *inuse, unsigned long *total)
{
	if (inuse) inuse[0] = ikmem_inuse;
	if (total) {
		total[0] = impage_psize * impage_inuse;
	}
}

void ikmem_option(unsigned long watermark)
{
	ikmem_water_mark = watermark;
}

static struct IMEMCACHE* ikmem_search(const char *name, int needlock)
{
	struct IMEMCACHE *cache = NULL, *result = NULL;
	struct IQUEUEHEAD *head = NULL;
	long index = 0;

	for (index = 0; index < ikmem_count; index++) {
		cache = ikmem_array[index];
		if (strcmp(cache->name, name) == 0) return cache;
	}
	result = NULL;
	if (needlock) imutex_lock(ikmem_lock);
	for (head = ikmem_head.next; head != &ikmem_head; head = head->next) {
		cache = iqueue_entry(head, struct IMEMCACHE, queue);
		if (strcmp(cache->name, name) == 0) {
			result = cache;
			break;
		}
	}
	if (needlock) imutex_unlock(ikmem_lock);
	return result;
}

struct IMEMCACHE* ikmem_get(const char *name)
{
	return ikmem_search(name, 1);
}

struct IMEMCACHE* ikmem_create(const char *name, unsigned long size)
{
	struct IMEMCACHE *cache = NULL;
	struct IMEMGFP *gfp = NULL;
	if (ikmem_inited == 0) ikmem_init(NULL);
	if (size >= impage_psize) return NULL;

	gfp = ikmem_choose_gfp(size, NULL);
	imutex_lock(ikmem_lock);
	cache = ikmem_search(name, 0);
	if (cache != NULL) {
		imutex_unlock(ikmem_lock);
		return NULL;
	}
	cache = imcache_create(name, size, gfp);
	if (cache == NULL) {
		imutex_unlock(ikmem_lock);
		return NULL;
	}
	cache->flags |= IMCACHE_FLAG_ONQUEUE;
	cache->user = (long)gfp;
	iqueue_add_tail(&ikmem_head, &cache->queue);
	imutex_unlock(ikmem_lock);

	return cache;
}

void ikmem_delete(struct IMEMCACHE *cache)
{
	assert(IMCACHE_SYSTEM(cache) == 0);
	assert(IMCACHE_ONQUEUE(cache));
	if (IMCACHE_SYSTEM(cache)) return;
	if (IMCACHE_ONQUEUE(cache) == 0) return;
	imutex_lock(ikmem_lock);
	iqueue_del(&cache->queue);
	imutex_unlock(ikmem_lock);
	imcache_release(cache);
}

void *ikmem_cache_alloc(struct IMEMCACHE *cache)
{
	char *ptr;
	assert(cache);
	ptr = (char*)imcache_alloc(cache);
	return ptr;
}

void ikmem_cache_free(struct IMEMCACHE *cache, void *ptr)
{
	imcache_free(cache, ptr);
}


/*====================================================================*/
/* IVECTOR / IMEMNODE MANAGEMENT                                      */
/*====================================================================*/
static void* ikmem_allocator_malloc(struct IALLOCATOR *, unsigned long);
static void ikmem_allocator_free(struct IALLOCATOR *, void *);

struct IALLOCATOR ikmem_allocator = 
{
	ikmem_allocator_malloc,
	ikmem_allocator_free,
	0, 0
};


static void* ikmem_allocator_malloc(struct IALLOCATOR *a, unsigned long len)
{
	return ikmem_malloc(len);
}

static void ikmem_allocator_free(struct IALLOCATOR *a, void *ptr)
{
	assert(ptr);
	ikmem_free(ptr);
}


ivector_t *iv_create(void)
{
	ivector_t *vec;
	vec = (ivector_t*)ikmem_malloc(sizeof(ivector_t));
	if (vec == NULL) return NULL;
	iv_init(vec, &ikmem_allocator);
	return vec;
}

void iv_delete(ivector_t *vec)
{
	assert(vec);
	iv_destroy(vec);
	ikmem_free(vec);
}

imemnode_t *imnode_create(int nodesize, int grow_limit)
{
	imemnode_t *mnode;
	mnode = (imemnode_t*)ikmem_malloc(sizeof(imemnode_t));
	if (mnode == NULL) return NULL;
	imnode_init(mnode, nodesize, &ikmem_allocator);
	mnode->grow_limit = grow_limit;
	return mnode;
}

void imnode_delete(imemnode_t *mnode)
{
	assert(mnode);
	imnode_destroy(mnode);
	ikmem_free(mnode);
}

