#ifndef _BSP_CAN_H_
#define _BSP_CAN_H_

#include "BSP_CONFIG.h"
#include "osal_def.h"
#include "can.h"
#include <stdint.h>

/* 接收模式枚举 */
typedef enum {
    CAN_MODE_BLOCKING,
    CAN_MODE_IT
} CAN_Mode;

/* CAN设备实例结构体 */
typedef struct
{
    CAN_HandleTypeDef *can_handle;  // CAN句柄
    // 发送配置
    CAN_TxHeaderTypeDef txconf;     // 发送配置
    uint32_t tx_id;                 // 发送ID
    uint32_t tx_mailbox;            // 发送邮箱号
    uint8_t tx_buff[8];             // 发送缓冲区
    CAN_Mode tx_mode;
    // 接收配置
    uint32_t rx_id;                 // 接收ID
    uint8_t rx_buff[8];          // 接收缓冲区
    uint8_t rx_len;                 // 接收长度
    CAN_Mode rx_mode;
    // 事件
    uint32_t eventflag;
} Can_Device;

/* 初始化配置结构体 */
typedef struct
{
    CAN_HandleTypeDef *can_handle;
    uint32_t tx_id;
    uint32_t rx_id;
    CAN_Mode tx_mode;
    CAN_Mode rx_mode;
} Can_Device_Init_Config_s;

/* CAN总线管理结构 */
typedef struct {
    CAN_HandleTypeDef *hcan;
    Can_Device devices[MAX_DEVICES_PER_CAN_BUS];
    osal_mutex_t bus_mutex;
    uint8_t device_count;
} CANBusManager;

typedef struct
{
    CAN_HandleTypeDef *can_handle;
    CAN_TxHeaderTypeDef txconf;     // 发送配置
    uint32_t tx_mailbox;            // 发送邮箱号
    uint8_t tx_buff[8];             // 发送缓冲区
} CanTxMessage_t;

typedef struct
{
    CAN_HandleTypeDef *can_handle;
    CAN_RxHeaderTypeDef rxconf;     // 接收配置
    uint8_t rx_buff[8];             // 接收缓冲区
} CanRxMessage_t;

/**
 * @description: 初始化CAN设备
 * @param {Can_Device_Init_Config_s*} config
 * @return {Can_Device*}，CAN设备指针
 */
Can_Device* BSP_CAN_Device_Init(Can_Device_Init_Config_s *config);
/**
 * @description: 发送CAN设备
 * @param {Can_Device*} dev
 * @return {osal_status_t}，osal_scucess表示成功，其他表示失败
 */
osal_status_t BSP_CAN_SendDevice(Can_Device *device);
/**
 * @description: 发送CAN设备
 * @param {CanTxMessage_t *}，CAN发送消息结构体指针 
 * @param {mode}，发送模式CAN_MODE_BLOCKING/CAN_MODE_IT
 * @return {osal_status_t}，osal_scucess表示成功，其他表示失败
 */
osal_status_t BSP_CAN_SendMessage(CanTxMessage_t *tx_message,uint8_t mode);
/**
 * @description: 读取单个CAN设备数据
 * @param {Can_Device*} device
 * @param {osal_tick_t} timeout - 超时时间
 * @return {osal_status_t}，osal_scucess表示成功，其他表示失败
 */
osal_status_t BSP_CAN_ReadSingleDevice(Can_Device *device,osal_tick_t timeout);
/**
 * @description: 读取多个CAN设备数据
 * @note 此函数只支持it模式，blocking模式请使用BSP_CAN_ReadSingleDevice
 * @param {Can_Device**} devices - 设备指针数组
 * @param {uint8_t} device_count - 设备数量
 * @param {osal_tick_t} timeout - 超时时间
 * @return {uint32_t}，返回触发事件的设备flag，0表示超时或错误
 */
uint32_t BSP_CAN_ReadMultipleDevice(Can_Device** devices, uint8_t device_count, osal_tick_t timeout);

#endif // _BSP_CAN_H_