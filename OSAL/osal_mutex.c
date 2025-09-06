#include "osal_def.h"

#if (OSAL_RTOS_TYPE == OSAL_THREADX)

osal_status_t osal_mutex_create(osal_mutex_t *mutex, const char *name)
{
    UINT result;
    
    result = tx_mutex_create((TX_MUTEX*)mutex, (CHAR*)name, TX_NO_INHERIT);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_mutex_lock(osal_mutex_t *mutex, osal_tick_t timeout)
{
    if (mutex == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    UINT result;

    if (timeout == OSAL_WAIT_FOREVER) {
        result = tx_mutex_get((TX_MUTEX*)mutex, TX_WAIT_FOREVER);
    } else {
        result = tx_mutex_get((TX_MUTEX*)mutex, timeout);
    }
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else if (result == TX_NOT_AVAILABLE) {
        return OSAL_TIMEOUT;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_mutex_unlock(osal_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    UINT result;
    
    result = tx_mutex_put((TX_MUTEX*)mutex);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_mutex_delete(osal_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    UINT result;
    
    result = tx_mutex_delete((TX_MUTEX*)mutex);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)

osal_status_t osal_mutex_create(osal_mutex_t *mutex, const char *name)
{
    // 使用静态内存分配方式创建互斥量
    mutex->mutex_handle = xSemaphoreCreateMutexStatic(&mutex->mutex_buffer);
    
    if (mutex->mutex_handle != NULL) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_mutex_lock(osal_mutex_t *mutex, osal_tick_t timeout)
{
    // 检查参数有效性
    if (!mutex) {
        return OSAL_INVALID_PARAM;
    }

    if (mutex->mutex_handle != NULL)
    { 
        BaseType_t result;
        
        if (timeout == OSAL_WAIT_FOREVER) {
            result = xSemaphoreTake(mutex->mutex_handle, portMAX_DELAY);
        } else {
            result = xSemaphoreTake(mutex->mutex_handle, timeout);
        }
        
        if (result == pdTRUE) {
            return OSAL_SUCCESS;
        } else {
            return OSAL_TIMEOUT;
        }
    }
    else
    {
        return OSAL_ERROR;
    }
}

osal_status_t osal_mutex_unlock(osal_mutex_t *mutex)
{
    // 检查参数有效性
    if (!mutex) {
        return OSAL_INVALID_PARAM;
    }
    
    if (mutex->mutex_handle != NULL)
    {
        BaseType_t result;
        
        result = xSemaphoreGive(mutex->mutex_handle);
        
        if (result == pdTRUE) {
            return OSAL_SUCCESS;
        } else {
            return OSAL_ERROR;
        }
    }
    else
    {
        return OSAL_ERROR;
    }
}

osal_status_t osal_mutex_delete(osal_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    vSemaphoreDelete(mutex->mutex_handle);
    mutex->mutex_handle = NULL;
    return OSAL_SUCCESS;
}

#endif