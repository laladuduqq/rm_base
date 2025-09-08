#ifndef _BSP_SPI_H_
#define _BSP_SPI_H_

#include "BSP_CONFIG.h"
#include "osal_def.h"
#include "spi.h"


/* SPI事件定义 */
#define SPI_EVENT_TX_DONE_EVENT    (0x01 << 0)
#define SPI_EVENT_RX_DONE_EVENT    (0x01 << 1)
#define SPI_EVENT_TX_RX_DONE_EVENT (0x01 << 2)
#define SPI_EVENT_ERR_EVENT        (0x01 << 3)

/* 传输模式枚举 */
typedef enum {
    SPI_MODE_BLOCKING,
    SPI_MODE_IT,
    SPI_MODE_DMA
} SPI_Mode;


/* SPI设备实例结构体 */
typedef struct {
    SPI_HandleTypeDef* hspi;      // SPI句柄
    GPIO_TypeDef* cs_port;        // 片选端口
    uint16_t cs_pin;              // 片选引脚
    SPI_Mode tx_mode;             // 发送模式
    SPI_Mode rx_mode;             // 接收模式
} SPI_Device;

/* 初始化配置结构体 */
typedef struct {
    SPI_HandleTypeDef* hspi;
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
    SPI_Mode tx_mode;
    SPI_Mode rx_mode;
} SPI_Device_Init_Config;

/* SPI总线管理结构 */
typedef struct {
    SPI_HandleTypeDef* hspi;                    // SPI句柄
    SPI_Device devices[MAX_DEVICES_PER_BUS];    // 设备列表
    osal_event_t bus_event;                     // 总线事件
    osal_mutex_t bus_mutex;                     // 总线互斥锁
    uint8_t device_count;                       // 当前设备数量
    volatile SPI_Device* active_dev;            // 当前活动设备
} SPI_Bus_Manager;


/**
 * @description:  初始化SPI设备
 * @param {SPI_Device_Init_Config*} config，初始化配置，见SPI_Device_Init_Config结构体
 * @return {SPI_Device*} 成功返回设备实例指针，失败返回NULL
 */
SPI_Device* BSP_SPI_Device_Init(SPI_Device_Init_Config* config);
/**
 * @description:  销毁SPI设备实例
 * @param {SPI_Device*} dev，要销毁的设备实例
 * @return {*}
 */
void BSP_SPI_Device_DeInit(SPI_Device* dev);
/**
 * @description: SPI发送和接受数据
 * @details      发送数据并接收响应，适用于全双工通信
 * @details      如果只需要发送或接收，请使用BSP_SPI_Transmit或BSP_SPI_Receive
 * @param {SPI_Device*} dev，要操作的SPI设备实例
 * @param {uint8_t*} tx_data，发送数据指针
 * @param {uint8_t*} rx_data，接收数据指针
 * @param {uint16_t} size，数据大小，注意这里是发送和接收同时进行，所以发送和接收的数据量是相等的
 * @return {osal_status_t} 返回操作状态,OSAL_SUCCESS表示成功，其他值表示失败
 */
osal_status_t BSP_SPI_TransReceive(SPI_Device* dev, const uint8_t* tx_data, uint8_t* rx_data, uint16_t size);
/**
 * @description: SPI发送数据
 * @details      适用于单向发送数据
 * @param {SPI_Device*} dev，要操作的SPI设备实例
 * @param {uint8_t*} tx_data，发送数据指针
 * @param {uint16_t} size，数据大小
 * @return {osal_status_t} 返回操作状态,OSAL_SUCCESS表示成功，其他值表示失败
 */
osal_status_t BSP_SPI_Transmit(SPI_Device* dev, const uint8_t* tx_data, uint16_t size);
/**
 * @description: SPI接收数据
 * @details      适用于单向接收数据
 * @param {SPI_Device*} dev，要操作的SPI设备实例
 * @param {uint8_t*} rx_data，接收数据指针
 * @param {uint16_t} size，数据大小
 * @return {osal_status_t} 返回操作状态,OSAL_SUCCESS表示成功，其他值表示失败
 */
osal_status_t BSP_SPI_Receive(SPI_Device* dev, uint8_t* rx_data, uint16_t size);
/**
 * @description: SPI发送和接收数据，适用于需要连续发送两次数据的场景
 * @details      例如在某些协议中需要先发送命令再发送数据
 * @details      该函数是阻塞发送
 * @param {SPI_Device*} dev，要操作的SPI设备实例
 * @param {const uint8_t*} tx_data1，第一次发送的数据指针
 * @param {uint16_t} size1，第一次发送的数据大小
 * @param {const uint8_t*} tx_data2，第二次发送的数据指针
 * @param {uint16_t} size2，第二次发送的数据大小
 * @return {osal_status_t} 返回操作状态,OSAL_SUCCESS表示成功，其他值表示失败
 */
osal_status_t BSP_SPI_TransAndTrans(SPI_Device* dev, const uint8_t* tx_data1, uint16_t size1, 
                                       const uint8_t* tx_data2, uint16_t size2);

#endif // _BSP_SPI_H_