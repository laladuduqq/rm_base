/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 10:31:16
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 22:15:31
 * @FilePath: /rm_base/modules/BEEP/beep.c
 * @Description: 蜂鸣器模块，基于BSP_PWM组件实现
 */
#include "beep.h"
#include "bsp_pwm.h"
#include "bsp_dwt.h"
#include "log.h"
#include "osal_def.h"
#include "tim.h"
#include <stdint.h>
#include <string.h>

#if BEEP_ENBALE

// 蜂鸣器设备实例
static PWM_Device* beep_pwm_dev = NULL;
static osal_timer_callback_t beep_callback = NULL;
static osal_timer_t beep_timer;

osal_status_t beep_init(uint32_t frequency, uint32_t beep_time_period, osal_timer_callback_t callback)
{
    osal_status_t status = OSAL_SUCCESS;
    // 配置PWM设备
    PWM_Init_Config pwm_config = {
        .htim = &htim4,
        .channel = PWM_CHANNEL_3,
        .frequency = frequency, // 根据自动重装载值计算频率
        .duty_cycle_x10 = 0, 
        .mode = PWM_MODE_BLOCKING
    };
    // 初始化PWM设备
    beep_pwm_dev = BSP_PWM_Device_Init(&pwm_config);
    if (beep_pwm_dev == NULL) {
        return OSAL_ERROR;
    }
    // 启动PWM输出
    status = BSP_PWM_Start(beep_pwm_dev);
    if (status != OSAL_SUCCESS)
    {
        beep_pwm_dev = NULL;
        return OSAL_ERROR;
    }
    // 默认关闭蜂鸣器
    beep_set_tune(0, 0);
    
    // 保存回调函数
    beep_callback = callback;

    status = osal_timer_create(&beep_timer, "beep_timer", beep_callback, 
                               NULL, beep_time_period, OSAL_TIMER_MODE_PERIODIC);
    status = osal_timer_start(&beep_timer);
    return status;
}

osal_status_t beep_set_tune(uint16_t tune, uint16_t ctrl)
{
    if (beep_pwm_dev == NULL) {
        return OSAL_ERROR;
    }
    
    if (tune == 0) {
        // 关闭蜂鸣器
        BSP_PWM_Set_Duty_Cycle(beep_pwm_dev, 0);
    } else {
        // 设置频率和占空比
        uint32_t frequency = 1000000 / tune;
        uint8_t duty_cycle = (ctrl * 1000) / tune;
        
        BSP_PWM_Set_Frequency(beep_pwm_dev, frequency);
        BSP_PWM_Set_Duty_Cycle(beep_pwm_dev, duty_cycle);
    }

    return OSAL_SUCCESS;
}
#else  
osal_status_t beep_init(uint32_t frequency, uint32_t beep_time_period, osal_timer_callback_t callback){
    return OSAL_ERROR;
}
osal_status_t beep_set_tune(uint16_t tune, uint16_t ctrl){
    return OSAL_SUCCESS;
}
#endif

