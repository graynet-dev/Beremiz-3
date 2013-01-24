/*
 * DEBUGGER code
 * 
 * On "publish", when buffer is free, debugger stores arbitrary variables 
 * content into, and mark this buffer as filled
 * 
 * 
 * Buffer content is read asynchronously, (from non real time part), 
 * and then buffer marked free again.
 *  
 * 
 * */
#include "iec_types_all.h"
#include "POUS.h"
/*for memcpy*/
#include <string.h>
#include <stdio.h>

#define BUFFER_SIZE %(buffer_size)d

/* Atomically accessed variable for buffer state */
#define BUFFER_FREE 0
#define BUFFER_BUSY 1
static long buffer_state = BUFFER_FREE;

/* The buffer itself */
char debug_buffer[BUFFER_SIZE];

/* Buffer's cursor*/
static char* buffer_cursor = debug_buffer;
static unsigned int retain_offset = 0;
/***
 * Declare programs 
 **/
%(programs_declarations)s

/***
 * Declare global variables from resources and conf 
 **/
%(extern_variables_declarations)s

typedef void(*__for_each_variable_do_fp)(void*, __IEC_types_enum);
void __for_each_variable_do(__for_each_variable_do_fp fp)
{
%(for_each_variable_do_code)s
}

__IEC_types_enum __find_variable(unsigned int varindex, void ** varp)
{
    switch(varindex){
%(find_variable_case_code)s
     default:
      *varp = NULL;
      return UNKNOWN_ENUM;
    }
}

