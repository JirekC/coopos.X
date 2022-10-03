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

#define MAX_TASKS   (12u)
#define WALL_CLK_T  uint64_t

typedef struct {
    jmp_buf buf;                    ///< registers stored by setjmp()
    volatile int32_t sleep_for;     ///< positive: task is sleeping for N sys-tick beats; 0: active; negative: task is disabled
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

#ifndef INTERRUPT_PROTECT
#define INTERRUPT_PROTECT(x) do { \
    char saved_ipl; \
    SET_AND_SAVE_CPU_IPL(saved_ipl,7); \
    x; \
    RESTORE_CPU_IPL(saved_ipl); \
} while(0);
#endif

/**
 * @brief   Initializes task's enviroment and call it for first time.
 * @note    Stack-limiter restoration handled explicitly - setjmp() don't care about SPLIM
 * @todo    handle situation when malloc returns NULL
 * @param tc_ptr    [out] task_context_t* : pointer to created task context structure, NULL when tasks[] is full
 * @param func      [in] name of task function, do not use function pointer (is passed by stack and stack is exchanged here)
 * @param st_size   [in] sizeof stack (in Bytes, will be allocated here)
 */
#define TaskInit(tc_ptr, func, st_size) do { \
    if(kernel_context.nr_of_registered_tasks >= MAX_TASKS) { \
        (tc_ptr)= NULL; \
        break; } \
    kernel_context.active_task = &tasks[kernel_context.nr_of_registered_tasks]; \
    (tc_ptr) = kernel_context.active_task; \
    kernel_context.nr_of_registered_tasks++; \
    kernel_context.active_task->stack_size = (st_size); \
    kernel_context.active_task->stack = malloc(kernel_context.active_task->stack_size); \
    INTERRUPT_PROTECT(kernel_context.active_task->sleep_for = 0); \
    if(setjmp(kernel_context.buf) == 0) { \
        SPLIM = 0xFFFF; \
        WREG15 = (uint16_t)(kernel_context.active_task->stack); \
        SPLIM =  (uint16_t)(kernel_context.active_task->stack) + kernel_context.active_task->stack_size - 4u; \
        func(kernel_context.active_task); } \
    SPLIM = kernel_context.splim; \
} while(0)

/**
 * @brief   Stores current-task's context, exchange stacks and returns control to the kernel.
 * @warning Do not use this macro from interrupt.
 * @param sleep_for_    positive: sleep this task for N sys-tick beats; 0: stay active; negative: disable this task
 */
#define Yield(sleep_for_) do { \
    INTERRUPT_PROTECT(kernel_context.active_task->sleep_for = (sleep_for_)); \
    if(setjmp(kernel_context.active_task->buf) == 0) { \
        SPLIM = 0xFFFF; \
        longjmp(kernel_context.buf, 1); } \
    SPLIM = (uint16_t)(kernel_context.active_task->stack) + kernel_context.active_task->stack_size - 4u; \
} while(0)

/**
 * @brief   Enables task pointed by argument. Task will be scheduled as soon as possible in from round-robin scheduler.
 * @note    Interrupt safe, can be used in interrupt.
 * @param tc_ptr task_context_t* : pointer to task.
 */
#define TaskEnable(tc_ptr) INTERRUPT_PROTECT((tc_ptr)->sleep_for = 0)

/**
 * @brief   Disables task pointed by argument. Can be re-eanbled by \ref TaskEnable()
 * @note    Interrupt safe, can be used in interrupt.
 * @param tc_ptr task_context_t* : pointer to task.
 */
#define TaskDisable(tc_ptr) INTERRUPT_PROTECT((tc_ptr)->sleep_for = -1)

/**
 * @return Actual wall-clock (number of SysTick beats from reset)
 */
WALL_CLK_T GetWallClock(void);

/**
 * @brief   Updates wall-clock time and .sleep_for property in each task's context.
 * @warning User must set-up system timer and call this function from timer's ISR.
 */
void SysTickISR(void);

/**
 * @brief   Declaration of HW initialization fnc.
 * @warning User must provide this function
 */
extern void SystemInit(void);

/**
 * @brief   Declaration of function which adds tasks to task-queue.
 * @warning User must provide this function
 */
extern void TasksInit(void);

#endif	/* COOPOS_H */

