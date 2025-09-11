#include "bsp_spi.h"
#include "gpio.h"
#include "osal_def.h"
#include "string.h"
#include <stdio.h>

#define log_tag "BSP_SPI"
#include "log.h"

// SPI总线管理器数组
static SPI_Bus_Manager spi_buses[SPI_BUS_NUM];

// 内部函数声明
static SPI_Bus_Manager* BSP_SPI_Get_Bus_Manager(SPI_HandleTypeDef* hspi);
static void BSP_SPI_Select_Device(SPI_Device* dev);
static void BSP_SPI_Deselect_Device(SPI_Device* dev);

SPI_Device* BSP_SPI_Device_Init(SPI_Device_Init_Config* config)
{
    if (config == NULL || config->hspi == NULL) {
        LOG_ERROR("Invalid config or hspi is NULL");
        return NULL;
    }

    // 获取对应的总线管理器
    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(config->hspi);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for hspi=%p", (void*)config->hspi);
        return NULL;
    }

    // 检查设备数量是否已达上限
    if (bus_manager->device_count >= MAX_DEVICES_PER_BUS) {
        LOG_ERROR("Device count reached maximum limit: %d", MAX_DEVICES_PER_BUS);
        return NULL;
    }

    // 初始化总线管理器（如果尚未初始化）
    if (bus_manager->hspi == NULL) {
        bus_manager->hspi = config->hspi;
        bus_manager->device_count = 0;
        
        // 创建总线事件和互斥锁
        static char event_name[32];
        static char mutex_name[32];
        snprintf(event_name, sizeof(event_name), "spi_event_%p", (void*)config->hspi);
        snprintf(mutex_name, sizeof(mutex_name), "spi_mutex_%p", (void*)config->hspi);
        
        if (osal_event_create(&bus_manager->bus_event, event_name) != OSAL_SUCCESS) {
            LOG_ERROR("Failed to create bus event");
            return NULL;
        }
        
        if (osal_mutex_create(&bus_manager->bus_mutex, mutex_name) != OSAL_SUCCESS) {
            LOG_ERROR("Failed to create bus mutex");
            osal_event_delete(&bus_manager->bus_event);
            return NULL;
        }
    }

    // 添加新设备
    SPI_Device* dev = &bus_manager->devices[bus_manager->device_count];
    dev->hspi = config->hspi;
    dev->cs_port = config->cs_port;
    dev->cs_pin = config->cs_pin;
    dev->tx_mode = config->tx_mode;
    dev->rx_mode = config->rx_mode;
    
    // 初始化时拉高CS引脚
    BSP_SPI_Deselect_Device(dev);
    
    bus_manager->device_count++;

     LOG_INFO("SPI %p device initialized successfully. Device count: %d", (void *)dev->hspi,bus_manager->device_count);
    
    return dev;
}

void BSP_SPI_Device_DeInit(SPI_Device* dev)
{
    if (dev == NULL) {
        return;
    }

    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(dev->hspi);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hspi=%p", (void*)dev->hspi);
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
        osal_event_delete(&bus_manager->bus_event);
        osal_mutex_delete(&bus_manager->bus_mutex);
        memset(bus_manager, 0, sizeof(SPI_Bus_Manager));
    }
}


