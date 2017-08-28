/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errno.h"
#include "inttypes.h"

/**
 * You should store the head of your free list in this variable.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
sf_free_header* freelist_head = NULL;
void *heap_beginning= NULL;
void *heap_end = NULL;
int coalesce;
double peak;
double peak_temp;


void *sf_malloc(size_t size) {

	peak = peak + size;
	double diff = (uintptr_t)heap_end - (uintptr_t)heap_beginning;

	if(size == 0 || size > 16384 || diff > 16384)
		return NULL;

	//getting a page of memory and setting a free block
	if(freelist_head == NULL)
	{
		freelist_head = sf_sbrk(4096);
		heap_beginning = (void*)freelist_head;

	 	if((void*)freelist_head == (void*) -1)
			return NULL;

		//memset((void*)freelist_head, 0, 8);
	 	freelist_head -> next = NULL;
	 	freelist_head -> prev = NULL;
	 	freelist_head -> header.alloc = 0;
	 	freelist_head -> header.splinter = 0;
	 	freelist_head -> header.block_size = 4096 >> 4;
	 	freelist_head -> header.requested_size = 0;
	 	freelist_head -> header.unused_bits = 0;
	 	freelist_head -> header.splinter_size = 0;
	 	freelist_head -> header.padding_size = 0;

	 	char* foot;
	 	foot = (char*)freelist_head + (freelist_head->header.block_size << 4) - 8;
	 	sf_footer* g = (sf_footer*) foot;
	 	g -> alloc = 0;
	 	g -> splinter = 0;
	 	g -> block_size = 4096 >> 4;
	 	//printf("beginning header: %p\n", (void*)freelist_head);
	 	//printf("beginning footer: %p\n", (void*)g);

	}
	//finding a best fit block to malloc
	int og_size = size;
	size = size + (16-((size + 16) % 16))%16 + 16;

	sf_free_header *s = freelist_head;
	sf_free_header *best_fit = NULL;
	while(s != NULL)
	{
		if(size == s -> header.block_size << 4)
		{
			best_fit = s;
			break;
		}
		else if (size < s -> header.block_size << 4)
		{
			if(best_fit == NULL)
			{
				best_fit = s;
			}
			else if(best_fit -> header.block_size << 4 > s -> header.block_size << 4)
				best_fit = s;
		}

		s = s -> next;
	}


	// if block is bigger than how much space we have on the heap
	if(best_fit == NULL)
		{
			void *place = sf_sbrk(size);
			//printf("place: %p\n", place);
			if (place == (void*) -1)
				return NULL;
			void* new_place = sf_sbrk(0);
			//printf("new place: %p\n", new_place);
			//footer
			char *l = (char*)place - 8;
			sf_footer* f = (sf_footer*) l;
			//printf("footer: %p\n", (void*)f);
			//printf("footer block_size: %d\n",(int) (f -> block_size << 4 ));
			sf_free_header* last;
			if(f -> alloc == 0)
			{
				coalesce++;
				char *h = (char*)f - ((f-> block_size << 4) - 8);
				//printf("%d\n", (f-> block_size << 4));
				last = (sf_free_header*) h;
				//printf("header: %p\n", (void*)last);
				int last_size = (last -> header.block_size << 4);
				//printf("%d\n", last_size);
				int new_size = (char*)new_place - (char*)place;
				//printf("%d\n", new_size);
				char *n = (char*) new_place - 8;
				sf_footer* new_footer = (sf_footer*) n;
				int new_new_size = last_size + new_size;
				last -> header.block_size  = new_new_size >> 4;
				//printf("%d\n", new_new_size);
				//printf("new footer: %p\n", (void*)new_footer);
				new_footer -> block_size = new_new_size >> 4;
				new_footer -> alloc = 0;
				new_footer -> splinter = 0;
			}
			else
			{
				//printf("%s\n", "here");
				char *n = (char*) new_place - 8;
				sf_footer* new_footer = (sf_footer*) n;
				int new_size = (char*)new_place - (char*)place;
				//printf("%d\n", new_size);
				last = (sf_free_header*)place;
				last -> header.block_size = new_size >> 4;
				new_footer -> block_size = new_size >> 4;
				new_footer -> alloc = 0;
				new_footer -> splinter = 0;
			}
			best_fit = last;
		}
		//printf("%d\n", (int)(best_fit -> header.block_size << 4));
		//splinter
		if(((best_fit -> header.block_size << 4) - size) < 32)
		{
			//printf("%s\n", "splinter");
			best_fit -> header.alloc = 1;
			best_fit -> header.splinter = 1;
			best_fit -> header.splinter_size = 16;
			best_fit -> header.requested_size = og_size;
			best_fit -> header.padding_size = size - og_size - 16;
			best_fit -> header.unused_bits = 0;

			char *foot = (char*)best_fit + ((best_fit -> header.block_size << 4) - 8);
			sf_footer* f = (sf_footer*) foot;
			f -> block_size = (best_fit -> header.block_size << 4);
			f -> splinter = 1;
			f -> alloc = 1;

			if(best_fit -> prev != NULL)
				(best_fit -> prev) -> next = best_fit -> next;

			if(best_fit -> next != NULL)
				(best_fit -> next) -> prev = best_fit -> prev;
			best_fit -> next = NULL;
			best_fit -> prev = NULL;
		}

		// splitting
	if((best_fit -> header.block_size << 4) - size > 31)
	{
		split((void*)best_fit, size, og_size);
	}
	//printf("freelist header: %p\n", (void*)freelist_head);
	heap_end = sf_sbrk(0);
	return (char*)best_fit +8;
}

void *sf_realloc(void *ptr, size_t size) {
	//if valid pointer, but size = 0
	if(valid(ptr) == 0 && size == 0)
	{
		sf_free(ptr);
		return NULL;
	}
	//if invalid pointer
	if (valid(ptr) != 0)
	{
		errno = EINVAL;
		return NULL;
	}
	int og_size = size;
	size = size + (16-((size + 16) % 16))%16 + 16;
	char *p = (char*)ptr - 8;
	sf_free_header* header = (sf_free_header*)p;
	//printf("header to be realloced: %p\n", (void*)header);

	//adjacent block
	char *n = (char*)header + (header -> header.block_size << 4);
	sf_free_header* next = (sf_free_header*)n;
	//if allocating to a smaller block (splitting)
	if((int)((header -> header.block_size << 4) - size) > 0)
	{
		//printf("%d\n",(int)((header -> header.block_size << 4) - size) );
		//printf("%s\n", "here");
		//if adjacent block is free
		if(next -> header.alloc == 0)
		{
			int new_size = (next -> header.block_size << 4) + ((header -> header.block_size << 4) - size);
			//split
			header -> header.block_size = size >> 4;
			header -> header.requested_size = og_size;
			header -> header.splinter = 0;
			header -> header.splinter_size = 0;
			header -> header.padding_size = size - og_size - 16;
			header -> header.alloc = 1;
			char *f = (char*)header + ((header -> header.block_size << 4) - 8);
			sf_footer* foot = (sf_footer*)f;
			foot -> block_size = size >> 4;
			foot -> alloc = 1;
			foot -> splinter= 0;

			// printf("realloced header: %p\n", (void*)header);
			// printf("realloced footer: %p\n", (void*)foot);

			//coalesce
			char* n = (char*)header+ (header -> header.block_size << 4);
			sf_free_header* new = (sf_free_header*)n;
			new -> header.block_size = new_size >> 4;
			new -> header.alloc = 0;
			char *f_n = ((char*)new) + (new_size << 4) - 8;
			sf_footer* footer = (sf_footer*)f_n;
			footer -> block_size = new_size >> 4;
			footer -> alloc = 0;
			new -> prev = next -> prev;
			new -> next = next -> next;
			(next -> next) -> prev = new;
			//(next -> prev) -> next = new;

			next -> next = NULL;
			next -> prev = NULL;
			// printf("realloced free header: %p\n", (void*)new);
			// printf("realloced free footer: %p\n", (void*)footer);
		}

		// keep splinter don't split and update info
		else
		{
			if((header -> header.block_size << 4) - size > 31)
			{
				split((void*)header, size, og_size);
			}
			else
			{
			header -> header.alloc = 1;
			header -> header.splinter = 1;
			int splint_size = (header -> header.block_size << 4) - size;
			header -> header.splinter_size = splint_size;
			header -> header.requested_size = og_size;
			header -> header.padding_size = size - og_size - 16;
			char *f = (char*)header + ((header -> header.block_size << 4) - 8);
			sf_footer* foot = (sf_footer*)f;
			foot -> splinter = 1;
			foot -> alloc = 1;
			}
		}
		char *h = (char*)header + 8;
		sf_free_header* head = (sf_free_header*)h;
		return head;
	}
	//findng a block for best fit
	else
	{
		int needed_size = size - (header -> header.block_size << 4);
		if(next -> header.alloc == 0 && (((int)(next -> header.block_size << 4)) >= needed_size))
		{
			//coalesce
			//printf("%s\n", "here");
			int new_size = (header -> header.block_size << 4) + needed_size;
			int free_block_size = (next -> header.block_size << 4) - needed_size;
			header -> header.block_size = new_size >> 4;
			header -> header.requested_size = og_size;
			header -> header.padding_size = size - og_size - 16;
			char *f = (char*)header + (header -> header.block_size << 4) - 8;
			sf_footer* foot = (sf_footer*)f;
			//printf("%p\n", (void*)foot);
			foot -> block_size = new_size >> 4;
			foot -> alloc = 1;
			foot -> splinter = 0;
			//printf("%d\n",(int)free_block_size );
			//if splinter
			if(free_block_size < 32)
			{
				header -> header.splinter = 1;
				header -> header.splinter_size = free_block_size >> 4;
				foot -> splinter = 1;
			}
			//if no splinter and need to add free list
			else
			{
				//printf("%s\n", "greater than");
				char *n = (char*)header + (header -> header.block_size << 4);
				sf_free_header* new = (sf_free_header*)n;
				//printf("new header: %p\n", (void*)new);
				new -> header.block_size = free_block_size >> 4;
				new -> header.alloc = 0;
				char *f = (char*)header + (header -> header.block_size << 4) - 8;
				sf_footer* foott = (sf_footer*)f;
				foott -> block_size = free_block_size >> 4;
				foott -> alloc = 0;
				new -> prev = next -> prev;
				new -> next = next -> next;
				(next -> next) -> prev = new;
				next -> prev = NULL;
				next -> next = NULL;
				foot -> block_size = new_size >> 4;
				foot -> alloc = 1;
				foot -> splinter = 0;
				//add to free list
			}
			char *h = (char*)header + 8;
		sf_free_header* head = (sf_free_header*)h;
		return head;
		}
		else
		{
			void* ret = sf_malloc(og_size);
			if(ret == NULL)
				return NULL;
			char* p = (char*)ptr - 8;
			sf_free_header* pt = (sf_free_header*)p;
			memcpy(ptr, ret, pt -> header.block_size);
			sf_free(ptr);
			return ret;
		}
	}
	heap_end = sf_sbrk(0);

	return NULL;
}

void sf_free(void* ptr) {

	if(valid(ptr) == 0)
	{
		//free that block
		int head_flag = -1;
		char *p = (char*) ptr - 8;
		sf_free_header *header = (sf_free_header*) p;
		peak = peak + (header -> header.requested_size << 4);
		char *f = (char*)p + ((header -> header.block_size << 4) - 8);
		sf_footer *footer = (sf_footer*)f;
		//printf("free header: %p\n", (void*)header);
		//printf("free footer: %p\n", (void*)footer);
		header -> header.alloc = 0;
		footer -> alloc = 0;
		//printf("%d\n", (int)header -> header.block_size << 4);
		//set prev and next for this newly allocated block
		sf_free_header *w = freelist_head;
		if(w != NULL && w > header)
		{
			freelist_head = header;
			header -> prev = NULL;
			header -> next = w;
			w -> prev = header;
			head_flag = 0;
		}
		sf_free_header *o = freelist_head;
		if(head_flag != 0)
		{
			while(o != NULL)
			{
				if(o > header)
				{
					header -> prev = o -> prev;
					header -> next = o;
					o -> prev = header;
					break;
				}
				o = o -> next;
			}
		}

		//printf("header prev: %p\n", (void*)header -> prev);
		//printf("header: %p\n", (void*)header);
		//printf("header next: %p\n", (void*)header -> next );
		sf_free_header *constant = header;

		//check if prev node in free list is free
		sf_footer* previous_f;
		if(head_flag != 0)
		{
			char *prv = ((char*)constant) - 8;
			previous_f = (sf_footer*)prv;
		}
		else
			previous_f = NULL;

		int col_flag = -1;
		//sf_free_header *previous = header -> prev;
		//printf("header size: %d\n",(int)(constant -> header.block_size << 4));
		if(head_flag != 0 && previous_f != NULL && previous_f -> alloc == 0)
		{
			coalesce++;
			col_flag = 0;
			char *h_prv = (char*)previous_f - (previous_f -> block_size << 4) + 8;
			sf_free_header* previous = (sf_free_header*)h_prv;
			//printf("%s\n", "in previous");
			int new_size = (header -> header.block_size << 4) + (previous -> header.block_size << 4);
			previous -> header.block_size = new_size >> 4;
			char* footer = (char*)previous + new_size - 8;
			sf_footer* f = (sf_footer*)footer;
			f ->block_size = new_size >> 4;
			previous -> next = header -> next;
			(previous -> next) -> prev = previous;
			//previous -> prev = header -> prev -> prev;
			header -> prev = NULL;
			header -> next = NULL;
			constant = previous;
		}

		//check if next node in free list is free
		char *nxt = ((char*)constant) + (constant -> header.block_size << 4) ;
		sf_free_header* nexter = (sf_free_header*)nxt;
		//printf("nexter: %p\n", (void*)nexter);
		//sf_free_header *nexter = constant -> next;
		if((void*)nexter < sf_sbrk(0) && nexter != NULL && nexter -> header.alloc == 0)
		{
			if(col_flag != 0)
				coalesce++;
			//printf("%s\n", "in next");
			int new_size = (constant -> header.block_size << 4) + (nexter -> header.block_size << 4);
			//printf("%d\n",(int)new_size );
			constant -> header.block_size = new_size >> 4;
			char *footer = (char*)constant + new_size - 8;
			sf_footer* foot = (sf_footer*)footer;
			foot -> block_size = new_size >> 4;
			foot -> alloc = 0;
			constant -> next = nexter -> next;
			if(nexter -> next != NULL)
				(nexter -> next) -> prev = constant;
			// 	constant -> prev = nexter -> prev -> prev;

			nexter -> prev = NULL;
			nexter -> next = NULL;
			if(nexter == freelist_head)
				constant = freelist_head;
		}

		// printf("header prev: %p\n", (void*)constant -> prev);
		// printf("header: %p\n", (void*)constant);
		// printf("header next: %p\n", (void*)constant -> next );

		//printf("after freeing size: %d\n", (int)constant -> header.block_size << 4);
		//printf("freelist head: %p\n", (void*)freelist_head);

	}
	else
		errno = EINVAL;
	//return;
}

int valid(void *ptr)
{
	int ret = -1;
	char *p = (char*) ptr - 8;
	sf_free_header *header = (sf_free_header*) p;
	char *f = (char*)p + ((header -> header.block_size << 4) - 8);
	sf_footer *footer = (sf_footer*)f;
	if((void*)header >= heap_beginning && (header -> header.alloc == footer -> alloc &&  header -> header.alloc == 1) && (header -> header.splinter == footer -> splinter) && (header -> header.block_size == footer -> block_size))
		ret = 0;
	return ret;
}

void split(void *ptr, size_t size, int og_size)
{
	sf_free_header* best_fit = (sf_free_header*)ptr;
	int new_block_size = (best_fit -> header.block_size << 4) - size;
		best_fit -> header.block_size = size >> 4;
		best_fit -> header.alloc = 1;
		best_fit -> header.splinter = 0;
		best_fit -> header.splinter_size = 0;
		best_fit -> header.requested_size = og_size;
		best_fit -> header.padding_size = size - og_size - 16;
		best_fit -> header.unused_bits = 0;
		int best_fit_size = best_fit -> header.block_size << 4;
		char *foot = (char*)best_fit + best_fit_size - 8;
		sf_footer* f = (sf_footer*) foot;
		f -> block_size = size >> 4;
		f -> splinter = 0;
		f -> alloc = 1;
		//printf("best_fit header: %p\n", (void*)best_fit);
		//printf("best_fit footer: %p\n", (void*)f);

		sf_free_header* newBlock;
		char *nh = (char*)f + 8;
		newBlock = (sf_free_header*)nh;
		//memset((void*)newBlock, 0, 8);
		newBlock -> header.block_size = new_block_size >> 4;
		char *another_foot = ((char*) newBlock) + (new_block_size - 8);
		sf_footer *a = (sf_footer*) another_foot;
		a -> block_size = new_block_size >> 4;
		a -> alloc = 0;
		a -> splinter = 0;
		//printf("new header: %p\n", (void*)newBlock);
		//printf("new footer: %p\n", (void*)a);

		newBlock -> prev = best_fit -> prev;
		newBlock -> next = best_fit -> next;
		if(best_fit == freelist_head)
			freelist_head = newBlock;
		if(best_fit -> prev != NULL)
			(best_fit -> prev) -> next = newBlock;
		if(best_fit -> next != NULL)
			(best_fit -> next) -> prev = newBlock;
		best_fit -> prev = NULL;
		best_fit -> next = NULL;
		//printf("%d\n", freelist_head -> header.block_size << 4);
		//printf("new header prev: %p\n", (void*)newBlock -> prev);
		//printf("new header: %p\n", (void*)newBlock);
		//printf("new header next: %p\n", (void*)newBlock -> next );
}

int sf_info(info* ptr) {
	double diff = (uintptr_t)heap_end - (uintptr_t)heap_beginning;
	// printf("heap start: %p\n", (void*)heap_beginning);
 //    printf("heap end: %p\n",(void*)heap_end );
	// printf("size of heap: %f\n", diff);
	if(diff == 0)
		return -1;
	if(ptr == NULL)
		return-1;
	int allocBlocks = 0;
	sf_free_header *w = freelist_head;
	while(w != NULL)
	{
		allocBlocks++;
		w = w -> next;
	}
	ptr -> allocatedBlocks = allocBlocks;
	//printf("Allocated Blocks: %d\n", allocBlocks);
	char* b = (char*) heap_beginning;
	sf_header *block = (sf_header*)b;
	int splinter = 0;
	int pad = 0;
	int splinter_size = 0;
	//int coalesces = 0;
	// printf("initial block: %p\n",(void*)block );
	// printf("block size: %d\n",(int)block -> block_size << 4 );
	// printf("heap end: %p\n",(void*)heap_end );
	while((void*)block < heap_end)
	{
		if(block -> alloc == 1 && block -> splinter == 1)
		{
			//printf("%s\n", "splint");
			splinter++;
			splinter_size = splinter_size + (block -> splinter_size);
		}
		if(block -> alloc == 1 && block -> padding_size > 0)
		{
			//printf("%s\n", "pad");
			pad = pad + (block -> padding_size);
		}
		char *bl = (char*)block + (block -> block_size << 4);
		block = (sf_header*)bl;
		// printf("block size: %d\n",(int)block -> block_size << 4 );
		// printf("block: %p\n",(void*)block );
	}
	peak = peak/ diff;
	ptr -> peakMemoryUtilization = peak;
	ptr -> splinterBlocks = splinter;
	ptr -> splintering = splinter_size;
	ptr -> padding = pad;
	ptr -> coalesces = coalesce;

	// printf("peak: %f\n", ptr -> peakMemoryUtilization);
	// printf("splinter blocks:  %d\n",(int)ptr -> splinterBlocks);
	// printf("splinter size:  %d\n",(int)ptr -> splintering);
	// printf("padding:  %d\n",(int)ptr -> padding);
	// printf("coalesces:  %d\n",(int)ptr -> coalesces);
	return 0;
}
