/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 11:11:09
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 13:07:30
 * @FilePath: /rm_base/modules/IST8310/ist8310.c
 * @Description: 
 */
#include "ist8310.h"
#include "bsp_dwt.h"
#include "bsp_i2c.h"
#include "osal_def.h"
#include <stdint.h>
#include <string.h>

#define log_tag "IST8310"
#include "log.h"

static IST8310_Instance_t ist8310_instance ={0}; // 用于存储IST8310实例

#define IST8310_WRITE_REG_NUM 4     // 方便阅读
#define IST8310_DATA_REG 0x03       // ist8310的数据寄存器
#define IST8310_WHO_AM_I 0x00       // ist8310 id 寄存器值
#define IST8310_WHO_AM_I_VALUE 0x10 // 用于检测是否连接成功,读取ist8310的whoami会返回该值

// -------------------初始化写入数组,只使用一次,详见datasheet-------------------------
// the first column:the registers of IST8310. 第一列:IST8310的寄存器
// the second column: the value to be writed to the registers.第二列:需要写入的寄存器值
// the third column: return error value.第三列:返回的错误码
static uint8_t ist8310_write_reg_data_error[IST8310_WRITE_REG_NUM][3] = {
    {0x0B, 0x08, 0x01},  // enalbe interrupt  and low pin polarity.开启中断，并且设置低电平
    {0x41, 0x09, 0x02},  // average 2 times.平均采样两次
    {0x42, 0xC0, 0x03},  // must be 0xC0. 必须是0xC0
    {0x0A, 0x0B, 0x04}}; // 200Hz output rate.200Hz输出频率

IST8310_Instance_t *IST8310_Init()
{
    IST8310_Instance_t *ist = &ist8310_instance;
    memset(ist, 0, sizeof(IST8310_Instance_t));

    // 配置初始化参数（阻塞模式）
    I2C_Device_Init_Config config = {
        .hi2c = &hi2c3,
        .dev_address = IST8310_IIC_ADDRESS << 1,  // 设备地址左移1位
        .tx_mode = I2C_MODE_BLOCKING,
        .rx_mode = I2C_MODE_BLOCKING
    };
    ist8310_instance.i2c_device = BSP_I2C_Device_Init(&config);
    if (ist8310_instance.i2c_device == NULL) {
        LOG_ERROR("IST8310 init failed");
        return NULL;
    }
    uint8_t check_who_i_am = 0;          // 用于检测ist8310是否连接成功

    // 重置IST8310,需要HAL_Delay()等待传感器完成Reset
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_RESET);
    DWT_Delay(50 *0.001f);
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_SET);
    DWT_Delay(50 *0.001f);

    // 读取IST8310的ID,如果不是0x10(whoami macro的值),则返回错误
    BSP_I2C_Mem_Write_Read(ist8310_instance.i2c_device, IST8310_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &check_who_i_am, 1,0);
    if (check_who_i_am != IST8310_WHO_AM_I_VALUE)
    {
        LOG_ERROR("IST8310_WHO_AM_I error");
        return NULL;
    }
    // 进行初始化配置写入并检查是否写入成功
    for (uint8_t i = 0; i < IST8310_WRITE_REG_NUM; i++)
    { // 写入配置,写一句就读一下
        BSP_I2C_Mem_Write_Read(ist8310_instance.i2c_device, ist8310_write_reg_data_error[i][0], I2C_MEMADD_SIZE_8BIT, &ist8310_write_reg_data_error[i][1], 1,1);
        BSP_I2C_Mem_Write_Read(ist8310_instance.i2c_device, ist8310_write_reg_data_error[i][0], I2C_MEMADD_SIZE_8BIT, &check_who_i_am, 1,0);
        if (check_who_i_am != ist8310_write_reg_data_error[i][1]){
            LOG_ERROR("[ist8310] init error, code %d", ist8310_write_reg_data_error[i][2]);
            return NULL;
        }
    }
    return ist;
}

osal_status_t IST8310_ReadData(IST8310_Instance_t *ist)
{ 
    osal_status_t status  = OSAL_SUCCESS;
    status = BSP_I2C_Mem_Write_Read(ist8310_instance.i2c_device, IST8310_DATA_REG, I2C_MEMADD_SIZE_8BIT, ist8310_instance.iic_buffer, 6,0);
    if (status == OSAL_SUCCESS)
    {
        int16_t mag_raw[3];
        memcpy(mag_raw, ist8310_instance.iic_buffer, sizeof(uint8_t)* 6);
        for (uint8_t i = 0; i < 3; i++){
            ist->mag[i] = (float)mag_raw[i] * MAG_SEN; // 乘以灵敏度转换成uT(微特斯拉)
        }
    }
    else
    {
        LOG_ERROR("IST8310_ReadData failed");
    }
    return status;
}
