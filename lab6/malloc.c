/*
	Author: Samuel Steinberg
	October 2018
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "malloc.h"

/* Struct for node: I left out the blink so I would only have a minimum padding of 16 */
typedef struct flist 
{
	int size;
	struct flist *flink; //next available memory
} *Flist;

/* Globals: I added the global Flist to get around the long casts */
void *malloc_head = NULL;
Flist mem_block;

/*In this function I resize the heap: Since it is called when the heap is less than or equal to 8192
  I set the size of the flist to the passed size. I then perform pointer arithmetic to get to the right
  address by moving the flinks of the flist and arrow */
void resize_heap(void *arrow, unsigned int size, int temp)
{
	mem_block->size = size;
	temp = mem_block->size - size;
	mem_block->flink = arrow;
	if ( (int)temp > 8){
	arrow += size;
	mem_block = (Flist)arrow;
	mem_block->size = temp;
	mem_block->flink = ((Flist)arrow)->flink;
	arrow -= size;
	}
}
/*This fucntion will initialize the heap when the malloc_head is NULL to size 8192 and set the flink to NULL */
void initialize_heap()
{
	if (malloc_head == NULL)
	{
		malloc_head = sbrk(8192);
		mem_block = (Flist)malloc_head;
		mem_block->size = 8192;
		mem_block->flink = NULL;
	}
}
/*This function handles the situation where the heap needs to allocate more space.
  I call sbrk(8192) to get 8192 more and then iterate down the list to search for more memory. I then increment the size of the heap by 8192. */
void oversbrk(void *arrow)
{
	sbrk(8192); //make new memory block
	arrow = (Flist)malloc_head;
	while(((Flist)arrow)->flink != NULL)
	{
		arrow = ((Flist)arrow)->flink;
	}
	mem_block = (Flist)arrow;
	mem_block->size += 8192;
}
/*This function will set the head to the next free block. Then calls resize heap to set the size. */
void undersbrk(void *arrow, unsigned int size, int temp)
{
	if (mem_block->flink == NULL)
	{
		malloc_head += size;
	}
	else
	{
		malloc_head = mem_block->flink;
	}
	resize_heap(arrow,size,temp);
}
/*This function will search the heap for the next available memory block. */
void *search_memory(unsigned int size)
{
	int temp = 0;
	void *arrow = NULL;
	initialize_heap();
	arrow = (Flist)malloc_head;
	/*Traverse the flist...*/
	while(arrow != NULL)
	{
		mem_block = (Flist)arrow;
		/*If the heap is large enough enter the statement and set the size accordingly and return the pointer.*/
		if (mem_block->size >= size)
		{
			undersbrk(arrow,size,temp);
			return arrow + 8;
		}
		/*Iterate by setting the arrow to its next link */
		arrow = ((Flist)arrow)->flink;
	}
	
	/*When the arrow hits NULL allocate more heap space and recursively go back through: This ensures enough
	  space is given with sbrk. */
	if (arrow == NULL)
	{
		oversbrk(arrow);
		search_memory(size);
	}
}
/*This fucntion will take the size the user wants to allocate, round it to the nearest multiple of 8, and pad it by 8. */
unsigned int set_padding(unsigned int size)
{
	size = (size + 7) / 8 * 8;
	size += 8;
	return size;
}
/*Accessible location: Adds the free space back into the list*/
void process_free(void *ptr)
{
	//printf("Malloced pointer at: %p\n", ptr);
	ptr -= 8;
	if (malloc_head == NULL)
	{
		malloc_head = ptr;
		mem_block = (Flist)malloc_head;
		mem_block->flink = NULL;
	//	printf("Location of freed pointer %p\n", ptr);
	}
	else
	{
		mem_block = (Flist)ptr;
		mem_block->flink = (Flist)malloc_head;
		malloc_head = (void*)mem_block;
	//	printf("Location of freed pointer %p\n", ptr);
		return;
	}
}
/*Malloc function sets a pad and allocates the desired memory for the user. */
void *malloc(unsigned int size)
{	
	void *buffer = NULL;
	/* If size is zero or a negative then malloc returns a NULL as specified by stdlib*/
	int signed_size = (int)size;
	if (signed_size <= 0)
	{
		return NULL;
	}
	
//	printf("%d number of bytes requested.\n", size);
	size = set_padding(size);
//	printf("%d number of bytes allocated.\n",size); 
	buffer = search_memory(size);
//	printf("Malloced memory address at: 0x%x\n", buffer);
	return buffer;
}
/*This function will check to see if the specified heap space is valid, and if so...*/ 
void free(void *ptr)
{ 
	if (ptr != NULL)
	{
		//mem_block = (Flist)(ptr-8);
		/*Accessible location: Adds the free space back into the list*/
		process_free(ptr);
	}
	/*If pointer is NULL return*/
	else
	{
		return;
	}
}
/*Calloc will call malloc and then zero the value out. */
void *calloc(unsigned int nmemb, unsigned int size)
{
	int total = size + nmemb;
	void *c_holder = NULL;
	c_holder = malloc(total);
	bzero(c_holder, total);
	return c_holder;
}
/*Realloc deallocates the old object and returns a new pointer to a new object with the specifies size */
void *realloc(void *ptr, unsigned int size)
{
	void *r_holder = malloc(size);
	bcopy(ptr, r_holder, size);
	free(ptr);
	return r_holder;
}

