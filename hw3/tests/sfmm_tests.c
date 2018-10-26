#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"

#define MIN_BLOCK_SIZE (32)

sf_free_list_node *find_free_list_for_size(size_t size) {
    sf_free_list_node *fnp = sf_free_list_head.next;
    while(fnp != &sf_free_list_head && fnp->size < size)
	fnp = fnp->next;
    if(fnp == &sf_free_list_head || fnp->size != size)
	return NULL;
    return fnp;
}

int free_list_count(sf_header *ahp) {
    int count = 0;
    sf_header *hp = ahp->links.next;
    while(hp != ahp) {
	count++;
	hp = hp->links.next;
    }
    return count;
}

void assert_free_list_count(size_t size, int count) {
    sf_free_list_node *fnp = sf_free_list_head.next;
    while(fnp != &sf_free_list_head) {
	if(fnp->size == size)
	    break;
	fnp = fnp->next;
    }
    cr_assert(fnp != &sf_free_list_head && fnp->size == size,
	      "No free list of size %lu was found", size);
    int flc = free_list_count(&fnp->head);
    cr_assert_eq(flc, count,
		 "Wrong number of blocks in free list for size %lu (exp=%d, found=%d)",
		 size, flc);
}

void assert_free_block_count(int count) {
    int n = 0;
    sf_free_list_node *fnp = sf_free_list_head.next;
    while(fnp != &sf_free_list_head) {
	n += free_list_count(&fnp->head);
	fnp = fnp->next;
    }
    cr_assert_eq(n, count, "Wrong number of free blocks (exp=%d, found=%d)", count, n);
}

Test(sf_memsuite_student, malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(1);
	assert_free_list_count(PAGE_SZ - sizeof(sf_prologue) - sizeof(sf_epilogue) - MIN_BLOCK_SIZE, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, malloc_three_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(3 * PAGE_SZ - sizeof(sf_prologue) - sizeof(sf_epilogue) - MIN_BLOCK_SIZE);

	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sf_memsuite_student, malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 2);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_double_free, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int));
	sf_free(x);
	sf_free(x);
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(sizeof(long));
	void *y = sf_malloc(sizeof(double) * 10);
	/* void *z = */ sf_malloc(sizeof(char));

	sf_free(y);

	assert_free_block_count(2);
	assert_free_list_count(96, 1);
	assert_free_list_count(3888, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(sizeof(long));
	void *x = sf_malloc(sizeof(double) * 11);
	void *y = sf_malloc(sizeof(char));
	/* void *z = */ sf_malloc(sizeof(int));

	sf_free(y);
	sf_free(x);

	assert_free_block_count(2);
	assert_free_list_count(128, 1);
	assert_free_list_count(3856, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *u = sf_malloc(1); //32
	/* void *v = */ sf_malloc(40); //48
	void *w = sf_malloc(152); //160
	/* void *x = */ sf_malloc(536); //544
	void *y = sf_malloc(1); // 32
	/* void *z = */ sf_malloc(2072); //2080

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(4);
	assert_free_list_count(32, 2);
	assert_free_list_count(160, 1);
	assert_free_list_count(1152, 1);

	// First block in list should be the most recently freed block.
	sf_free_list_node *fnp = find_free_list_for_size(32);
	cr_assert_eq(fnp->head.links.next, (sf_header *)((char *)y - sizeof(sf_footer)),
		     "Wrong first block in free list (32): (found=%p, exp=%p)",
                     fnp->head.links.next, (sf_header *)((char *)y - sizeof(sf_footer)));
}

Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 10);

	cr_assert_not_null(x, "x is NULL!");
	sf_header *hp = (sf_header *)((char *)x - sizeof(sf_footer));
	cr_assert(hp->info.allocated == 1, "Allocated bit is not set!");
	cr_assert(hp->info.block_size << 4 == 48, "Realloc'ed block size not what was expected!");

	assert_free_block_count(2);
	assert_free_list_count(32, 1);
	assert_free_list_count(3936, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int) * 8);
	void *y = sf_realloc(x, sizeof(char));

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_header *hp = (sf_header *)((char*)y - sizeof(sf_footer));
	cr_assert(hp->info.allocated == 1, "Allocated bit is not set!");
	cr_assert(hp->info.block_size << 4 == 48, "Block size not what was expected!");
	cr_assert(hp->info.requested_size == 1, "Requested size not what was expected!");

	// There should be only one free block of size 4000.
	assert_free_block_count(1);
	assert_free_list_count(4000, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_header *hp = (sf_header *)((char*)y - sizeof(sf_footer));
	cr_assert(hp->info.allocated == 1, "Allocated bit is not set!");
	cr_assert(hp->info.block_size << 4 == 32, "Realloc'ed block size not what was expected!");
	cr_assert(hp->info.requested_size == 4, "Requested size not what was expected!");

	// After realloc'ing x, we can return a block of size 48 to the freelist.
	// This block will coalesce with the block of size 3968.
	assert_free_block_count(1);
	assert_free_list_count(4016, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

Test(sf_memsuit_student,remove_block_from_freelist_after_malloc, .init = sf_mem_init, .fini = sf_mem_fini)
{
	void * x = sf_malloc (sizeof(int)); //size = 32
	sf_malloc(sizeof(double)*10);
	sf_free(x);

	sf_malloc(sizeof(int));
	//sf_free(z);
	sf_free_list_node * size_32_list = find_free_list_for_size(32);


	cr_assert(size_32_list->head.links.next==&size_32_list->head,"block freed isn't removed from the free list!%p,%p",
		size_32_list->head.links.next,&size_32_list->head);
	cr_assert(size_32_list->head.links.next==size_32_list->head.links.prev,"block freed isn't removed from the free list!");

		/*allocating size 32 block, followed by an allocated block (to avoid coalescing after free)
		when freeing the same block, it should clear the prev and next pointers in the corresponding free list
		The sentinel head then will point to itself
		*/
}

Test(sf_memsuit_student,check_block_info_after_malloc, .init = sf_mem_init, .fini = sf_mem_fini)
{
	void *y = sf_malloc(sizeof(char)*120); //128


	void * x = sf_malloc(sizeof(int)); //32

	sf_header *hp = (sf_header *)((char *)x - sizeof(sf_footer));
	cr_assert(hp->info.prev_allocated == 1,"found two free blocks adjacent to each other!");
	cr_assert(hp->info.requested_size == 4, "Requested size not what was expected!");
	cr_assert(hp->info.block_size<<4 == 32, "Block size not what was expected!");
	cr_assert(hp->info.allocated == 1, "Block not allocated!!");

	sf_header *hp1 = (sf_header *)((char *)y - sizeof(sf_footer));
	cr_assert(hp1->info.prev_allocated == 1,"found two free blocks adjacent to each other!");
	cr_assert(hp1->info.requested_size == 120, "Requested size not what was expected!");
	cr_assert(hp1->info.block_size<<4 == 128, "Block size not what was expected!");
	cr_assert(hp1->info.allocated == 1, "Block not allocated!!");



	/*when we allocate space, we're guarenteed to get a block with prev block that's already allocated
		since we coalesce adjacent blocks when freeing memory
		and we take the beginning of the block, and free the rest
	*/

}

Test(sf_memsuit_student,realloc_bigger_block_content_consistency, .init = sf_mem_init, .fini = sf_mem_fini)
{
	char * x = sf_malloc(sizeof(char)*32);
	char * y = sf_malloc(sizeof(char)*5);
	int i;
	char a = 'a';
	for(i=0;i<5;i++)
	{
		*(x+i) = a;
		*(y+i) = a++;
	}
	x = sf_realloc(x,100);
	for(i=0;i<5;i++)
	{
		cr_assert(*(x+i) == *(y+i), "Content after realloc is not consistent!");
	}

}

Test(sf_memsuit_student,realloc_smaller_block_content_consistency, .init = sf_mem_init, .fini = sf_mem_fini)
{
	char * x = sf_malloc(sizeof(char)*100);
	char * y = sf_malloc(sizeof(char)*5);
	int i;
	char a = 'a';
	for(i=0;i<5;i++)
	{
		*(x+i) = a;
		*(y+i) = a++;
	}
	x = sf_realloc(x,5);
	for(i=0;i<5;i++)
	{
		cr_assert(*(x+i) == *(y+i), "Content after realloc is not consistent!");
	}

}
Test(sf_memsuit_student,realloc_size_0, .init = sf_mem_init, .fini = sf_mem_fini)
{
	char * x = sf_malloc(sizeof(char)*100);
	x = sf_realloc(x,0);
	//realloc to 0 size is same as freeing the block
	cr_assert(x==NULL, "Pointer was expected to be freed but isn't");

}


Test(sf_memsuite_student, free_invalid_block_size, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
	//because of the two zeroes, block_size is always a multiple of 16, so we'll test for block_size <32
	void *x = sf_malloc(sizeof(int));
	sf_header *hp = (sf_header *)((char *)x - sizeof(sf_footer));
	hp->info.block_size = 0;
	sf_free(x);
}

Test(sf_memsuite_student, free_block_size_less_than_requested_size, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int));//32
	sf_header *hp = (sf_header *)((char *)x - sizeof(sf_footer));
	hp->info.requested_size = 25;//25+8 = 33 > block_size of 32
	//block_size should always be at least 8 bytes more than the requested size (header info)
	sf_free(x);
}