osal_status_t BSP_SPI_TransReceive(SPI_Device* dev, const uint8_t* tx_data, uint8_t* rx_data, uint16_t size)
{
    if (dev == NULL || tx_data == NULL || rx_data == NULL || size == 0) {
        LOG_ERROR("Invalid parameters: dev=%p, tx_data=%p, rx_data=%p, size=%d", 
                  (void*)dev, (void*)tx_data, (void*)rx_data, size);
        return OSAL_INVALID_PARAM;
    }

    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(dev->hspi);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hspi=%p", (void*)dev->hspi);
        return OSAL_ERROR;
    }

    // 等待获取总线使用权
    if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        LOG_ERROR("Failed to acquire bus mutex");
        return OSAL_ERROR;
    }

    // 设置当前活动设备
    bus_manager->active_dev = dev;

    // 选中设备
    BSP_SPI_Select_Device(dev);

    HAL_StatusTypeDef hal_status;
    osal_status_t osal_status = OSAL_SUCCESS;

    // 根据配置的模式执行传输
    switch (dev->tx_mode) {
        case SPI_MODE_BLOCKING:
            hal_status = HAL_SPI_TransmitReceive(dev->hspi, (uint8_t*)tx_data, rx_data, size, HAL_MAX_DELAY);
            if (hal_status != HAL_OK) {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case SPI_MODE_IT:
            hal_status = HAL_SPI_TransmitReceive_IT(dev->hspi, (uint8_t*)tx_data, rx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                            SPI_EVENT_TX_RX_DONE_EVENT | SPI_EVENT_ERR_EVENT,
                                                            OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                            OSAL_WAIT_FOREVER,
                                                            &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & SPI_EVENT_ERR_EVENT) {
                    osal_status = OSAL_ERROR;
                }
            } else {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case SPI_MODE_DMA:
            hal_status = HAL_SPI_TransmitReceive_DMA(dev->hspi, (uint8_t*)tx_data, rx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                            SPI_EVENT_TX_RX_DONE_EVENT | SPI_EVENT_ERR_EVENT,
                                                            OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                            OSAL_WAIT_FOREVER,
                                                            &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & SPI_EVENT_ERR_EVENT) {
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

    // 取消选中设备
    BSP_SPI_Deselect_Device(dev);

    // 释放总线使用权
    osal_mutex_unlock(&bus_manager->bus_mutex);

    return osal_status;
}

osal_status_t BSP_SPI_Transmit(SPI_Device* dev, const uint8_t* tx_data, uint16_t size)
{
    if (dev == NULL || tx_data == NULL || size == 0) {
        LOG_ERROR("Invalid parameters: dev=%p, tx_data=%p, size=%d", (void*)dev, (void*)tx_data, size);
        return OSAL_INVALID_PARAM;
    }

    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(dev->hspi);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hspi=%p", (void*)dev->hspi);
        return OSAL_ERROR;
    }

    // 等待获取总线使用权
    if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        LOG_ERROR("Failed to acquire bus mutex");
        return OSAL_ERROR;
    }

    // 设置当前活动设备
    bus_manager->active_dev = dev;

    // 选中设备
    BSP_SPI_Select_Device(dev);

    HAL_StatusTypeDef hal_status;
    osal_status_t osal_status = OSAL_SUCCESS;

    // 根据配置的模式执行传输
    switch (dev->tx_mode) {
        case SPI_MODE_BLOCKING:
            hal_status = HAL_SPI_Transmit(dev->hspi, (uint8_t*)tx_data, size, HAL_MAX_DELAY);
            if (hal_status != HAL_OK) {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case SPI_MODE_IT:
            hal_status = HAL_SPI_Transmit_IT(dev->hspi, (uint8_t*)tx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                            SPI_EVENT_TX_DONE_EVENT | SPI_EVENT_ERR_EVENT,
                                                            OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                            OSAL_WAIT_FOREVER,
                                                            &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & SPI_EVENT_ERR_EVENT) {
                    osal_status = OSAL_ERROR;
                }
            } else {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case SPI_MODE_DMA:
            hal_status = HAL_SPI_Transmit_DMA(dev->hspi, (uint8_t*)tx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                            SPI_EVENT_TX_DONE_EVENT | SPI_EVENT_ERR_EVENT,
                                                            OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                            OSAL_WAIT_FOREVER,
                                                            &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & SPI_EVENT_ERR_EVENT) {
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

    // 取消选中设备
    BSP_SPI_Deselect_Device(dev);

    // 释放总线使用权
    osal_mutex_unlock(&bus_manager->bus_mutex);

    return osal_status;
}


osal_status_t BSP_SPI_Receive(SPI_Device* dev, uint8_t* rx_data, uint16_t size)
{
    if (dev == NULL || rx_data == NULL || size == 0) {
        LOG_ERROR("Invalid parameters: dev=%p, rx_data=%p, size=%d", (void*)dev, (void*)rx_data, size);
        return OSAL_INVALID_PARAM;
    }

    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(dev->hspi);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hspi=%p", (void*)dev->hspi);
        return OSAL_ERROR;
    }

    // 等待获取总线使用权
    if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        LOG_ERROR("Failed to acquire bus mutex");
        return OSAL_ERROR;
    }

    // 设置当前活动设备
    bus_manager->active_dev = dev;

    // 选中设备
    BSP_SPI_Select_Device(dev);

    HAL_StatusTypeDef hal_status;
    osal_status_t osal_status = OSAL_SUCCESS;

    // 根据配置的模式执行传输
    switch (dev->rx_mode) {
        case SPI_MODE_BLOCKING:
            hal_status = HAL_SPI_Receive(dev->hspi, rx_data, size, HAL_MAX_DELAY);
            if (hal_status != HAL_OK) {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case SPI_MODE_IT:
            hal_status = HAL_SPI_Receive_IT(dev->hspi, rx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                            SPI_EVENT_RX_DONE_EVENT | SPI_EVENT_ERR_EVENT,
                                                            OSAL_EVENT_WAIT_FLAG_OR| OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                            OSAL_WAIT_FOREVER,
                                                            &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & SPI_EVENT_ERR_EVENT) {
                    osal_status = OSAL_ERROR;
                }
            } else {
                osal_status = OSAL_ERROR;
            }
            break;
            
        case SPI_MODE_DMA:
            hal_status = HAL_SPI_Receive_DMA(dev->hspi, rx_data, size);
            if (hal_status == HAL_OK) {
                // 等待传输完成或出错
                unsigned int actual_flags;
                osal_status_t wait_status = osal_event_wait(&bus_manager->bus_event, 
                                                            SPI_EVENT_RX_DONE_EVENT | SPI_EVENT_ERR_EVENT,
                                                            OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                            OSAL_WAIT_FOREVER,
                                                            &actual_flags);
                if (wait_status != OSAL_SUCCESS) {
                    osal_status = OSAL_ERROR;
                } else if (actual_flags & SPI_EVENT_ERR_EVENT) {
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

    // 取消选中设备
    BSP_SPI_Deselect_Device(dev);

    // 释放总线使用权
    osal_mutex_unlock(&bus_manager->bus_mutex);

    return osal_status;
}


osal_status_t BSP_SPI_TransAndTrans(SPI_Device* dev, const uint8_t* tx_data1, uint16_t size1, 
                                   const uint8_t* tx_data2, uint16_t size2)
{
    if (dev == NULL || tx_data1 == NULL || tx_data2 == NULL || (size1 == 0 && size2 == 0)) {
        LOG_ERROR("Invalid parameters: dev=%p, tx_data1=%p, tx_data2=%p, size1=%d, size2=%d", 
                  (void*)dev, (void*)tx_data1, (void*)tx_data2, size1, size2);
        return OSAL_INVALID_PARAM;
    }

    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(dev->hspi);
    if (bus_manager == NULL) {
        LOG_ERROR("Failed to get bus manager for device hspi=%p", (void*)dev->hspi);
        return OSAL_ERROR;
    }

    // 等待获取总线使用权
    if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        LOG_ERROR("Failed to acquire bus mutex");
        return OSAL_ERROR;
    }

    // 设置当前活动设备
    bus_manager->active_dev = dev;

    // 选中设备
    BSP_SPI_Select_Device(dev);

    HAL_StatusTypeDef hal_status1 = HAL_OK, hal_status2 = HAL_OK;

    // 发送第一次数据
    if (size1 > 0) {
        hal_status1 = HAL_SPI_Transmit(dev->hspi, (uint8_t*)tx_data1, size1, HAL_MAX_DELAY);
    }

    // 发送第二次数据
    if (size2 > 0 && hal_status1 == HAL_OK) {
        hal_status2 = HAL_SPI_Transmit(dev->hspi, (uint8_t*)tx_data2, size2, HAL_MAX_DELAY);
    }

    // 取消选中设备
    BSP_SPI_Deselect_Device(dev);

    // 释放总线使用权
    osal_mutex_unlock(&bus_manager->bus_mutex);

    // 检查结果
    if (hal_status1 != HAL_OK || hal_status2 != HAL_OK) {
        return OSAL_ERROR;
    }

    return OSAL_SUCCESS;
}

/**
 * @description: 获取SPI总线管理器
 * @param {SPI_HandleTypeDef*} hspi，SPI句柄
 * @return {SPI_Bus_Manager*} 总线管理器指针
 */
static SPI_Bus_Manager* BSP_SPI_Get_Bus_Manager(SPI_HandleTypeDef* hspi)
{
    if (hspi == NULL) {
        LOG_ERROR("NULL hspi parameter");
        return NULL;
    }

    for (int i = 0; i < SPI_BUS_NUM; i++) {
        if (spi_buses[i].hspi == hspi || spi_buses[i].hspi == NULL) {
            return &spi_buses[i];
        }
    }

    return NULL;
}

/**
 * @description: 选中SPI设备（拉低CS引脚）
 * @param {SPI_Device*} dev，要操作的SPI设备实例
 */
static void BSP_SPI_Select_Device(SPI_Device* dev)
{
    if (dev != NULL && dev->cs_port != NULL) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    }
}

/**
 * @description: 取消选中SPI设备（拉高CS引脚）
 * @param {SPI_Device*} dev，要操作的SPI设备实例
 */
static void BSP_SPI_Deselect_Device(SPI_Device* dev)
{
    if (dev != NULL && dev->cs_port != NULL) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
    }
}

/*  SPI回调函数  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(hspi);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, SPI_EVENT_TX_DONE_EVENT);
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(hspi);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, SPI_EVENT_RX_DONE_EVENT);
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(hspi);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, SPI_EVENT_TX_RX_DONE_EVENT);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager* bus_manager = BSP_SPI_Get_Bus_Manager(hspi);
    if (bus_manager != NULL) {
        osal_event_set(&bus_manager->bus_event, SPI_EVENT_ERR_EVENT);
    }
}