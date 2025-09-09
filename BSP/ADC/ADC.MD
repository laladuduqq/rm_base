# BSP ADC 驱动文档

## 概述

BSP ADC 是一个基于 OSAL（操作系统抽象层）的 ADC 驱动程序，提供了统一的接口来操作 STM32 微控制器上的 ADC 设备。该驱动支持阻塞、中断和 DMA 三种工作模式，并利用事件机制实现异步数据采集。

## 特性

- 支持阻塞、中断和 DMA 三种工作模式
- 双缓冲区机制提高数据采集效率
- 基于 OSAL 事件的异步通知机制
- 支持多个 ADC 实例同时工作
- 自动管理缓冲区切换
- 支持单通道和多通道数据采集
- 自动重启动转换机制

## 数据结构

### ADC_Mode

```c
typedef enum {
    ADC_MODE_BLOCKING,  // 阻塞模式
    ADC_MODE_IT,        // 中断模式
    ADC_MODE_DMA        // DMA模式
} ADC_Mode;
```

### ADC_Device

```c
typedef struct {
    ADC_HandleTypeDef* hadc;                    // ADC句柄
    ADC_ChannelConfTypeDef* channel_configs;    // 通道配置数组
    uint8_t channel_count;                      // 通道数量
    ADC_Mode mode;                              // 工作模式
    uint32_t (*values)[2];                     // 指向外部定义的双缓冲区
    volatile uint8_t active_buf;               // 当前活动缓冲区索引
    uint16_t expected_count;                   // 预期转换数量
    osal_mutex_t adc_mutex;                     // 互斥锁
    osal_event_t adc_event;                     // ADC事件
} ADC_Device;
```

### ADC_Device_Init_Config

```c
typedef struct {
    ADC_HandleTypeDef* hadc;                    // ADC句柄
    ADC_ChannelConfTypeDef* channel_configs;    // 通道配置数组
    uint8_t channel_count;                      // 通道数量
    uint32_t (*values)[2];                     // 外部双缓冲区指针
    uint16_t expected_count;                   // 预期转换数量
    ADC_Mode mode;                              // 工作模式
} ADC_Device_Init_Config;
```

## 事件定义

```c
#define ADC_EVENT_DONE (0x01 << 0)  // 转换完成事件
#define ADC_EVENT_ERROR (0x01 << 1) // ADC错误事件
```

## API 接口

```c
ADC_Device* BSP_ADC_Device_Init(ADC_Device_Init_Config* config);
```

```c
void BSP_ADC_Device_DeInit(ADC_Device* dev);
```

```c
osal_status_t BSP_ADC_Start(ADC_Device* dev);
```

```c
osal_status_t BSP_ADC_Stop(ADC_Device* dev);
```

```c
osal_status_t BSP_ADC_Get_Values(ADC_Device* dev);
```

```c
uint32_t BSP_ADC_Get_Channel_Value(ADC_Device* dev, uint8_t channel_index);
```

```c
void BSP_ADC_Init(void);
```

## 使用示例

### 阻塞模式使用

```c
// 1. 定义缓冲区和通道配置
static uint32_t adc_values[1][2];  // 单通道双缓冲区
static ADC_ChannelConfTypeDef channel_config = {0};

// 配置ADC通道
channel_config.Channel = ADC_CHANNEL_0;
channel_config.Rank = 1;
channel_config.SamplingTime = ADC_SAMPLETIME_3CYCLES;

// 2. 配置初始化参数（阻塞模式）
ADC_Device_Init_Config config = {
    .hadc = &hadc1,
    .channel_configs = &channel_config,
    .channel_count = 1,
    .values = (uint32_t (*)[2])adc_values,
    .mode = ADC_MODE_BLOCKING
};

// 3. 初始化设备
ADC_Device* device = BSP_ADC_Device_Init(&config);
if (device == NULL) {
    // 错误处理
    return;
}

// 4. 启动ADC转换
if (BSP_ADC_Start(device) != OSAL_SUCCESS) {
    // 启动失败处理
    return;
}

// 5. 获取ADC数据（阻塞方式）
if (BSP_ADC_Get_Values(device) == OSAL_SUCCESS) {
    uint32_t adc_value = BSP_ADC_Get_Channel_Value(device, 0);
    // 处理ADC值
}

// 6. 反初始化
BSP_ADC_Device_DeInit(device);
```

