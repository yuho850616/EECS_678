/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
	int index;
	void* memory;
	int order;
} page_t;


/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	for (i = 0; i < (1<<MAX_ORDER) / PAGE_SIZE ;i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */
		g_pages[i].memory = PAGE_TO_ADDR(i);
		g_pages[i].index = i;
		g_pages[i].order = MAX_ORDER;
		INIT_LIST_HEAD(&g_pages[i].list);
	}

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	if(size > (1 << MAX_ORDER))
	{
		return NULL;
	}
	int botorder = MIN_ORDER;
	while(((1 << botorder) < size)&& (botorder < MAX_ORDER) )
	{
		botorder++;
	}
	int temporder;
	for(temporder = botorder; temporder < MAX_ORDER; temporder++)
	{
		if(!list_empty(&free_area[temporder]))
		{
			break;
		}
	}
	if((temporder == MAX_ORDER) && (list_empty(&free_area[temporder])))
	{
		return NULL;
	}
	while(temporder != botorder)
	{
		page_t* ptr = list_entry(free_area[temporder].next, page_t, list);
		temporder--;
		g_pages[(ADDR_TO_PAGE(BUDDY_ADDR(ptr->memory,temporder)))].order = temporder;
		list_add(&g_pages[(ADDR_TO_PAGE(BUDDY_ADDR(ptr->memory,temporder)))].list, &free_area[temporder]);
		list_move(free_area[temporder+1].next, &free_area[temporder]);
	}
	page_t* ptr = list_entry(free_area[temporder].next, page_t, list);
	ptr->order = temporder;
	list_del_init(free_area[temporder].next);
	return ptr->memory;
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	page_t *ptr = &g_pages[ADDR_TO_PAGE(addr)];
	if(list_empty(&free_area[ptr->order]))
	{
		list_add(&ptr->list, &free_area[ptr->order]);
	}
	else
	{
		struct list_head *pos;
		list_for_each(pos, &free_area[ptr->order])
		{
			page_t* tempptr = list_entry(pos, page_t, list);
			if((ptr->index) < (tempptr->index))
			{
				break;
			}
		}
		list_add(&ptr->list, pos->prev);
	}
	int b_index = ADDR_TO_PAGE(BUDDY_ADDR(ptr->memory, ptr->order));
	while((ptr->order < MAX_ORDER)&& ptr->order == g_pages[b_index].order)
	{
		if(b_index < ptr->index)
		{
			list_del_init(&ptr->list);
			g_pages[b_index].order = ptr->order+1;
			ptr->order = MAX_ORDER;
			ptr = &g_pages[b_index];
		}
		else if(b_index >= ptr->index)
		{
			list_del_init(&g_pages[b_index].list);
			ptr->order++;
			g_pages[b_index].order = MAX_ORDER;
		}
		if(list_empty(&free_area[ptr->order]))
		{
			list_move(&ptr->list, &free_area[ptr->order]);
		}
		else
		{
			list_del_init(&ptr->list);
			struct list_head *pos;
			list_for_each(pos, &free_area[ptr->order])
			{
				page_t* tempptr = list_entry(pos, page_t, list);
				if((ptr->index) < (tempptr->index))
				{
					break;
				}
			}
			list_add(&ptr->list, pos->prev);
		}
		b_index = ADDR_TO_PAGE(BUDDY_ADDR(ptr->memory, ptr->order));
	}
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
