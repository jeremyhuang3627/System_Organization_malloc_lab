/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "The Instructors",
    /* First member's full name */
    "Andrew Case",
    /* First member's NYU NetID */
    "aic219",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's NYU NetID (leave blank if none) */
    ""
};

#define SEGLISTLEN 20;
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

char * seglist[SEGLISTLEN];

void rmNode(void * p)
{
    int b_size = *(p-1)>>1;

    // using a struct analogy: p->prev->next = p->next;
    if(*(p + b_size) != 0){
        char * prev_head_p = *(p+ b_size);
        *prev_head_p = *(p-2);  
    }

    // using a struct analogy: p->next->prev = p->prev;
    if(*(p-2)!=0){
        int p_next_b_size = (*(p-2) + 1) >> 1; 
        *(*(p-2) + p_next_b_size + 2) = *(p + b_size);
    }
}

// insert the block pointed to by p to seglist[index]
void insertToList(int index,void * p){ 
    int b_size = *(p-1)>>1;
    if (seglist[index] == 0){
        seglist[index] = p-2;
    }else{
        //p->next = root->next;
        *(p-2) = seglist[index];

        //p->prev = 0;
        *(p + b_size) = 0;

        seglist[index] = p-2;
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int i;
    for(i=0;i<SEGLISTLEN;i++)
    {
        seglist[i] = 0;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size = ALIGN(size + SIZE_T_SIZE);
    int index = log2(size);
    char * cur = seglist[index];
    int found = 0;
    int b_size;
    while(!found){
        while((*cur)!=0){
            // if the first bit of the next position is 1 => is a free block
            char nxtChar = (*(cur + 1));
            if (nxtChar&1){
                b_size = nxtChar >> 1;
                if(b_size >= size){
                    found = 1;
                    break;
                }
            }
            cur = *cur;
        }

        if(!found){
            index ++;
            cur = seglist[index];
        }
    }

    void  * p = (void *) cur + 2;

    *(p - 1) = b_size << 1; // head size & isFree info;
    if(b_size - size > 4 ){
        // need to split the block
        *(p + size) = 0; // foot pointer
        *(p + 1 + size) = b_size << 1; // foot size & isFree info;
        *cur = 0;

        index = log2(b_size - size - 4 ); // remaining block size;
        cur = seglist[index];
        while(*cur!=0){
            cur = *cur;
        }
        *cur = p + size + 2;
        *(p + b_size) = cur;
    }
    return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if((*(ptr-1) & 1)){
        printf("Unable to free a free block !\n");
        return;
    }

    int b_size = *(ptr - 1) >> 1;  // block size of the block pointed to by ptr;
    int isPrevFree = (*(ptr - 3) & 1);
    int isNextFree = (*(ptr + b_size + 3) & 1);

    int prev_b_size = *(prev_p-1) >> 1;
    int next_b_size = *(next_p-1) >> 1;

    char * prev_p = ptr - 3 - prev_b_size - 1;
    char * next_p = ptr + b_size + 2;

    if(isNextFree && isPrevFree){

        rmNode(prev_p);
        rmNode(next_p);
        
        int newsize = b_size + prev_b_size + next_b_size + 4 * 2; // 4*2 is the metadata space ;  
        *(prev_p - 1) = newsize << 1; // update the size info for the new block in head
        *(next_p + next_b_size + 1 ) = newsize << 1; // update the size info for the new block in foot; 

        // insert the new block into the list;
        int index = log2(newsize);
        // add at the root;
        insertToList(index,prev_p);
    }else if(isNextFree){
        rmNode(next_p); 
        int newsize = b_size + next_b_size + 4; 
        *(ptr-1) = newsize << 1; // update the size info for the new block in head
        *(next_p + next_b_size + 1 ) = newsize << 1; // update the size info for the new block in foot; 
        int index = log2(newsize);
        insertToList(index,ptr);
    }else if(isPrevFree){
        rmNode(prev_p); 
        int newsize = b_size +prev_b_size + 4; 
        *(prev_p - 1) = newsize << 1;  // update the size info for the new block in head
        *(ptr + b_size + 1 ) = newsize << 1; // update the size info for the new block in foot; 
        int index = log2(newsize);
        insertToList(index,prev_p);
    }else{
        *(ptr-1) = b_size<<1; // update the size info for the new block in head
        *(ptr + b_size + 1) = b_size<<1; // update the size info for the new block in foot;
        int index = log2(b_size);
        insertToList(index,ptr);
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














