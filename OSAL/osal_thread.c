/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-06 09:56:03
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-06 22:56:17
 * @FilePath: /rm_base/OSAL/osal_thread.c
 * @Description: 
 */
#include "osal_def.h"

#if (OSAL_RTOS_TYPE == OSAL_THREADX)

#include "tx_api.h"


/* ThreadX下的线程实现 */
osal_status_t osal_thread_create(osal_thread_t *thread, 
                                 const char *name,
                                 osal_thread_entry_t entry,
                                 void *argument,
                                 void *stack_pointer,
                                 unsigned int stack_size,
                                 osal_thread_priority_t priority)
{
    UINT result;
    
    result = tx_thread_create((TX_THREAD*)thread, (CHAR*)name, 
                              (VOID(*)(ULONG))entry, (ULONG)argument,
                              (VOID*)stack_pointer, stack_size,
                              priority, priority,
                              TX_NO_TIME_SLICE, TX_DONT_START);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_thread_start(osal_thread_t *thread)
{
    UINT result;
    result = tx_thread_resume((TX_THREAD*)thread);
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_thread_stop(osal_thread_t *thread)
{
    UINT result;
    result = tx_thread_suspend((TX_THREAD*)thread);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_thread_delete(osal_thread_t *thread)
{
    UINT result;

    result = tx_thread_terminate((TX_THREAD*)thread);
    result = tx_thread_delete((TX_THREAD*)thread);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)

#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS下的线程实现 */
osal_status_t osal_thread_create(osal_thread_t *thread, 
                                 const char *name,
                                 osal_thread_entry_t entry,
                                 void *argument,
                                 void *stack_pointer,
                                 unsigned int stack_size,
                                 osal_thread_priority_t priority)
{
    
    thread->task_handle = xTaskCreateStatic((TaskFunction_t)entry,
                               name,
                               stack_size / sizeof(StackType_t),
                               argument,
                               priority,
                               (StackType_t*)stack_pointer,
                               &thread->task_buffer);
    
    if (thread->task_handle != NULL) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_thread_start(osal_thread_t *thread)
{
    vTaskResume(thread->task_handle);
    return OSAL_SUCCESS;
}

osal_status_t osal_thread_stop(osal_thread_t *thread)
{
    vTaskSuspend(thread->task_handle);
    return OSAL_SUCCESS;
}

osal_status_t osal_thread_delete(osal_thread_t *thread)
{
    vTaskDelete((TaskHandle_t)thread);
    return OSAL_SUCCESS;
}
#endif

/* 通用延时函数 */
void osal_delay_ms(unsigned int ms)
{
#if (OSAL_RTOS_TYPE == OSAL_BARE_METAL)
    // 简单的循环延时
    for (volatile int i = 0; i < ms * 1000; i++){asm volatile("nop");}
#elif (OSAL_RTOS_TYPE == OSAL_THREADX)
    tx_thread_sleep(ms);
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
    vTaskDelay(pdMS_TO_TICKS(ms));
#endif
}

void osal_delay_us(unsigned int us)
{
    // 简单的循环延时,由于rtos无法精确到微秒级别，这里与裸机模式实现相同
    for (volatile int i = 0; i < us; i++){asm volatile("nop");}
}