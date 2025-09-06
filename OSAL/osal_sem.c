#include "osal_def.h"

#if (OSAL_RTOS_TYPE == OSAL_THREADX)

osal_status_t osal_sem_create(osal_sem_t *sem, const char *name, unsigned int initial_count)
{
    UINT result;
    
    result = tx_semaphore_create((TX_SEMAPHORE*)sem, (CHAR*)name, initial_count);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_sem_wait(osal_sem_t *sem, osal_tick_t timeout)
{
    if (sem == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    UINT result;

    if (timeout == OSAL_WAIT_FOREVER)
    {
        result = tx_semaphore_get((TX_SEMAPHORE*)sem, TX_WAIT_FOREVER);
    }
    else
    {
        result = tx_semaphore_get((TX_SEMAPHORE*)sem, timeout);
    }
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else if (result == TX_NO_INSTANCE) {
        return OSAL_TIMEOUT;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_sem_post(osal_sem_t *sem)
{
    if (sem == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    UINT result;
    
    result = tx_semaphore_put((TX_SEMAPHORE*)sem);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_sem_delete(osal_sem_t *sem)
{
    if (sem == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    UINT result;
    
    result = tx_semaphore_delete((TX_SEMAPHORE*)sem);
    
    if (result == TX_SUCCESS) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
#include <limits.h>
osal_status_t osal_sem_create(osal_sem_t *sem, const char *name, unsigned int initial_count)
{
    // 使用静态内存分配方式创建计数信号量
    sem->sem_handle = xSemaphoreCreateCountingStatic(UINT_MAX, initial_count, &sem->sem_buffer);
    
    if (sem->sem_handle != NULL) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_ERROR;
    }
}

osal_status_t osal_sem_wait(osal_sem_t *sem, osal_tick_t timeout)
{
    // 检查参数有效性
    if (!sem) {
        return OSAL_INVALID_PARAM;
    }

    if (sem->sem_handle != NULL)
    { 
        BaseType_t result;
    
        // 判断是否在中断环境中
        if (xPortIsInsideInterrupt()) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            // 在中断中只支持立即获取
            if (timeout == OSAL_NO_WAIT) {
                result = xSemaphoreTakeFromISR(sem->sem_handle, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            } else {
                // 中断中不支持阻塞操作，返回错误
                return OSAL_ERROR;
            }
        } else {
            if (timeout == OSAL_WAIT_FOREVER) {
                result = xSemaphoreTake(sem->sem_handle, portMAX_DELAY);
            } else {
                result = xSemaphoreTake(sem->sem_handle, timeout);
            }
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

osal_status_t osal_sem_post(osal_sem_t *sem)
{
    // 检查参数有效性
    if (!sem) {
        return OSAL_INVALID_PARAM;
    }
    if (sem->sem_handle != NULL)
    {
        BaseType_t result;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        // 判断是否在中断环境中
        if (xPortIsInsideInterrupt()) {
            result = xSemaphoreGiveFromISR(sem->sem_handle, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        } else {
            result = xSemaphoreGive(sem->sem_handle);
        }
        
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

osal_status_t osal_sem_delete(osal_sem_t *sem)
{
    if (sem == NULL)
    {
        return OSAL_INVALID_PARAM;
    }
    vSemaphoreDelete(sem->sem_handle);
    sem->sem_handle = NULL;
    return OSAL_SUCCESS;
}

#endif