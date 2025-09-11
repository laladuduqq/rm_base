/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 13:43:14
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 18:27:35
 * @FilePath: /rm_base/modules/BMI088/bmi088.h
 * @Description: 
 */
#ifndef _BMI088_H_
#define _BMI088_H_

/* BMI088数据*/
#include "BMI088_reg.h"
#include "bsp_pwm.h"
#include "bsp_spi.h"
#include "controller.h"
#include "osal_def.h"
#include <stdint.h>

typedef struct {
    float gyro[3];     // 陀螺仪数据,xyz
    float acc[3];      // 加速度计数据,xyz
    float temperature; // 温度
} BMI088_Raw_Data_t ;

typedef struct{
    // 标定数据
    float AccelScale;
    float GyroOffset[3];
    float gNorm;          // 重力加速度模长,从标定获取
    float TempWhenCali;   //标定时温度
}BMI088_Cali_Offset_t;

typedef struct
{
    uint8_t buf[8];
    SPI_Device *gyro_device;
    SPI_Device *acc_device;
    PWM_Device *bmi088_pwm;
    PIDInstance pid_temp;
    BMI088_Raw_Data_t BMI088_Raw_Data;
    BMI088_Cali_Offset_t BMI088_Cali_Offset;
    BMI088_ERORR_CODE_e BMI088_ERORR_CODE;
} BMI088_Instance_t;

/**
 * @description: 初始化BMI088
 * @return {BMI088_Instance_t*},初始化成功返回BMI088实例，否则返回NULL
 */
BMI088_Instance_t* BMI088_init(void);
/**
 * @description: 获取BMI088加速度数据
 * @param {BMI088_Instance_t*} ist BMI088实例
 * @return {osal_status_t},获取成功返回OSAL_SUCCESS，否则返回OSAL_ERROR
 */
osal_status_t bmi088_get_accel(BMI088_Instance_t *ist);
/**
 * @description: 获取BMI088陀螺仪数据
 * @param {BMI088_Instance_t*} ist BMI088实例
 * @return {osal_status_t},获取成功返回OSAL_SUCCESS，否则返回OSAL_ERROR
 */
osal_status_t bmi088_get_gyro(BMI088_Instance_t *ist);
/**
 * @description: 获取BMI088温度数据
 * @param {BMI088_Instance_t*} ist BMI088实例
 * @return {osal_status_t},获取成功返回OSAL_SUCCESS，否则返回OSAL_ERROR
 */
osal_status_t bmi088_get_temp(BMI088_Instance_t *ist);
/**
 * @description: BMI088温度控制
 * @return {*}
 */
void bmi088_temp_ctrl(void);
/**
 * @description: BMI088零飘标定
 * @param {BMI088_Instance_t*} ist BMI088实例
 * @return {osal_status_t},获取成功返回OSAL_SUCCESS，否则返回OSAL_ERROR
 */
osal_status_t Calibrate_BMI088_Offset(BMI088_Instance_t *ist);

#endif // _BMI088_H_