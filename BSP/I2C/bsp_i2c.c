/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-08 10:44:23
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 13:03:34
 * @FilePath: /rm_base/BSP/I2C/bsp_i2c.c
 * @Description: 
 */
#include "bsp_i2c.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdio.h>
#include <string.h>

#define log_tag "BSP_I2C"
#include "log.h"

static I2C_Bus_Manager i2c_buses[I2C_BUS_NUM] = {0};

// 内部函数声明
static I2C_Bus_Manager* BSP_I2C_Get_Bus_Manager(I2C_HandleTypeDef* hi2c);

I2C_Device* BSP_I2C_Device_Init(I2C_Device_Init_Config* config)
{
    if (config == NULL || config->hi2c == NULL) {
        LOG_ERROR("Invalid config or hi2c is NULL");
        return NULL;
    }

    // 获取对应的总线管理器
    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(config->hi2c);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for hi2c=%p", (void*)config->hi2c);
        return NULL;
    }

    // 检查设备数量是否已达上限
    if (bus_manager->device_count >= MAX_DEVICES_PER_I2C_BUS) {
        LOG_ERROR("Device count reached maximum limit: %d", MAX_DEVICES_PER_I2C_BUS);
        return NULL;
    }

    // 初始化总线管理器（如果尚未初始化）
    if (bus_manager->hi2c == NULL) {
        bus_manager->hi2c = config->hi2c;
        bus_manager->device_count = 0;
        
        // 创建总线互斥锁和事件
        static char mutex_name[32];
        static char event_name[32];
        snprintf(mutex_name, sizeof(mutex_name), "i2c_mutex_%p", (void*)config->hi2c);
        snprintf(event_name, sizeof(event_name), "i2c_event_%p", (void*)config->hi2c);
        
        if (osal_mutex_create(&bus_manager->bus_mutex, mutex_name) != OSAL_SUCCESS) {
            LOG_ERROR("Failed to create bus mutex");
            return NULL;
        }
        
        if (osal_event_create(&bus_manager->bus_event, event_name) != OSAL_SUCCESS) {
            LOG_ERROR("Failed to create bus event");
            osal_mutex_delete(&bus_manager->bus_mutex);
            return NULL;
        }
    }

    // 添加新设备
    I2C_Device* dev = &bus_manager->devices[bus_manager->device_count];
    dev->hi2c = config->hi2c;
    dev->dev_address = config->dev_address;
    dev->tx_mode = config->tx_mode;
    dev->rx_mode = config->rx_mode;
    
    bus_manager->device_count++;
    LOG_INFO("I2C %p device initialized successfully. Device count: %d", (void*)dev->hi2c,bus_manager->device_count);
    return dev;
}

void BSP_I2C_Device_DeInit(I2C_Device* dev)
{
    if (dev == NULL) {
        return;
    }

    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(dev->hi2c);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hi2c=%p", (void*)dev->hi2c);
        return;
    }

    // 查找设备在数组中的位置
    uint8_t i;
    for (i = 0; i < bus_manager->device_count; i++) {
        if (&bus_manager->devices[i] == dev) {
            break;
        }
    }

    // 如果找到了设备，则从数组中移除
    if (i < bus_manager->device_count) {
        // 将后面的设备向前移动
        for (; i < bus_manager->device_count - 1; i++) {
            bus_manager->devices[i] = bus_manager->devices[i + 1];
        }
        bus_manager->device_count--;
    }

    // 如果没有设备了，清理总线管理器资源
    if (bus_manager->device_count == 0) {
        osal_mutex_delete(&bus_manager->bus_mutex);
        osal_event_delete(&bus_manager->bus_event);
        memset(bus_manager, 0, sizeof(I2C_Bus_Manager));
    }
}

osal_status_t BSP_I2C_Transmit(I2C_Device* dev, uint8_t* tx_data, uint16_t size)
{
    if (dev == NULL || tx_data == NULL || size == 0) {
        LOG_ERROR("Invalid parameters: dev=%p, tx_data=%p, size=%d", (void*)dev, (void*)tx_data, size);
        return OSAL_INVALID_PARAM;
    }

    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(dev->hi2c);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hi2c=%p", (void*)dev->hi2c);
        return OSAL_ERROR;
    }

    // 获取总线互斥锁
    if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        LOG_ERROR("Failed to acquire bus mutex");
        return OSAL_ERROR;
    }

    // 设置当前活动设备
    bus_manager->active_dev = dev;

    osal_status_t osal_status = OSAL_SUCCESS;
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    // 根据配置的模式执行传输
    switch (dev->tx_mode) {
        case I2C_MODE_BLOCKING:
            hal_status = HAL_I2C_Master_Transmit(dev->hi2c, dev->dev_address, tx_data, size, HAL_MAX_DELAY);
            if (hal_status != HAL_OK) {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case I2C_MODE_IT:
            hal_status = HAL_I2C_Master_Transmit_IT(dev->hi2c, dev->dev_address, tx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                           I2C_EVENT_TX_COMPLETE | I2C_EVENT_ERROR,
                                                           OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                           OSAL_WAIT_FOREVER,
                                                           &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & I2C_EVENT_ERROR) {
                    osal_status = OSAL_ERROR;
                }
            } else {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case I2C_MODE_DMA:
            hal_status = HAL_I2C_Master_Transmit_DMA(dev->hi2c, dev->dev_address, tx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                           I2C_EVENT_TX_COMPLETE | I2C_EVENT_ERROR,
                                                           OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                           OSAL_WAIT_FOREVER,
                                                           &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & I2C_EVENT_ERROR) {
                    osal_status = OSAL_ERROR;
                }
            } else {
                osal_status = OSAL_ERROR;
            }
            break;
            
        default:
            LOG_ERROR("Invalid transmit mode: %d", dev->tx_mode);
            osal_status = OSAL_ERROR;
            break;
    }

    // 释放总线互斥锁
    osal_mutex_unlock(&bus_manager->bus_mutex);

    return osal_status;
}

