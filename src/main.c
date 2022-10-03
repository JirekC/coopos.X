/*
 * File:   main.c
 * Author: Jura
 *
 * Created on 7. z�?� 2022, 21:02
 * 
 * @note compile witch "-std=c99" option
 */

#include "coopos.h"
#include "demo_tasks.h"

task_context_t tasks[MAX_TASKS];
kernel_context_t kernel_context;
WALL_CLK_T wall_clock = 0;

/**
 * @brief   Stores kernel's context, exchange stacks and returns to the task.
 * @param my_tcon   pointer to task context structure
 */
#define TaskRecall(tc_ptr) do { \
    kernel_context.active_task = (tc_ptr); \
    if(setjmp(kernel_context.buf) == 0) { \
        SPLIM = 0xFFFF; \
        longjmp(kernel_context.active_task->buf, 1); } \
    SPLIM = kernel_context.splim; \
} while(0)

int main(void)
{
    char saved_ipl;
    kernel_context.splim = SPLIM; // backup main's stack-limit
    kernel_context.nr_of_registered_tasks = 0;
    
    // initialization without interrupts
    SET_AND_SAVE_CPU_IPL(saved_ipl,7);
    
    /*** CPU & HW initialization ***/
    SystemInit();
    /******/
    
    /*** Tasks initialization ***/
    TasksInit();
    /******/

    RESTORE_CPU_IPL(saved_ipl);
    
    /*** simple round-robin scheduler ***/
    while(1)
    {
        for(uint8_t task_nr = 0; task_nr < kernel_context.nr_of_registered_tasks; task_nr++)
        {
//            ClrWdt(); // watchdog is checking cooperation of all tasks
            /*typeof(task_context_t.sleep_for)*/ int32_t s;
            INTERRUPT_PROTECT(s = tasks[task_nr].sleep_for);
            if(s == 0)
            {
                TaskRecall(&tasks[task_nr]);
            }
        }
    }
    /******/
    
    return 0; // never reach here :)
}

WALL_CLK_T GetWallClock(void)
{
    WALL_CLK_T wclk;
    INTERRUPT_PROTECT(wclk = wall_clock);
    return wclk;
}

void SysTickISR(void)
{
    wall_clock++;
    for(uint8_t i = 0; i < kernel_context.nr_of_registered_tasks; i++)
    {
        // update all task-slots
        // no nested interrupts here
        INTERRUPT_PROTECT(
            if(tasks[i].sleep_for > 0)
            {
                tasks[i].sleep_for--;
            }
        );
    }
}
