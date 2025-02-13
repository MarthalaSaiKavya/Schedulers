#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lab1.h>

unsigned long currSP;
extern int ctxsw(int, int, int, int);
int getmaxgoodness();
int getmaxgoodnessnode();
int currprio = 0;
int epoch = 0;

int scheduler_class = -1;

void setschedclass(int sched_class)
{
    // kprintf("Setting scheduler class to: %d\n", sched_class);
    scheduler_class = sched_class;
}

int getschedclass()
{
    // kprintf("Getting scheduler class: %d\n", scheduler_class);
    return scheduler_class;
}

int findMaxGoodnessProcess() {
    static int last_selected = -1;
    int current;
    int maxGoodnessValue = findMaxGoodness(); 
    int next_pid = -1;
    int found_last = 0;

    if (maxGoodnessValue <= 0) {
        last_selected = -1;
        return getlast(rdytail); 
    }

    current = q[rdyhead].qnext;

    while (current != rdytail) {
        if (proctab[current].dynamic_goodness == maxGoodnessValue) {
            if (last_selected == -1) { 
                next_pid = current;
                break;
            }
            if (found_last) { 
                next_pid = current;
                break;
            }
            if (current == last_selected) {
                found_last = 1; 
            }
        }
        current = q[current].qnext;
    }

    if (next_pid == -1) {
        current = q[rdyhead].qnext;
        while (current != rdytail) {
            if (proctab[current].dynamic_goodness == maxGoodnessValue) {
                next_pid = current;
                break;
            }
            current = q[current].qnext;
        }
    }

    if (next_pid != -1) {
        last_selected = next_pid;
    } else {
        current = q[rdyhead].qnext;
        while (current != rdytail) {
            if (proctab[current].dynamic_goodness == maxGoodnessValue) {
                next_pid = current;
                last_selected = next_pid;
                break;
            }
            current = q[current].qnext;
        }
    }

    return (next_pid != -1) ? next_pid : 0;
}

int findMaxGoodness()
{
    int current = q[rdyhead].qnext;
    int maxGoodnessValue = -1;
    while (current != rdytail)
    {
        if (proctab[current].dynamic_goodness > maxGoodnessValue)
            maxGoodnessValue = proctab[current].dynamic_goodness;
        current = q[current].qnext;
    }
    return maxGoodnessValue;
}

int resched()
{
    register struct pentry *optr;
    register struct pentry *nptr;
    int curr;

    int sched_class = getschedclass();

    // kprintf("Scheduler Class: %d\n", sched_class);

    if (sched_class == AGESCHED)
    {
        register struct pentry *optr; 
        register struct pentry *nptr; 

        int curr = q[rdyhead].qnext;

        while (curr != rdytail)
        {
            if (curr != 0)
            {
                proctab[curr].pprio = (proctab[curr].pprio + 2 > 99) ? 99 : proctab[curr].pprio + 2;
                q[curr].qkey = proctab[curr].pprio; 
            }
            curr = q[curr].qnext;
        }

        optr = &proctab[currpid];
        currprio = (currprio > optr->pprio) ? currprio : optr->pprio;

        if ((optr->pstate == PRCURR) && (lastkey(rdytail) < currprio))
        {
            return OK;
        }

        if (optr->pstate == PRCURR)
        {
            optr->pstate = PRREADY;
            insert(currpid, rdyhead, currprio);
        }

        currprio = lastkey(rdytail);
        nptr = &proctab[(currpid = getlast(rdytail))];

        nptr->pstate = PRCURR; 
#ifdef RTCLOCK
        preempt = QUANTUM; 
#endif

        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

        return OK;
    }

    else if (sched_class == LINUXSCHED)
    {
        if (epoch <= 0 || isempty(rdyhead) || (findMaxGoodness() <= 0))
        {
            int i = 0;
            epoch = 0;
            for (i = 1; i < NPROC; i++)
            {
                if (proctab[i].pstate != PRFREE)
                {
                    int carryover = proctab[i].unused_quantum / 2;
                    proctab[i].time_quantum = proctab[i].pprio + carryover;
                    proctab[i].unused_quantum = proctab[i].time_quantum;
                    proctab[i].dynamic_goodness = proctab[i].pprio + proctab[i].unused_quantum;
                    epoch += proctab[i].time_quantum;
                    
                    if (proctab[i].pstate != PRREADY && proctab[i].pstate != PRCURR) {
            proctab[i].dynamic_goodness = 0;
            proctab[i].unused_quantum = 0;
        }
                }
            }
            // kprintf("Epoch reset: %d\n", epoch);
        }

        optr = &proctab[currpid];

        if (currpid != 0)
        {
            int used_quantum = optr->unused_quantum - preempt;
            epoch -= used_quantum;
            optr->dynamic_goodness = preempt + optr->pprio;
            optr->unused_quantum = preempt;

            /*  kprintf("PID %d: used=%d, remain=%d, new_goodness=%d\n",
          currpid,
          (optr->unused_quantum - preempt),
          preempt,
          optr->dynamic_goodness); */

            if (optr->unused_quantum <= 0)
            {
                optr->unused_quantum = 0;
                optr->dynamic_goodness = 0;
            }
        }

        if ((optr->pstate == PRCURR) && !isempty(rdyhead) && (findMaxGoodness() < optr->dynamic_goodness))
        {
            // kprintf("Continuing process: %d\n", currpid);
            return OK;
        }

        if (optr->pstate == PRCURR)
        {
            optr->pstate = PRREADY;
            insert(currpid, rdyhead, optr->dynamic_goodness);
        }

        if (findMaxGoodness() > 0 && findMaxGoodnessProcess() > 0)
        {
            nptr = &proctab[(currpid = dequeue(findMaxGoodnessProcess()))];
            preempt = nptr->unused_quantum;
        }
        else
        {
            nptr = &proctab[(currpid = dequeue(0))];
            preempt = QUANTUM;
        }


       // kprintf("Switching to process: %d with preempt: %d\n", currpid, preempt);

        nptr->pstate = PRCURR;
        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
        return OK;
    }

    else
    {

        if (((optr = &proctab[currpid])->pstate == PRCURR) &&
            (lastkey(rdytail) < optr->pprio))
        {
            return (OK);
        }

        if (optr->pstate == PRCURR)
        {
            optr->pstate = PRREADY;
            insert(currpid, rdyhead, optr->pprio);
        }

        nptr = &proctab[(currpid = getlast(rdytail))];
        nptr->pstate = PRCURR;
#ifdef RTCLOCK
        preempt = QUANTUM;
#endif

        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

        return OK;
    }
}
