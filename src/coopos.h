/* 
 * File:   coopos.h
 * Author: Jura
 *
 * Created on 8. z�?� 2022, 15:22
 */

#ifndef COOPOS_H
#define	COOPOS_H

#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include "xc.h"

#define MAX_TASKS   (8u)

/**
 * @brief Declaration of HW initialization fnc.
 * @warning User must provide this function
 */
extern void SystemInit(void);

/**
 * @brief Declaration of function which adds tasks to task-queue.
 * @warning User must provide this function
 */
extern void TasksInit(void);

typedef struct {
    jmp_buf buf;                    ///< registers stored by setjmp()
    uint16_t sleep_for;             ///< if > 0: sleep this task for N scheduler periods, if 0: task is active
    uint16_t stack_size;            ///< sizeof stack for given task, must be word-aligned
    uint8_t* stack;                 ///< pointer to stack, allocated by \ref TaskInit()
} task_context_t;

typedef struct {
    jmp_buf buf;                    ///< registers stored by setjmp()
    uint16_t splim;                 ///< original stack-limiter
    uint8_t nr_of_registered_tasks; ///< number of task total (running and sleeping)
    task_context_t* active_task;    ///< pointer to context of active task
} kernel_context_t;

extern task_context_t tasks[];
extern kernel_context_t kernel_context;

/**
 * @brief   Initializes task's enviroment and call it for first time.
 * @note    Stack and stack-limiter initialization is handled explicitly
 * @note    Stack-limiter restoration handled explicitly - setjmp() don't care about SPLIM
 * @pre     tc must be valid and tc.stack_size must be initialized properly
 * @todo    handle situation when malloc returns NULL
 * @param tc_ptr    pointer to initialized task context structure, must be in CPU-register not on stack
 * @param func      name of task function, do not use function pointer (is passed by stack and stack is exchanged here)
 */
#define TaskInit(tc_ptr, func) \
    (tc_ptr)->sleep_for = 0; \
    (tc_ptr)->stack = malloc((tc_ptr)->stack_size); \
    kernel_context.active_task = (tc_ptr); \
    if(setjmp(kernel_context.buf) == 0) { \
        SPLIM = 0xFFFF; \
        WREG15 = (uint16_t)((tc_ptr)->stack); \
        SPLIM =  (uint16_t)((tc_ptr)->stack) + (tc_ptr)->stack_size - 4u; \
        func(tc_ptr); } \
    SPLIM = kernel_context.splim;

/**
 * @brief   Stores kernel's context, exchange stacks and returns to the task.
 * @param my_tcon   pointer to task context structure
 */
#define TaskRecall(tc_ptr) \
    kernel_context.active_task = (tc_ptr); \
    if(setjmp(kernel_context.buf) == 0) { \
        SPLIM = 0xFFFF; \
        longjmp((tc_ptr)->buf, 1); } \
    SPLIM = kernel_context.splim;

/**
 * @brief   Stores current-task's context, exchange stacks and returns control to the kernel.
 * @param sleep_for_    sleep this task for N scheduler periods, pass 0 if task have to stay active
 */
#define Yield(sleep_for_) \
    kernel_context.active_task->sleep_for = (sleep_for_); \
    if(setjmp(kernel_context.active_task->buf) == 0) { \
        SPLIM = 0xFFFF; \
        longjmp(kernel_context.buf, 1); } \
    SPLIM = (uint16_t)(kernel_context.active_task->stack) + kernel_context.active_task->stack_size - 4u;

#endif	/* COOPOS_H */

