/* resched.c  -  resched */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include<lab1.h>
#include <q.h>
#define RAND_MAX 32767

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
int sched_class=0;
void setschedclass(int);
int getschedclass();
int find_max_proc();
void initilize_proc();
//double log(double );
//double expdev(double);

//int  switchProc(struct pentry **optr,struct pentry **nptr,int nPID);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	switch(getschedclass()){
		case EXPDISTSCHED:
			return exponential_distribution_sched();
		case LINUXSCHED:
			return linux_like_sched();
		default:
			return default_sched();
	}
	return OK;
}
void setschedclass(int sched_type) {
     sched_class = sched_type;
}
int getschedclass(){
	return sched_class;
}
//Finds process with maximum goodness
int find_max_proc(){
	int i;
	int maxPID=0;
	int maxGud=0;
	for (i = q[rdytail].qprev; i != rdyhead; i = q[i].qprev) {
		if (proctab[i].goodness > maxGud) {
			maxPID = i;
			maxGud = proctab[i].goodness;
		}
	}
	return maxPID;
}
//Initialized process and prepare them to be executed in next epoch
void initilize_processes(){
		struct pentry *p;
		int i;
		for(i = 0; i < NPROC; i++){
			p = &proctab[i];
			if (p->pstate == PRFREE)
					continue;
			else if (p->counter == p->quantum||p->counter == 0) 
			{
			  p->quantum = p->pprio;
			}else{
			  p->quantum = (p->counter) / 2 + p->pprio;
			}
				p->counter=p->quantum;
				p->goodness=p->counter + p->pprio;
		}
}
//XENU schedular
int default_sched(){
	
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) {
		return(OK);
	}
	
	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}

	/* remove highest priority process at end of ready list */

	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;		/* mark it currently running	*/
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}
//Linux like schedular
int linux_like_sched(){
	
    register struct pentry *optr; /* pointer to old process entry */
    register struct pentry *nptr; /* pointer to new process entry */
	//if the queue is empty hence run the NULLPROC
	if(isempty(rdyhead)||(q[firstkey(rdyhead)].qnext==NULLPROC && q[lastkey(rdytail)].qprev==NULLPROC)){
		nptr = &proctab[NULLPROC];
		nptr->pstate = PRCURR;
		currpid = NULLPROC;
		preempt = QUANTUM;
		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
		return OK;
	}
	optr = &proctab[currpid];
	optr->goodness = optr->pprio + preempt;
	optr->counter = preempt;
	int maxG = 0;
	int k = 0;
	int newPID = 0;
	if (currpid == NULLPROC||optr->counter <= 0) 
	{
		optr->counter = 0; 
		optr->goodness = 0; 
		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, optr->pprio);
		}
	}
	//Now find which process to execute next 
	newPID=find_max_proc();
	maxG=proctab[newPID].goodness;
	//best case when we find a process which has maximum goodness and available in same epoch
	//context switch will be called when the currently executed process changes its status or its counter gets exhausted 
	if((maxG > 0 )&&(optr->goodness<maxG||optr->pstate!=PRCURR||optr->counter ==0))
	{
		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, optr->pprio);
		}
		nptr = &proctab[newPID];
		nptr->pstate = PRCURR;
		dequeue(newPID);
		currpid = newPID;
		preempt = nptr->counter;
		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
		return OK;
	}/*if there are no other process in epoch ready queue which has higher goodness than current process then run the same process and update the preement.*/ 
	else if (optr->goodness >= maxG && optr->goodness > 0 && optr->pstate == PRCURR) {
		preempt = optr->counter;
		return OK;
	}
	else if(maxG == 0 && (optr->pstate != PRCURR || optr->counter == 0)) {
		initilize_processes();
		int nPID=find_max_proc();
		int maxP=proctab[nPID].goodness;
		
		if(maxP==0){
			/*means no process to execute, run the NULLPROC*/
		nptr = &proctab[NULLPROC];
		nptr->pstate = PRCURR;
		currpid = NULLPROC;
		preempt = QUANTUM;
		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
		return OK;
		
	}else{
		/*update the preemet with the new process which has the higher Goodness*/
		preempt = optr->counter;
	}
		
		/*restart new EPOC*/
		linux_like_sched();	
	}
	return SYSERR;
}


int exponential_distribution_sched(){
	
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	double lamda=0.1;
	double rand_no=expdev(lamda);
	int rand_num=(int)rand_no;
	optr= &proctab[currpid];
	if (optr->pstate == PRCURR) { 
			insert(currpid, rdyhead, optr->pprio);
		}
	if(firstkey(rdyhead)>= rand_num )
	{	
		
	/* remove process with the least priority. */
	   
	nptr = &proctab[ (currpid = getfirst(rdyhead)) ];
	nptr->pstate = PRCURR;		
#ifdef	RTCLOCK
	preempt = QUANTUM;		
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	return OK;
	
	}	
	else if(rand_num >= lastkey(rdytail)){
	/* remove highest priority process at end the queue */
	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;		
#ifdef	RTCLOCK
	preempt = QUANTUM;		
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	return OK;
	}
	/*else case to find the process which has bigger priority than random number*/
	else
	{
		
		int k,newPID;
		for (k = q[rdyhead].qnext; k != rdytail; k = q[k].qnext) 
		{
			if(proctab[k].pprio >= rand_num) 
			{
			
			newPID=k;
			break;
		    }
		}
		/*Implemented Round Robin logic here if there are process with same priority then always choose the process which is near to tail of ready queue, coz enqueue happends from back hence it will follow round robin*/
		while((q[newPID].qnext!=rdytail)&&(proctab[newPID].pprio==proctab[q[newPID].qnext].pprio)){
			
			newPID=q[newPID].qnext;
		}
		currpid = newPID;
		nptr = &proctab[currpid];
		nptr->pstate = PRCURR;
		dequeue(newPID);
#ifdef	RTCLOCK
		preempt = QUANTUM;
#endif
		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
		return OK;
	}
 }


