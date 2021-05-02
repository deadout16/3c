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

    // initialize the swap data structures, e.g. the pool of free blocks

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


    // free all swap space used by the process


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

    return P3_OUT_OF_SWAP;
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
    int result = P1_SUCCESS;

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

    return result;
}