#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(sizeof(int));
  *x = 4;
  cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");

}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *pointer = sf_malloc(sizeof(short));
  sf_free(pointer);
  pointer = (char*)pointer - 8;
  sf_header *sfHeader = (sf_header *) pointer;
  cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
  sf_footer *sfFooter = (sf_footer *) ((char*)pointer + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, SplinterSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini){
  //printf("%s\n", "BEFORE MALLOC X!!!!!!!!!!!!");
  void* x = sf_malloc(32);
  //printf("%s\n", "AFTER MALLOC X!!!!!!!!!!!!");
  //printf("%s\n", "BEFORE MALLOC Y!!!!!!!!!!!!");
  void* y = sf_malloc(32);
  //printf("%s\n", "AFTER MALLOC Y!!!!!!!!!!!!");
  (void)y;

  //printf("%s\n", "BEFORE FREE X!!!!!!!!!!!!");
  sf_free(x);
  //printf("%s\n", "AFTER FREE X!!!!!!!!!!!!");

  //printf("%s\n", "BEFORE MALLOC 16!!!!!!!!!!!!");
  x = sf_malloc(16);
  //sf_varprint(x);
  //printf("%s\n", "AFTER MALLOC 16!!!!!!!!!!!!");

  sf_header *sfHeader = (sf_header *)((char*)x - 8);
  cr_assert(sfHeader->splinter == 1, "Splinter bit in header is not 1!");
  cr_assert(sfHeader->splinter_size == 16, "Splinter size is not 16");

  sf_footer *sfFooter = (sf_footer *)((char*)sfHeader + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->splinter == 1, "Splinter bit in header is not 1!");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  memset(x, 0, 0);
  cr_assert(freelist_head->next == NULL);
  cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
  //printf("%s\n", "BEFORE MALLOC X!!!!!!!");
  int *x = sf_malloc(4);
  //printf("%s\n", "AFTER MALLOC X!!!!!!!");
  //printf("%s\n", "BEFORE MALLOC Y!!!!!!!");
  int *y = sf_malloc(4);
  //printf("%s\n", "AFTER MALLOC Y!!!!!!!");
  memset(y, 0, 0);
  //printf("%s\n", "BEFORE FREE X!!!!!!!");
  sf_free(x);
  //printf("%s\n", "AFTER FREE X!!!!!!!");

  //just simply checking there are more than two things in list
  //and that they point to each other
  cr_assert(freelist_head->next != NULL);
  cr_assert(freelist_head->next->prev != NULL);
}

//#
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//#

Test(sf_memsuite, Check_one_free_node, .init = sf_mem_init, .fini = sf_mem_fini) {
  //printf("%s\n", "BEFORE NEW TEST!!!!!!!");
  void* x = sf_malloc(56);
  //sf_varprint(x);
  //printf("\n");
  void* y = sf_malloc(64);
  //sf_varprint(x);
  cr_assert(freelist_head->next == NULL);
  cr_assert(freelist_head->prev == NULL);

  sf_header *xer = (sf_header *)((char*)x - 8);
  sf_header *yer = (sf_header *)((char*)y - 8);
  cr_assert(xer -> alloc == 1);
  cr_assert(yer -> alloc == 1);
  }

  Test(sf_memsuite, Check_new_page, .init = sf_mem_init, .fini = sf_mem_fini) {
    //printf("%s\n", "HERE!!!!!!!!!");
    void* x = sf_malloc(4);
    //sf_varprint(x);
    void* y = sf_malloc(500);
    //sf_varprint(y);
    void* z = sf_malloc(5062);
    //sf_varprint(z);

    sf_header* header = (sf_header*)((char*)z - 8);
    sf_header* header_one = (sf_header*)((char*)y - 8);
    sf_header* header_two = (sf_header*)((char*)x - 8);

    cr_assert(header -> alloc == 1);
    cr_assert(header_one -> alloc == 1);
    cr_assert(header_two -> alloc == 1);

    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
  }

  Test(sf_memsuite, Check_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    //printf("%s\n", "HURRRR!!!!!!!!");
    void* x = sf_malloc(16);
    //sf_varprint(x);
    void* y = sf_malloc(4);
    //sf_varprint(y);
    void* z = sf_malloc(16);
    //sf_varprint(z);

    sf_free(y);
    //sf_varprint(y);

    sf_header* header = (sf_header*)((char*)z - 8);
    sf_header* header_one = (sf_header*)((char*)x - 8);

    cr_assert(freelist_head->next != NULL);
    cr_assert(freelist_head->prev == NULL);

    cr_assert(header -> alloc == 1);
    cr_assert(header_one -> alloc == 1);
  }

  Test(sf_memsuite, Check_yes_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    //printf("%s\n", "OVA HEEEEREEEE !!!!");
    void* x = sf_malloc(16);
    //sf_varprint(x);
    void* y = sf_malloc(4);
    //sf_varprint(y);
    sf_free(x);
    //sf_varprint(x);
    void *z = sf_malloc(52);
    //sf_varprint(z);
    void* w = sf_malloc(32);
    //sf_varprint(w);
    sf_free(y);
    //sf_varprint(x);
    sf_free(w);
    //sf_varprint(w);
    sf_free(z);
    //sf_varprint(x);

    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);

    //cr_assert(freelist_head-> next -> prev != NULL);
  }
  Test(sf_memsuite, Check_header_change, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = sf_malloc(16);
    void* y = sf_malloc(4);
    void* z = sf_malloc(16);
    sf_free(y);

    sf_header* header = (sf_header*)((char*)z - 8);
    sf_header* header_one = (sf_header*)((char*)x - 8);

    cr_assert(header -> alloc == 1);
    cr_assert(header_one -> alloc == 1);
    char* yer = (char*) y -8;
    cr_assert(freelist_head == (void*)yer);
  }

  Test(sf_memsuite, Check_calid_pointer, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = malloc(45678);
    void* ans = sf_realloc(x, 6);

    cr_assert(ans == NULL);
  }

