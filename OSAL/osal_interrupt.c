/*
 * @Author: your name
 * @Date: 2025-09-07 12:40:41
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-07 13:20:49
 * @FilePath: /rm_base/OSAL/osal_interrupt.c
 * @Description: OSAL中断管理接口实现
 */

#include "osal_def.h"

#if (OSAL_RTOS_TYPE == OSAL_THREADX)

/* ThreadX下的中断临界区实现 */
osal_status_t osal_enter_critical(osal_critical_state_t *crit)
{
    if (crit == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 使用ThreadX宏方法保存中断状态并禁用中断 */
    TX_INTERRUPT_SAVE_AREA
    TX_DISABLE
    *crit = interrupt_save;
    return OSAL_SUCCESS;
}

osal_status_t osal_exit_critical(osal_critical_state_t *crit)
{
    if (crit == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 使用ThreadX宏方法恢复中断状态 */
    TX_INTERRUPT_SAVE_AREA
    interrupt_save = *crit;
    TX_RESTORE
    return OSAL_SUCCESS;
}

#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)

/* FreeRTOS下的中断临界区实现 */
osal_status_t osal_enter_critical(osal_critical_state_t *crit)
{
    if (crit == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 判断是否在中断环境中 */
    if (xPortIsInsideInterrupt()) {
        /* 在中断中使用中断安全的API */
        *crit = taskENTER_CRITICAL_FROM_ISR();
    } else {
        /* 在任务中使用普通API */
        taskENTER_CRITICAL();
        *crit = 0; /* 在任务环境中不需要保存状态 */
    }
    return OSAL_SUCCESS;
}

osal_status_t osal_exit_critical(osal_critical_state_t *crit)
{
    if (crit == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 判断是否在中断环境中 */
    if (xPortIsInsideInterrupt()) {
        /* 在中断中使用中断安全的API */
        taskEXIT_CRITICAL_FROM_ISR(*crit);
    } else {
        /* 在任务中使用普通API */
        taskEXIT_CRITICAL();
    }
    return OSAL_SUCCESS;
}

#endif