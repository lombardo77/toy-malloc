#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK 1000000
#define WSIZE 4

#define HNEXT 8
#define PTR 8
#define FREE_BLOCK_NEXT(addr) (addr + 8)
#define FREE_BLOCK_PREV(addr) (addr + 16)
#define FREE_BLOCK_SIZE(addr) *(int*)(addr + 4)
#define SIZE_GIVEN_PAYLOAD(addr) *(int*)(addr - 4)
#define FIRST_FIT 0 
#define NEXT_FIT 1
#define BEST_FIT 2


static char* my_heap;
static char* my_brk;
static char* next_bin;
static int fit_algo;

void myinit(int algo)
{
    fit_algo = algo;
    my_heap = malloc(BLOCK);
    my_brk = my_heap;
    int free = 0x1; //size is 4
    int size = 32; // size is 4
    char* next = my_heap + size; // size is 8
    char* prev = NULL; // size is 8

    *my_brk = free; // write to block
    my_brk = my_brk + sizeof(int); //move up by 4 byte

    *(int*)my_brk = size; //write to block
    my_brk = my_brk + sizeof(int); // move up by 4 bytes

    *(char**)my_brk = next; // write to block
    my_brk = my_brk + sizeof(char*);
    
    *(char**)my_brk = prev; // write to block
    my_brk = my_brk + sizeof(char*);

    my_brk = next;
}

char* new_block(int blockSize, char* addr)
{
    *addr = 0x1; // not free
    addr += sizeof(int);
    *(int*)addr = blockSize; // the size of block
    addr += sizeof(int);
    return addr;
}

void* coalecse(char* free_block)
{
    // (1) iterate through list
    // (2) if tmp == free_block - size(tmp), then coalese backwards
    char* tmp = my_heap;
    while (tmp != my_brk) {
        if (free_block - FREE_BLOCK_SIZE(tmp) == tmp) {
            return tmp;
        }
        tmp = *(char**)FREE_BLOCK_NEXT(tmp);
    }
    return 0;

}

void* myfree(void* alloced_mem)
{
    if (alloced_mem == NULL)
        return NULL;
    char* coal = coalecse(alloced_mem - HNEXT);
    alloced_mem = alloced_mem - HNEXT;
    size_t size_of_freed = FREE_BLOCK_SIZE(alloced_mem);
    *(char*)alloced_mem = 0x0;

    // head_next is the address where the address of the next node is stored
    char* head_next = FREE_BLOCK_NEXT(my_heap);
    // tmp is the address where the actual node is stored
    char* tmp = *(char**)head_next;
    // if coal is not 0, then set the contents of head_next to be equal to 
    // the address stored in coal.
    *(char**)head_next = coal != 0 ? coal : alloced_mem;
    // if coal, then comabine two blocks
    if (coal) {
        size_t size_of_coal = FREE_BLOCK_SIZE(*(char**)head_next); 
        head_next = *(char**)head_next + 4;
        *(int*)head_next = size_of_coal + size_of_freed;
    }
    alloced_mem = FREE_BLOCK_NEXT(alloced_mem);
    *(char**)alloced_mem = tmp;
    alloced_mem = alloced_mem + PTR; // prev
    *(char**)alloced_mem = my_heap;
}


char* next_fit(int blockSize, char* nextFree)
{
    if (next_bin == NULL)
        return NULL;
    else if (blockSize == FREE_BLOCK_SIZE(next_bin)) {
        char* prev = *(char**)FREE_BLOCK_PREV(next_bin);
        prev = FREE_BLOCK_NEXT(prev);
        *(char**)prev = FREE_BLOCK_NEXT(next_bin);
        next_bin = FREE_BLOCK_NEXT(next_bin) != my_brk ? 
            FREE_BLOCK_NEXT(next_bin) : NULL;
        return next_bin;
    }
    return next_fit(blockSize, nextFree);
}

char* first_fit(int blockSize, char* nextFree)
{
    /*iterate through free blocks to find next free
     * whose size >= blockSize*/
    do {
        if (FREE_BLOCK_SIZE(nextFree) >= blockSize) {
            //previous points to next
            char* prev = *(char**)FREE_BLOCK_PREV(nextFree);
            prev = FREE_BLOCK_NEXT(prev);
            *(char**)prev = *(char**)FREE_BLOCK_NEXT(nextFree);
            return nextFree;
        } else 
            nextFree = *(char**)FREE_BLOCK_NEXT(nextFree);
    }
    while (*(char**)nextFree < my_brk);
}

char* best_fit(int blockSize, char* nextFree)
{
    char* tmp = my_heap;
    char* best = *(char**)FREE_BLOCK_NEXT(tmp);
    char* prev = *(char**)FREE_BLOCK_PREV(best);
    while (tmp != my_brk) {
        tmp = *(char**)FREE_BLOCK_NEXT(tmp);
        if (FREE_BLOCK_SIZE(tmp) > blockSize && 
                FREE_BLOCK_SIZE(tmp) <= FREE_BLOCK_SIZE(best)) {
             best = tmp;
             prev = *(char**)FREE_BLOCK_PREV(best);
        }
    }
    prev = FREE_BLOCK_NEXT(prev);
    *(char**)prev = *(char**)FREE_BLOCK_NEXT(best);
    return best;
}

char* get_block(int blockSize)
{
    /*the first address in the heap will have
     * some data in the header that will reveal
     * whether that block is free of not
     * if it is and its size if > what I need
     * then we will store our data there
     * else, we will continue to look for new
     * free blocks. I think it might be useful
     * to have a global address where the last
     * allocated block is.*/

    char* nextFree = *(char**)FREE_BLOCK_NEXT(my_heap);
    char* tmp = my_heap;

    // in the case that we are appending to end of heap
    if (nextFree == my_brk) {
        tmp = FREE_BLOCK_NEXT(my_heap);
        *(char**)tmp = nextFree + blockSize;
        my_brk = my_brk + blockSize; // extend the heap
        return NULL; 
    }
    
    switch (fit_algo) {
        case FIRST_FIT:
            return first_fit(blockSize, nextFree);
        case NEXT_FIT:
            return next_fit(blockSize, nextFree);
        case BEST_FIT:
            return best_fit(blockSize, nextFree);
    }
}

int fetch_blocksize(size_t size)
{
    if (size <= 2*WSIZE) 
        return WSIZE * 4;
    else 
        return WSIZE + WSIZE * ((size/WSIZE) + 1);
}

void* mymalloc(size_t size)
{
    if (!size)
        return NULL;
    int blockSize;
    blockSize = fetch_blocksize(size);

    void* nextFree;
    if ((nextFree = get_block(blockSize)) == NULL)
        return new_block(blockSize, my_brk - blockSize);
    else 
        return new_block(blockSize, nextFree);
}

void* myrealloc(void* ptr, size_t size)
{
    if (!size && ptr == NULL)
        return NULL;
    if (!size) {
        myfree(ptr);
        return NULL;
    }
    if (ptr == NULL)
        return mymalloc(size);
    if (SIZE_GIVEN_PAYLOAD(ptr) >= fetch_blocksize(size))
        return ptr;
    
    void* new_block = mymalloc(size);
    memcpy(new_block, ptr, SIZE_GIVEN_PAYLOAD(ptr) - 2*WSIZE);
    myfree(ptr);
    return new_block; 
}

void mycleanup()
{
    memset(my_heap, 0, BLOCK);
    free(my_heap);
}
