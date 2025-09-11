/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 11:11:15
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 15:45:10
 * @FilePath: /rm_base/modules/IST8310/ist8310.h
 * @Description: 
 */
#ifndef _IST8310_H_
#define _IST8310_H_

#include "bsp_i2c.h"
#include "modules_config.h"
#include "osal_def.h"

// 传感器灵敏度系数
#define MAG_SEN 0.3f // 原始整型数据变成 单位ut
#define IST8310_IIC_ADDRESS 0x0E  // IST8310的从设备IIC地址

typedef struct
{
    I2C_Device *i2c_device;       // iic实例
    uint8_t iic_buffer[8];     // iic接收缓冲区
    float mag[3];              // 三轴磁力计数据,[x,y,z]
} IST8310_Instance_t;

/**
 * @description: IST8310初始化
 * @return {IST8310_Instance_t *},返回IST8310实例指针
 */
IST8310_Instance_t *IST8310_Init();
/**
 * @description: 读取IST8310数据
 * @param {IST8310_Instance_t *} ist,IST8310实例指针
 * @return {osal_status_t},返回操作结果,OSAL_SCUCCESS为成功
 */
osal_status_t IST8310_ReadData(IST8310_Instance_t *ist);

#endif // _IST8310_H_