/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 21:12:29
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 22:07:38
 * @FilePath: /rm_base/modules/BEEP/beep.h
 * @Description: 
 */
#ifndef _BEEP_H_
#define _BEEP_H_

#include "modules_config.h"
#include "bsp_pwm.h"
#include "osal_def.h"
#include "tim.h"
#include <stdint.h>

/**
 * @description: 蜂鸣器初始化
 * @param {uint32_t} frequency,频率
 * @param {uint32_t} beep_time_period，蜂鸣器时间周期
 * @param {osal_timer_callback_t} callback，定时器回调函数
 * @return {osal_status_t},OSAL_SCUCCESS成功,其余失败
 */
osal_status_t beep_init(uint32_t frequency, uint32_t beep_time_period, osal_timer_callback_t callback);
/**
 * @description: 设置蜂鸣器音调和音量
 * @param {uint16_t} tune 自动重装载值，影响频率
 * @param {uint16_t} ctrl 比较值，影响占空比
 * @return {osal_status_t},OSAL_SCUCCESS成功,其余失败
 */
osal_status_t beep_set_tune(uint16_t tune, uint16_t ctrl);

#endif // _BEEP_H_