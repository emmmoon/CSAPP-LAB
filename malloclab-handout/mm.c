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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

static char *heap_listp;
static char *empty_listp[10] = {NULL};

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place_in(char *bp, size_t asize);
static void deleteempty(char *bp);
static void makeempty(char *bp);
static int getindex(size_t size);

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// #define DEBUG

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /*Create the initial empty heap*/
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    for(int i = 0;i < MAX_LIST;i++)
        empty_listp[i] = NULL;

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){
        makeempty(bp);
        return bp;
    }

    else if(prev_alloc && !next_alloc) {
        deleteempty(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        makeempty(bp);
    }

    else if(!prev_alloc && next_alloc) {
        deleteempty(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        makeempty(bp);
    }

    else{
        deleteempty(NEXT_BLKP(bp));
        deleteempty(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        makeempty(bp);
    }

    return bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // #ifdef DEBUG
    //     printf("the size of alloc = %zu\n",size);
    // #endif
    size_t asize;
    size_t extendsize;
    char *bp;
    if(size == 0)
        return NULL;
    
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/ DSIZE);

    if((bp = find_fit(asize)) != NULL){
        place_in(bp, asize);
        return bp;
    }

    extendsize = asize;
    if(extendsize <= DSIZE)
        extendsize = 2*DSIZE;
    else
        extendsize = DSIZE * ((extendsize + (DSIZE) + (DSIZE - 1)) / DSIZE);


    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place_in(bp,asize);
    return bp;
}

static void *find_fit(size_t asize)
{
    char *bp;
    int index = getindex(asize);
    for(int i = index;i < MAX_LIST;i++){
        bp = empty_listp[i];
        while(bp && GET_SIZE(HDRP(bp)) <= asize)
            bp = GET_ADDRESS(SUCC(bp));
        if(bp == NULL)
            continue;
        else
            return bp;
    }
    return NULL;
}

static void place_in(char *bp, size_t asize)
{
    size_t old_size = GET_SIZE(HDRP(bp));
    deleteempty(bp);
    if((old_size - asize)>= 2 * DSIZE){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(old_size - asize, 0));
        PUT(FTRP(bp), PACK(old_size - asize, 0));
        coalesce(bp);
    }
    else{
        PUT(HDRP(bp), PACK(old_size, 1));
        PUT(FTRP(bp), PACK(old_size, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    // #ifdef DEBUG
    //     printf("the size of realloc = %zu\n",size);
    //     printf("the ptr address of realloc = %p\n",ptr);
    // #endif
    size_t asize;
    size_t this_size,next_size,next_alloc,new_size;
    void *bp;
    if(ptr == NULL){
        return mm_malloc(size);
    }
    else if(size == 0){
        free(ptr);
        return NULL;
    }
    else{
        if(size <= DSIZE)
            asize = 2 * DSIZE;
        else
            asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
        
        this_size = GET_SIZE(HDRP(ptr));
        next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        // prev_size = GET_SIZE(HDRP(PREV_BLKP(ptr)));
        // prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
        // #ifdef DEBUG
        //     printf("this size is %d\n",this_size);
        //     printf("next size is %d\n",new_size);
        //     printf("next alloc is %d\n",next_alloc);
        // #endif
        if(this_size < asize){
            if(next_size >= (asize - this_size) && next_alloc == 0){
            
                new_size = this_size + next_size;
                bp = NEXT_BLKP(ptr);
                deleteempty(bp);
                if(new_size - asize < 2 * DSIZE){
                    // #ifdef DEBUG
                    //     printf("1\n");
                    // #endif               
                    PUT(HDRP(ptr), PACK(new_size, 1));
                    PUT(FTRP(ptr), PACK(new_size, 1));

                    return ptr;
                }
                else{
                    // #ifdef DEBUG
                    //     printf("2\n");
                    // #endif                
                    PUT(HDRP(ptr), PACK(asize, 1));
                    PUT(FTRP(ptr), PACK(asize, 1));
                    bp = NEXT_BLKP(ptr);
                    PUT(HDRP(bp), PACK(new_size - asize, 0));
                    PUT(FTRP(bp), PACK(new_size - asize, 0));
                    coalesce(bp);

                    return ptr;
                }
            }
            // else if(prev_size >= (asize - this_size) && prev_alloc == 0){

            //     new_size = prev_size + this_size;
            //     ptr = PREV_BLKP(ptr);
            //     if(new_size - asize < 2 * DSIZE){
            //         PUT(HDRP(ptr), PACK(new_size, 1));
            //         PUT(FTRP(ptr), PACK(new_size, 1));

            //         return ptr;
            //     }
            //     else{

            //         PUT(HDRP(ptr), PACK(asize, 1));
            //         PUT(FTRP(ptr), PACK())

            //     }
            // }
            else{

                // #ifdef DEBUG
                //     printf("3\n");
                // #endif                
                bp = mm_malloc(asize);
                // #ifdef DEBUG
                //     printf("the realloc block is %p\n",bp);
                //     printf("the realloc size is %d\n",GET_SIZE(HDRP(bp)));
                // #endif  
                memcpy(bp, ptr, this_size);
                mm_free(ptr);

                return bp;
            }
        }

        else if(this_size == asize){
            // #ifdef DEBUG
            //     printf("4\n");
            // #endif
            return ptr;
        }

        else if(this_size > asize){
            if(this_size - asize < 2 * DSIZE){
                // #ifdef DEBUG
                //     printf("5\n");
                // #endif
                return ptr;
            }
            else{
                // #ifdef DEBUG
                //     printf("6\n");
                // #endif
                PUT(HDRP(ptr), PACK(asize, 1));
                PUT(FTRP(ptr), PACK(asize, 1));
                bp = NEXT_BLKP(ptr);
                PUT(HDRP(bp), PACK(this_size - asize, 0));
                PUT(FTRP(bp), PACK(this_size - asize, 0));
                coalesce(bp);

                return ptr;
            }
        }
    }
    return ptr;
}

static void deleteempty(char *bp)
{
    // #ifdef DEBUG
    //     printf("the pred p is %p\n",GET_ADDRESS(PRED(bp)));
    //     printf("the succ p is %p\n",GET_ADDRESS(SUCC(bp)));
    // #endif
    size_t size = GET_SIZE(HDRP(bp));
    int index = getindex(size);
    if(empty_listp[index] == NULL)
        printf("error:no such empty block in the list.\n");
    else if(GET_ADDRESS(PRED(bp)) == NULL && GET_ADDRESS(SUCC(bp)) == NULL){
        #ifdef DEBUG
            printf("1\n");
        #endif
        empty_listp[index] = NULL;
    }

    else if(GET_ADDRESS(PRED(bp)) == NULL && GET_ADDRESS(SUCC(bp)) != NULL){
        #ifdef DEBUG
            printf("2\n");
        #endif

        char *next_p = GET_ADDRESS(SUCC(bp));
        GET_ADDRESS(PRED(next_p)) = NULL;
        GET_ADDRESS(SUCC(bp)) = NULL;
        empty_listp[index] = next_p;
    }

    else if(GET_ADDRESS(PRED(bp)) != NULL && GET_ADDRESS(SUCC(bp)) == NULL){
        #ifdef DEBUG
            printf("3\n");
        #endif

        char *temp = GET_ADDRESS(PRED(bp));
        GET_ADDRESS(SUCC(temp)) = NULL;
        GET_ADDRESS(PRED(bp))=NULL;
    }

    else{
        #ifdef DEBUG
            printf("4\n");
        #endif
        char *temp = GET_ADDRESS(PRED(bp));
        char *next_p = GET_ADDRESS(SUCC(bp));
        GET_ADDRESS(SUCC(temp)) = next_p;
        GET_ADDRESS(PRED(next_p)) = temp;
        GET_ADDRESS(PRED(bp)) = NULL;
        GET_ADDRESS(SUCC(bp)) = NULL;
    }
    
}


static void makeempty(char *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    int index = getindex(size);
    if(empty_listp[index] == NULL){
        empty_listp[index] = bp;
        GET_ADDRESS(PRED(bp)) = NULL;
        GET_ADDRESS(SUCC(bp)) = NULL;
    } 
    else{
        char *temp = NULL;
        char *next_p = empty_listp[index];
        while(next_p && (GET_SIZE(HDRP(next_p)) <= size)){
            temp = next_p;
            next_p = GET_ADDRESS(SUCC(next_p));
        }
        if(next_p == NULL){
            GET_ADDRESS(SUCC(temp)) = bp;
            GET_ADDRESS(PRED(bp)) = temp;
            GET_ADDRESS(SUCC(bp)) = NULL;
        }
        else{
            if(next_p == empty_listp[index]){
                GET_ADDRESS(SUCC(bp)) = next_p;
                GET_ADDRESS(PRED(bp)) = NULL;
                GET_ADDRESS(PRED(next_p)) = bp;
                empty_listp[index] = bp;

            }
            else{
                GET_ADDRESS(SUCC(temp)) = bp;
                GET_ADDRESS(PRED(bp)) = temp;
                GET_ADDRESS(SUCC(bp)) = next_p;
                GET_ADDRESS(PRED(next_p)) = bp;
            }
        }
    }
}


static int getindex(size_t size)
{
    if(size <= 8)
        return 0;
    else if(size <= 16)
        return 1;
    else if(size <= 32)
        return 2;
    else if(size <= 64)
        return 3;
    else if(size <= 128)
        return 4;
    else if(size <= 256)
        return 5;
    else if(size <= 512)
        return 6;
    else if(size <= 2048)
        return 7;
    else if(size <= 4096)
        return 8;
    else
        return 9;
    
}
