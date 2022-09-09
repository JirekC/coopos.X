/*
 * File:   main.c
 * Author: Jura
 *
 * Created on 7. zá?í 2022, 21:02
 * 
 * @note compile witch "-std=c99" option
 */

#include "coopos.h"
#include "demo_tasks.h"

task_context_t tasks[MAX_TASKS];
kernel_context_t kernel_context;

int main(void)
{
    kernel_context.splim = SPLIM; // backup main's stack-limit
    kernel_context.nr_of_registered_tasks = 0;
    
    /*** CPU & HW initialization ***/
    SystemInit();
    /******/
    
    /*** Tasks initialization ***/
    TasksInit();
    /******/

    /*** simple round-robin scheduler ***/
    while(1)
    {
        for(uint8_t task_nr = 0; task_nr < kernel_context.nr_of_registered_tasks; task_nr++)
        {
//            ClrWdt(); // watchdog is checking cooperation of all tasks
            if(tasks[task_nr].sleep_for > 0)
            {
                tasks[task_nr].sleep_for--; // this task has lower priority so sleeping for now
            }
            else
            {
                TaskRecall(&tasks[task_nr]);
            }
        }
    }
    /******/
    
    return 0; // never reach here :)
}