Test(sf_memsuite, Check_realloc_to_smaller_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = sf_malloc(33);
    void* y = sf_malloc(59);
    //sf_varprint(y);
    void* z = sf_malloc(16);
    sf_free(y);
    //sf_varprint(y);
    //sf_varprint(x);
    sf_realloc(x, 27);
    //sf_varprint(x);
    sf_free(x);
    //sf_varprint(x);

    sf_header* header = (sf_header*)((char*)z - 8);
    //sf_header* header_one = (sf_header*)((char*)x - 8);

    cr_assert(header -> alloc == 1);
    //cr_assert(header_one -> alloc == 1);
}

Test(sf_memsuite, Check_realloc_to_larger_with_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
  void* x = sf_malloc(234);
    void* y = sf_malloc(678);
    //sf_varprint(y);
    void* z = sf_malloc(16);
    sf_free(y);
    //sf_varprint(y);
    //sf_varprint(x);
    void* new = sf_realloc(x, 678);
    //sf_varprint(new);
    sf_free(x);
    //sf_varprint(x);

    sf_header* header = (sf_header*)((char*)z - 8);
    cr_assert(header -> alloc == 1);
    cr_assert(x == new);

  }

  Test(sf_memsuite, Check_realloc_to_larger_without_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
  void* x = sf_malloc(234);
    void* y = sf_malloc(678);
    //sf_varprint(y);
    void* z = sf_malloc(16);

    //sf_varprint(x);
    void* new = sf_realloc(x, 678);
    //sf_varprint(new);
    //sf_varprint(x);
    sf_free(x);
    //sf_varprint(x);

    sf_header* header = (sf_header*)((char*)z - 8);
    cr_assert(header -> alloc == 1);
    sf_header* header_one = (sf_header*)((char*)y - 8);
    cr_assert(header_one -> alloc == 1);
    cr_assert(x != new);

  }

Test(sf_memsuite, Check_realloc_to_smaller, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = sf_malloc(16);
    void* y = sf_malloc(58);
    //sf_varprint(y);
    void* z = sf_malloc(16);
    sf_free(x);
    sf_realloc(y, 47);
    //sf_varprint(y);

    sf_header* header = (sf_header*)((char*)z - 8);
    sf_header* header_one = (sf_header*)((char*)x - 8);

    cr_assert(header -> alloc == 1);
    cr_assert(header_one -> alloc == 0);

    //char* heap_beginning_eight = (char*)heap_beginning + 8;
    //sf_varprint((void*)heap_beginning_eight);

    info* pointer = (info*)sf_malloc(sizeof(info));
    int i = sf_info(pointer);

    cr_assert(i == 0);
  }

Test(sf_memsuite, Check, .init = sf_mem_init, .fini = sf_mem_fini) {

}


