/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-08 08:31:35
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-08 10:10:35
 * @FilePath: /rm_base/BSP/PWM/bsp_pwm.c
 * @Description: 
 */
#include "bsp_pwm.h"

static PWM_Device pwm_devices[MAX_PWM_DEVICES];
static uint8_t device_count = 0;

// 内部函数声明
static uint32_t BSP_PWM_Get_Timer_Clock(TIM_HandleTypeDef* htim);

PWM_Device* BSP_PWM_Device_Init(PWM_Init_Config* config)
{
    if (config == NULL || config->htim == NULL) {
        return NULL;
    }

    // 检查占空比参数有效性 (0-1000表示0.0%-100.0%)
    if (config->duty_cycle_x10 > 1000) {
        return NULL;
    }

    if (device_count >= MAX_PWM_DEVICES) {
        return NULL;
    }
    
    PWM_Device* dev = &pwm_devices[device_count++];
    dev->htim = config->htim;
    dev->channel = config->channel;
    dev->frequency = config->frequency;
    dev->duty_cycle_x10 = config->duty_cycle_x10;
    dev->mode = config->mode;
    
    // 设置频率和计算周期
    if (BSP_PWM_Set_Frequency(dev, config->frequency) != OSAL_SUCCESS) {
        device_count--;
        return NULL;
    }
    
    return dev;
}

void BSP_PWM_Device_DeInit(PWM_Device* dev)
{
    if (dev == NULL) {
        return;
    }
    
    // 停止PWM输出
    BSP_PWM_Stop(dev);
    
    // 清空设备结构体
    memset(dev, 0, sizeof(PWM_Device));
}

osal_status_t BSP_PWM_Start(PWM_Device* dev)
{
    if (dev == NULL || dev->htim == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef hal_status;
    
    switch (dev->mode) {
        case PWM_MODE_BLOCKING:
            hal_status = HAL_TIM_PWM_Start(dev->htim, (uint32_t)dev->channel);
            break;
            
        case PWM_MODE_IT:
            hal_status = HAL_TIM_PWM_Start_IT(dev->htim, (uint32_t)dev->channel);
            break;
            
        case PWM_MODE_DMA:
            hal_status = HAL_TIM_PWM_Start_DMA(dev->htim, (uint32_t)dev->channel, 
                                              (uint32_t*)&dev->pulse, 1);
            break;
            
        default:
            return OSAL_ERROR;
    }
    
    if (hal_status != HAL_OK) {
        return OSAL_ERROR;
    }
    
    return OSAL_SUCCESS;
}

/**
 * @description: 停止PWM输出
 * @param {PWM_Device*} dev - PWM设备指针
 * @return {osal_status_t} 操作状态
 */
osal_status_t BSP_PWM_Stop(PWM_Device* dev)
{
    if (dev == NULL || dev->htim == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef hal_status;
    
    switch (dev->mode) {
        case PWM_MODE_BLOCKING:
            hal_status = HAL_TIM_PWM_Stop(dev->htim, (uint32_t)dev->channel);
            break;
            
        case PWM_MODE_IT:
            hal_status = HAL_TIM_PWM_Stop_IT(dev->htim, (uint32_t)dev->channel);
            break;
            
        case PWM_MODE_DMA:
            hal_status = HAL_TIM_PWM_Stop_DMA(dev->htim, (uint32_t)dev->channel);
            break;
            
        default:
            return OSAL_ERROR;
    }
    
    if (hal_status != HAL_OK) {
        return OSAL_ERROR;
    }
    
    return OSAL_SUCCESS;
}


osal_status_t BSP_PWM_Set_Duty_Cycle(PWM_Device* dev, uint16_t duty_cycle_x10)
{
    if (dev == NULL || dev->htim == NULL || duty_cycle_x10 > 1000) {
        return OSAL_INVALID_PARAM;
    }
    
    // 更新占空比值
    dev->duty_cycle_x10 = duty_cycle_x10;
    
    // 重新计算脉冲宽度
    dev->pulse = (dev->period * dev->duty_cycle_x10) / 1000;
    
    // 设置新的脉冲宽度
    __HAL_TIM_SET_COMPARE(dev->htim, (uint32_t)dev->channel, dev->pulse);
    
    return OSAL_SUCCESS;
}

osal_status_t BSP_PWM_Set_Frequency(PWM_Device* dev, uint32_t frequency)
{
    if (dev == NULL || dev->htim == NULL || frequency == 0) {
        return OSAL_INVALID_PARAM;
    }
    
    // 保存频率
    dev->frequency = frequency;
    
    // 获取定时器时钟频率
    uint32_t tim_clock = BSP_PWM_Get_Timer_Clock(dev->htim);
    
    // 计算预分频系数和周期值
    // 我们希望周期值在合理范围内(避免过大或过小)
    uint32_t period = tim_clock / frequency;
    uint32_t prescaler = 1;
    
    // 如果period过大，需要增加预分频
    while (period > 0xFFFF && prescaler < 0xFFFF) {
        prescaler++;
        period = tim_clock / (frequency * prescaler);
    }
    
    // 设置定时器参数
    dev->htim->Init.Prescaler = prescaler - 1;
    dev->htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    dev->htim->Init.Period = period - 1;
    dev->htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    
    // 保存周期值
    dev->period = period - 1;
    
    // 重新计算脉冲宽度
    dev->pulse = (dev->period * dev->duty_cycle_x10) / 1000;
    
    // 重新初始化定时器
    if (HAL_TIM_PWM_Init(dev->htim) != HAL_OK) {
        return OSAL_ERROR;
    }
    
    return OSAL_SUCCESS;
}

/**
 * @description: 获取定时器时钟频率
 * @param {TIM_HandleTypeDef*} htim - 定时器句柄
 * @return {uint32_t} 定时器时钟频率(Hz)
 */
static uint32_t BSP_PWM_Get_Timer_Clock(TIM_HandleTypeDef* htim)
{
    uint32_t tim_clock = SystemCoreClock;
    
    // 根据定时器类型确定时钟源
    if (htim->Instance == TIM1 || htim->Instance == TIM8 || 
        htim->Instance == TIM9 || htim->Instance == TIM10 || 
        htim->Instance == TIM11) {
        // 高级定时器和部分通用定时器通常连接到APB2总线
        tim_clock = HAL_RCC_GetPCLK2Freq();
        
        // 如果APB2预分频器不为1，则时钟频率需要倍频
        if ((RCC->CFGR & RCC_CFGR_PPRE2) != 0) {
            tim_clock *= 2;
        }
    } else {
        // 其他定时器通常连接到APB1总线
        tim_clock = HAL_RCC_GetPCLK1Freq();
        
        // 如果APB1预分频器不为1，则时钟频率需要倍频
        if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0) {
            tim_clock *= 2;
        }
    }
    
    return tim_clock;
}