osal_status_t BSP_I2C_Receive(I2C_Device* dev, uint8_t* rx_data, uint16_t size)
{
    if (dev == NULL || rx_data == NULL || size == 0) {
        LOG_ERROR("Invalid parameters: dev=%p, rx_data=%p, size=%d", (void*)dev, (void*)rx_data, size);
        return OSAL_INVALID_PARAM;
    }

    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(dev->hi2c);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hi2c=%p", (void*)dev->hi2c);
        return OSAL_ERROR;
    }

    // 获取总线互斥锁
    if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        LOG_ERROR("Failed to acquire bus mutex");
        return OSAL_ERROR;
    }

    // 设置当前活动设备
    bus_manager->active_dev = dev;

    osal_status_t osal_status = OSAL_SUCCESS;
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    // 根据配置的模式执行传输
    switch (dev->rx_mode) {
        case I2C_MODE_BLOCKING:
            hal_status = HAL_I2C_Master_Receive(dev->hi2c, dev->dev_address, rx_data, size, HAL_MAX_DELAY);
            if (hal_status != HAL_OK) {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case I2C_MODE_IT:            
            hal_status = HAL_I2C_Master_Receive_IT(dev->hi2c, dev->dev_address, rx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                           I2C_EVENT_RX_COMPLETE | I2C_EVENT_ERROR,
                                                           OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                           OSAL_WAIT_FOREVER,
                                                           &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & I2C_EVENT_ERROR) {
                    osal_status = OSAL_ERROR;
                }
            } else {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case I2C_MODE_DMA:
            hal_status = HAL_I2C_Master_Receive_DMA(dev->hi2c, dev->dev_address, rx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                           I2C_EVENT_RX_COMPLETE | I2C_EVENT_ERROR,
                                                           OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                           OSAL_WAIT_FOREVER,
                                                           &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & I2C_EVENT_ERROR) {
                    osal_status = OSAL_ERROR;
                }
            } else {
                osal_status = OSAL_ERROR;
            }
            break;
            
        default:
            LOG_ERROR("Invalid receive mode: %d", dev->rx_mode);
            osal_status = OSAL_ERROR;
            break;
    }

    // 释放总线互斥锁
    osal_mutex_unlock(&bus_manager->bus_mutex);

    return osal_status;
}

