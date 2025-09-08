# BSP SPI 驱动文档

## 概述

BSP SPI 是一个基于 OSAL（操作系统抽象层）的 SPI 驱动程序，提供了统一的接口来操作 STM32 微控制器上的 SPI 设备。该驱动支持阻塞、中断和 DMA 三种工作模式，并利用事件机制实现异步通信。驱动内部实现了总线管理机制，支持多个 SPI 设备共享同一 SPI 总线。

## 特性

- 支持阻塞、中断和 DMA 三种工作模式
- 基于 OSAL 事件的异步通知机制
- 支持多个 SPI 设备共享同一 SPI 总线
- 自动管理片选信号（CS）
- 支持全双工和单向数据传输
- 支持连续发送两次数据的特殊场景
  
  ## 数据结构
  
  ### SPI_Mode
  
  ```c
  typedef enum {
      SPI_MODE_BLOCKING,  // 阻塞模式
      SPI_MODE_IT,        // 中断模式
      SPI_MODE_DMA        // DMA模式
  } SPI_Mode;
  ```
  
  ### SPI_Device
  
  ```c
  typedef struct {
      SPI_HandleTypeDef* hspi;      // SPI句柄
      GPIO_TypeDef* cs_port;        // 片选端口
      uint16_t cs_pin;              // 片选引脚
      SPI_Mode tx_mode;             // 发送模式
      SPI_Mode rx_mode;             // 接收模式
  } SPI_Device;
  ```
  
  ### SPI_Device_Init_Config
  
  ```c
  typedef struct {
      SPI_HandleTypeDef* hspi;      // SPI句柄
      GPIO_TypeDef* cs_port;        // 片选端口
      uint16_t cs_pin;              // 片选引脚
      SPI_Mode tx_mode;             // 发送模式
      SPI_Mode rx_mode;             // 接收模式
  } SPI_Device_Init_Config;
  ```
  
  ## 事件定义
  
  ```c
  #define SPI_EVENT_TX_DONE_EVENT    (0x01 << 0)  // 发送完成事件
  #define SPI_EVENT_RX_DONE_EVENT    (0x01 << 1)  // 接收完成事件
  #define SPI_EVENT_TX_RX_DONE_EVENT (0x01 << 2)  // 发送接收完成事件
  #define SPI_EVENT_ERR_EVENT        (0x01 << 3)  // SPI错误事件
  ```
  
  ## API 接口
  
  ```c
  SPI_Device* BSP_SPI_Device_Init(SPI_Device_Init_Config* config);
  ```
  
  ```c
  void BSP_SPI_Device_DeInit(SPI_Device* dev);
  ```
  
  ```c
  osal_status_t BSP_SPI_TransReceive(SPI_Device* dev, const uint8_t* tx_data, uint8_t* rx_data, uint16_t size);
  ```
  
  ```c
  osal_status_t BSP_SPI_Transmit(SPI_Device* dev, const uint8_t* tx_data, uint16_t size);
  ```
  
  ```c
  osal_status_t BSP_SPI_Receive(SPI_Device* dev, uint8_t* rx_data, uint16_t size);
  ```
  
  ```c
  osal_status_t BSP_SPI_TransAndTrans(SPI_Device* dev, const uint8_t* tx_data1, uint16_t size1, 
                                     const uint8_t* tx_data2, uint16_t size2);
  ```
  
  ## 使用示例(block/it/dma使用方式一样)
  
  ```c
  // 1. 定义设备结构
  static SPI_Device spi_device;
  
  // 2. 配置初始化参数
  SPI_Device_Init_Config config = {
      .hspi = &hspi1,
      .cs_port = GPIOA,
      .cs_pin = GPIO_PIN_4,
      .tx_mode = SPI_MODE,
      .rx_mode = SPI_MODE
  };
  
  // 3. 初始化设备
  SPI_Device* device = BSP_SPI_Device_Init(&config);
  if (device == NULL) {
      // 错误处理
      return;
  }
  
  // 4. 发送和接收数据（全双工）
  uint8_t tx_data[] = {0x01, 0x02, 0x03, 0x04};
  uint8_t rx_data[4] = {0};
  if (BSP_SPI_TransReceive(device, tx_data, rx_data, sizeof(tx_data)) == OSAL_SUCCESS) {
      // 处理接收到的数据
  }
  
  // 5. 单向发送数据
  uint8_t send_data[] = {0x10, 0x20, 0x30};
  BSP_SPI_Transmit(device, send_data, sizeof(send_data));
  
  // 6. 单向接收数据
  uint8_t recv_data[4] = {0};
  BSP_SPI_Receive(device, recv_data, sizeof(recv_data));
  
  // 7. 连续发送两次数据
  uint8_t cmd[] = {0x80};
  uint8_t data[] = {0x01, 0x02, 0x03};
  BSP_SPI_TransAndTrans(device, cmd, sizeof(cmd), data, sizeof(data));
  
  // 8. 反初始化
  BSP_SPI_Device_DeInit(device);
  ```
  
  ## 工作原理
  
  ### 阻塞模式
  - 发送和接收都采用轮询方式
  - 函数调用会阻塞直到传输完成
  - 不使用事件机制
  
  ### 中断/DMA模式
  
  - 启动后台传输，通过中断或DMA完成数据传输
  - 传输完成时通过相应事件通知
  - 函数内部等待事件并返回结果
  
  ### 总线管理
  
  - 驱动内部维护SPI总线管理器，支持多个设备共享同一SPI总线
  - 使用互斥锁保证同一时间只有一个设备可以访问SPI总线
  - 自动管理片选信号（CS），在传输开始前拉低CS，传输结束后拉高CS
  
  ## 注意事项
  
  1. **模式选择**：根据应用需求选择合适的传输模式，阻塞模式简单但会阻塞线程，中断/DMA模式效率高但需要处理事件。
  
  2. **总线共享**：多个设备可以共享同一SPI总线，驱动会自动管理总线访问权限。
  
  3. **片选管理**：驱动会自动管理片选信号，用户无需手动控制CS引脚。
  
  4. **内存管理**：驱动使用静态内存管理，避免动态内存分配，提高系统稳定性。
  
  5. **中断回调**：需要确保 HAL 库的中断回调函数能正确调用 BSP SPI 的处理函数。
  
  ## 错误处理
  
  驱动通过 SPI_ERR_EVENT 事件通知错误发生，用户可以通过等待该事件来处理错误情况：
  
  ```c
  
  ```
  
  
