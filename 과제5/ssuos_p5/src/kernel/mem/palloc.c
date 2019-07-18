#include <mem/palloc.h>
#include <bitmap.h>
#include <type.h>
#include <round.h>
#include <mem/mm.h>
#include <synch.h>
#include <device/console.h>
#include <mem/paging.h>
#include <proc/proc.h>

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  
   */

/* page struct */
struct kpage{
	uint32_t type;
	uint32_t *vaddr;
	uint32_t nalloc;
	pid_t pid;
};


static struct kpage *kpage_list;
static uint32_t page_alloc_index;

/* Initializes the page allocator. */
	void
init_palloc (void) 
{
	/* Calculate the space needed for the kpage list */
	size_t pool_size = sizeof(struct kpage) * PAGE_POOL_SIZE;

	/* kpage list alloc */
	kpage_list = (struct kpage *)(KERNEL_ADDR);

	/* initialize */
	memset((void*)kpage_list, 0, pool_size);
	page_alloc_index = 0;
}

/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   */
	uint32_t *
palloc_get_multiple (uint32_t page_type, size_t page_cnt)
{
	void *pages = NULL;
	struct kpage *kpage = kpage_list;
	size_t page_idx;
	int i,j;

	if (page_cnt == 0)
		return NULL;
	while(kpage -> nalloc != 0){ //가장 마지막 kpage까지 반복
		if(kpage->nalloc == page_cnt && kpage->type == FREE__ && page_type != STACK__){ //free페이지가 존재하는 경우 할당
			pages = kpage->vaddr;
			kpage->type = page_type;
			break;
		}
		kpage += 1;
	}
	if(pages ==NULL){
		switch(page_type){
			case HEAP__: //힙을 새로 배정하는 경우
				kpage -> nalloc = page_cnt;
				kpage -> type = page_type;
				kpage -> vaddr = (void*)(VKERNEL_HEAP_START - page_alloc_index * PAGE_SIZE - PAGE_SIZE*page_cnt); //가상주소 할당
				page_alloc_index += page_cnt;
				pages = kpage->vaddr;
				break;
			case STACK__: //스택을 새로 배정하는 경우
				kpage -> nalloc = page_cnt;
				kpage -> type = page_type;
				pages = (uint32_t*)VKERNEL_STACK_ADDR;
				kpage -> pid = cur_process -> pid;
				kpage->vaddr = pages - 1*PAGE_SIZE;
			//(2)

			break;
			default:
				return NULL;
		}
	}
	if(pages != NULL){
		if(page_type == HEAP__){
			memset (pages, 0, PAGE_SIZE * page_cnt);
		}
		else{
			memset ((void*)(VKERNEL_STACK_ADDR - PAGE_SIZE * 2) , 0, PAGE_SIZE * 2);
		}
	}

	return (uint32_t*)pages; 
}

/* Obtains a single free page and returns its address.
   */
	uint32_t *
palloc_get_page (uint32_t page_type) 
{
	return palloc_get_multiple (page_type, 1);
}

/* Frees the PAGE_CNT pages starting at PAGES. */
	void
palloc_free_multiple (void *pages, size_t page_cnt) 
{

	struct kpage *kpage = kpage_list;
	if (pages == NULL || page_cnt == 0)
		return;
	while(kpage -> nalloc != 0){ 
		if(kpage-> vaddr == pages){ //free하려는 페이지가 kpage에 존재하는 경우
			kpage -> type = FREE__;
			return;
		}
		
		kpage = kpage + 1;
	}

}

/* Frees the page at PAGE. */
	void
palloc_free_page (void *page) 
{
	palloc_free_multiple (page, 1);
}


uint32_t *
va_to_ra (uint32_t *va){
	struct kpage *kpage = kpage_list;
	int i = 0;
	while(kpage -> nalloc != 0){
		if(va == kpage->vaddr){ //변환하려는 va가 kpage에 존재하는 경우
			if(kpage->type == STACK__){ //스택인 경우
				if(cur_process->pid == kpage -> pid){ //스택의 경우 가상주소가 같으므로 pid 확인
					return (uint32_t *)(RKERNEL_HEAP_START + i*PAGE_SIZE);
				}
			}
			else{
				return (uint32_t *)(RKERNEL_HEAP_START + i*PAGE_SIZE);
			}
			
		}
		i+=kpage->nalloc;
		kpage = kpage +1;
	}
}

uint32_t *
ra_to_va (uint32_t *ra){
	struct kpage *kpage = kpage_list;
	while(va_to_ra(kpage->vaddr) != ra){ //변환 하려는 ra의 값을 va_to_ra를 통해 비교
		if(kpage -> nalloc == 0){
			break;
		}
		kpage += 1;
	}
	return (uint32_t *)kpage->vaddr;
	
}


void palloc_pf_test(void)
{
	uint32_t *one_page1 = palloc_get_page(HEAP__);
	uint32_t *one_page2 = palloc_get_page(HEAP__);
	uint32_t *two_page1 = palloc_get_multiple(HEAP__,2);
	uint32_t *three_page;
	printk("one_page1 = %x\n", one_page1); 
	printk("one_page2 = %x\n", one_page2); 
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	palloc_free_page(one_page1);
	palloc_free_page(one_page2);
	palloc_free_multiple(two_page1,2);

	one_page1 = palloc_get_page(HEAP__);
	two_page1 = palloc_get_multiple(HEAP__,2);
	one_page2 = palloc_get_page(HEAP__);

	printk("one_page1 = %x\n", one_page1);
	printk("one_page2 = %x\n", one_page2);
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	three_page = palloc_get_multiple(HEAP__,3);

	printk("three_page = %x\n", three_page);
	palloc_free_page(one_page1);
	palloc_free_page(one_page2);
	palloc_free_multiple(two_page1,2);
	palloc_free_multiple(three_page, 3);
}
