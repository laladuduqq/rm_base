/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-09 13:37:54
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-09 16:54:44
 * @FilePath: /rm_base/BSP/ADC/bsp_adc.h
 * @Description: 
 */
#ifndef _BSP_ADC_H_
#define _BSP_ADC_H_

#include "BSP_CONFIG.h"
#include "osal_def.h"
#include "adc.h"

/* ADC事件定义 */
#define ADC_EVENT_DONE (0x01 << 0)
#define ADC_EVENT_ERROR (0x01 << 1)

/* ADC模式枚举 */
typedef enum {
    ADC_MODE_BLOCKING,           // 阻塞模式
    ADC_MODE_IT,                 // 中断模式
    ADC_MODE_DMA                 // DMA模式
} ADC_Mode;

/* ADC设备实例结构体 */
typedef struct {
    ADC_HandleTypeDef* hadc;                    // ADC句柄
    ADC_ChannelConfTypeDef* channel_configs;    // 通道配置数组
    uint8_t channel_count;                      // 通道数量
    ADC_Mode mode;                              // 工作模式
    uint32_t (*values)[2];                     // 指向外部定义的双缓冲区
    volatile uint8_t active_buf;               // 当前活动缓冲区
    osal_mutex_t adc_mutex;                     // 互斥锁
    osal_event_t adc_event;                     // 事件
} ADC_Device;

/* ADC初始化配置结构体 */
typedef struct {
    ADC_HandleTypeDef* hadc;
    ADC_ChannelConfTypeDef* channel_configs;
    uint8_t channel_count;
    uint32_t (*values)[2];                     // 外部双缓冲区指针
    ADC_Mode mode;
} ADC_Device_Init_Config;

/**
 * @description: 初始化ADC设备
 * @param {ADC_Device_Init_Config*} config ADC初始化配置
 * @return {ADC_Device*} 成功返回ADC设备实例指针，失败返回NULL
 */
ADC_Device* BSP_ADC_Device_Init(ADC_Device_Init_Config* config);

/**
 * @description: 销毁ADC设备实例
 * @param {ADC_Device*} dev ADC设备实例
 * @return {void}
 */
void BSP_ADC_Device_DeInit(ADC_Device* dev);

/**
 * @description: 启动ADC转换
 * @param {ADC_Device*} dev ADC设备实例
 * @return {osal_status_t},osal_scucess 成功
 */
osal_status_t BSP_ADC_Start(ADC_Device* dev);

/**
 * @description: 停止ADC转换
 * @param {ADC_Device*} dev ADC设备实例
 * @return {osal_status_t},osal_scucess 成功
 */
osal_status_t BSP_ADC_Stop(ADC_Device* dev);

/**
 * @description: 获取ADC转换值
 * @param {ADC_Device*} dev ADC设备实例
 * @return {osal_status_t},osal_scucess 成功
 */
osal_status_t BSP_ADC_Get_Values(ADC_Device* dev);

/**
 * @description: 获取指定通道的ADC转换值
 * @note  该函数与BSP_ADC_Get_Values()共同使用，必须先调用BSP_ADC_Get_Values()获取转换值
 * @param {ADC_Device*} dev ADC设备实例
 * @param {uint8_t} channel_index 通道索引
 * @return {uint32_t} ADC转换值
 */
uint32_t BSP_ADC_Get_Channel_Value(ADC_Device* dev, uint8_t channel_index);

/**
 * @description: ADC初始化（系统初始化时调用一次）
 * @return {void}
 */
void BSP_ADC_Init(void);

#endif // _BSP_ADC_H_