# BSP CAN 驱动文档

## 概述

BSP CAN 是一个基于 OSAL（操作系统抽象层）的 CAN 驱动程序，提供了统一的接口来操作 STM32 微控制器上的 CAN 设备。该驱动支持阻塞和中断两种工作模式，并利用事件机制实现异步通信。

## 特性

- 支持阻塞和中断两种工作模式
- 基于 OSAL 事件的异步通知机制
- 支持多个 CAN 设备实例同时工作
- 自动管理 CAN 总线互斥访问
- 支持单个和多个设备数据接收
- 自动配置 CAN 过滤器
  
  ## 数据结构
  
  ### CAN_Mode
  
  ```c
  typedef enum {
      CAN_MODE_BLOCKING,  // 阻塞模式
      CAN_MODE_IT         // 中断模式
  } CAN_Mode;
  ```
  
  ### Can_Device
  
  ```c
  typedef struct {
      CAN_HandleTypeDef *can_handle;  // CAN句柄
      // 发送配置
      CAN_TxHeaderTypeDef txconf;     // 发送配置
      uint32_t tx_id;                 // 发送ID
      uint32_t tx_mailbox;            // 发送邮箱号
      uint8_t tx_buff[8];             // 发送缓冲区
      CAN_Mode tx_mode;
      // 接收配置
      uint32_t rx_id;                 // 接收ID
      uint8_t rx_buff[8];             // 接收缓冲区
      uint8_t rx_len;                 // 接收长度
      CAN_Mode rx_mode;
      // 事件
      uint32_t eventflag;
  } Can_Device;
  ```
  
  ### Can_Device_Init_Config_s
  
  ```c
  typedef struct {
      CAN_HandleTypeDef *can_handle;
      uint32_t tx_id;
      uint32_t rx_id;
      CAN_Mode tx_mode;
      CAN_Mode rx_mode;
  } Can_Device_Init_Config_s;
  ```
  
  ### CanTxMessage_t
  
  ```c
  typedef struct {
      CAN_HandleTypeDef *can_handle;
      CAN_TxHeaderTypeDef txconf;     // 发送配置
      uint32_t tx_mailbox;            // 发送邮箱号
      uint8_t tx_buff[8];             // 发送缓冲区
  } CanTxMessage_t;
  ```
  
  ### CanRxMessage_t
  
  ```c
  typedef struct {
      CAN_HandleTypeDef *can_handle;
      CAN_RxHeaderTypeDef rxconf;     // 接收配置
      uint8_t rx_buff[8];             // 接收缓冲区
  } CanRxMessage_t;
  ```
  
  ## 事件定义
  
  ```c
  #define CAN_EVENT_TX_MAILBOX0_DONE    (0x01 << 0)  // 发送邮箱0完成事件
  #define CAN_EVENT_TX_MAILBOX1_DONE    (0x01 << 1)  // 发送邮箱1完成事件
  #define CAN_EVENT_TX_MAILBOX2_DONE    (0x01 << 2)  // 发送邮箱2完成事件
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
  ```
  
  ## API 接口
  
  ```c
  Can_Device* BSP_CAN_Device_Init(Can_Device_Init_Config_s *config);
  ```
  
  ```c
  osal_status_t BSP_CAN_SendDevice(Can_Device *device);
  ```
  
  ```c
  osal_status_t BSP_CAN_SendMessage(CanTxMessage_t *tx_message, uint8_t mode);
  ```
  
  ```c
  osal_status_t BSP_CAN_ReadSingleDevice(Can_Device *device, osal_tick_t timeout);
  ```
  
  ```c
  uint32_t BSP_CAN_ReadMultipleDevice(Can_Device** devices, uint8_t device_count, osal_tick_t timeout);
  ```
  
  ## 使用示例
  
  ### 阻塞模式/中断模式使用（单设备）
  
  ```c
  // 1. 定义设备配置
  Can_Device_Init_Config_s config = {
      .can_handle = &hcan1,
      .tx_id = 0x123,
      .rx_id = 0x456,
      .tx_mode = CAN_MODE_BLOCKING,
      .rx_mode = CAN_MODE_BLOCKING
  };
  
  // 2. 初始化设备
  Can_Device* device = BSP_CAN_Device_Init(&config);
  if (device == NULL) {
      // 错误处理
      return;
  }
  
  // 3. 准备发送数据
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  memcpy(device->tx_buff, data, sizeof(data));
  
  // 4. 发送数据
  if (BSP_CAN_SendDevice(device) != OSAL_SUCCESS) {
      // 错误处理
      return;
  }
  
  // 5. 接收数据（阻塞方式）
  if (BSP_CAN_ReadSingleDevice(device, 1000) == OSAL_SUCCESS) {
      // 处理接收到的数据
      uint8_t *rx_data = device->rx_buff;
      uint8_t len = device->rx_len;
      process_data(rx_data, len);
  }
  ```
  
  ### 阻塞模式/中断模式使用（多设备）
  
  ```c
  // 1. 初始化多个设备
  Can_Device* devices[3];
  Can_Device_Init_Config_s configs[3] = {
      {
          .can_handle = &hcan1,
          .tx_id = 0x123,
          .rx_id = 0x456,
          .tx_mode = CAN_MODE_IT,
          .rx_mode = CAN_MODE_IT
      },
      {
          .can_handle = &hcan1,
          .tx_id = 0x124,
          .rx_id = 0x457,
          .tx_mode = CAN_MODE_IT,
          .rx_mode = CAN_MODE_IT
      },
      {
          .can_handle = &hcan2,
          .tx_id = 0x125,
          .rx_id = 0x458,
          .tx_mode = CAN_MODE_IT,
          .rx_mode = CAN_MODE_IT
      }
  };
  
  for (int i = 0; i < 3; i++) {
      devices[i] = BSP_CAN_Device_Init(&configs[i]);
      if (devices[i] == NULL) {
          // 错误处理
          return;
      }
  }
  
  // 2. 等待任一设备接收数据
  uint32_t triggered_flag = BSP_CAN_ReadMultipleDevice(devices, 3, 1000);
  if (triggered_flag != 0) {
      // 处理接收到的数据
      // 查找触发事件的设备
      for (int i = 0; i < 3; i++) {
          if (devices[i] != NULL && devices[i]->eventflag == triggered_flag) {
              process_data(devices[i]->rx_buff, devices[i]->rx_len);
              break;
          }
      }
  }
  ```
  
  ## 工作原理
  
  ### 阻塞模式
  - 发送和接收都采用轮询方式
  - 发送时直接调用 HAL 库函数添加发送消息
  - 接收时直接调用 HAL 库函数获取接收消息
  
  ### 中断模式
  
  - 发送时通过邮箱机制异步发送，发送完成后通过事件通知
  - 接收时通过 FIFO 中断机制，接收到数据后通过事件通知
  - BSP_CAN_ReadSingleDevice 和 BSP_CAN_ReadMultipleDevice 函数等待事件并返回接收到的数据
  
  ## 注意事项
  
  1. **CAN过滤器**：驱动会自动为每个设备配置CAN过滤器，只接收指定ID的数据
  
  2. **阻塞模式**：在阻塞模式下，发送和接收函数会直接使用HAL库函数进行操作，不涉及事件机制
  
  3. **中断模式**：在中断模式下，发送和接收都会通过事件机制进行异步通知
  
  4. **内存管理**：驱动使用静态内存管理，避免动态内存分配，提高系统稳定性
  
  5. **总线互斥**：驱动通过互斥锁保护CAN总线，防止多个设备同时发送数据造成冲突
  
  6. **中断回调**：需要确保 HAL 库的中断回调函数能正确调用 BSP CAN 的处理函数
  
  ## 错误处理
  
  驱动在发送和接收过程中会返回相应的状态码：
  
  - `OSAL_SUCCESS` 表示操作成功
  - `OSAL_INVALID_PARAM` 表示参数无效
  - `OSAL_ERROR` 表示操作失败
  
  用户应检查函数返回值并进行相应处理：
  
  ```c
  osal_status_t status = BSP_CAN_SendDevice(device);
  if (status != OSAL_SUCCESS) {
      // 处理发送错误
      handle_send_error(status);
  }
  ```
  
  


