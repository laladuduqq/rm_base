/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-08 08:31:41
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-08 10:10:24
 * @FilePath: /rm_base/BSP/PWM/bsp_pwm.h
 * @Description: 
 */
#ifndef _BSP_PWM_H_
#define _BSP_PWM_H_

#include "osal_def.h"
#include "tim.h"
#include "BSP_CONFIG.h"

/* PWM模式枚举 */
typedef enum {
    PWM_MODE_BLOCKING,     
    PWM_MODE_IT,
    PWM_MODE_DMA    
} PWM_Mode;

/* PWM通道枚举 */
typedef enum {
    PWM_CHANNEL_1 = TIM_CHANNEL_1,
    PWM_CHANNEL_2 = TIM_CHANNEL_2,
    PWM_CHANNEL_3 = TIM_CHANNEL_3,
    PWM_CHANNEL_4 = TIM_CHANNEL_4
} PWM_Channel;

/* PWM设备实例结构体 */
typedef struct {
    TIM_HandleTypeDef* htim;      // 定时器句柄
    PWM_Channel channel;          // PWM通道
    uint32_t period;              // 周期值
    uint32_t pulse;               // 脉冲宽度值
    uint32_t frequency;           // 频率(Hz)
    uint16_t duty_cycle_x10;      // 占空比(0-1000)，表示0.0%到100.0%，支持小数点后一位
    PWM_Mode mode;                // PWM模式
    osal_event_t pwm_event;       // PWM事件,目前不用，预留
} PWM_Device;

/* PWM初始化配置结构体 */
typedef struct {
    TIM_HandleTypeDef* htim;
    PWM_Channel channel;
    uint32_t frequency;           // 频率(Hz)
    uint16_t duty_cycle_x10;      // 初始占空比(0-1000)，表示0.0%到100.0%
    PWM_Mode mode;                // PWM模式
} PWM_Init_Config;

/* 函数声明 */
/**
 * @description: 初始化PWM设备
 * @param {PWM_Init_Config*} config - PWM初始化配置
 * @return {PWM_Device*} 成功返回PWM设备指针，失败返回NULL
 */
PWM_Device* BSP_PWM_Device_Init(PWM_Init_Config* config);

/**
 * @description: 销毁PWM设备
 * @param {PWM_Device*} dev - PWM设备指针
 * @return {*}
 */
void BSP_PWM_Device_DeInit(PWM_Device* dev);

/**
 * @description: 启动PWM输出
 * @param {PWM_Device*} dev - PWM设备指针
 * @return {osal_status_t} 操作状态
 */
osal_status_t BSP_PWM_Start(PWM_Device* dev);

/**
 * @description: 停止PWM输出
 * @param {PWM_Device*} dev - PWM设备指针
 * @return {osal_status_t} 操作状态
 */
osal_status_t BSP_PWM_Stop(PWM_Device* dev);

/**
 * @description: 设置PWM占空比
 * @param {PWM_Device*} dev - PWM设备指针
 * @param {uint16_t} duty_cycle_x10 - 占空比(0-1000)，表示0.0%到100.0%
 * @return {osal_status_t} 操作状态
 */
osal_status_t BSP_PWM_Set_Duty_Cycle(PWM_Device* dev, uint16_t duty_cycle_x10);

/**
 * @description: 设置PWM频率
 * @param {PWM_Device*} dev - PWM设备指针
 * @param {uint32_t} frequency - 频率(Hz)
 * @return {osal_status_t} 操作状态
 */
osal_status_t BSP_PWM_Set_Frequency(PWM_Device* dev, uint32_t frequency);

#endif // _BSP_PWM_H_