Test(sf_memsuite_student, coalescing_into_one_big_block, .init = sf_mem_init, .fini = sf_mem_fini)
{
	void *x = sf_malloc(sizeof(int));
    void *y = sf_malloc(sizeof(long)*200);
    void *z = sf_malloc(sizeof(short)*34);
    sf_free(x);
    sf_free(z);
    sf_free(y);

    sf_header *X = (sf_header*)(x-8);



    cr_assert(X->info.block_size<<4 == PAGE_SZ - sizeof(sf_prologue) - sizeof(sf_epilogue)
    	, "Did not collesce successfully!");
}

Test(sf_memsuite_student, check_freelist_head_after_insertion, .init = sf_mem_init, .fini = sf_mem_fini)
{
	void *w = sf_malloc(32);
	sf_malloc(1);
	void *x = sf_malloc(32);
	sf_malloc(1);
    void *y = sf_malloc(32);
    sf_malloc(1);
    void *z = sf_malloc(32);

    sf_free(x);
    sf_free(z);
    sf_free(y);
    sf_free(w);

    sf_header *headOfW = (sf_header*)(w-8);
    sf_free_list_node* size_32_node = find_free_list_for_size(48);




    cr_assert(size_32_node->head.links.next = headOfW
    	, "Insertion into freelist is out of order!");
}
Test(sf_memsuite_student, realloc_same_size, .init = sf_mem_init, .fini = sf_mem_fini)
{
	void *w = sf_malloc(32);
	void *y = sf_realloc(w,32);

    cr_assert(w==y
    	, "Returned different block than expected");
}

//////////////////////////////////////////////////









