// os345p2.c - 5 state scheduling	08/08/2013
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include <time.h>
#include "os345.h"
#include "os345signals.h"

#define my_printf	printf

// ***********************************************************************
// project 2 variables
static Semaphore* sTask10;					// task 1 semaphore
static Semaphore* sTask11;					// task 2 semaphore

extern TCB tcb[];							// task control block
extern int curTask;							// current task #
extern Semaphore* semaphoreList;			// linked list of active semaphores
extern jmp_buf reset_context;				// context of kernel stack
extern Semaphore* tics10sec;

extern PQueue * rq;							//ready queue

// ***********************************************************************
// project 2 functions and tasks

int signalTask(int, char**);
int ImAliveTask(int, char**);
int reEntrantTask(int argc, char**);

// ***********************************************************************
// ***********************************************************************
// project2 command
int P2_project2(int argc, char* argv[])
{
	static char* s1Argv[] = { "signal1", "sTask10" };
	static char* s2Argv[] = { "signal2", "sTask11" };
	static char* aliveArgv[] = { "I'm Alive", "3" };

	printf("\nStarting Project 2");
	SWAP;

	// start tasks looking for sTask semaphores
	for (int i = 0; i < 9; i++)	{
		createTask("TenSeconds", reEntrantTask, HIGH_PRIORITY, 0, 0);
	}

	createTask("sTask1",				// task name
		signalTask,				// task
		VERY_HIGH_PRIORITY,	// task priority
		2,							// task argc
		s1Argv);					// task argument pointers

	createTask("sTask2",				// task name
		signalTask,				// task
		VERY_HIGH_PRIORITY,	// task priority
		2,							// task argc
		s2Argv);					// task argument pointers

	createTask("ImAlive",				// task name
		ImAliveTask,			// task
		LOW_PRIORITY,			// task priority
		2,							// task argc
		aliveArgv);				// task argument pointers

	createTask("ImAlive",				// task name
		ImAliveTask,			// task
		LOW_PRIORITY,			// task priority
		2,							// task argc
		aliveArgv);				// task argument pointers

	return 0;
} // end P2_project2



// ***********************************************************************
// ***********************************************************************
// list tasks command
int P2_listTasks(int argc, char* argv[])
{
	int i;
	Semaphore* curSem = semaphoreList;

	//	?? 1) List all tasks in all queues
	// ?? 2) Show the task state (new, running, blocked, ready)
	// ?? 3) If blocked, indicate which semaphore

	for (int i = rq->count - 1; i >= 0; i--) {
		printf("\n%4d%20s%4d  ", rq->queue[i].tid,
			tcb[rq->queue[i].tid].name, tcb[rq->queue[i].tid].priority);
		if (tcb[rq->queue[i].tid].signal & mySIGSTOP) my_printf("Paused");
		else if (tcb[rq->queue[i].tid].state == S_NEW) my_printf("New");
		else if (tcb[rq->queue[i].tid].state == S_READY) my_printf("Ready");
		else if (tcb[rq->queue[i].tid].state == S_RUNNING) my_printf("Running");
		else if (tcb[rq->queue[i].tid].state == S_BLOCKED) my_printf("Blocked    %s", tcb[rq->queue[i].tid].event->name);
		else if (tcb[rq->queue[i].tid].state == S_EXIT) my_printf("Exiting");
	}

	while (curSem) {
		for (int i = curSem->pq->count - 1; i >= 0; i--) {
			printf("\n%4d%20s%4d  ", curSem->pq->queue[i].tid,
				tcb[curSem->pq->queue[i].tid].name, tcb[curSem->pq->queue[i].tid].priority);
			if (tcb[curSem->pq->queue[i].tid].signal & mySIGSTOP) my_printf("Paused");
			else if (tcb[curSem->pq->queue[i].tid].state == S_NEW) my_printf("New");
			else if (tcb[curSem->pq->queue[i].tid].state == S_READY) my_printf("Ready");
			else if (tcb[curSem->pq->queue[i].tid].state == S_RUNNING) my_printf("Running");
			else if (tcb[curSem->pq->queue[i].tid].state == S_BLOCKED) my_printf("Blocked    %s", tcb[curSem->pq->queue[i].tid].event->name);
			else if (tcb[curSem->pq->queue[i].tid].state == S_EXIT) my_printf("Exiting");
		}
		curSem = curSem->semLink;
	}

	return 0;
} // end P2_listTasks



