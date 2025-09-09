/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-09 17:03:48
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-10 00:01:47
 * @FilePath: /rm_base/BSP/CAN/bsp_can.c
 * @Description: 
 */

#include "bsp_can.h"
#include "osal_def.h"
#include "tx_port.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* CAN 事件定义 */
#define CAN_EVENT_TX_MAILBOX0_DONE (0x01 << 0)
#define CAN_EVENT_TX_MAILBOX1_DONE (0x01 << 1)
#define CAN_EVENT_TX_MAILBOX2_DONE (0x01 << 2)

// 全局CAN事件
static osal_event_t global_can_event;
// CAN总线管理器数组
static CANBusManager can_bus_managers[CAN_BUS_NUM];
// 静态声明32个事件标志
static uint32_t can_device_event_flags[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};
// CAN过滤器索引
static uint8_t can1_filter_idx = 0, can2_filter_idx = 14; // 0-13给can1用,14-27给can2用


/**
 * @description: 添加CAN过滤器
 * @param {Can_Device*} device
 * @return {*}
 */
static void CANAddFilter(Can_Device *device)
{
    CAN_FilterTypeDef can_filter_conf;
    
    can_filter_conf.FilterMode = CAN_FILTERMODE_IDLIST;                                                       // 使用id list模式,即只有将rxid添加到过滤器中才会接收到,其他报文会被过滤
    can_filter_conf.FilterScale = CAN_FILTERSCALE_16BIT;                                                      // 使用16位id模式,即只有低16位有效
    can_filter_conf.FilterFIFOAssignment = (device->tx_id & 1) ? CAN_RX_FIFO0 : CAN_RX_FIFO1;              // 奇数id的模块会被分配到FIFO0,偶数id的模块会被分配到FIFO1
    can_filter_conf.SlaveStartFilterBank = 14;                                                                // 从第14个过滤器开始配置从机过滤器(在STM32的BxCAN控制器中CAN2是CAN1的从机)
    can_filter_conf.FilterIdLow = device->rx_id << 5;                                                      // 过滤器寄存器的低16位,因为使用STDID,所以只有低11位有效,高5位要填0
    can_filter_conf.FilterBank = device->can_handle == &hcan1 ? (can1_filter_idx++) : (can2_filter_idx++); // 根据can_handle判断是CAN1还是CAN2,然后自增
    can_filter_conf.FilterActivation = CAN_FILTER_ENABLE;                                                     // 启用过滤器

    HAL_CAN_ConfigFilter(device->can_handle, &can_filter_conf);
}

/**
 * @description: 检查设备ID冲突
 * @param {CANBusManager*} bus
 * @param {uint16_t} tx_id
 * @param {uint16_t} rx_id
 * @return {bool} true表示有冲突，false表示无冲突
 */