### 中断模式/DMA模式

```c
// 1. 定义缓冲区和通道配置
static uint32_t adc_values[2][2];  // 双通道双缓冲区
static ADC_ChannelConfTypeDef channel_configs[2];

// 配置ADC通道
channel_configs[0].Channel = ADC_CHANNEL_0;
channel_configs[0].Rank = 1;
channel_configs[0].SamplingTime = ADC_SAMPLETIME_3CYCLES;

channel_configs[1].Channel = ADC_CHANNEL_1;
channel_configs[1].Rank = 2;
channel_configs[1].SamplingTime = ADC_SAMPLETIME_3CYCLES;

// 2. 配置初始化参数（中断模式）
ADC_Device_Init_Config config = {
    .hadc = &hadc1,
    .channel_configs = channel_configs,
    .channel_count = 2,
    .values = (uint32_t (*)[2])adc_values,
    .mode = ADC_MODE_IT
};

// 3. 初始化设备
ADC_Device* device = BSP_ADC_Device_Init(&config);
if (device == NULL) {
    // 错误处理
    return;
}

// 4. 启动ADC转换
if (BSP_ADC_Start(device) != OSAL_SUCCESS) {
    // 启动失败处理
    return;
}

// 5. 获取ADC数据（函数内部等待事件）
while (1) {
    if (BSP_ADC_Get_Values(device) == OSAL_SUCCESS) {
        uint32_t adc_value0 = BSP_ADC_Get_Channel_Value(device, 0);
        uint32_t adc_value1 = BSP_ADC_Get_Channel_Value(device, 1);
        // 处理ADC值
    }
}

// 6. 停止ADC转换
BSP_ADC_Stop(device);

// 7. 反初始化
BSP_ADC_Device_DeInit(device);
```

## 工作原理

### 阻塞模式

- 启动后使用轮询方式等待转换完成
- BSP_ADC_Get_Values 函数直接执行轮询转换并将结果存储到活动缓冲区
- 不使用后台转换机制

### 中断模式

- 启动后台转换，使用双缓冲区机制
- 转换完成时通过 ADC_EVENT_DONE 事件通知
- BSP_ADC_Get_Values 函数等待事件并将结果从非活动缓冲区复制到活动缓冲区
- 中断回调中自动重启动下一次转换

### DMA模式

- 启动后台DMA转换，使用双缓冲区机制
- 数据通过DMA直接写入非活动缓冲区
- 转换完成时通过 ADC_EVENT_DONE 事件通知
- BSP_ADC_Get_Values 函数等待事件并切换活动缓冲区
- 中断回调中自动重启动下一次DMA转换

## 注意事项

1. **缓冲区管理**：ADC缓冲区需要由用户在外部定义并提供给驱动，所有模式均使用双缓冲区机制。

2. **阻塞模式**：在阻塞模式下，BSP_ADC_Get_Values 函数直接执行轮询转换并返回数据。

3. **中断/DMA模式**：在中断/DMA模式下，BSP_ADC_Get_Values 函数会等待转换完成事件，然后处理数据并切换缓冲区。

4. **内存管理**：驱动使用静态内存管理，避免动态内存分配，提高系统稳定性。

5. **自动重启动**：在中断和DMA模式下，驱动会自动重启动下一次转换，实现连续数据采集。

6. **数据访问**：BSP_ADC_Get_Channel_Value 函数根据工作模式从相应缓冲区获取数据。

## 错误处理

驱动通过 ADC_EVENT_ERROR 事件通知错误发生，用户可以通过等待该事件来处理错误情况：

```c

```


