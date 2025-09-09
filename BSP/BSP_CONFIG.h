/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-07 13:55:47
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-09 13:44:11
 * @FilePath: /rm_base/BSP/BSP_CONFIG.h
 * @Description: bsp配置文件
 */
#ifndef _BSP_CONFIG_H_
#define _BSP_CONFIG_H_

/* UART 配置 */
#define UART_MAX_INSTANCE_NUM 3        //可用串口数量

/* SPI 配置 */
#define SPI_BUS_NUM 2                  // 总线数量
#define MAX_DEVICES_PER_BUS 4          // 每条总线最大设备数

/* PWM 配置 */
#define MAX_PWM_DEVICES 10             // 最大PWM设备数

/* I2C 配置 */
#define I2C_BUS_NUM 2                  // 总线数量
#define MAX_DEVICES_PER_I2C_BUS 4      // 每条总线最大设备数

/* GPIO 配置 */
#define GPIO_EXTI_DEVICE_NUM 16        // 最大GPIO中断回调设备数

/* ADC 配置 */
#define ADC_DEVICE_NUM 4               // 最大ADC设备数

#endif // _BSP_CONFIG_H_