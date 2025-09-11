/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 13:43:14
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 14:43:26
 * @FilePath: /rm_base/modules/BMI088/bmi088.h
 * @Description: 
 */
#ifndef _BMI088_H_
#define _BMI088_H_

/* BMI088数据*/
#include "BMI088_reg.h"
#include "bsp_spi.h"
#include "osal_def.h"
#include <stdint.h>

typedef struct {
    float gyro[3];     // 陀螺仪数据,xyz
    float acc[3];      // 加速度计数据,xyz
    float temperature; // 温度
} BMI088_Raw_Data_t ;

typedef struct
{
    uint8_t buf[8];
    SPI_Device *gyro_device;
    SPI_Device *acc_device;
    BMI088_Raw_Data_t BMI088_Raw_Data;
    uint8_t BMI088_initialized;
    BMI088_ERORR_CODE_e BMI088_ERORR_CODE;
} BMI088_Instance_t;

BMI088_Instance_t* BMI088_init(void);
osal_status_t bmi088_get_accel(BMI088_Instance_t *ist);
osal_status_t bmi088_get_gyro(BMI088_Instance_t *ist);
osal_status_t bmi088_get_temp(BMI088_Instance_t *ist);

#endif // _BMI088_H_