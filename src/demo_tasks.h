/* 
 * File:   demo_tasks.h
 * Author: Jura
 *
 * Created on 8. zá?í 2022, 15:20
 */

#ifndef DEMO_TASKS_H
#define	DEMO_TASKS_H

/**
 * @brief HW initialization demo-function.
 */
void SystemInit(void);

/**
 * @brief This function shows how to add tasks to task-queue.
 */
void TasksInit(void);

/**
 * @brief Prototype (and demo) task-function. UART parser.
 * @param my_tcon   pointer to my task context structure
 */
void Task_A(task_context_t* my_tcon);

/**
 * @brief Prototype (and demo) task-function. Checking global variable.
 * @param my_tcon   pointer to my task context structure
 */
void Task_B(task_context_t* my_tcon);

/**
 * @brief Prototype (and demo) task-function. LED blinking.
 * @param my_tcon   pointer to my task context structure
 */
void Task_C(task_context_t* my_tcon);


#endif	/* DEMO_TASKS_H */
