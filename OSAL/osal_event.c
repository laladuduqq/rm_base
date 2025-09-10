/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-06 09:56:03
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-10 13:22:56
 * @FilePath: /rm_base/OSAL/osal_event.c
 * @Description: OSAL事件管理接口实现
 */

#include "osal_def.h"

#if (OSAL_RTOS_TYPE == OSAL_THREADX)

/* ThreadX下的事件实现 */
osal_status_t osal_event_create(osal_event_t *event, const char *name)
{
    UINT result;
    
    result = tx_event_flags_create((TX_EVENT_FLAGS_GROUP*)event, (CHAR*)name);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_event_set(osal_event_t *event, unsigned int flags)
{
    UINT result;
    
    result = tx_event_flags_set((TX_EVENT_FLAGS_GROUP*)event, (ULONG)flags, TX_OR);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_event_wait(osal_event_t *event, unsigned int requested_flags, 
                              unsigned int options, osal_tick_t timeout, unsigned int *actual_flags)
{
    UINT result;
    ULONG actual_flags_local;
    UINT get_option = TX_AND_CLEAR;  /* 默认使用AND逻辑并清除 */
    
    if (event == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 根据options参数设置获取选项 */
    if (options & OSAL_EVENT_WAIT_FLAG_OR) {
        get_option = TX_OR_CLEAR;  /* OR逻辑并清除 */
    }
    
    /* 如果明确指定不清除，则使用不带CLEAR的选项 */
    if (!(options & OSAL_EVENT_WAIT_FLAG_CLEAR)) {
        if (get_option == TX_AND_CLEAR) {
            get_option = TX_AND;
        } else {
            get_option = TX_OR;
        }
    }
    
    result = tx_event_flags_get((TX_EVENT_FLAGS_GROUP*)event, (ULONG)requested_flags,
                                get_option, &actual_flags_local, timeout);
    
    if (actual_flags != NULL) {
        *actual_flags = (unsigned int)actual_flags_local;
    }
    
    if (result == TX_SUCCESS) {
        // 当TX_SUCCESS时，表示已成功获取到满足条件的事件标志
        return OSAL_SUCCESS;
    } else if (result == TX_NO_EVENTS) {
        return OSAL_TIMEOUT;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_event_clear(osal_event_t *event, unsigned int flags)
{
    /* 由于threadx不支持单独clear操作，如果需要要在wait中清除标志 */
    if (event == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    /* 直接返回成功，不执行实际清除操作 */
    return OSAL_SUCCESS;
}

osal_status_t osal_event_delete(osal_event_t *event)
{
    UINT result;
    
    if (event == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    result = tx_event_flags_delete((TX_EVENT_FLAGS_GROUP*)event);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)

/* FreeRTOS下的事件实现 */
osal_status_t osal_event_create(osal_event_t *event, const char *name)
{
    // name参数在FreeRTOS中未使用
    (void)name;
    
    if (event == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    // 使用静态分配方式创建事件组
    event->handle = xEventGroupCreateStatic(&event->buffer);
    
    if (event->handle != NULL) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_event_set(osal_event_t *event, unsigned int flags)
{
    BaseType_t result;
    
    if (event == NULL || event->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    // 在中断中使用特殊API
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xEventGroupSetBitsFromISR(event->handle, (EventBits_t)flags, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        xEventGroupSetBits(event->handle, (EventBits_t)flags);
        result = pdTRUE;
    }
    
    if (result == pdTRUE) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_event_wait(osal_event_t *event, unsigned int requested_flags, 
                              unsigned int options, osal_tick_t timeout, unsigned int *actual_flags)
{
    EventBits_t bits;
    BaseType_t clear_on_exit = pdTRUE;
    BaseType_t wait_for_all_bits = pdTRUE;
    
    if (event == NULL || event->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    // 根据options参数设置获取选项
    // FreeRTOS默认使用AND逻辑(pdTRUE)，如果需要OR逻辑则设置为pdFALSE
    if (options & 0x01) {  // 假设最低位为OR标志
        wait_for_all_bits = pdFALSE;
    }
    
    // 如果需要不清除标志，则设置为pdFALSE
    if (options & 0x02) {  // 假设第二位为不清除标志
        clear_on_exit = pdFALSE;
    }
    
    if (timeout == OSAL_WAIT_FOREVER) {
        bits = xEventGroupWaitBits(event->handle, (EventBits_t)requested_flags, 
                                   clear_on_exit, wait_for_all_bits, portMAX_DELAY);
    } else {
        bits = xEventGroupWaitBits(event->handle, (EventBits_t)requested_flags,
                                   clear_on_exit, wait_for_all_bits, timeout);
    }
    
    if (actual_flags != NULL) {
        *actual_flags = (unsigned int)bits;
    }
    
    // 检查是否获取到了所需的事件标志
    if ((wait_for_all_bits == pdTRUE && (bits & requested_flags) == requested_flags) ||
        (wait_for_all_bits == pdFALSE && (bits & requested_flags) != 0)) {
        return OSAL_SUCCESS;
    } else if (timeout == 0 && bits == 0) {
        return OSAL_TIMEOUT;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_event_clear(osal_event_t *event, unsigned int flags)
{
    EventBits_t bits;
    
    if (event == NULL || event->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    bits = xEventGroupClearBits(event->handle, (EventBits_t)flags);
    
    // 检查是否成功清除
    if ((bits & flags) == 0) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_event_delete(osal_event_t *event)
{
    if (event == NULL || event->handle == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    // 静态创建的事件组不能被删除，只需将句柄置为NULL
    event->handle = NULL;
    
    return OSAL_SUCCESS;
}

#endif