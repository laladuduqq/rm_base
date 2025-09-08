/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-08 10:44:28
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-08 11:27:08
 * @FilePath: /rm_base/BSP/I2C/bsp_i2c.h
 * @Description: 
 */
#ifndef _BSP_I2C_H_
#define _BSP_I2C_H_

#include "BSP_CONFIG.h"
#include "i2c.h"
#include "osal_def.h"

/* I2C 事件定义 */
#define I2C_EVENT_TX_COMPLETE (0x01 << 0)
#define I2C_EVENT_RX_COMPLETE (0x01 << 1)
#define I2C_EVENT_ERROR       (0x01 << 2)

/* 传输模式枚举 */
typedef enum {
    I2C_MODE_BLOCKING,
    I2C_MODE_IT,
    I2C_MODE_DMA
} I2C_Mode;

/* I2C设备实例结构体 */
typedef struct {
    I2C_HandleTypeDef* hi2c;         // I2C句柄
    uint16_t dev_address;           // 设备地址,注意在stm32中是左移1位的地址
    I2C_Mode tx_mode;               // 发送模式
    I2C_Mode rx_mode;               // 接收模式
} I2C_Device;

/* 初始化配置结构体 */
typedef struct {
    I2C_HandleTypeDef* hi2c;
    uint16_t dev_address;
    I2C_Mode tx_mode;
    I2C_Mode rx_mode;  
} I2C_Device_Init_Config;

/* I2C总线管理结构 */
typedef struct {
    I2C_HandleTypeDef* hi2c;                      // I2C句柄
    I2C_Device devices[MAX_DEVICES_PER_I2C_BUS];  // 设备列表
    osal_mutex_t bus_mutex;                       // 互斥锁
    osal_event_t bus_event;                       // 事件通知
    uint8_t device_count;                         // 当前设备数量
    I2C_Device* active_dev;                       // 当前活动设备
} I2C_Bus_Manager;


/**
 * @description:  初始化I2C设备
 * @param {I2C_Device_Init_Config*} config，初始化配置，见I2C_Device_Init_Config结构体
 * @return {I2C_Device*} 成功返回设备实例指针，失败返回NULL
 */
I2C_Device* BSP_I2C_Device_Init(I2C_Device_Init_Config* config);

/**
 * @description:  销毁I2C设备实例
 * @param {I2C_Device*} dev，要销毁的设备实例
 * @return {*}
 */
void BSP_I2C_Device_DeInit(I2C_Device* dev);

/**
 * @description: I2C发送数据
 * @details      适用于单向发送数据
 * @param {I2C_Device*} dev，要操作的I2C设备实例
 * @param {uint8_t*} tx_data，发送数据指针
 * @param {uint16_t} size，数据大小
 * @return {osal_status_t} 返回操作状态,OSAL_SUCCESS表示成功，否则返回失败
 */
osal_status_t BSP_I2C_Transmit(I2C_Device* dev, uint8_t* tx_data, uint16_t size);

/**
 * @description: I2C接收数据
 * @details      适用于单向接收数据
 * @param {I2C_Device*} dev，要操作的I2C设备实例
 * @param {uint8_t*} rx_data，接收数据指针
 * @param {uint16_t} size，数据大小
 * @return {osal_status_t} 返回操作状态,OSAL_SUCCESS表示成功，否则返回失败
 */
osal_status_t BSP_I2C_Receive(I2C_Device* dev, uint8_t* rx_data, uint16_t size);

/**
 * @description: I2C内存读写操作
 * @details      适用于需要指定内存地址的读写操作
 * @param {I2C_Device*} dev，要操作的I2C设备实例
 * @param {uint16_t} mem_address，内存地址
 * @param {uint16_t} mem_add_size，选择I2C_MEMADD_SIZE_8BIT或I2C_MEMADD_SIZE_16BIT
 * @param {uint8_t*} data，数据指针
 * @param {uint16_t} size，数据大小
 * @param {uint8_t} is_write，1表示写操作，0表示读操作
 * @return {osal_status_t} 返回操作状态,OSAL_SUCCESS表示成功，否则返回失败
 */
osal_status_t BSP_I2C_Mem_Write_Read(I2C_Device* dev, uint16_t mem_address, 
                                        uint16_t mem_add_size, uint8_t* data, 
                                        uint16_t size, uint8_t is_write);

#endif // _BSP_I2C_H_