/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-07 12:41:40
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-07 18:54:59
 * @FilePath: /rm_base/BSP/UART/bsp_uart.c
 * @Description: 
 */
#include "bsp_uart.h"
#include "osal_def.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static UART_Device registered_uart[UART_MAX_INSTANCE_NUM] = {0}; // 结构体数组
static bool uart_used[UART_MAX_INSTANCE_NUM] = {false};         // 添加使用状态标记

/* 内部函数 */
static void Start_Rx(UART_Device *device) {
    // 阻塞模式不需要启动后台接收
    if (device->rx_mode == UART_MODE_BLOCKING) {
        return;
    }
    uint8_t next_buf = !device->rx_active_buf;
    uint16_t max_len = device->rx_buf_size;
    HAL_StatusTypeDef status = HAL_OK;
    switch(device->rx_mode){
        case UART_MODE_BLOCKING:
            break;
        case UART_MODE_IT:
            status = HAL_UARTEx_ReceiveToIdle_IT(device->huart, (uint8_t*)&device->rx_buf[next_buf], 
                device->expected_rx_len ? device->expected_rx_len : max_len);
            break;
        case UART_MODE_DMA:
            status = HAL_UARTEx_ReceiveToIdle_DMA(device->huart, (uint8_t*)&device->rx_buf[next_buf], 
                device->expected_rx_len ? device->expected_rx_len : max_len);
            break;
    }
    // 只有在启动成功时才切换缓冲区
    if (status == HAL_OK && device->rx_mode != UART_MODE_BLOCKING) {
        device->rx_active_buf = next_buf;
    }
}

static void Process_Rx_Complete(UART_Device *device, uint16_t Size) {
    // 计算实际接收长度
    if(device->expected_rx_len == 0){
        if(device->rx_mode == UART_MODE_DMA){
            // 获取单缓冲区大小
            Size = device->rx_buf_size - __HAL_DMA_GET_COUNTER(device->huart->hdmarx);
            // 长度校验
            if(Size > device->rx_buf_size){Size = device->rx_buf_size;}
        }
    }
    device->real_rx_len = Size;
    // 事件通知
    osal_event_set(&device->uart_event, UART_RX_DONE_EVENT);
}

/* 对外接口函数 */
UART_Device* BSP_UART_Device_Init(UART_Device_init_config *config){
    // 参数检查
    if (!config || !config->huart) {
        return NULL;
    }
    // 检查实例是否已存在
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == config->huart)
        {   
            return NULL;
        }
    }
    // 查找空闲槽位
    int free_index = -1;
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(!uart_used[i]){
            free_index = i;
            break;
        }
    }
    if(free_index == -1) {
        return NULL;
    }
    // 初始化参数
    UART_Device *device = &registered_uart[free_index];
    memset(device, 0, sizeof(UART_Device));
    device->huart = config->huart;
    // 缓冲区配置部分
    if (config->rx_buf != NULL) {
        device->rx_buf = (uint8_t (*)[2])config->rx_buf;
        device->rx_buf_size = config->rx_buf_size;
    } 
    else{
        return NULL;
    }
    // 配置接收长度
    device->expected_rx_len = config->expected_rx_len;
    if (device->expected_rx_len > device->rx_buf_size) {device->expected_rx_len = device->rx_buf_size;} // 确保不超过缓冲区大小
    // 配置模式
    device->rx_mode = config->rx_mode;
    device->tx_mode = config->tx_mode;
    // 创建具有唯一标识的事件名称
    char event_name[16];
    // 使用UART实例的地址后几位作为标识，确保唯一性
    snprintf(event_name, sizeof(event_name), "uart_%04X", (unsigned int)((uintptr_t)device->huart & 0xFFFF));
    // 创建事件组
    osal_status_t status = osal_event_create(&device->uart_event, event_name);
    if (status != OSAL_SUCCESS) {return NULL;}
    // 增加使用计数
    uart_used[free_index] = true;
    Start_Rx(device);
    return device;
}
int BSP_UART_Send(UART_Device *inst, uint8_t *data, uint16_t len)
{
    if (inst == NULL || data == NULL || len == 0) {
        return -1;
    }

    HAL_StatusTypeDef hal_status;

    // 根据发送模式选择发送方式
    switch (inst->tx_mode) {
        case UART_MODE_BLOCKING:
            hal_status = HAL_UART_Transmit(inst->huart, data, len, HAL_MAX_DELAY);
            return (hal_status == HAL_OK) ? len : -1;
        case UART_MODE_IT:
            hal_status = HAL_UART_Transmit_IT(inst->huart, data, len);
            break;
        case UART_MODE_DMA:
            hal_status = HAL_UART_Transmit_DMA(inst->huart, data, len);
            break;

        default:
            return -1;
    }

    // 对于非阻塞模式，等待发送完成事件
    if (hal_status == HAL_OK) {
        unsigned int actual_flags;
        osal_status_t status = osal_event_wait(&inst->uart_event, UART_TX_DONE_EVENT,
                                              OSAL_EVENT_WAIT_FLAG_OR, OSAL_WAIT_FOREVER, &actual_flags);
        // 清除事件标志
        osal_event_clear(&inst->uart_event, UART_TX_DONE_EVENT);
        
        return (status == OSAL_SUCCESS) ? len : -1;
    }

    return -1;
}
int BSP_UART_Read(UART_Device *inst, uint8_t *data)
{
    if (inst == NULL || data == NULL) {
        return -1;
    }
    // 对于阻塞模式，直接接收数据
    if (inst->rx_mode == UART_MODE_BLOCKING) {
        uint16_t max_len = inst->expected_rx_len ? inst->expected_rx_len : inst->rx_buf_size;
        HAL_StatusTypeDef status = HAL_UART_Receive(inst->huart, data, max_len, HAL_MAX_DELAY);
        if (status == HAL_OK) {
            return max_len;
        }
        return -1;
    }
    else
    {
        // 使用memcpy复制非活动缓冲区的数据
        memcpy(data, inst->rx_buf[!inst->rx_active_buf], inst->real_rx_len);
        return inst->real_rx_len;
    }
}
void BSP_UART_Deinit(UART_Device *device) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(&registered_uart[i] == device){
            HAL_UART_Abort(device->huart);
            osal_event_delete(&device->uart_event);
            memset(device, 0, sizeof(UART_Device));
            uart_used[i] = false;
            break;
        }
    }
}

/* 中断函数 */

// 接收完成回调
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == huart){
            Process_Rx_Complete(&registered_uart[i], Size);
            Start_Rx(&registered_uart[i]); // 重启接收
            break;
        }
    }
}

//发送完成回调
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == huart){
            osal_event_set(&registered_uart[i].uart_event, UART_TX_DONE_EVENT);
            break;
        }
    }
}


// 错误处理回调
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == huart){     
            osal_event_set(&registered_uart[i].uart_event, UART_ERR_EVENT);   
            HAL_UART_AbortReceive(huart);
            Start_Rx(&registered_uart[i]);
            break;
        }
    }
}