// ***********************************************************************
// ***********************************************************************
// list semaphores command
//
int match(char* mask, char* name)
{
	int i, j;

	// look thru name
	i = j = 0;
	if (!mask[0]) return 1;
	while (mask[i] && name[j])
	{
		if (mask[i] == '*') return 1;
		if (mask[i] == '?');
		else if ((mask[i] != toupper(name[j])) && (mask[i] != tolower(name[j]))) return 0;
		i++;
		j++;
	}
	if (mask[i] == name[j]) return 1;
	return 0;
} // end match

int P2_listSems(int argc, char* argv[])				// listSemaphores
{
	Semaphore* sem = semaphoreList;
	while (sem)
	{
		if ((argc == 1) || match(argv[1], sem->name))
		{
			printf("\n%20s  %c  %d  %s", sem->name, (sem->type ? 'C' : 'B'), sem->state,
				tcb[sem->taskNum].name);
		}
		sem = (Semaphore*)sem->semLink;
	}
	return 0;
} // end P2_listSems



// ***********************************************************************
// ***********************************************************************
// reset system
int P2_reset(int argc, char* argv[])						// reset
{
	longjmp(reset_context, POWER_DOWN_RESTART);
	// not necessary as longjmp doesn't return
	return 0;

} // end P2_reset



// ***********************************************************************
// ***********************************************************************
// kill task

int P2_killTask(int argc, char* argv[])			// kill task
{
	int taskId = INTEGER(argv[1]);				// convert argument 1

	if (taskId > 0) printf("\nKill Task %d", taskId);
	else printf("\nKill All Tasks");

	// kill task
	if (killTask(taskId)) printf("\nkillTask Error!");

	return 0;
} // end P2_killTask



// ***********************************************************************
// ***********************************************************************
// signal command
void sem_signal(Semaphore* sem)		// signal
{
	if (sem)
	{
		printf("\nSignal %s", sem->name);
		SEM_SIGNAL(sem);
	}
	else my_printf("\nSemaphore not defined!");
	return;
} // end sem_signal



// ***********************************************************************
int P2_signal1(int argc, char* argv[])		// signal1
{
	SEM_SIGNAL(sTask10);
	return 0;
} // end signal

int P2_signal2(int argc, char* argv[])		// signal2
{
	SEM_SIGNAL(sTask11);
	return 0;
} // end signal



// ***********************************************************************
// ***********************************************************************
// signal task
//
#define COUNT_MAX	5
//
int signalTask(int argc, char* argv[])
{
	int count = 0;					// task variable

	// create a semaphore
	Semaphore** mySem = (!strcmp(argv[1], "sTask10")) ? &sTask10 : &sTask11;
	*mySem = createSemaphore(argv[1], 0, 0);

	// loop waiting for semaphore to be signaled
	while (count < COUNT_MAX)
	{
		SEM_WAIT(*mySem);			// wait for signal
		printf("\n%s  Task[%d], count=%d", tcb[curTask].name, curTask, ++count);
	}
	return 0;						// terminate task
} // end signalTask



// ***********************************************************************
// ***********************************************************************
// I'm alive task
int ImAliveTask(int argc, char* argv[])
{
	int i;							// local task variable
	while (1)
	{
		printf("\n(%d) I'm Alive!", curTask);
		for (i = 0; i<100000; i++) swapTask();
	}
	return 0;						// terminate task
} // end ImAliveTask



int reEntrantTask(int argc, char* argv[]) {

	while (1) {
		SEM_WAIT(tics10sec);
		time_t t;
		ctime(&t);
		printf("\nTask[%d], %s", curTask, ctime(&t));
	}
}

// **********************************************************************
// **********************************************************************
// read current time
//
char* myTime(char* svtime)
{
	time_t cTime;						// current time

	time(&cTime);						// read current time
	strcpy(svtime, asctime(localtime(&cTime)));
	svtime[strlen(svtime) - 1] = 0;		// eliminate nl at end
	return svtime;
} // end myTime
