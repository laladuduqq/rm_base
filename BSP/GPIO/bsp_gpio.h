/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-08 11:46:47
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-08 15:25:45
 * @FilePath: /rm_base/BSP/GPIO/bsp_gpio.h
 * @Description: 
 */
#ifndef _BSP_GPIO_H_
#define _BSP_GPIO_H_

#include "BSP_CONFIG.h"
#include "osal_def.h"
#include <stdint.h>


/* 事件定义 */
#define GPIO_EXTI_EVENT (0x01 << 0)


/* GPIO EXTI设备结构体 */
typedef struct {
    uint16_t pin;               // GPIO引脚
    void (*callback)();         // 用户回调函数
    uint8_t is_enabled;         // 中断使能状态
    osal_event_t exti_event;    // EXTI事件标志
} GPIO_EXTI_Device;

/* GPIO EXTI设备结构体 */
typedef struct {
    uint16_t pin;          // GPIO引脚
    void (*callback)();    // 用户回调函数
    uint8_t is_enabled;    // 中断使能状态
} GPIO_EXTI_Init_Config;

/* 函数声明 */

/**
 * @description: 注册GPIO EXTI中断
 * @param {GPIO_EXTI_Init_Config*} config - GPIO EXTI初始化配置指针
 * @return {osal_status_t} 操作状态, OSAL_SUCCESS表示成功，其他值表示失败
 */
GPIO_EXTI_Device* BSP_GPIO_EXTI_Register(GPIO_EXTI_Init_Config *config);
/**
 * @description: 使能GPIO EXTI中断
 * @param {GPIO_EXTI_Device*} dev - GPIO EXTI设备指针
 * @return {osal_status_t} 操作状态, OSAL_SUCCESS表示成功，其他值表示失败
 */
osal_status_t BSP_GPIO_EXTI_Enable(GPIO_EXTI_Device *dev);
/**
 * @description: 禁用GPIO EXTI中断
 * @param {GPIO_EXTI_Device*} dev - GPIO EXTI设备指针
 * @return {osal_status_t} 操作状态, OSAL_SUCCESS表示成功，其他值表示失败
 */
osal_status_t BSP_GPIO_EXTI_Disable(GPIO_EXTI_Device *dev);
/**
 * @description: 等待GPIO EXTI事件触发
 * @param {GPIO_EXTI_Device*} dev - GPIO EXTI设备指针
 * @param {osal_tick_t} timeout - 超时时间
 * @return {osal_status_t} 操作状态, OSAL_SUCCESS表示成功，其他值表示失败
 */
osal_status_t BSP_GPIO_EXTI_Wait(GPIO_EXTI_Device* dev, osal_tick_t timeout);
/**
 * @description: 反注册GPIO EXTI中断
 * @param {GPIO_EXTI_Device*} dev - GPIO EXTI设备指针
 */
void BSP_GPIO_EXTI_Unregister(GPIO_EXTI_Device* dev);

#endif // _BSP_GPIO_H_