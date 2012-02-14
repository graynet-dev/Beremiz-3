/**
 * Win32 specific code
 **/

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include <windows.h>
#include <locale.h>

/* provided by POUS.C */
extern unsigned long long common_ticktime__;

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
void PLC_SetTimer(unsigned long long next, unsigned long long period)
{
	LARGE_INTEGER liDueTime;
	/* arg 2 of SetWaitableTimer take 100 ns interval*/
	liDueTime.QuadPart =  next / (-100);

	if (!SetWaitableTimer(PLC_timer, &liDueTime, period/1000000, NULL, NULL, 0))
    {
        printf("SetWaitableTimer failed (%d)\n", GetLastError());
    }
}

/* Variable used to stop plcloop thread */
void PlcLoop()
{
    while(WaitForSingleObject(PLC_timer, INFINITE) == WAIT_OBJECT_0)
    {
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
    setlocale(LC_NUMERIC, "C");
	/* Define Ttick to 1ms if common_ticktime not defined */
    Ttick = common_ticktime__?common_ticktime__:1000000;

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
        PLC_SetTimer(Ttick,Ttick);
        PLC_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlcLoop, NULL, 0, &thread_id);
    }
    else{
        return 1;
    }
    return 0;
}
static unsigned long __debug_tick;

int TryEnterDebugSection(void)
{
	//printf("TryEnterDebugSection\n");
    if(WaitForSingleObject(debug_sem, 0) == WAIT_OBJECT_0){
        /* Only enter if debug active */
        if(__DEBUG){
            return 1;
        }
        ReleaseSemaphore(debug_sem, 1, NULL);
    }
    return 0;
}

void LeaveDebugSection(void)
{
	ReleaseSemaphore(debug_sem, 1, NULL);
    //printf("LeaveDebugSection\n");
}

int stopPLC()
{
    CloseHandle(PLC_timer);
    WaitForSingleObject(PLC_thread, INFINITE);
    __cleanup();
    CloseHandle(debug_wait_sem);
    CloseHandle(debug_sem);
    CloseHandle(python_wait_sem);
    CloseHandle(python_sem);
    CloseHandle(PLC_thread);
}

/* from plc_debugger.c */
int WaitDebugData(unsigned long *tick)
{
    *tick = __debug_tick;
    /* Wait signal from PLC thread */
	return WaitForSingleObject(debug_wait_sem, INFINITE) != WAIT_OBJECT_0;
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

int suspendDebug(int disable)
{
    /* Prevent PLC to enter debug code */
    WaitForSingleObject(debug_sem, INFINITE);
    __DEBUG = !disable;
    if(disable)
        ReleaseSemaphore(debug_sem, 1, NULL);
    return 0;
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
	return WaitForSingleObject(python_wait_sem, INFINITE);
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

int CheckRetainBuffer(void)
{
	return 1;
}

void ValidateRetainBuffer(void)
{
}

void InValidateRetainBuffer(void)
{
}

void Retain(unsigned int offset, unsigned int count, void * p)
{
    /*
    unsigned int position;
    for(position=0; position<count; position++ ){
        printf("%d : 0x%2.2x\n", offset+position, ((char*)p)[position]);
    }
    */
}

void Remind(unsigned int offset, unsigned int count, void *p)
{
}

