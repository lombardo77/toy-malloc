#ifndef MY_MALLOC
#define MY_MALLOC

void myinit(int algo);
void* mymalloc(size_t size);
void* myrealloc(void* ptr,size_t size);
void* myfree(void* alloced_mem);
void mycleanup();
char* new_block(int blockSize, char* addr);
char* next_fit(int blockSize, char* nextFree);
char* first_fit(int blockSize, char* nextFree);
char* best_fit(int blockSize, char* nextFree);
char* get_block(int blockSize);
int fetch_blocksize(size_t size);
void* coalecse(char* free_block);
#endif
