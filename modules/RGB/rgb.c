/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 10:26:48
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 10:46:56
 * @FilePath: /rm_base/modules/RGB/rgb.c
 * @Description: 
 */
#include "rgb.h"
#include "bsp_pwm.h"
#include <string.h>

#define log_tag "RGB"
#include "log.h"

static PWM_Device* pwm_r = NULL;
static PWM_Device* pwm_g = NULL;
static PWM_Device* pwm_b = NULL;
  
void RGB_init(void)
{
    PWM_Init_Config config = {
        .channel = PWM_CHANNEL_3,
        .duty_cycle_x10 = 0,
        .frequency = 1000,
        .htim = &htim5,
        .mode = PWM_MODE_BLOCKING
    };
    pwm_r = BSP_PWM_Device_Init(&config);
    config.channel = PWM_CHANNEL_2;
    pwm_g = BSP_PWM_Device_Init(&config);
    config.channel = PWM_CHANNEL_1;
    pwm_b = BSP_PWM_Device_Init(&config);

    if (pwm_r == NULL || pwm_g == NULL || pwm_b == NULL)
    {
        LOG_ERROR("PWM init failed");
    }else
    {
        BSP_PWM_Start(pwm_r);
        BSP_PWM_Start(pwm_g);
        BSP_PWM_Start(pwm_b);
        LOG_INFO("RGB init success");
    }
} 

void RGB_show(uint32_t aRGB)
{
    if (!pwm_r || !pwm_g || !pwm_b) {
        return;
    }
    
    static uint8_t alpha;
    static uint8_t red, green, blue;
    
    // 提取ARGB值
    alpha = (aRGB & 0xFF000000) >> 24;
    red = (aRGB & 0x00FF0000) >> 16;
    green = (aRGB & 0x0000FF00) >> 8;
    blue = (aRGB & 0x000000FF) >> 0;
    
    // 应用alpha通道（如果需要）
    if (alpha != 0xFF) {
        red = (red * alpha) / 255;
        green = (green * alpha) / 255;
        blue = (blue * alpha) / 255;
    }
    
    // 将0-255的值转换为0-1000的占空比
    uint8_t red_duty = (red * 1000) / 255;
    uint8_t green_duty = (green * 1000) / 255;
    uint8_t blue_duty = (blue * 1000) / 255;
    
    // 设置PWM占空比
    BSP_PWM_Set_Duty_Cycle(pwm_r,red_duty);
    BSP_PWM_Set_Duty_Cycle(pwm_g,green_duty);
    BSP_PWM_Set_Duty_Cycle(pwm_b,blue_duty);
}
