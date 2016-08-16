/**********************************************************************
 *
 * imembase.h - basic interface of memory operation
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

#ifndef __IMEMBASE_H__
#define __IMEMBASE_H__


/*====================================================================*/
/* QUEUE DEFINITION                                                   */
/*====================================================================*/
#ifndef __IQUEUE_DEF__
#define __IQUEUE_DEF__

struct IQUEUEHEAD {
	struct IQUEUEHEAD *next, *prev;
};

#ifndef INLINE
#ifdef __GNUC__
#if __GNUC__ >= 3
#define inline __inline__ __attribute__((always_inline))
#else
#define inline __inline__
#endif
#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define inline __inline
#else
#define inline 
#endif
#define INLINE inline
#endif

#ifdef _MSC_VER
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)
#endif

typedef struct IQUEUEHEAD iqueue_head;


/*--------------------------------------------------------------------*/
/* queue init                                                         */
/*--------------------------------------------------------------------*/
#define IQUEUE_HEAD_INIT(name) { &(name), &(name) }
#define IQUEUE_HEAD(name) \
	struct IQUEUEHEAD name = IQUEUE_HEAD_INIT(name)

#define IQUEUE_INIT(ptr) ( \
	(ptr)->next = (ptr), (ptr)->prev = (ptr))

#define IOFFSETOF(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)

#define ICONTAINEROF(ptr, type, member) ( \
		(type*)( ((char*)((type*)ptr)) - IOFFSETOF(type, member)) )

#define IQUEUE_ENTRY(ptr, type, member) ICONTAINEROF(ptr, type, member)


/*--------------------------------------------------------------------*/
/* queue operation                                                    */
/*--------------------------------------------------------------------*/
#define IQUEUE_ADD(node, head) ( \
	(node)->prev = (head), (node)->next = (head)->next, \
	(head)->next->prev = (node), (head)->next = (node))

#define IQUEUE_ADD_TAIL(node, head) ( \
	(node)->prev = (head)->prev, (node)->next = (head), \
	(head)->prev->next = (node), (head)->prev = (node))

#define IQUEUE_DEL_BETWEEN(p, n) ((n)->prev = (p), (p)->next = (n))

#define IQUEUE_DEL(entry) (\
	(entry)->next->prev = (entry)->prev, \
	(entry)->prev->next = (entry)->next, \
	(entry)->next = 0, (entry)->prev = 0)

#define IQUEUE_DEL_INIT(entry) do { \
	IQUEUE_DEL(entry); IQUEUE_INIT(entry); } while (0);

#define IQUEUE_IS_EMPTY(entry) ((entry) == (entry)->next)

#define iqueue_init		IQUEUE_INIT
#define iqueue_entry	IQUEUE_ENTRY
#define iqueue_add		IQUEUE_ADD
#define iqueue_add_tail	IQUEUE_ADD_TAIL
#define iqueue_del		IQUEUE_DEL
#define iqueue_del_init	IQUEUE_DEL_INIT
#define iqueue_is_empty IQUEUE_IS_EMPTY

#define IQUEUE_FOREACH(iterator, head, TYPE, MEMBER) \
	for ((iterator) = iqueue_entry((head)->next, TYPE, MEMBER); \
		&((iterator)->MEMBER) != (head); \
		(iterator) = iqueue_entry((iterator)->MEMBER.next, TYPE, MEMBER))

#define iqueue_foreach(iterator, head, TYPE, MEMBER) \
	IQUEUE_FOREACH(iterator, head, TYPE, MEMBER)

#endif


/*====================================================================*/
/* IMEMSLAB                                                           */
/*====================================================================*/
struct IMEMSLAB
{
	struct IQUEUEHEAD queue;
	unsigned long coloroff;
	void*membase;
	long memsize;
	long inuse;
	void*bufctl;
	void*extra;
};

typedef struct IMEMSLAB IMemSlab;

/* init slab structure with given memory block and coloroff */
long imslab_init(struct IMEMSLAB *s, void *mem, long len, int dlen, int off);

/* alloc data from slab */ 
void *imslab_alloc(struct IMEMSLAB *s);

/* free data into slab */
void imslab_free(struct IMEMSLAB *s, void *ptr);


#define IMEMSLAB_ISFULL(s) ((s)->bufctl == 0)
#define IMEMSLAB_ISEMPTY(s) ((s)->inuse == 0)

#define IMROUNDSHIFT	3
#define IMROUNDSIZE		(1 << IMROUNDSHIFT)
#define IMROUNDUP(s)	(((s) + IMROUNDSIZE - 1) & ~(IMROUNDSIZE - 1))


/*====================================================================*/
/* IALLOCATOR                                                        */
/*====================================================================*/
struct IALLOCATOR
{
    void *(*alloc)(struct IALLOCATOR *, unsigned long);
    void (*free)(struct IALLOCATOR *, void *);
    void *udata;
    long reserved;
};