static bool check_device_id_conflict(CANBusManager *bus, uint16_t tx_id, uint16_t rx_id) {
    for(uint8_t i = 0; i < MAX_DEVICES_PER_CAN_BUS; i++) {
        if(bus->devices[i].can_handle != NULL) {  // 使用can_handle判断设备是否存在
            if(bus->devices[i].rx_id == rx_id) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @description: 初始化全局CAN事件和事件标志
 * @return {*}
 */
static void BSP_CAN_Init_Global(void)
{
    static uint8_t initialized = 0;
    
    if (!initialized) {
        // 创建全局CAN事件
        osal_event_create(&global_can_event, "GlobalCANEvent");
        
        initialized = 1;
    }
}

/**
 * @description: 初始化CAN总线管理器
 * @param {CAN_HandleTypeDef*} hcan
 * @return {*}
 */
static CANBusManager* BSP_CAN_InitBusManager(CAN_HandleTypeDef *hcan)
{
    // 查找现有的总线管理器
    for (int i = 0; i < CAN_BUS_NUM; i++) {
        if (can_bus_managers[i].hcan == hcan) {
            return &can_bus_managers[i];
        }
    }
    
    // 查找空闲的总线管理器
    for (int i = 0; i < CAN_BUS_NUM; i++) {
        if (can_bus_managers[i].hcan == NULL) {
            can_bus_managers[i].hcan = hcan;
            can_bus_managers[i].device_count = 0;
            
            // 创建总线互斥锁
            char mutex_name[16];
            snprintf(mutex_name, sizeof(mutex_name), "CAN_Mutex_%d", i);
            osal_mutex_create(&can_bus_managers[i].bus_mutex, mutex_name);
            
            return &can_bus_managers[i];
        }
    }
    
    return NULL;
}

/**
 * @description: 初始化CAN设备
 * @param {Can_Device_Init_Config_s*} config
 * @return {*}
 */
Can_Device* BSP_CAN_Device_Init(Can_Device_Init_Config_s *config)
{
    if (config == NULL || config->can_handle == NULL) {
        return NULL;
    }
    
    // 初始化全局资源
    BSP_CAN_Init_Global();
    
    // 获取或初始化总线管理器
    CANBusManager *bus_manager = BSP_CAN_InitBusManager(config->can_handle);
    if (bus_manager == NULL || bus_manager->device_count >= MAX_DEVICES_PER_CAN_BUS) {
        return NULL;
    }
    
    // 检查ID冲突
    if (check_device_id_conflict(bus_manager, config->tx_id, config->rx_id)) {
        return NULL;
    }
    
    // 获取设备实例
    Can_Device *device = &bus_manager->devices[bus_manager->device_count];
    memset(device, 0, sizeof(Can_Device));
    
    // 配置设备参数
    device->can_handle = config->can_handle;
    device->tx_id = config->tx_id;
    device->rx_id = config->rx_id;
    device->tx_mode = config->tx_mode;
    device->rx_mode = config->rx_mode;
    
    // 配置发送参数
    device->txconf.StdId = config->tx_id;
    device->txconf.ExtId = 0;
    device->txconf.IDE = CAN_ID_STD;
    device->txconf.RTR = CAN_RTR_DATA;
    device->txconf.DLC = 8;
    device->txconf.TransmitGlobalTime = DISABLE;
    
    // 分配事件标志
    int flag_index = bus_manager->device_count + (bus_manager - can_bus_managers) * MAX_DEVICES_PER_CAN_BUS;
    if (flag_index < 32) {
        device->eventflag = can_device_event_flags[flag_index];
    } else {
        return NULL; // 超出可用事件标志范围
    }
    
    // 添加CAN过滤器
    CANAddFilter(device);
    
    // 增加设备计数
    bus_manager->device_count++;
    
    // 启动CAN接收中断
    if (config->rx_mode == CAN_MODE_IT) {
        HAL_CAN_ActivateNotification(config->can_handle, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING);
    }
    
    return device;
}

osal_status_t BSP_CAN_SendDevice(Can_Device *device)
{
    if (device == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef status;
    
    // 获取互斥锁保护
    CANBusManager *bus_manager = NULL;
    for (int i = 0; i < CAN_BUS_NUM; i++) {
        if (can_bus_managers[i].hcan == device->can_handle) {
            bus_manager = &can_bus_managers[i];
            break;
        }
    }
    
    if (bus_manager != NULL) {
        if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
            return OSAL_ERROR;
        }
    }
    
    // 发送数据
    status = HAL_CAN_AddTxMessage(device->can_handle, &device->txconf, 
                                 device->tx_buff, &device->tx_mailbox);
    
    // 释放互斥锁
    if (bus_manager != NULL) {
        osal_mutex_unlock(&bus_manager->bus_mutex);
    }
    
    if (status != HAL_OK) {
        return OSAL_ERROR;
    }
    
    // 如果是中断模式，则等待发送完成事件
    if (device->tx_mode == CAN_MODE_IT) {
        unsigned int actual_flags;
        uint32_t tx_event_flag = 0;
        
        // 根据邮箱号选择对应的事件标志
        switch (device->tx_mailbox) {
            case CAN_TX_MAILBOX0:
                tx_event_flag = CAN_EVENT_TX_MAILBOX0_DONE;
                break;
            case CAN_TX_MAILBOX1:
                tx_event_flag = CAN_EVENT_TX_MAILBOX1_DONE;
                break;
            case CAN_TX_MAILBOX2:
                tx_event_flag = CAN_EVENT_TX_MAILBOX2_DONE;
                break;
            default:
                return OSAL_ERROR;
        }
        
        // 等待发送完成事件
        osal_status_t wait_status = osal_event_wait(&global_can_event, 
                                                   tx_event_flag,
                                                   OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                   OSAL_WAIT_FOREVER, 
                                                   &actual_flags);
        
        if (wait_status == OSAL_SUCCESS && (actual_flags & tx_event_flag)) {
            return OSAL_SUCCESS;
        } else {
            return OSAL_ERROR;
        }
    }
    
    return OSAL_SUCCESS;
}

/**
 * @description: 发送CAN消息
 * @param {CanTxMessage_t*} tx_message
 * @return {*}
 */
osal_status_t BSP_CAN_SendMessage(CanTxMessage_t *tx_message,uint8_t mode)
{
    if (tx_message == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    // 获取互斥锁保护
    CANBusManager *bus_manager = NULL;
    for (int i = 0; i < CAN_BUS_NUM; i++) {
        if (can_bus_managers[i].hcan == tx_message->can_handle) {
            bus_manager = &can_bus_managers[i];
            break;
        }
    }
    
    if (bus_manager != NULL) {
        if (osal_mutex_lock(&bus_manager->bus_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
            return OSAL_ERROR;
        }
    }
    
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(tx_message->can_handle, &tx_message->txconf, 
                                                   tx_message->tx_buff, &tx_message->tx_mailbox);
    
    // 释放互斥锁
    if (bus_manager != NULL) {
        osal_mutex_unlock(&bus_manager->bus_mutex);
    }
                                                   
    if (status != HAL_OK) {
        return OSAL_ERROR;
    }
    
    // 如果是中断模式，则等待发送完成事件
    if (mode == CAN_MODE_IT) {
        unsigned int actual_flags;
        uint32_t tx_event_flag = 0;
        
        // 根据邮箱号选择对应的事件标志
        switch (tx_message->tx_mailbox) {
            case CAN_TX_MAILBOX0:
                tx_event_flag = CAN_EVENT_TX_MAILBOX0_DONE;
                break;
            case CAN_TX_MAILBOX1:
                tx_event_flag = CAN_EVENT_TX_MAILBOX1_DONE;
                break;
            case CAN_TX_MAILBOX2:
                tx_event_flag = CAN_EVENT_TX_MAILBOX2_DONE;
                break;
            default:
                return OSAL_ERROR;
        }
        
        // 等待发送完成事件
        osal_status_t wait_status = osal_event_wait(&global_can_event, 
                                                   tx_event_flag,
                                                   OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                                   OSAL_WAIT_FOREVER, 
                                                   &actual_flags);
        
        if (wait_status == OSAL_SUCCESS && (actual_flags & tx_event_flag)) {
            return OSAL_SUCCESS;
        } else {
            return OSAL_ERROR;
        }
    }
    
    return OSAL_SUCCESS;
}


osal_status_t BSP_CAN_ReadSingleDevice(Can_Device *device, osal_tick_t timeout)
{
    if (device == NULL) {
        return OSAL_INVALID_PARAM;
    }
    
    // 阻塞模式下直接轮询接收数据
    if (device->rx_mode == CAN_MODE_BLOCKING) {
        CAN_RxHeaderTypeDef rx_header;
        uint8_t rx_data[8];
        HAL_StatusTypeDef status = HAL_OK;
        
        // 轮询等待接收数据
        if ((device->tx_id & 1))
        {
            status = HAL_CAN_GetRxMessage(device->can_handle, CAN_RX_FIFO0, 
                                        &rx_header, rx_data);
        }
        else
        {
            status = HAL_CAN_GetRxMessage(device->can_handle, CAN_RX_FIFO1, 
                                        &rx_header, rx_data);            
        }

        
        if (status == HAL_OK && rx_header.StdId == device->rx_id) {
            // 更新设备缓冲区
            memcpy(device->rx_buff, rx_data, rx_header.DLC);
            device->rx_len = rx_header.DLC;
            return OSAL_SUCCESS;
        }
        
        return OSAL_ERROR;
    }
    
    // 等待设备事件（中断模式）
    unsigned int actual_flags;
    osal_status_t status = osal_event_wait(&global_can_event, device->eventflag,
                                          OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                                          timeout, &actual_flags);
    
    if (status == OSAL_SUCCESS && (actual_flags & device->eventflag)) {
        return OSAL_SUCCESS;
    }
    
    return OSAL_ERROR;
}

uint32_t BSP_CAN_ReadMultipleDevice(Can_Device** devices, uint8_t device_count, osal_tick_t timeout)
{
    if (devices == NULL || device_count == 0) {
        return 0;
    }

    
    // 构建设备事件标志组合（中断模式）
    uint32_t combined_event_flags = 0;
    for (int i = 0; i < device_count; i++) {
        if (devices[i] != NULL) {combined_event_flags |= devices[i]->eventflag;}
    }
    
    if (combined_event_flags == 0) {
        return 0;
    }
    
    // 等待任意一个设备的事件
    unsigned int actual_flags;
    osal_status_t status = osal_event_wait(&global_can_event, combined_event_flags,
                                          OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,timeout, 
                                          &actual_flags);
    if (status == OSAL_SUCCESS) {
        // 查找触发事件的设备并返回其eventflag
        for (int i = 0; i < device_count; i++) {
            if (devices[i] != NULL && (actual_flags & devices[i]->eventflag)) {
                return devices[i]->eventflag;
            }
        }
    }
    return 0;
}

/**
 * @description: CAN接收中断回调函数
 * @param {CAN_HandleTypeDef*} hcan
 * @param {uint32_t} RxFifo
 * @return {*}
 */
static void BSP_CAN_RxCallback(CAN_HandleTypeDef *hcan, uint32_t RxFifo)
{
    // 查找对应的总线管理器
    CANBusManager *bus_manager = NULL;
    for (int i = 0; i < CAN_BUS_NUM; i++) {
        if (can_bus_managers[i].hcan == hcan) {
            bus_manager = &can_bus_managers[i];
            break;
        }
    }
    
    if (bus_manager == NULL) {
        return;
    }
    
    // 处理接收到的消息
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    
    while (HAL_CAN_GetRxFifoFillLevel(hcan, RxFifo) > 0) {
        if (HAL_CAN_GetRxMessage(hcan, RxFifo, &rx_header, rx_data) == HAL_OK) {
            // 查找对应的设备
            for (int i = 0; i < bus_manager->device_count; i++) {
                Can_Device *device = &bus_manager->devices[i];
                if (device->rx_id == rx_header.StdId) {
                    // 更新设备缓冲区
                    memcpy(device->rx_buff, rx_data, rx_header.DLC);
                    device->rx_len = rx_header.DLC;
                    // 设置设备事件标志
                    osal_event_set(&global_can_event, device->eventflag);
                    break;
                }
            }
        }
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    BSP_CAN_RxCallback(hcan, CAN_RX_FIFO0);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    BSP_CAN_RxCallback(hcan, CAN_RX_FIFO1);
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    // 设置发送完成事件
    osal_event_set(&global_can_event, CAN_EVENT_TX_MAILBOX0_DONE);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    // 设置发送完成事件
    osal_event_set(&global_can_event, CAN_EVENT_TX_MAILBOX1_DONE);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    // 设置发送完成事件
    osal_event_set(&global_can_event, CAN_EVENT_TX_MAILBOX2_DONE);
}