#define __Unpack_case_t(TYPENAME) \
        case TYPENAME##_ENUM :\
            *flags = ((__IEC_##TYPENAME##_t *)varp)->flags;\
            forced_value_p = *real_value_p = &((__IEC_##TYPENAME##_t *)varp)->value;\
            break;

#define __Unpack_case_p(TYPENAME)\
        case TYPENAME##_O_ENUM :\
            *flags = __IEC_OUTPUT_FLAG;\
        case TYPENAME##_P_ENUM :\
            *flags |= ((__IEC_##TYPENAME##_p *)varp)->flags;\
            *real_value_p = ((__IEC_##TYPENAME##_p *)varp)->value;\
            forced_value_p = &((__IEC_##TYPENAME##_p *)varp)->fvalue;\
            break;

void* UnpackVar(void* varp, __IEC_types_enum vartype, void **real_value_p, char *flags)
{
    void *forced_value_p = NULL;
    *flags = 0;
    /* find data to copy*/
    switch(vartype){
        __ANY(__Unpack_case_t)
        __ANY(__Unpack_case_p)
    default:
        break;
    }
    if (*flags & __IEC_FORCE_FLAG)
        return forced_value_p;
    return *real_value_p;
}

void Remind(unsigned int offset, unsigned int count, void * p);

void RemindIterator(void* varp, __IEC_types_enum vartype)
{
    void *real_value_p = NULL;
    char flags = 0;
    UnpackVar(varp, vartype, &real_value_p, &flags);

    if(flags & __IEC_RETAIN_FLAG){
        USINT size = __get_type_enum_size(vartype);
        /* compute next cursor positon*/
        unsigned int next_retain_offset = retain_offset + size;
        /* if buffer not full */
        Remind(retain_offset, size, real_value_p);
        /* increment cursor according size*/
        retain_offset = next_retain_offset;
    }
}

extern int CheckRetainBuffer(void);

void __init_debug(void)
{
    /* init local static vars */
    buffer_cursor = debug_buffer;
    retain_offset = 0;
    buffer_state = BUFFER_FREE;
    /* Iterate over all variables to fill debug buffer */
    if(CheckRetainBuffer())
    	__for_each_variable_do(RemindIterator);
    retain_offset = 0;
}

extern void InitiateDebugTransfer(void);

extern unsigned long __tick;

void __cleanup_debug(void)
{
    buffer_cursor = debug_buffer;
    InitiateDebugTransfer();
}

void __retrieve_debug(void)
{
}


void Retain(unsigned int offset, unsigned int count, void * p);

inline void BufferIterator(void* varp, __IEC_types_enum vartype, int do_debug)
{
    void *real_value_p = NULL;
    void *visible_value_p = NULL;
    char flags = 0;

    visible_value_p = UnpackVar(varp, vartype, &real_value_p, &flags);

    if(flags & ( __IEC_DEBUG_FLAG | __IEC_RETAIN_FLAG)){
        USINT size = __get_type_enum_size(vartype);
        if(flags & __IEC_DEBUG_FLAG){
            /* copy visible variable to buffer */;
            if(do_debug){
                /* compute next cursor positon.
                   No need to check overflow, as BUFFER_SIZE
                   is computed large enough */
                char* next_cursor = buffer_cursor + size;
                /* copy data to the buffer */
                memcpy(buffer_cursor, visible_value_p, size);
                /* increment cursor according size*/
                buffer_cursor = next_cursor;
            }
            /* re-force real value of outputs (M and Q)*/
            if((flags & __IEC_FORCE_FLAG) && (flags & __IEC_OUTPUT_FLAG)){
                memcpy(real_value_p, visible_value_p, size);
            }
        }
        if(flags & __IEC_RETAIN_FLAG){
            /* compute next cursor positon*/
            unsigned int next_retain_offset = retain_offset + size;
            /* if buffer not full */
            Retain(retain_offset, size, real_value_p);
            /* increment cursor according size*/
            retain_offset = next_retain_offset;
        }
    }
}

void DebugIterator(void* varp, __IEC_types_enum vartype){
    BufferIterator(varp, vartype, 1);
}

void RetainIterator(void* varp, __IEC_types_enum vartype){
    BufferIterator(varp, vartype, 0);
}

extern int TryEnterDebugSection(void);
extern long AtomicCompareExchange(long*, long, long);
extern void LeaveDebugSection(void);
extern void ValidateRetainBuffer(void);
extern void InValidateRetainBuffer(void);

void __publish_debug(void)
{
    retain_offset = 0;
    InValidateRetainBuffer();
    /* Check there is no running debugger re-configuration */
    if(TryEnterDebugSection()){
        /* Lock buffer */
        long latest_state = AtomicCompareExchange(
            &buffer_state,
            BUFFER_FREE,
            BUFFER_BUSY);
            
        /* If buffer was free */
        if(latest_state == BUFFER_FREE)
        {
            /* Reset buffer cursor */
            buffer_cursor = debug_buffer;
            /* Iterate over all variables to fill debug buffer */
            __for_each_variable_do(DebugIterator);
            
            /* Leave debug section,
             * Trigger asynchronous transmission 
             * (returns immediately) */
            InitiateDebugTransfer(); /* size */
        }else{
            /* when not debugging, do only retain */
            __for_each_variable_do(RetainIterator);
        }
        LeaveDebugSection();
    }else{
        /* when not debugging, do only retain */
        __for_each_variable_do(RetainIterator);
    }
    ValidateRetainBuffer();
}

#define __RegisterDebugVariable_case_t(TYPENAME) \
        case TYPENAME##_ENUM :\
            ((__IEC_##TYPENAME##_t *)varp)->flags |= flags;\
            if(force)\
             ((__IEC_##TYPENAME##_t *)varp)->value = *((TYPENAME *)force);\
            break;
#define __RegisterDebugVariable_case_p(TYPENAME)\
        case TYPENAME##_P_ENUM :\
            ((__IEC_##TYPENAME##_p *)varp)->flags |= flags;\
            if(force)\
             ((__IEC_##TYPENAME##_p *)varp)->fvalue = *((TYPENAME *)force);\
            break;\
        case TYPENAME##_O_ENUM :\
            ((__IEC_##TYPENAME##_p *)varp)->flags |= flags;\
            if(force){\
             ((__IEC_##TYPENAME##_p *)varp)->fvalue = *((TYPENAME *)force);\
             *(((__IEC_##TYPENAME##_p *)varp)->value) = *((TYPENAME *)force);\
            }\
            break;
void RegisterDebugVariable(int idx, void* force)
{
    void *varp = NULL;
    unsigned char flags = force ? __IEC_DEBUG_FLAG | __IEC_FORCE_FLAG : __IEC_DEBUG_FLAG;
    switch(__find_variable(idx, &varp)){
        __ANY(__RegisterDebugVariable_case_t)
        __ANY(__RegisterDebugVariable_case_p)
    default:
        break;
    }
}

#define __ResetDebugVariablesIterator_case_t(TYPENAME) \
        case TYPENAME##_ENUM :\
            ((__IEC_##TYPENAME##_t *)varp)->flags &= ~(__IEC_DEBUG_FLAG|__IEC_FORCE_FLAG);\
            break;

#define __ResetDebugVariablesIterator_case_p(TYPENAME)\
        case TYPENAME##_P_ENUM :\
        case TYPENAME##_O_ENUM :\
            ((__IEC_##TYPENAME##_p *)varp)->flags &= ~(__IEC_DEBUG_FLAG|__IEC_FORCE_FLAG);\
            break;

void ResetDebugVariablesIterator(void* varp, __IEC_types_enum vartype)
{
    /* force debug flag to 0*/
    switch(vartype){
        __ANY(__ResetDebugVariablesIterator_case_t)
        __ANY(__ResetDebugVariablesIterator_case_p)
    default:
        break;
    }
}

void ResetDebugVariables(void)
{
    __for_each_variable_do(ResetDebugVariablesIterator);
}

void FreeDebugData(void)
{
    /* atomically mark buffer as free */
    long latest_state;
    latest_state = AtomicCompareExchange(
        &buffer_state,
        BUFFER_BUSY,
        BUFFER_FREE);
}
int WaitDebugData(unsigned long *tick);
/* Wait until debug data ready and return pointer to it */
int GetDebugData(unsigned long *tick, unsigned long *size, void **buffer){
    int wait_error = WaitDebugData(tick);
    if(!wait_error){
        *size = buffer_cursor - debug_buffer;
        *buffer = debug_buffer;
    }
    return wait_error;
}


/* LOGGING
*/

#define LOG_BUFFER_SIZE (1<<14) /*16Ko*/
#define LOG_BUFFER_MASK (LOG_BUFFER_SIZE-1)
static char LogBuff[LOG_BUFFER_SIZE];
void inline copy_to_log(uint32_t buffpos, void* buf, uint32_t size){
    if(buffpos + size < LOG_BUFFER_SIZE){
        memcpy(&LogBuff[buffpos], buf, size);
    }else{
        uint32_t remaining = LOG_BUFFER_SIZE - buffpos - 1; 
        memcpy(&LogBuff[buffpos], buf, remaining);
        memcpy(LogBuff, buf + remaining, size - remaining);
    }
}
void inline copy_from_log(uint32_t buffpos, void* buf, uint32_t size){
    if(buffpos + size < LOG_BUFFER_SIZE){
        memcpy(buf, &LogBuff[buffpos], size);
    }else{
        uint32_t remaining = LOG_BUFFER_SIZE - buffpos - 1; 
        memcpy(buf, &LogBuff[buffpos], remaining);
        memcpy(buf + remaining, LogBuff, size - remaining);
    }
}

/* Log buffer structure

 |<-Tail1.msgsize->|<-sizeof(mTail)->|<--Tail2.msgsize-->|<-sizeof(mTail)->|...
 |  Message1 Body  |      Tail1      |   Message2 Body   |      Tail2      |

*/
typedef struct {
    uint32_t msgidx;
    uint32_t msgsize;
    /*XXX tick*/
    /*XXX RTC*/
} mTail;

/* Log cursor : 64b
   |63 ... 32|31 ... 0|
   | Message | Buffer |
   | counter | Index  | */
static uint64_t LogCursor = 0x0;

/* Store one log message of give size */
int LogMessage(char* buf, uint32_t size){
    if(size < LOG_BUFFER_SIZE - sizeof(mTail)){
        uint32_t buffpos;
        mTail tail;
        /* We cannot increment both msg index and string pointer 
           in a single atomic operation but we can detect having been interrupted.
           So we can try with atomic compare and swap in a loop until operation
           succeeds non interrupted */
        uint64_t new_cursor, old_cursor;
        do{
            old_cursor = LogCursor;
            buffpos = (uint32_t)old_cursor;
            tail.msgidx = (old_cursor >> 32); 
            new_cursor = ((uint64_t)(tail.msgidx + 1)<<32) 
                         | (uint64_t)((buffpos + size + sizeof(mTail)) & LOG_BUFFER_MASK);
        }while(!__sync_bool_compare_and_swap(&LogCursor,old_cursor,new_cursor));

        copy_to_log(buffpos, buf, size);
        tail.msgsize = size;
        /*XXX tick*/
        /*XXX RTC*/
        copy_to_log((buffpos + size) & LOG_BUFFER_MASK, &tail, sizeof(mTail));

        return 1; /* Success */
    }else{
        char mstr[] = "Logging error : message too big";
        LogMessage(mstr, sizeof(mstr));
    }
    return 0;
}

uint32_t GetLogCount(){
    return (uint64_t)LogCursor >> 32;
}

/* Return message size and content */
uint32_t GetLogMessage(uint32_t msgidx, char* buf, uint32_t max_size){
    uint64_t cursor = LogCursor;
    if(cursor){
        /* feeding cursor values */
        uint32_t curbuffpos = (uint32_t)cursor;
        uint32_t curmsgidx = (cursor >> 32);

        /* seach cursor */
        uint32_t stailpos = (curbuffpos - sizeof(mTail)) & LOG_BUFFER_MASK; 
        uint32_t smsgidx;
        mTail tail;
        tail.msgidx = curmsgidx;

        /* Message search loop */
        do {
            smsgidx = tail.msgidx;
            copy_from_log(stailpos, &tail, sizeof(mTail));
            stailpos = (stailpos - sizeof(mTail) - tail.msgsize ) & LOG_BUFFER_MASK;
        }while(tail.msgidx == smsgidx - 1 && tail.msgidx > msgidx);

        if(tail.msgidx == msgidx){
            uint32_t sbuffpos = (stailpos - tail.msgsize ) & LOG_BUFFER_MASK; 
            uint32_t totalsize = tail.msgsize + sizeof(mTail);
            copy_from_log(stailpos, &tail, totalsize > max_size ? max_size : totalsize);
            return totalsize;
        }
    }
    return 0;
}
