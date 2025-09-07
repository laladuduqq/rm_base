# BSP UART 驱动文档

## 概述

BSP UART 是一个基于 OSAL（操作系统抽象层）的 UART 驱动程序，提供了统一的接口来操作 STM32 微控制器上的 UART 设备。该驱动支持阻塞、中断和 DMA 三种工作模式，并利用事件机制实现异步通信。

## 特性

- 支持阻塞、中断和 DMA 三种工作模式
- 双缓冲区机制提高接收效率（适用于中断和DMA模式）
- 基于 OSAL 事件的异步通知机制
- 支持多个 UART 实例同时工作
- 自动管理接收缓冲区切换
- 支持不定长和固定长度数据接收

## 数据结构

### UART_Mode

```c
typedef enum {
    UART_MODE_BLOCKING,  // 阻塞模式
    UART_MODE_IT,        // 中断模式
    UART_MODE_DMA        // DMA模式
} UART_Mode;
```

### UART_Device

```c
typedef struct {
    UART_HandleTypeDef *huart;        // UART句柄
    uint8_t (*rx_buf)[2];             // 指向外部定义的双缓冲区
    uint16_t rx_buf_size;             // 缓冲区大小
    volatile uint8_t rx_active_buf;   // 当前活动缓冲区
    uint16_t real_rx_len;             // 实际接收数据长度
    uint16_t expected_rx_len;         // 预期长度（0为不定长）
    osal_event_t uart_event;          // UART事件
    UART_Mode rx_mode;                // 接收模式
    UART_Mode tx_mode;                // 发送模式
    uint32_t error_status;            // 错误状态
} UART_Device;
```

### UART_Device_init_config

```c
typedef struct {
    UART_HandleTypeDef *huart;        // UART句柄
    uint8_t (*rx_buf)[2];             // 外部接收缓冲区指针
    uint16_t rx_buf_size;             // 接收缓冲区大小
    uint16_t expected_rx_len;         // 预期长度（0为不定长）
    UART_Mode rx_mode;                // 接收模式
    UART_Mode tx_mode;                // 发送模式
} UART_Device_init_config;
```

## 事件定义

```c
#define UART_RX_DONE_EVENT    (0x01 << 0)  // 接收完成事件
#define UART_TX_DONE_EVENT    (0x01 << 1)  // 发送完成事件
#define UART_ERR_EVENT        (0x01 << 2)  // UART错误事件
```

## API 接口

```c
UART_Device* BSP_UART_Device_Init(UART_Device_init_config *config);
```

```c
int BSP_UART_Send(UART_Device *inst, uint8_t *data, uint16_t len);
```

```c
int BSP_UART_Read(UART_Device *inst, uint8_t *data);
```

```c
void BSP_UART_Deinit(UART_Device *inst);
```

## 使用示例

### 阻塞模式使用

```c
// 1. 定义缓冲区和设备结构
static uint8_t uart_rx_buf[2][256];
static UART_Device uart_device;

// 2. 配置初始化参数（阻塞模式）
UART_Device_init_config config = {
    .huart = &huart1,
    .rx_buf = uart_rx_buf,
    .rx_buf_size = 256,
    .expected_rx_len = 0,  // 不定长接收
    .rx_mode = UART_MODE_BLOCKING,
    .tx_mode = UART_MODE_BLOCKING
};

// 3. 初始化设备
UART_Device* device = BSP_UART_Device_Init(&config);
if (device == NULL) {
    // 错误处理
    return;
}

// 4. 发送数据
uint8_t data[] = "Hello UART!\r\n";
BSP_UART_Send(device, data, sizeof(data) - 1);

// 5. 接收数据（阻塞方式）
uint8_t rx_buffer[256];
int len = BSP_UART_Read(device, rx_buffer);
if (len > 0) {
    // 处理接收到的数据
}

// 6. 反初始化
BSP_UART_Deinit(device);
```

### 中断/DMA模式使用（事件驱动）

```c
// 1. 定义缓冲区和设备结构
static uint8_t uart_rx_buf[2][256];
static UART_Device uart_device;

// 2. 配置初始化参数（中断模式）
UART_Device_init_config config = {
    .huart = &huart1,
    .rx_buf = uart_rx_buf,
    .rx_buf_size = 256,
    .expected_rx_len = 0,  // 不定长接收
    .rx_mode = UART_MODE_IT,
    .tx_mode = UART_MODE_IT
};

// 3. 初始化设备
UART_Device* device = BSP_UART_Device_Init(&config);
if (device == NULL) {
    // 错误处理
    return;
}

// 4. 发送数据
uint8_t data[] = "Hello UART!\r\n";
BSP_UART_Send(device, data, sizeof(data) - 1);

// 5. 接收数据（事件驱动方式）
unsigned int actual_flags;
osal_status_t status = osal_event_wait(&device->uart_event, UART_RX_DONE_EVENT,
                                      OSAL_EVENT_WAIT_FLAG_OR, 5000, &actual_flags);
if (status == OSAL_SUCCESS && (actual_flags & UART_RX_DONE_EVENT)) {
    // 直接访问非活动缓冲区获取数据
    uint8_t* rx_data = device->rx_buf[!device->rx_active_buf];
    int rx_len = device->real_rx_len;
    
    // 处理接收到的数据
    process_data(rx_data, rx_len);
    
    // 清除事件标志
    osal_event_clear(&device->uart_event, UART_RX_DONE_EVENT);
}

// 6. 反初始化
BSP_UART_Deinit(device);
```

## 工作原理

### 阻塞模式

- 发送和接收都采用轮询方式
- 不使用后台接收机制
- BSP_UART_Read 函数直接执行轮询接收

### 中断/DMA模式

- 启动后台接收，使用双缓冲区机制
- 接收完成时通过 UART_RX_DONE_EVENT 事件通知
- 用户可以直接访问非活动缓冲区获取数据
- BSP_UART_Read 函数主要用于阻塞模式，中断/DMA模式下也可使用但会复制数据

## 注意事项

1. **缓冲区管理**：接收缓冲区需要由用户在外部定义并提供给驱动，中断和DMA模式使用双缓冲区机制提高效率。

2. **事件处理**：中断和DMA模式下，接收完成通过 UART_RX_DONE_EVENT 事件通知，用户应等待事件后再处理数据。

3. **阻塞模式**：在阻塞模式下，双缓冲区机制不生效，BSP_UART_Read 函数直接执行轮询接收。

4. **内存管理**：驱动使用静态内存管理，避免动态内存分配，提高系统稳定性。

5. **中断回调**：需要确保 HAL 库的中断回调函数能正确调用 BSP UART 的处理函数。

## 错误处理

驱动通过 UART_ERR_EVENT 事件通知错误发生，用户可以通过等待该事件来处理错误情况：

```c

```


