/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-09 13:37:54
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-09 16:56:03
 * @FilePath: /rm_base/BSP/ADC/bsp_adc.c
 * @Description: 
 */
#include "bsp_adc.h"
#include "adc.h"
#include "osal_def.h"
#include "stm32f4xx_hal_adc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ADC设备实例数组
static ADC_Device adc_devices[ADC_DEVICE_NUM] = {0};
static uint8_t adc_device_used[ADC_DEVICE_NUM] = {0};
static uint8_t adc_initialized = 0;

// 参考电压校准值 (默认为3.3V)
static float adc_reference_voltage = 3.3f;
// 内部参考电压倒数比例因子
static float voltage_vrefint_proportion = 1.0f;

ADC_Device* BSP_ADC_Device_Init(ADC_Device_Init_Config* config)
{
    if (config == NULL || config->hadc == NULL || config->channel_configs == NULL || 
        config->channel_count == 0 || config->values == NULL) {
        return NULL;
    }
    
    // 检查ADC句柄是否已经被使用
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (adc_device_used[i] && adc_devices[i].hadc == config->hadc) {
            // 同一个ADC句柄已被使用，返回错误
            return NULL;
        }
    }
    
    // 查找空闲设备槽位
    int free_index = -1;
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (!adc_device_used[i]) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) {
        return NULL;
    }
    
    // 获取设备实例
    ADC_Device* device = &adc_devices[free_index];
    memset(device, 0, sizeof(ADC_Device));
    
    // 配置设备参数
    device->hadc = config->hadc;
    device->channel_configs = config->channel_configs;
    device->channel_count = config->channel_count;
    device->mode = config->mode;
    // 缓冲区配置
    if (config->values != NULL) {
        device->values = (uint32_t (*)[2])config->values;
    } 
    else{
        return NULL;
    }
    
    // 创建互斥锁
    char mutex_name[20];
    snprintf(mutex_name, sizeof(mutex_name), "ADC_Mutex_%d", free_index);
    osal_mutex_create(&device->adc_mutex, mutex_name);
    
    // 创建事件
    char event_name[20];
    snprintf(event_name, sizeof(event_name), "ADC_Event_%d", free_index);
    osal_event_create(&device->adc_event, event_name);
    
    // 标记设备已使用
    adc_device_used[free_index] = 1;
    
    return device;
}

void BSP_ADC_Device_DeInit(ADC_Device* dev)
{
    if (dev == NULL) {
        return;
    }
    
    // 查找设备索引
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (&adc_devices[i] == dev) {
            // 删除互斥锁和事件
            osal_mutex_delete(&dev->adc_mutex);
            osal_event_delete(&dev->adc_event);
            
            // 清空设备结构体
            memset(dev, 0, sizeof(ADC_Device));
            
            // 标记设备未使用
            adc_device_used[i] = 0;
            break;
        }
    }
}