extern void *(*imv_malloc_ptr)(unsigned long size);
extern void (*imv_free_ptr)(void *mem);

void* imv_malloc(struct IALLOCATOR *allocator, unsigned long size);
void imv_free(struct IALLOCATOR *allocator, void *mem);


/*====================================================================*/
/* IVECTOR                                                           */
/*====================================================================*/
struct IVECTOR
{
	unsigned char*data;       
	unsigned long length;      
	unsigned long block;       
	struct IALLOCATOR *allocator;
};

void iv_init(struct IVECTOR *v, struct IALLOCATOR *allocator);
void iv_destroy(struct IVECTOR *v);
int iv_resize(struct IVECTOR *v, unsigned long newsize);



/*====================================================================*/
/* IMEMNODE                                                           */
/*====================================================================*/
struct IMEMNODE
{
	struct IALLOCATOR *allocator;   /* memory allocator        */

	struct IVECTOR vprev;           /* prev node link vector   */
	struct IVECTOR vnext;           /* next node link vector   */
	struct IVECTOR vnode;           /* node information data   */
	struct IVECTOR vdata;           /* node data buffer vector */
	struct IVECTOR vmode;           /* mode of allocation      */
	long *mprev;                    /* prev node array         */
	long *mnext;                    /* next node array         */
	long *mnode;                    /* node info array         */
	void**mdata;                    /* node data array         */
	long *mmode;                    /* node mode array         */
	void *extra;                    /* extra user data         */
	long node_free;                 /* number of free nodes    */
	long node_max;                  /* number of all nodes     */
	long grow_limit;                /* limit of growing        */

	int node_size;                  /* node data fixed size    */
	int node_shift;                 /* node data size shift    */

	struct IVECTOR vmem;            /* mem-pages in the pool   */
	char **mmem;                    /* mem-pages array         */
	long mem_max;                   /* max num of memory pages */
	long mem_count;                 /* number of mem-pages     */

	long list_open;                 /* the entry of open-list  */
	long list_close;                /* the entry of close-list */
	long total_mem;                 /* total memory size       */
};

void imnode_init(struct IMEMNODE *mn, long nodesize, struct IALLOCATOR *ac);
void imnode_destroy(struct IMEMNODE *mnode);
long imnode_new(struct IMEMNODE *mnode);
void imnode_del(struct IMEMNODE *mnode, long index);
long imnode_head(struct IMEMNODE *mnode);
long imnode_next(struct IMEMNODE *mnode, long index);
void*imnode_data(struct IMEMNODE *mnode, long index);

#define IMNODE_NODE(mnodeptr, i) ((mnodeptr)->mnode[i])
#define IMNODE_PREV(mnodeptr, i) ((mnodeptr)->mprev[i])
#define IMNODE_NEXT(mnodeptr, i) ((mnodeptr)->mnext[i])
#define IMNODE_DATA(mnodeptr, i) ((mnodeptr)->mdata[i])
#define IMNODE_MODE(mnodeptr, i) ((mnodeptr)->mmode[i])


/*====================================================================*/
/* IMUTEXHOOK                                                         */
/*====================================================================*/
struct IMUTEXHOOK
{
	int mutex_size;
	int mspin_size;
	int (*mutex_init)(void *mutex);
	void (*mutex_destroy)(void *mutex);
	void (*mutex_lock)(void *mutex);
	void (*mutex_unlock)(void *mutex);
	int (*current_cpu)(void);
};

typedef void* imutex_t;

void imutex_setup(struct IMUTEXHOOK *hook);
void imutex_cleanup(void);
void imutex_neglect(int isneglect);

imutex_t imutex_new(void);
void imutex_del(imutex_t mutex);
void imutex_lock(imutex_t mutex);
void imutex_unlock(imutex_t mutex);


#ifndef IMUTEX_MAXCPU_SHIFT
#define IMUTEX_MAXCPU_SHIFT		2
#endif

#define IMUTEX_MAXCPU	(1 << (IMUTEX_MAXCPU_SHIFT))

int imutex_currentcpu(void);


/*====================================================================*/
/* IMLOGGING                                                          */
/*====================================================================*/
typedef void (*imlogging_t)(const char *text);
void imlog_setup(imlogging_t handler);
void imlog_destroy(void);
void imlog_setmask(int logmask);
void imlog_write(int level, const char *fmt, ...);
#ifdef IMLOG_ENABLE
void imlog_setup_debug(int needprint);
#endif

#define IMLOG_PAGE		1
#define IMLOG_PAGEDBG	2
#define IMLOG_GFP		4
#define IMLOG_SLAB		8
#define IMLOG_SLABDBG	16
#define IMLOG_LIST3		32
#define IMLOG_LISTDBG	64
#define IMLOG_CACHE		128
#define IMLOG_KMEM		256


