/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-08 18:21:23
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-08 18:38:11
 * @FilePath: /rm_base/BSP/FLASH/bsp_flash.h
 * @Description: 
 */
#ifndef _BSP_FLASH_H_
#define _BSP_FLASH_H_

#include "stm32f4xx_hal.h"
#include "osal_def.h"

/* FLASH状态枚举 */
typedef enum {
    BSP_FLASH_OK       = 0x00U,
    BSP_FLASH_ERROR    = 0x01U,
    BSP_FLASH_BUSY     = 0x02U,
    BSP_FLASH_TIMEOUT  = 0x03U,
    BSP_FLASH_INVALID_ADDRESS = 0x04U,
    BSP_FLASH_INVALID_SIZE    = 0x05U
} BSP_FLASH_Status;

/* FLASH操作类型枚举 */
typedef enum {
    BSP_FLASH_TYPE_ERASE_SECTOR  = 0x00U,
    BSP_FLASH_TYPE_PROGRAM_WORD  = 0x01U,
    BSP_FLASH_TYPE_PROGRAM_HALFWORD = 0x02U,
    BSP_FLASH_TYPE_PROGRAM_BYTE  = 0x03U
} BSP_FLASH_Type;

/* FLASH扇区枚举 (针对STM32F4系列) */
typedef enum {
    BSP_FLASH_SECTOR_0  = 0x00U,  /*!< Sector Number 0   */
    BSP_FLASH_SECTOR_1  = 0x01U,  /*!< Sector Number 1   */
    BSP_FLASH_SECTOR_2  = 0x02U,  /*!< Sector Number 2   */
    BSP_FLASH_SECTOR_3  = 0x03U,  /*!< Sector Number 3   */
    BSP_FLASH_SECTOR_4  = 0x04U,  /*!< Sector Number 4   */
    BSP_FLASH_SECTOR_5  = 0x05U,  /*!< Sector Number 5   */
    BSP_FLASH_SECTOR_6  = 0x06U,  /*!< Sector Number 6   */
    BSP_FLASH_SECTOR_7  = 0x07U,  /*!< Sector Number 7   */
    BSP_FLASH_SECTOR_8  = 0x08U,  /*!< Sector Number 8   */
    BSP_FLASH_SECTOR_9  = 0x09U,  /*!< Sector Number 9   */
    BSP_FLASH_SECTOR_10 = 0x0AU,  /*!< Sector Number 10  */
    BSP_FLASH_SECTOR_11 = 0x0BU   /*!< Sector Number 11  */
} BSP_FLASH_Sector;

/* FLASH错误代码 */
#define BSP_FLASH_ERROR_NONE         0x00000000U    /*!< No error                */
#define BSP_FLASH_ERROR_RD           0x00000001U    /*!< Read Protection error   */
#define BSP_FLASH_ERROR_PG           0x00000002U    /*!< Programming error       */
#define BSP_FLASH_ERROR_WRP          0x00000004U    /*!< Write protection error  */
#define BSP_FLASH_ERROR_OPTV         0x00000008U    /*!< Option validity error   */
#define BSP_FLASH_ERROR_SIZE         0x00000010U    /*!< Invalid Size error      */
#define BSP_FLASH_ERROR_SIZ          0x00000010U    /*!< Invalid Size error      */
#define BSP_FLASH_ERROR_PGS          0x00000020U    /*!< Programming sequence error */
#define BSP_FLASH_ERROR_MIS          0x00000040U    /*!< Fast programming data miss error */
#define BSP_FLASH_ERROR_FAST         0x00000080U    /*!< Fast programming error */
#define BSP_FLASH_ERROR_ENDHV        0x00000100U    /*!< End of high voltage error */
#define BSP_FLASH_ERROR_OPE          0x00000200U    /*!< Operation error */

/* FLASH地址范围定义 */
#define BSP_FLASH_BASE_ADDR          0x08000000U
#define BSP_FLASH_END_ADDR           0x081FFFFFU
#define BSP_FLASH_SECTOR_SIZE        0x00004000U    /* 16KB for sector 0-3 */
#define BSP_FLASH_SECTOR_SIZE_512K   0x00020000U    /* 128KB for sector 10-11 */

/* 函数声明 */

/**
 * @description: 擦除指定扇区
 * @param {BSP_FLASH_Sector} sector - 要擦除的扇区
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Erase_Sector(BSP_FLASH_Sector sector);

/**
 * @description: 在指定地址写入32位数据
 * @param {uint32_t} address - 写入地址 (必须是4字节对齐)
 * @param {uint32_t} data - 要写入的数据
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Write_Word(uint32_t address, uint32_t data);

/**
 * @description: 在指定地址写入16位数据
 * @param {uint32_t} address - 写入地址 (必须是2字节对齐)
 * @param {uint16_t} data - 要写入的数据
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Write_HalfWord(uint32_t address, uint16_t data);

/**
 * @description: 在指定地址写入8位数据
 * @param {uint32_t} address - 写入地址
 * @param {uint8_t} data - 要写入的数据
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Write_Byte(uint32_t address, uint8_t data);

/**
 * @description: 在指定地址写入数据块
 * @param {uint32_t} address - 写入地址
 * @param {uint8_t*} data - 要写入的数据指针
 * @param {uint32_t} size - 数据大小(字节)
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Write_Buffer(uint32_t address, uint8_t* data, uint32_t size);

/**
 * @description: 从指定地址读取32位数据
 * @param {uint32_t} address - 读取地址
 * @param {uint32_t*} data - 读取到的数据指针
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Read_Word(uint32_t address, uint32_t* data);

/**
 * @description: 从指定地址读取16位数据
 * @param {uint32_t} address - 读取地址
 * @param {uint16_t*} data - 读取到的数据指针
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Read_HalfWord(uint32_t address, uint16_t* data);

/**
 * @description: 从指定地址读取8位数据
 * @param {uint32_t} address - 读取地址
 * @param {uint8_t*} data - 读取到的数据指针
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Read_Byte(uint32_t address, uint8_t* data);

/**
 * @description: 从指定地址读取数据块
 * @param {uint32_t} address - 读取地址
 * @param {uint8_t*} data - 读取到的数据指针
 * @param {uint32_t} size - 数据大小(字节)
 * @return {BSP_FLASH_Status} 操作状态
 */
BSP_FLASH_Status BSP_FLASH_Read_Buffer(uint32_t address, uint8_t* data, uint32_t size);

/**
 * @description: 获取扇区大小
 * @param {BSP_FLASH_Sector} sector - 扇区编号
 * @return {uint32_t} 扇区大小(字节)
 */
uint32_t BSP_FLASH_Get_Sector_Size(BSP_FLASH_Sector sector);

/**
 * @description: 获取扇区起始地址
 * @param {BSP_FLASH_Sector} sector - 扇区编号
 * @return {uint32_t} 扇区起始地址
 */
uint32_t BSP_FLASH_Get_Sector_Address(BSP_FLASH_Sector sector);

/**
 * @description: 检查地址是否有效
 * @param {uint32_t} address - 要检查的地址
 * @return {uint8_t} 1表示有效，0表示无效
 */
uint8_t BSP_FLASH_Is_Address_Valid(uint32_t address);

#endif // _BSP_FLASH_H_