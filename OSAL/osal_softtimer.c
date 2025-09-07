#include "osal_def.h"
#include <stdbool.h>
#include <stdint.h>

#if (OSAL_RTOS_TYPE == OSAL_THREADX)

/* ThreadX下的定时器实现 */
osal_status_t osal_timer_create(osal_timer_t *timer, 
                                const char *name,
                                osal_timer_callback_t callback,
                                void *argument,
                                unsigned int timeout_ms,
                                osal_timer_mode_t mode)
{
    UINT result;
    ULONG timer_ticks;
    
    if (timer == NULL || callback == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 将毫秒转换为ticks */
    timer_ticks = (timeout_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;
    if (timer_ticks == 0) {
        timer_ticks = 1; /* 至少1个tick */
    }
    
    /* 创建定时器 */
    result = tx_timer_create((TX_TIMER*)timer, 
                             (CHAR*)name,
                             (VOID(*)(ULONG))callback,
                             (ULONG)argument,
                             timer_ticks,
                             (mode == OSAL_TIMER_MODE_PERIODIC) ? timer_ticks : 0,
                             TX_NO_ACTIVATE);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_start(osal_timer_t *timer)
{
    UINT result;
    
    if (timer == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    result = tx_timer_activate((TX_TIMER*)timer);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_stop(osal_timer_t *timer)
{
    UINT result;
    
    if (timer == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    result = tx_timer_deactivate((TX_TIMER*)timer);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_change_period(osal_timer_t *timer, unsigned int timeout_ms)
{
    UINT result;
    ULONG timer_ticks;
    
    if (timer == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 将毫秒转换为ticks */
    timer_ticks = (timeout_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;
    if (timer_ticks == 0) {
        timer_ticks = 1; /* 至少1个tick */
    }
    
    /* 先停止定时器 */
    tx_timer_deactivate((TX_TIMER*)timer);
    
    /* 更改定时器周期 */
    result = tx_timer_change((TX_TIMER*)timer, timer_ticks, timer_ticks);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_delete(osal_timer_t *timer)
{
    UINT result;
    
    if (timer == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    result = tx_timer_delete((TX_TIMER*)timer);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

uint8_t osal_timer_is_active(osal_timer_t *timer)
{
    if (timer == NULL) {
        return 0;
    }
    
    /* 检查定时器是否激活 */
    if (((TX_TIMER*)timer)->tx_timer_internal.tx_timer_internal_list_head != TX_NULL) {
        return 1;
    }
    return 0;
}

#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)

/* FreeRTOS下的定时器实现 */
osal_status_t osal_timer_create(osal_timer_t *timer, 
                                const char *name,
                                osal_timer_callback_t callback,
                                void *argument,
                                unsigned int timeout_ms,
                                osal_timer_mode_t mode)
{
    TickType_t timer_ticks;
    
    if (timer == NULL || callback == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 将毫秒转换为ticks */
    timer_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timer_ticks == 0) {
        timer_ticks = 1; /* 至少1个tick */
    }
    
    /* 创建定时器 */
    timer->handle = xTimerCreateStatic(name,
                                       timer_ticks,
                                       (mode == OSAL_TIMER_MODE_PERIODIC) ? pdTRUE : pdFALSE,
                                       argument,  // FreeRTOS标准参数传递方式
                                       callback,  // 直接使用回调函数，无需类型转换
                                       &timer->buffer);
    
    if (timer->handle != NULL) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_start(osal_timer_t *timer)
{
    BaseType_t result;
    
    if (timer == NULL || timer->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 判断是否在中断中 */
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerStartFromISR(timer->handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerStart(timer->handle, 0); /* 不阻塞 */
    }
    
    if (result == pdPASS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_stop(osal_timer_t *timer)
{
    BaseType_t result;
    
    if (timer == NULL || timer->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 判断是否在中断中 */
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerStopFromISR(timer->handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerStop(timer->handle, 0); /* 不阻塞 */
    }
    
    if (result == pdPASS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_change_period(osal_timer_t *timer, unsigned int timeout_ms)
{
    BaseType_t result;
    TickType_t timer_ticks;
    
    if (timer == NULL || timer->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 将毫秒转换为ticks */
    timer_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timer_ticks == 0) {
        timer_ticks = 1; /* 至少1个tick */
    }
    
    /* 判断是否在中断中 */
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerChangePeriodFromISR(timer->handle, timer_ticks, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerChangePeriod(timer->handle, timer_ticks, 0); /* 不阻塞 */
    }
    
    if (result == pdPASS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_timer_delete(osal_timer_t *timer)
{
    BaseType_t result;
    
    if (timer == NULL || timer->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    result = xTimerDelete(timer->handle, 0); /* 不阻塞 */
    
    if (result == pdPASS) {
        timer->handle = NULL;
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

uint8_t osal_timer_is_active(osal_timer_t *timer)
{
    if (timer == NULL || timer->handle == NULL) {
        return false;
    }
    
    return (xTimerIsTimerActive(timer->handle) == pdTRUE) ? 1 : 0;
}

#endif