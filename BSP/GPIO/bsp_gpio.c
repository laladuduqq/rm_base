/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-08 11:46:41
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-08 15:26:18
 * @FilePath: /rm_base/BSP/GPIO/bsp_gpio.c
 * @Description: 
 */
#include "bsp_gpio.h"
#include "osal_def.h"
#include <string.h>
#include <stdio.h>

/* 内部GPIO EXTI设备数组 */
static GPIO_EXTI_Device gpio_exti_devices[GPIO_EXTI_DEVICE_NUM] = {0};
static uint8_t gpio_exti_device_count = 0;

GPIO_EXTI_Device* BSP_GPIO_EXTI_Register(GPIO_EXTI_Init_Config *config)
{
    if (config == NULL) {
        return NULL;
    }

    // 检查是否已达到最大设备数量
    if (gpio_exti_device_count >= GPIO_EXTI_DEVICE_NUM) {
        return NULL;
    }

    // 检查该引脚是否已经注册
    for (int i = 0; i < gpio_exti_device_count; i++) {
        if (gpio_exti_devices[i].pin == config->pin) {
            return NULL; // 引脚已注册
        }
    }

    // 获取设备实例
    GPIO_EXTI_Device* dev = &gpio_exti_devices[gpio_exti_device_count];
    
    // 初始化设备
    dev->pin = config->pin;
    dev->callback = config->callback;
    dev->is_enabled = 0; // 默认不使能

    // 创建事件
    char event_name[32];
    snprintf(event_name, sizeof(event_name), "gpio_exti_%d", config->pin);
    if (osal_event_create(&dev->exti_event, event_name) != OSAL_SUCCESS) {
        return NULL;
    }

    gpio_exti_device_count++;
    return dev;
}


osal_status_t BSP_GPIO_EXTI_Enable(GPIO_EXTI_Device *dev)
{
    if (dev == NULL) {
        return OSAL_INVALID_PARAM;
    }

    dev->is_enabled = 1;
    return OSAL_SUCCESS;
}


osal_status_t BSP_GPIO_EXTI_Disable(GPIO_EXTI_Device *dev)
{
    if (dev == NULL) {
        return OSAL_INVALID_PARAM;
    }

    dev->is_enabled = 0;
    return OSAL_SUCCESS;
}


osal_status_t BSP_GPIO_EXTI_Wait(GPIO_EXTI_Device* dev, osal_tick_t timeout)
{
    if (dev == NULL) {
        return OSAL_INVALID_PARAM;
    }

    unsigned int actual_flags;
    return osal_event_wait(&dev->exti_event,GPIO_EXTI_EVENT, 
                          OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR ,timeout,&actual_flags);
}


void BSP_GPIO_EXTI_Unregister(GPIO_EXTI_Device* dev)
{
    if (dev == NULL) {
        return;
    }

    // 查找设备在数组中的位置
    int index = -1;
    for (int i = 0; i < gpio_exti_device_count; i++) {
        if (&gpio_exti_devices[i] == dev) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return; // 未找到设备
    }

    // 删除事件
    osal_event_delete(&dev->exti_event);

    // 将后面的设备向前移动
    for (int i = index; i < gpio_exti_device_count - 1; i++) {
        gpio_exti_devices[i] = gpio_exti_devices[i + 1];
    }

    // 清除最后一个设备
    memset(&gpio_exti_devices[gpio_exti_device_count - 1], 0, sizeof(GPIO_EXTI_Device));
    
    gpio_exti_device_count--;
}

/**
 * @description: 内部函数 - 处理GPIO EXTI中断，由HAL回调调用
 * @param {uint16_t} pin - GPIO引脚
 */
void BSP_GPIO_EXTI_Handle_IRQ(uint16_t pin)
{
    // 查找对应的设备
    for (int i = 0; i < gpio_exti_device_count; i++) {
        if (gpio_exti_devices[i].pin == pin && gpio_exti_devices[i].is_enabled) {
            GPIO_EXTI_Device* dev = &gpio_exti_devices[i];
            
            // 设置事件标志
            osal_event_set(&dev->exti_event, GPIO_EXTI_EVENT);
            
            // 调用用户回调函数
            if (dev->callback != NULL) {
                dev->callback();
            }
            break;
        }
    }
}

/* 中断回调函数 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BSP_GPIO_EXTI_Handle_IRQ(GPIO_Pin);
}