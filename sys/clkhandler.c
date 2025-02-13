#include <proc.h>
extern int getschedclass();

void clkhandler(void) {
#ifdef RTCLOCK
    if (getschedclass() == LINUXSCHED && currpid != NULLPROC) {
        /* For Linux-like scheduler, decrease the quantum counter */
        proctab[currpid].counter--;
    }
    if (--preempt <= 0) {
        resched();
    }
#endif
}
