# BSP PWM 驱动文档

## 概述

BSP PWM 是一个基于 OSAL（操作系统抽象层）的 PWM 驱动程序，提供了统一的接口来操作 STM32 微控制器上的 PWM 设备。该驱动支持阻塞、中断和 DMA 三种工作模式。

## 特性

- 支持阻塞、中断和 DMA 三种工作模式
- 支持多个 PWM 实例同时工作
- 高精度占空比控制（支持小数点后一位）
- 动态频率调整功能
- 自动定时器时钟计算
  
  ## 数据结构
  
  ### PWM_Mode
  
  ```c
  typedef enum {
      PWM_MODE_BLOCKING,  // 阻塞模式
      PWM_MODE_IT,        // 中断模式
      PWM_MODE_DMA        // DMA模式
  } PWM_Mode;
  ```
  
  ### PWM_Channel
  
  ```c
  typedef enum {
      PWM_CHANNEL_1 = TIM_CHANNEL_1,
      PWM_CHANNEL_2 = TIM_CHANNEL_2,
      PWM_CHANNEL_3 = TIM_CHANNEL_3,
      PWM_CHANNEL_4 = TIM_CHANNEL_4
  } PWM_Channel;
  ```
  
  ### PWM_Device
  
  ```c
  typedef struct {
      TIM_HandleTypeDef* htim;      // 定时器句柄
      PWM_Channel channel;          // PWM通道
      uint32_t period;              // 周期值
      uint32_t pulse;               // 脉冲宽度值
      uint32_t frequency;           // 频率(Hz)
      uint16_t duty_cycle_x10;      // 占空比(0-1000)，表示0.0%到100.0%，支持小数点后一位
      PWM_Mode mode;                // PWM模式
      osal_event_t pwm_event;       // PWM事件
  } PWM_Device;
  ```
  
  ### PWM_Init_Config
  
  ```c
  typedef struct {
      TIM_HandleTypeDef* htim;
      PWM_Channel channel;
      uint32_t frequency;           // 频率(Hz)
      uint16_t duty_cycle_x10;      // 初始占空比(0-1000)，表示0.0%到100.0%
      PWM_Mode mode;                // PWM模式
  } PWM_Init_Config;
  ```
  
  ## 事件定义
  
  > 注意：当前版本PWM驱动未使用事件机制，保留以备将来扩展
  
  ## API 接口
  
  ```c
  PWM_Device* BSP_PWM_Device_Init(PWM_Init_Config* config);
  ```
  
  ```c
  void BSP_PWM_Device_DeInit(PWM_Device* dev);
  ```
  
  ```c
  osal_status_t BSP_PWM_Start(PWM_Device* dev);
  ```
  
  ```c
  osal_status_t BSP_PWM_Stop(PWM_Device* dev);
  ```
  
  ```c
  osal_status_t BSP_PWM_Set_Duty_Cycle(PWM_Device* dev, uint16_t duty_cycle_x10);
  ```
  
  ```c
  osal_status_t BSP_PWM_Set_Frequency(PWM_Device* dev, uint32_t frequency);
  ```
  
  ## 使用示例
  
  ```c
  // 1. 定义设备结构
  static PWM_Device pwm_device;
  
  // 2. 配置初始化参数（阻塞模式）
  PWM_Init_Config config = {
      .htim = &htim1,
      .channel = PWM_CHANNEL_1,
      .frequency = 1000,        // 1kHz
      .duty_cycle_x10 = 500,    // 50.0%占空比
      .mode = PWM_MODE_BLOCKING
  };
  
  // 3. 初始化设备
  PWM_Device* device = BSP_PWM_Device_Init(&config);
  if (device == NULL) {
      // 错误处理
      return;
  }
  
  // 4. 启动PWM输出
  if (BSP_PWM_Start(device) != OSAL_SUCCESS) {
      // 错误处理
      return;
  }
  
  // 5. 动态调整占空比
  BSP_PWM_Set_Duty_Cycle(device, 750);  // 调整为75.0%占空比
  
  // 6. 动态调整频率
  BSP_PWM_Set_Frequency(device, 2000);  // 调整为2kHz频率
  
  // 7. 停止PWM输出
  BSP_PWM_Stop(device);
  
  // 8. 反初始化
  BSP_PWM_Device_DeInit(device);
  ```
  
  ## 工作原理
  
  ### 阻塞模式
  - PWM启动和停止直接调用HAL库的阻塞函数
  - 不使用中断或DMA机制
  - 函数调用会立即返回
  
  ### 中断模式
  
  - PWM启动和停止调用HAL库的中断函数
  - 在中断模式下，定时器更新事件会触发中断
  - 适用于需要精确时序控制的应用
  
  ### DMA模式
  
  - PWM启动和停止调用HAL库的DMA函数
  - 使用DMA传输脉冲值
  - 适用于高负载系统，减少CPU占用
  
  ### 占空比精度
  
  - 使用uint16_t类型的duty_cycle_x10表示占空比
  - 取值范围0-1000，表示0.0%到100.0%
  - 例如：500表示50.0%，725表示72.5%
  
  ## 注意事项
  
  1. **定时器初始化**：在使用BSP PWM驱动前，需要确保对应的定时器已在HAL库中正确初始化。
  
  2. **占空比表示**：占空比使用duty_cycle_x10表示，支持小数点后一位精度，避免了浮点运算。
  
  3. **频率设置**：驱动会自动计算预分频器和周期值，以确保在给定频率下获得最佳精度。
  
  4. **多设备支持**：驱动支持多个PWM设备同时工作，但受MAX_PWM_DEVICES宏限制。
  
  5. **内存管理**：驱动使用静态内存管理，避免动态内存分配，提高系统稳定性。
  
  ## 错误处理
  
  驱动通过osal_status_t返回值通知操作结果：
  
  ```c
  
  ```
  
  