osal_status_t BSP_I2C_Mem_Write_Read(I2C_Device* dev, uint16_t mem_address, 
                                    uint16_t mem_add_size, uint8_t* data, 
                                    uint16_t size, uint8_t is_write)
{
    if (dev == NULL || data == NULL || size == 0) {
        LOG_ERROR("Invalid parameters: dev=%p, data=%p, size=%d", (void*)dev, (void*)data, size);
        return OSAL_INVALID_PARAM;
    }

    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(dev->hi2c);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hi2c=%p", (void*)dev->hi2c);
        return OSAL_ERROR;
    }

    // 获取总线互斥锁
    if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        LOG_ERROR("Failed to acquire bus mutex");
        return OSAL_ERROR;
    }

    // 设置当前活动设备
    bus_manager->active_dev = dev;

    osal_status_t osal_status = OSAL_SUCCESS;
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if (is_write) {
        // 写操作
        switch (dev->tx_mode) {
            case I2C_MODE_BLOCKING:
                hal_status = HAL_I2C_Mem_Write(dev->hi2c, dev->dev_address, mem_address, 
                                              mem_add_size, data, size, HAL_MAX_DELAY);
                if (hal_status != HAL_OK) {
                    osal_status = OSAL_ERROR;
                }
                break;
                
            case I2C_MODE_IT:               
                hal_status = HAL_I2C_Mem_Write_IT(dev->hi2c, dev->dev_address, mem_address, 
                                                 mem_add_size, data, size);
                if (hal_status == HAL_OK) {
                    // 等待传输完成或出错
                    unsigned int actual_flags;
                    osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                               I2C_EVENT_TX_COMPLETE | I2C_EVENT_ERROR,
                                                               OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                               OSAL_WAIT_FOREVER,
                                                               &actual_flags);
                    if (wait_status != OSAL_SUCCESS) {
                        osal_status = OSAL_ERROR;
                    } else if (actual_flags & I2C_EVENT_ERROR) {
                        osal_status = OSAL_ERROR;
                    }
                } else {
                    osal_status = OSAL_ERROR;
                }
                break;
                
            case I2C_MODE_DMA:
                hal_status = HAL_I2C_Mem_Write_DMA(dev->hi2c, dev->dev_address, mem_address, 
                                                  mem_add_size, data, size);
                if (hal_status == HAL_OK) {
                    // 等待传输完成或出错
                    unsigned int actual_flags;
                    osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                               I2C_EVENT_TX_COMPLETE | I2C_EVENT_ERROR,
                                                               OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                               OSAL_WAIT_FOREVER,
                                                               &actual_flags);
                    if (wait_status != OSAL_SUCCESS) {
                        osal_status = OSAL_ERROR;
                    } else if (actual_flags & I2C_EVENT_ERROR) {
                        osal_status = OSAL_ERROR;
                    }
                } else {
                    osal_status = OSAL_ERROR;
                }
                break;
                
            default:
                osal_status = OSAL_ERROR;
                LOG_ERROR("Invalid transmit mode for memory write: %d", dev->tx_mode);
                break;
        }
    } else {
        // 读操作
        switch (dev->rx_mode) {
            case I2C_MODE_BLOCKING:
                hal_status = HAL_I2C_Mem_Read(dev->hi2c, dev->dev_address, mem_address, 
                                             mem_add_size, data, size, HAL_MAX_DELAY);
                if (hal_status != HAL_OK) {
                    osal_status = OSAL_ERROR;
                }
                break;
                
            case I2C_MODE_IT:
                hal_status = HAL_I2C_Mem_Read_IT(dev->hi2c, dev->dev_address, mem_address, 
                                                mem_add_size, data, size);
                if (hal_status == HAL_OK) {
                    // 等待传输完成或出错
                    unsigned int actual_flags;
                    osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                               I2C_EVENT_RX_COMPLETE | I2C_EVENT_ERROR,
                                                               OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                               OSAL_WAIT_FOREVER,
                                                               &actual_flags);
                    if (wait_status != OSAL_SUCCESS) {
                        osal_status = OSAL_ERROR;
                    } else if (actual_flags & I2C_EVENT_ERROR) {
                        osal_status = OSAL_ERROR;
                    }
                } else {
                    osal_status = OSAL_ERROR;
                }
                break;
                
            case I2C_MODE_DMA:
                hal_status = HAL_I2C_Mem_Read_DMA(dev->hi2c, dev->dev_address, mem_address, 
                                                 mem_add_size, data, size);
                if (hal_status == HAL_OK) {
                    // 等待传输完成或出错
                    unsigned int actual_flags;
                    osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                               I2C_EVENT_RX_COMPLETE | I2C_EVENT_ERROR,
                                                               OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                               OSAL_WAIT_FOREVER,
                                                               &actual_flags);
                    if (wait_status != OSAL_SUCCESS) {
                        osal_status = OSAL_ERROR;
                    } else if (actual_flags & I2C_EVENT_ERROR) {
                        osal_status = OSAL_ERROR;
                    }
                } else {
                    osal_status = OSAL_ERROR;
                }
                break;
                
            default:
                osal_status = OSAL_ERROR;
                LOG_ERROR("Invalid receive mode for memory read: %d", dev->rx_mode);
                break;
        }
    }

    // 释放总线互斥锁
    osal_mutex_unlock(&bus_manager->bus_mutex);

    return osal_status;
}

/**
 * @description: 获取I2C总线管理器
 * @param {I2C_HandleTypeDef*} hi2c，I2C句柄
 * @return {I2C_Bus_Manager*} 总线管理器指针
 */
static I2C_Bus_Manager* BSP_I2C_Get_Bus_Manager(I2C_HandleTypeDef* hi2c)
{
    if (hi2c == NULL) {
        LOG_ERROR("NULL hi2c parameter");
        return NULL;
    }

    for (int i = 0; i < I2C_BUS_NUM; i++) {
        if (i2c_buses[i].hi2c == hi2c || i2c_buses[i].hi2c == NULL) {
            return &i2c_buses[i];
        }
    }
    return NULL;
}

/* 中断回调函数 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(hi2c);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, I2C_EVENT_TX_COMPLETE);
    }
}


void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(hi2c);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, I2C_EVENT_RX_COMPLETE);
    }
}


void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(hi2c);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, I2C_EVENT_TX_COMPLETE);
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(hi2c);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, I2C_EVENT_RX_COMPLETE);
    }
}


void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager* bus_manager = BSP_I2C_Get_Bus_Manager(hi2c);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, I2C_EVENT_ERROR);
    }
}