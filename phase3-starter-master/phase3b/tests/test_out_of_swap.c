/*
 * test_out_of_swap.c
 *  
 *  Tests that the Child terminates when the swap runs out.
 *
 */
#include <usyscall.h>
#include <libuser.h>
#include <assert.h>
#include <usloss.h>
#include <stdlib.h>
#include <phase3.h>
#include <stdarg.h>
#include <unistd.h>

#include "tester.h"
#include "phase3Int.h"

#define FRAMES 10           // # of frames
#define PAGES  FRAMES       // # of pages
#define PAGERS 1            // # of pagers

static char *vmRegion;
static int  pageSize;
static int  lock;
static int  cond;
static int  ready = FALSE;

static int passed = FALSE;

#ifdef DEBUG
int debugging = 1;
#else
int debugging = 0;
#endif /* DEBUG */

#define ACQUIRE(lock) { \
    int _rc = Sys_LockAcquire(lock); \
    assert(_rc == P1_SUCCESS); \
}

#define RELEASE(lock) { \
    int _rc = Sys_LockRelease(lock); \
    assert(_rc == P1_SUCCESS); \
}

#define WAIT(cond) { \
    int _rc = Sys_CondWait(cond); \
    assert(_rc == P1_SUCCESS); \
}

#define SIGNAL(cond) { \
    int _rc = Sys_CondSignal(cond); \
    assert(_rc == P1_SUCCESS); \
}

static void
Debug(char *fmt, ...)
{
    va_list ap;

    if (debugging) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}
static int
Child(void *arg)
{
    int     pid;
    int     pages = (int) arg;

    Sys_GetPid(&pid);
    Debug("Child (%d) starting.\n", pid);

    // write the first byte of each page.
    for (int page = 0; page < pages; page++) {
        *(vmRegion + page * pageSize) = 'A' + page;
    }

    // let P4_Startup know we wrote all the pages
    ACQUIRE(lock);
    ready = TRUE;
    SIGNAL(cond);
    RELEASE(lock);

    int rc = Sys_Sleep(1);
    TEST_RC(rc, P1_SUCCESS);
    return 9;
}

static PID childPID;

int
P4_Startup(void *arg)
{
    int     rc;
    int     pid;
    int     status;

    Debug("P4_Startup starting.\n");
    rc = Sys_VmInit(PAGES, PAGES, FRAMES, PAGERS, (void **) &vmRegion, &pageSize);
    TEST(rc, P1_SUCCESS);

    rc = Sys_LockCreate("childReadyLock", &lock);
    TEST(rc, P1_SUCCESS);

    rc = Sys_CondCreate("childReady", lock, &cond);
    TEST(rc, P1_SUCCESS);

    // Create a child that writes all the pages and uses up all the frames.
    rc = Sys_Spawn("Child0", Child, (void *) PAGES, USLOSS_MIN_STACK * 4, 3, &childPID);
    TEST_RC(rc, P1_SUCCESS);

    // wait for the child to write all of its pages
    ACQUIRE(lock);
    while(!ready) {
        WAIT(cond);
    }
    RELEASE(lock);

    // There should be no more frames left, this child should terminate with P3_OUT_OF_SWAP.
    rc = Sys_Spawn("Child1", Child, (void *) 1, USLOSS_MIN_STACK * 4, 3, &childPID);
    TEST_RC(rc, P1_SUCCESS);

    rc = Sys_Wait(&pid, &status);
    TEST_RC(rc, P1_SUCCESS);
    TEST(pid, childPID);
    TEST(status, P3_OUT_OF_SWAP);

    PASSED();
    return 0;
}


void test_setup(int argc, char **argv) {
}

void test_cleanup(int argc, char **argv) {
    if (passed) {
        USLOSS_Console("TEST PASSED.\n");
    }
}

void finish(int argc, char **argv) {}

// Phase 3c stubs

#include "phase3Int.h"

int P3SwapInit(int pages, int frames) {return P1_SUCCESS;}
int P3SwapFreeAll(PID pid) {return P1_SUCCESS;}
int P3SwapOut(int *frame) {return P3_OUT_OF_SWAP;}
int P3SwapIn(PID pid, int page, int frame) {return P3_PAGE_NOT_FOUND;}


