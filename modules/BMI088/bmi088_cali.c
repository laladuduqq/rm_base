/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 15:04:31
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 15:54:54
 * @FilePath: /rm_base/modules/BMI088/bmi088_cali.c
 * @Description: 
 */
#include "bmi088.h"
#include "bsp_dwt.h"
#include "bsp_flash.h"
#include "osal_def.h"
#include "rgb.h"
#include "math.h"

#define log_tag "BMI088"
#include "log.h"

#include "modules_config.h"

#if BMI088_ENABLE

#define GxOFFSET -0.00601393497f
#define GyOFFSET -0.00196841615f
#define GzOFFSET 0.00114696583f
#define gNORM 9.67463112f

static float gyroDiff[3], gNormDiff;
static BMI088_Cali_Offset_t caliOffset;
osal_status_t Calibrate_BMI088_Offset(BMI088_Instance_t *ist)
{
    osal_status_t status = OSAL_SUCCESS;
    static float dt,t;
    static uint32_t dt_cnt;
    static uint16_t CaliTimes = 6000;
    float gyroMax[3], gyroMin[3];
    float gNormTemp=0.0f, gNormMax=0.0f, gNormMin=0.0f;

    RGB_show(LED_Yellow);

    do
    {
        dt = DWT_GetDeltaT(&dt_cnt);
        t+=dt;
        if (t > 12)
        {
            caliOffset.GyroOffset[0] = GxOFFSET;
            caliOffset.GyroOffset[1] = GyOFFSET;
            caliOffset.GyroOffset[2] = GzOFFSET;
            caliOffset.gNorm = gNORM;
            caliOffset.TempWhenCali =40;
            LOG_ERROR("Calibrate BMI088 Offset Failed!");
            RGB_show(LED_Red);
            status = OSAL_ERROR;
            return status;
            break;
        }

        DWT_Delay(0.005);
        caliOffset.gNorm = 0;
        caliOffset.GyroOffset[0] = 0;
        caliOffset.GyroOffset[1] = 0;
        caliOffset.GyroOffset[2] = 0;

        for (uint16_t i = 0; i < CaliTimes; i++)
        {

            bmi088_get_accel(ist);
            gNormTemp = sqrtf(ist->BMI088_Raw_Data.acc[0] * ist->BMI088_Raw_Data.acc[0] +
                              ist->BMI088_Raw_Data.acc[1] * ist->BMI088_Raw_Data.acc[1] +
                              ist->BMI088_Raw_Data.acc[2] * ist->BMI088_Raw_Data.acc[2]);
            caliOffset.gNorm += gNormTemp;

            bmi088_get_gyro(ist);
            caliOffset.GyroOffset[0] += ist->BMI088_Raw_Data.gyro[0];
            caliOffset.GyroOffset[1] += ist->BMI088_Raw_Data.gyro[1];
            caliOffset.GyroOffset[2] += ist->BMI088_Raw_Data.gyro[2];
            if (i == 0)
            {
                gNormMax = gNormTemp;
                gNormMin = gNormTemp;
                for (uint8_t j = 0; j < 3; j++)
                {
                    gyroMax[j] = ist->BMI088_Raw_Data.gyro[j];
                    gyroMin[j] = ist->BMI088_Raw_Data.gyro[j];
                }
            }
            else
            {
                if (gNormTemp > gNormMax)
                    gNormMax = gNormTemp;
                if (gNormTemp < gNormMin)
                    gNormMin = gNormTemp;
                for (uint8_t j = 0; j < 3; j++)
                {
                    if (ist->BMI088_Raw_Data.gyro[j] > gyroMax[j]) gyroMax[j] = ist->BMI088_Raw_Data.gyro[j];
                    if (ist->BMI088_Raw_Data.gyro[j] < gyroMin[j]) gyroMin[j] = ist->BMI088_Raw_Data.gyro[j];
                }
            }
            gNormDiff = gNormMax - gNormMin;
            for (uint8_t j = 0; j < 3; j++) {gyroDiff[j] = gyroMax[j] - gyroMin[j];}
            if (gNormDiff > 0.5f ||gyroDiff[0] > 0.15f ||  gyroDiff[1] > 0.15f || gyroDiff[2] > 0.15f){break;}
            DWT_Delay(0.001);
        }

        caliOffset.gNorm /= (float)CaliTimes;
        for (uint8_t i = 0; i < 3; i++){caliOffset.GyroOffset[i] /= (float)CaliTimes;}

        //记录标定时的温度
        bmi088_get_temp(ist);
        caliOffset.TempWhenCali =ist->BMI088_Raw_Data.temperature;
        LOG_INFO("tempcali:%0.2f\n",caliOffset.TempWhenCali);

    } while (gNormDiff > 0.5f ||fabsf(caliOffset.gNorm - 9.8f) > 0.5f || gyroDiff[0] > 0.15f || gyroDiff[1] > 0.15f || gyroDiff[2] > 0.15f ||
             fabsf(caliOffset.GyroOffset[0]) > 0.01f || fabsf(caliOffset.GyroOffset[1]) > 0.01f || fabsf(caliOffset.GyroOffset[2]) > 0.01f);

    caliOffset.AccelScale = 9.81f / caliOffset.gNorm;

    static uint8_t tmpdata[sizeof(BMI088_Cali_Offset_t)+2]={0};
    memcpy(tmpdata,&caliOffset,sizeof(BMI088_Cali_Offset_t));
    tmpdata[sizeof(BMI088_Cali_Offset_t)+1]=0XAA;

    BSP_FLASH_Erase_Sector(BSP_FLASH_SECTOR_11); // 清空扇区11的数据
    BSP_FLASH_Write_Buffer(0x080E0000,tmpdata, sizeof(tmpdata)); // 向扇区11的初始地址写入数据
    LOG_INFO("calibrate BMI088 offset finished!");
    return status;
}

#else  
osal_status_t Calibrate_BMI088_Offset(BMI088_Instance_t *ist)
{
    return OSAL_SUCCESS;
}
#endif 
