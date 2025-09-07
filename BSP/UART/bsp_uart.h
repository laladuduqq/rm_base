/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-07 12:41:34
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-07 21:06:23
 * @FilePath: /rm_base/BSP/UART/bsp_uart.h
 * @Description: 
 */
#ifndef _BSP_UART_H_
#define _BSP_UART_H_

#include "BSP_CONFIG.h"
#include "osal_def.h"
#include "usart.h"

/* UART事件定义 */
#define UART_RX_DONE_EVENT    (0x01 << 0)  // 接收完成事件
#define UART_TX_DONE_EVENT    (0x01 << 1)  // 发送完成事件
#define UART_ERR_EVENT        (0x01 << 2)  // uart错误事件


// 模式选择
typedef enum {
    UART_MODE_BLOCKING,
    UART_MODE_IT,
    UART_MODE_DMA
} UART_Mode;

// UART设备结构体
typedef struct {
    UART_HandleTypeDef *huart;
    
    // 接收相关（需要缓冲区管理）
    uint8_t (*rx_buf)[2];             // 指向外部定义的双缓冲区
    uint16_t rx_buf_size;             // 缓冲区大小
    volatile uint8_t rx_active_buf;   // 当前活动缓冲区
    uint16_t real_rx_len;             // 实际接收数据长度
    uint16_t expected_rx_len;         // 预期长度（0为不定长）

    // uart事件（用于接收和发送完成通知）
    osal_event_t uart_event;
    
    // 配置模式
    UART_Mode rx_mode;
    UART_Mode tx_mode;
    
    // 错误状态
    uint32_t error_status;
} UART_Device;

// 初始化配置结构体
typedef struct {
    UART_HandleTypeDef *huart;        // UART句柄
    uint8_t (*rx_buf)[2];             // 外部接收缓冲区指针
    uint16_t rx_buf_size;             // 接收缓冲区大小
    uint16_t expected_rx_len;         // 预期长度（0为不定长）
    UART_Mode rx_mode;                // 接收模式
    UART_Mode tx_mode;                // 发送模式
} UART_Device_init_config;


/**
 * @description: UART初始化函数
 * @details      初始化UART设备
 * @param        device：UART_Device指针
 * @param        config：UART_Device_init_config指针
 * @return       UART_Device*：初始化成功返回UART_Device指针，失败返回NULL
 */
UART_Device*  BSP_UART_Device_Init(UART_Device_init_config *config);
/**
 * @description: UART发送函数
 * @details      发送数据到UART设备
 * @param        inst：UART_Device指针
 * @param        data：要发送的数据指针
 * @param        len：数据长度
 * @return       int：实际发送的字节数
 */
int BSP_UART_Send(UART_Device *inst, uint8_t *data, uint16_t len);
/**
 * @description: UART接收函数
 * @details      从UART设备接收数据
 * @param        inst：UART_Device指针
 * @return       uint8_t*：指向接收数据的指针，失败返回NULL
 */
uint8_t* BSP_UART_Read(UART_Device *device);
/**
 * @description: UART反初始化函数
 * @details      释放UART设备资源，删除事件标志组和信号量
 * @param        inst：UART_Device指针
 * @return {*}
 */
void BSP_UART_Deinit(UART_Device *inst);


#endif // _BSP_UART_H_