osal_status_t BSP_ADC_Start(ADC_Device* dev)
{
    if (dev == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef hal_status = HAL_ERROR;
    
    // 配置所有通道
    for (int i = 0; i < dev->channel_count; i++) {
        if (HAL_ADC_ConfigChannel(dev->hadc, &dev->channel_configs[i]) != HAL_OK) {
            return OSAL_ERROR;
        }
    }
    
    // 根据模式启动ADC
    switch (dev->mode) {
        case ADC_MODE_BLOCKING:
            hal_status = HAL_ADC_Start(dev->hadc);
            break;
            
        case ADC_MODE_IT:
            hal_status = HAL_ADC_Start_IT(dev->hadc);
            break;
            
        case ADC_MODE_DMA:
            // 使用非活动缓冲区进行DMA传输
            hal_status = HAL_ADC_Start_DMA(dev->hadc, 
                                          (uint32_t*)&dev->values[!dev->active_buf][0], 
                                          dev->channel_count);
            break;
            
        default:
            return OSAL_INVALID_PARAM;
    }
    
    return (hal_status == HAL_OK) ? OSAL_SUCCESS : OSAL_ERROR;
}

osal_status_t BSP_ADC_Stop(ADC_Device* dev)
{
    if (dev == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef hal_status = HAL_ERROR;
    
    // 根据模式停止ADC
    switch (dev->mode) {
        case ADC_MODE_BLOCKING:
            hal_status = HAL_ADC_Stop(dev->hadc);
            break;
            
        case ADC_MODE_IT:
            hal_status = HAL_ADC_Stop_IT(dev->hadc);
            break;
            
        case ADC_MODE_DMA:
            hal_status = HAL_ADC_Stop_DMA(dev->hadc);
            break;
            
        default:
            return OSAL_INVALID_PARAM;
    }
    
    return (hal_status == HAL_OK) ? OSAL_SUCCESS : OSAL_ERROR;
}

osal_status_t BSP_ADC_Get_Values(ADC_Device* dev)
{
    if (dev == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    // 获取互斥锁
    if (tx_mutex_get(&dev->adc_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return OSAL_ERROR;
    }
    
    HAL_StatusTypeDef hal_status = HAL_ERROR;
    
    // 根据模式获取ADC值
    switch (dev->mode) {
        case ADC_MODE_BLOCKING:
            // 轮询模式，依次获取每个通道的转换值并存储到活动缓冲区
            hal_status = HAL_OK;
            for (int i = 0; i < dev->channel_count; i++) {
                if (HAL_ADC_PollForConversion(dev->hadc, HAL_MAX_DELAY) == HAL_OK) {
                    dev->values[dev->active_buf][i] = HAL_ADC_GetValue(dev->hadc);
                } else {
                    hal_status = HAL_ERROR;
                    break;
                }
            }
            break;
            
        case ADC_MODE_IT:
        case ADC_MODE_DMA:
            // 中断/DMA模式，等待事件完成
            {
                unsigned int actual_flags;
                osal_status_t status = osal_event_wait(&dev->adc_event, ADC_EVENT_DONE,
                                                      OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                      TX_WAIT_FOREVER, &actual_flags);
                if (status == OSAL_SUCCESS && (actual_flags & ADC_EVENT_DONE)) {
                    // 对于IT模式，需要手动读取数据到非活动缓冲区
                    if (dev->mode == ADC_MODE_IT) {
                        for (int i = 0; i < dev->channel_count; i++) {
                            if (HAL_ADC_PollForConversion(dev->hadc, 10) == HAL_OK) {
                                dev->values[!dev->active_buf][i] = HAL_ADC_GetValue(dev->hadc);
                            }
                        }
                    }
                    // DMA模式下，数据已由DMA直接写入非活动缓冲区，无需额外处理
                    
                    // 切换活动缓冲区
                    dev->active_buf = !dev->active_buf;
                    hal_status = HAL_OK;
                }
            }
            break;
            
        default:
            tx_mutex_put(&dev->adc_mutex);
            return OSAL_INVALID_PARAM;
    }
    
    // 释放互斥锁
    tx_mutex_put(&dev->adc_mutex);
    
    return (hal_status == HAL_OK) ? OSAL_SUCCESS : OSAL_ERROR;
}


uint32_t BSP_ADC_Get_Channel_Value(ADC_Device* dev, uint8_t channel_index)
{
    if (dev == NULL || channel_index >= dev->channel_count) {
        return 0;
    }

    // 对于阻塞模式，从活动缓冲区获取数据
    if (dev->mode == ADC_MODE_BLOCKING) {
        return dev->values[dev->active_buf][channel_index];
    }
        
    // 对于IT和DMA模式，从非活动缓冲区获取数据
    return dev->values[!dev->active_buf][channel_index];

}


void BSP_ADC_Init(void)
{
    // 初始化所有ADC设备句柄为NULL
    memset(adc_devices, 0, sizeof(adc_devices));
   
    // 初始化参考电压校准值
    static ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    uint8_t i = 0;
    uint32_t total_adc = 0;
    for(i = 0; i < 200; i++)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        total_adc += (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    voltage_vrefint_proportion = 200 * 1.2f / total_adc;
    // 根据内部参考电压校准实际参考电压
    adc_reference_voltage = 3.3f * voltage_vrefint_proportion;

    adc_initialized = 1;
}

/**
 * @description: ADC转换完成回调函数
 * @param {ADC_HandleTypeDef*} hadc ADC句柄
 * @return {void}
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    // 查找对应的ADC设备
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (adc_device_used[i] && adc_devices[i].hadc == hadc) {
            ADC_Device* device = &adc_devices[i];
            
            // 设置事件标志
            osal_event_set(&device->adc_event, ADC_EVENT_DONE);
            
            // 自动重新启动下一次转换
            if (device->mode == ADC_MODE_IT) {
                HAL_ADC_Start_IT(device->hadc);
            } else if (device->mode == ADC_MODE_DMA) {
                HAL_ADC_Start_DMA(device->hadc, 
                                 (uint32_t*)&device->values[!device->active_buf][0], 
                                 device->channel_count);
            }
            return;
        }
    }
}

/**
 * @description: ADC错误回调函数
 * @param {ADC_HandleTypeDef*} hadc ADC句柄
 * @return {void}
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef* hadc)
{
    // 查找对应的ADC设备
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (adc_device_used[i] && adc_devices[i].hadc == hadc) {
            ADC_Device* device = &adc_devices[i];
            
            // 设置错误事件标志
            osal_event_set(&device->adc_event, ADC_EVENT_ERROR);
            
            // 自动重新启动下一次转换
            if (device->mode == ADC_MODE_IT) {
                HAL_ADC_Start_IT(device->hadc);
            } else if (device->mode == ADC_MODE_DMA) {
                HAL_ADC_Start_DMA(device->hadc, 
                                 (uint32_t*)&device->values[!device->active_buf][0], 
                                 device->channel_count);
            }
            return;
        }
    }
}