/*====================================================================*/
/* IMPAGE                                                             */
/*====================================================================*/
#define IMPAGE_DEFAULT 16

extern unsigned long impage_shift;
extern unsigned long*impage_map;

#define IMPAGE_TOPAGE(p) (((unsigned long)(p)) >> impage_shift)
#define IMPAGE_MAP(p) impage_map[IMPAGE_TOPAGE(p)]

void impage_setup(long pageshift);
void impage_destroy(void);
void*impage_alloc(void);
void impage_free(void *ptr);

void impage_state(long *inuse, long *in, long *out);
void impage_info(long *pageshift, long *pagesize);


/*====================================================================*/
/* IMSLABSET                                                          */
/*====================================================================*/
void imslabset_init(void);
void imslabset_destroy(void);
struct IMEMSLAB *imslabset_new(void);
void imslabset_del(struct IMEMSLAB *slab);


/*====================================================================*/
/* IMEMGFP                                                            */
/*====================================================================*/
struct IMEMGFP
{
	unsigned long pagesize;
	long refcnt;
	void*(*alloc_page)(struct IMEMGFP *gfp);
	void (*free_page)(struct IMEMGFP *gfp, void *ptr);
	void *extra;
	unsigned long pages_inuse;
	unsigned long pages_in;
	unsigned long pages_out;
};

void* imgfp_alloc_page(struct IMEMGFP *gfp);
void imgfp_free_page(struct IMEMGFP *gfp, void *ptr);


/*====================================================================*/
/* IMCACHE_LIST3                                                      */
/*====================================================================*/
struct IMCACHE_LIST3
{
	iqueue_head slab_partial;
	iqueue_head slab_full;
	iqueue_head slab_free;
	struct IMEMGFP *gfp;
	unsigned long obj_size;
	unsigned long unit_size;
	unsigned long page_size;
	unsigned long count_partial;
	unsigned long count_full;
	unsigned long count_free;
	unsigned long free_objects;
	unsigned long free_limit;
	unsigned long color_next;
	unsigned long color_max;
	imutex_t list_lock;
	int flags;
	void *extra;
	unsigned long pages_in;
	unsigned long pages_out;
};


#ifndef IMCACHE_ARRAYLIMIT
#define IMCACHE_ARRAYLIMIT 8
#endif

#ifndef IMCACHE_NODECOUNT_SHIFT
#define IMCACHE_NODECOUNT_SHIFT 0
#endif

#define IMCACHE_NODECOUNT (1 << (IMCACHE_NODECOUNT_SHIFT))

#ifndef IMCACHE_NAMESIZE
#define IMCACHE_NAMESIZE 32
#endif

struct IMCACHE_ARRAY
{
	int avial;
	int limit;
	int batchcount;
	imutex_t lock;
	void *entry[IMCACHE_ARRAYLIMIT];
};

/*====================================================================*/
/* IMEMECACHE                                                         */
/*====================================================================*/
struct IMEMCACHE
{
	struct IMCACHE_ARRAY array[IMUTEX_MAXCPU];
	struct IMCACHE_LIST3 list;
	struct IMEMGFP *gfp;
	struct IMEMGFP page_supply;
	unsigned long batchcount;
	unsigned long limit;
	unsigned long num;	
	unsigned long obj_size;
	unsigned long color_limit;
	unsigned long slab_size;
	unsigned long page_size;
	int flags;
	long index;
	long user;
	imutex_t lock;
	struct IQUEUEHEAD queue;
	char name[IMCACHE_NAMESIZE + 1];
	unsigned long pages_hiwater;
	unsigned long pages_inuse;
};


/*====================================================================*/
/* IKMEM INTERFACE                                                    */
/*====================================================================*/
void ikmem_init(unsigned long *sizelist);
void ikmem_quit(void);

void* ikmem_malloc(unsigned long size);
void* ikmem_realloc(void *ptr, unsigned long size);
void ikmem_free(void *ptr);

unsigned long ikmem_ptr_size(void *ptr);
void ikmem_info(unsigned long *inuse, unsigned long *total);
void ikmem_option(unsigned long watermark);

struct IMEMCACHE* ikmem_create(const char *name, unsigned long size);
void ikmem_delete(struct IMEMCACHE *cache);

struct IMEMCACHE* ikmem_get(const char *name);

void *ikmem_cache_alloc(struct IMEMCACHE *cache);
void ikmem_cache_free(struct IMEMCACHE *cache, void *ptr);


/*====================================================================*/
/* IVECTOR / IMEMNODE MANAGEMENT                                      */
/*====================================================================*/
extern struct IALLOCATOR ikmem_allocator;

typedef struct IVECTOR ivector_t;
typedef struct IMEMNODE imemnode_t;

ivector_t *iv_create(void);
void iv_delete(ivector_t *vec);

imemnode_t *imnode_create(int nodesize, int grow_limit);
void imnode_delete(imemnode_t *);


#endif


