/*
 * phase3c.c
 *
 */


#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>

#include "phase3.h"
#include "phase3Int.h"

#ifdef DEBUG
static int debugging3 = 1;
#else
static int debugging3 = 0;
#endif

int numFrames;
int numBlocks;
int sectorsInBlock;
int initialized;

typedef struct memory_node{
    int pid; // NOTE: Might not be important, the pid of the page
    int page; // what page is being stored here
    int frame; // frame in the clock, SHOULD NOT CHANGE
    void *frame_address;
    struct memory_node *next;
} memory_node;

typedef struct swap_space{
    int pid;
    int page;
    int block; // block number, calculated using sector size
    int sector; 
    struct swap_space *next;
} swap_space;

memory_node *head_memory;
swap_space *head_disk;

static void debug3(char *fmt, ...)
{
    va_list ap;

    if (debugging3) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * P3SwapInit --
 *
 *  Initializes the swap data structures.
 *
 * Results:
 *   P3_ALREADY_INITIALIZED:    this function has already been called
 *   P1_SUCCESS:                success
 *
 *----------------------------------------------------------------------
 */
int
P3SwapInit(int pages, int frames)
{
    int result = P1_SUCCESS;
    int rc;
    void *vmRegion;
    void *pmAddr;
    int pageSize, numPages, numFrames, mode, sectorSize, numSectors;
    int i;
    swap_space *cur_disk;
    memory_node *cur_mem;
    // check if initialized
    if(initialized == 1){
        return P3_ALREADY_INITIALIZED;
    }
    // sets global numFrames
    numFrames = frames;
    // get mmu info
    rc = USLOSS_MmuGetConfig(&vmRegion, &pmAddr, &pageSize, &numPages, &numFrames, &mode);
    assert(rc == USLOSS_MMU_OK);
    // memory init (will be used)
    // This is the clock
    head_memory = (memory_node *)malloc(sizeof(memory_node));
    head_memory->pid = -1;
    head_memory->page = -1;
    head_memory->frame = 0;
    head_memory->frame_address = pmAddr;
    cur_mem = head_memory;
    // create rest of linked list
    for(i = 1; i < frames; i++){
        cur_mem->next = (memory_node *)malloc(sizeof(memory_node));
        cur_mem = cur_mem->next;
        cur_mem->pid = -1;
        cur_mem->page = -1;
        cur_mem->frame = i;
        cur_mem->frame_address = pmAddr + (i * pageSize);
    }
    cur_mem->next = NULL;
    
    //swap space init
    rc = P2_DiskSize(P3_SWAP_DISK, &sectorSize, &numSectors);
    assert(rc == P1_SUCCESS);
    // sets max number of blocks on swap disk
    // pageSize / sectorSize gives us how many sectors are in a page
    // numSectors / ^ gives us how many page sized blocks are on the disk
    // This allows us to keep track of any free blocks in memory
    numBlocks = numSectors / (pageSize / sectorSize);
    sectorsInBlock = (pageSize / sectorSize);
    // set head (will be used)
    head_disk = (swap_space *)malloc(sizeof(swap_space));
    head_disk->pid = -1;
    head_disk->page = -1;
    head_disk->block = 0;
    head_disk->sector = 0;
    cur_disk = head_disk;
    // create rest of linked list
    for(i = 1; i < numBlocks; i++){
        cur_disk->next = (swap_space *)malloc(sizeof(swap_space));
        cur_disk = cur_disk->next;
        cur_disk->pid = -1;
        cur_disk->page = -1;
        cur_disk->block = i;
        cur_disk->sector = i * (pageSize / sectorSize);
    }
    cur_disk->next = NULL;
    initialized = 1;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3SwapFreeAll --
 *
 *  Frees all swap space used by a process
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3SwapInit has not been called
 *   P1_INVALID_PID:        pid is invalid      
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */

int
P3SwapFreeAll(int pid)
{
    int result = P1_SUCCESS;
    swap_space *cur = head_disk;
    // free all swap space used by the process
    if(initialized == 0){
        return P3_NOT_INITIALIZED;
    }
    while(cur->next != NULL){
        if(cur->pid == pid){
            cur->pid = -1;
            cur->page = -1;
        }
        cur = cur->next;
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3SwapOut --
 *
 * Uses the clock algorithm to select a frame to replace, writing the page that is in the frame out 
 * to swap if it is dirty. The page table of the page’s process is modified so that the page no 
 * longer maps to the frame. The frame that was selected is returned in *frame. 
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3SwapInit has not been called
 *   P1_OUT_OF_SWAP:        there is no more swap space
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3SwapOut(int *frame) 
{
    /*****************

    static int hand = -1;    // initially start with frame 0
    if not initialized
        return P3_NOT_INITIALIZED
    loop
        hand = (hand + 1) % numFrames
        get access bits for the frame indicated by hand (USLOSS_MmuGetAccess)
        if reference bit is not set in the access bits
            frame = hand
            break
        else
            clear frame's reference bit (USLOSS_MmuSetAccess)
    page = page that's in the selected frame
    pid = pid of the page
    if page doesn't already have swap space
        if there is free swap space
            allocate swap space for the page
        else
            return P3_OUT_OF_SWAP
    if dirty bit is set in the access bits or page isn't in swap space
        write page to its swap space (P2_DiskWrite)
        clear frame's dirty bit (USLOSS_MmuSetAccess)
    get page table for pid (P3PageTableGet)
    update page's PTE to indicate page is no longer in the frame

    *****************/
    static int hand = -1;
    int access_bits, rc, page, pid, page_in_swap = 0;
    USLOSS_PTE *table;
    memory_node *cur_mem = head_memory;
    swap_space *cur_disk = head_disk;
    swap_space *free_block = NULL;

    // error check
    if(initialized == 0){
        return P3_NOT_INITIALIZED;
    }
    // checks for frame to overwrite
    while(1){
        hand = (hand + 1) % numFrames;
        rc = USLOSS_MmuGetAccess(hand, &access_bits);
        assert(rc == USLOSS_MMU_OK);
        // if refererence bit is not set
        if(access_bits == 0 || access_bits == 2){
            *frame = hand;
            break;
        }
        // if reference bit is set, set it to 0
        else{
            rc = USLOSS_MmuSetAccess(hand, (access_bits - 1));
            assert(rc == USLOSS_MMU_OK);
        }
    }
    // gets the frame to be swapped
    while(cur_mem->frame != *frame){
        cur_mem = cur_mem->next;
    }

    // set page and pid (stored in frame being swapped)
    page = cur_mem->page;
    pid = cur_mem->pid;
    // goes through disk structure to see if page is in disk
    while(cur_disk->next != NULL){
        cur_disk = cur_disk->next;
        // is the page looking at the page being swapped out
        if(cur_disk->page == page){
            page_in_swap = 1;
            break;
        }
        // if no page in block make not of free block
        if(cur_disk->page == -1){
            free_block = cur_disk;
        }
    }
    // if the page is not in disk
    if(page_in_swap == 0){
        // allocates block
        if(free_block != NULL){
            free_block->pid = pid;
            free_block->page = page;
        }
        // out of swap if no more memory
        else{
            return P3_OUT_OF_SWAP;
        }
    }
    // if dirty bit is set
    if(access_bits == 2 || access_bits == 3){
        // write page to disk
        rc = P2_DiskWrite(P3_SWAP_DISK, free_block->sector, sectorsInBlock, cur_mem->frame_address);
        assert(rc == P1_SUCCESS);
        // frame is no longer dirty since written to disk, so set dirty bit to 0
        rc = USLOSS_MmuSetAccess(hand, (access_bits - 2));
        assert(rc == USLOSS_MMU_OK);
    }
    // modify page table for pid
    rc = P3PageTableGet(pid, &table);
    assert(rc == P1_SUCCESS);
    // incore is the bit that sets if in frame, 0 means its not in frame
    table->incore = 0;
    return P1_SUCCESS;
}
/*
 *----------------------------------------------------------------------
 *
 * P3SwapIn --
 *
 *
 * Results:
 *   P3_NOT_INITIALIZED:     P3SwapInit has not been called
 *   P1_INVALID_PID:         pid is invalid      
 *   P1_INVALID_PAGE:        page is invalid         
 *   P1_INVALID_FRAME:       frame is invalid
 *   P3_PAGE_NOT_FOUND:      page is not in swap
 *   P1_SUCCESS:             success
 *
 *----------------------------------------------------------------------
 */
int
P3SwapIn(int pid, int page, int frame)
{
    int pageInDisk = 0;
    int rc;
    memory_node *cur = head_memory;
    swap_space *cur_disk = head_disk;
    /*****************

    if not initialized
        return P3_NOT_INITIALIZED
    record that frame holds pid,page for use in P3SwapOut
    if page is on swap disk
        read page from swap disk into frame (P2_DiskRead)
        return P1_SUCCESS
    else
        return P3_PAGE_NOT_FOUND        

    *****************/
    if(initialized == 0){
        return P3_NOT_INITIALIZED;
    }
    // moves to the specified frame
    while(cur->frame != frame){
        cur = cur->next;
    }
    // sets the page and pid of the frame
    cur->page = page;
    cur->pid = pid;\
    // looks for the page in the disk. If doesn't find page returns P3_PAGE_NOT_FOUND
    while(cur_disk->next != NULL){
        if(cur_disk->page == page){
            pageInDisk = 1;
            break;
        }
        cur_disk = cur_disk->next;
    }
    // if page is in disk read the page into the frame
    if(pageInDisk == 1){
        rc = P2_DiskRead(P3_SWAP_DISK, cur_disk->sector, sectorsInBlock, cur->frame_address);
        return P1_SUCCESS;
    }
    else{
        return P3_PAGE_NOT_FOUND;
    }
}