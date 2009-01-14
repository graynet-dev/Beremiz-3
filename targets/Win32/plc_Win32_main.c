/**
 * Win32 specific code
 **/ 

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include <windows.h>

/* provided by POUS.C */
extern int common_ticktime__;

long AtomicCompareExchange(long* atomicvar, long compared, long exchange)
{
    return InterlockedCompareExchange(atomicvar, exchange, compared);
}

struct _timeb timetmp;
void PLC_GetTime(IEC_TIME *CURRENT_TIME)
{
	_ftime(&timetmp);
	
	(*CURRENT_TIME).tv_sec = timetmp.time;
	(*CURRENT_TIME).tv_nsec = timetmp.millitm * 1000000;
}

void PLC_timer_notify()
{
    PLC_GetTime(&__CURRENT_TIME);
    __run();
}

HANDLE PLC_timer = NULL;
void PLC_SetTimer(long long next, long long period)
{
	LARGE_INTEGER liDueTime;
	/* arg 2 of SetWaitableTimer take 100 ns interval*/
	liDueTime.QuadPart =  next / (-100);
	
	/*
	printf("SetTimer(%lld,%lld)\n",next, period);
	*/
	
	if (!SetWaitableTimer(PLC_timer, &liDueTime, common_ticktime__, NULL, NULL, 0))
    {
        printf("SetWaitableTimer failed (%d)\n", GetLastError());
    }
}

/* Variable used to stop plcloop thread */
int runplcloop;
void PlcLoop()
{
	runplcloop = 1;
	while(runplcloop)
	{
	// Set a timer
	PLC_SetTimer(Ttick,Ttick);
	if (WaitForSingleObject(PLC_timer, INFINITE) != WAIT_OBJECT_0)
	{
		printf("WaitForSingleObject failed (%d)\n", GetLastError());
	}
	PLC_timer_notify();
	}
}

HANDLE PLC_thread;
HANDLE debug_sem;
HANDLE debug_wait_sem; 
HANDLE python_sem;
HANDLE python_wait_sem; 

#define maxval(a,b) ((a>b)?a:b)
int startPLC(int argc,char **argv)
{
	unsigned long thread_id = 0;
	/* Translate PLC's microseconds to Ttick nanoseconds */
	Ttick = 1000000 * maxval(common_ticktime__,1);

	debug_sem = CreateSemaphore( 
							NULL,           // default security attributes
					        1,  			// initial count
					        1,  			// maximum count
					        NULL);          // unnamed semaphore
    if (debug_sem == NULL) 
    {
        printf("startPLC CreateSemaphore debug_sem error: %d\n", GetLastError());
        return;
    }
    
	debug_wait_sem = CreateSemaphore( 
					        NULL,           // default security attributes
					        0,  			// initial count
					        1,  			// maximum count
					        NULL);          // unnamed semaphore

    if (debug_wait_sem == NULL) 
    {
        printf("startPLC CreateSemaphore debug_wait_sem error: %d\n", GetLastError());
        return;
    }

	python_sem = CreateSemaphore( 
					        NULL,           // default security attributes
					        1,  			// initial count
					        1,  			// maximum count
					        NULL);          // unnamed semaphore

    if (python_sem == NULL) 
    {
        printf("startPLC CreateSemaphore python_sem error: %d\n", GetLastError());
        return;
    }
	python_wait_sem = CreateSemaphore( 
					        NULL,           // default security attributes
					        0,  			// initial count
					        1,  			// maximum count
					        NULL);          // unnamed semaphore


    if (python_wait_sem == NULL) 
    {
        printf("startPLC CreateSemaphore python_wait_sem error: %d\n", GetLastError());
        return;
    }

	
	/* Create a waitable timer */
    PLC_timer = CreateWaitableTimer(NULL, FALSE, "WaitableTimer");
    if(NULL == PLC_timer)
    {
        printf("CreateWaitableTimer failed (%d)\n", GetLastError());
        return 1;
    }
    if( __init(argc,argv) == 0 )
    {
    	printf("Tick Time : %d ms\n", common_ticktime__);
    	PLC_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlcLoop, NULL, 0, &thread_id);
    }
    else{
    	return 1;
    }
    return 0;
}
static int __debug_tick;

int TryEnterDebugSection(void)
{
	//printf("TryEnterDebugSection\n");
	return WaitForSingleObject(debug_sem, 0) == WAIT_OBJECT_0;
}

void LeaveDebugSection(void)
{
	ReleaseSemaphore(debug_sem, 1, NULL);
    //printf("LeaveDebugSection\n");
}

int stopPLC()
{
	runplcloop = 0;
	WaitForSingleObject(PLC_thread, INFINITE);
	__cleanup();
	__debug_tick = -1;
	ReleaseSemaphore(debug_wait_sem, 1, NULL);
	CloseHandle(debug_sem);
	CloseHandle(debug_wait_sem);
	CloseHandle(PLC_timer);
	CloseHandle(PLC_thread);
}

/* from plc_debugger.c */
int WaitDebugData()
{
	WaitForSingleObject(debug_wait_sem, INFINITE);
	return __debug_tick;
}
 
/* Called by PLC thread when debug_publish finished
 * This is supposed to unlock debugger thread in WaitDebugData*/
void InitiateDebugTransfer()
{
    /* remember tick */
    __debug_tick = __tick;
    /* signal debugger thread it can read data */
    ReleaseSemaphore(debug_wait_sem, 1, NULL);
}

void suspendDebug()
{
	__DEBUG = 0;
    /* Prevent PLC to enter debug code */
	WaitForSingleObject(debug_sem, INFINITE);  
}

void resumeDebug()
{
	__DEBUG = 1;
    /* Let PLC enter debug code */
	ReleaseSemaphore(debug_sem, 1, NULL);
}

/* from plc_python.c */
int WaitPythonCommands(void)
{
    /* Wait signal from PLC thread */
	WaitForSingleObject(python_wait_sem, INFINITE);
}
 
/* Called by PLC thread on each new python command*/
void UnBlockPythonCommands(void)
{
    /* signal debugger thread it can read data */
	ReleaseSemaphore(python_wait_sem, 1, NULL);
}

int TryLockPython(void)
{
	return WaitForSingleObject(python_sem, 0) == WAIT_OBJECT_0;
}

void UnLockPython(void)
{
	ReleaseSemaphore(python_sem, 1, NULL);
}

void LockPython(void)
{
	WaitForSingleObject(python_sem, INFINITE);
}

