# BSP GPIO EXTI 驱动文档

## 概述

BSP GPIO EXTI 是一个基于 OSAL（操作系统抽象层）的 GPIO 外部中断驱动程序，提供了统一的接口来管理 STM32 微控制器上的 GPIO 外部中断。该驱动支持事件机制和回调函数两种方式来处理中断，简化了 GPIO 中断的使用。

## 特性

- 基于 OSAL 事件的异步通知机制
- 支持回调函数方式处理中断
- 支持多个 GPIO EXTI 实例同时工作
- 动态使能/禁用中断功能
- 自动管理中断事件标志
- 支持标准 EXTI 线路（EXTI0-EXTI15）
  
  ## 数据结构
  
  ### GPIO_EXTI_Device
  
  ```c
  typedef struct {
      uint16_t pin;               // GPIO引脚
      void (*callback)();         // 用户回调函数
      uint8_t is_enabled;         // 中断使能状态
      osal_event_t exti_event;    // EXTI事件标志
  } GPIO_EXTI_Device;
  ```
  
  ### GPIO_EXTI_Init_Config
  
  ```c
  typedef struct {
      uint16_t pin;          // GPIO引脚
      void (*callback)();    // 用户回调函数
      uint8_t is_enabled;    // 中断使能状态
  } GPIO_EXTI_Init_Config;
  ```
  
  ## 事件定义
  
  ```c
  #define GPIO_EXTI_EVENT (0x01 << 0)
  ```
  
  ## API 接口
  
  ```c
  GPIO_EXTI_Device* BSP_GPIO_EXTI_Register(GPIO_EXTI_Init_Config *config);
  ```
  
  ```c
  osal_status_t BSP_GPIO_EXTI_Enable(GPIO_EXTI_Device *dev);
  ```
  
  ```c
  osal_status_t BSP_GPIO_EXTI_Disable(GPIO_EXTI_Device *dev);
  ```
  
  ```c
  osal_status_t BSP_GPIO_EXTI_Wait(GPIO_EXTI_Device* dev, osal_tick_t timeout);
  ```
  
  ```c
  void BSP_GPIO_EXTI_Unregister(GPIO_EXTI_Device* dev);
  ```
  
  ## 使用示例
  
  ```c
  // 1. 定义设备结构
  static GPIO_EXTI_Device button_device;
  
  // 2. 定义回调函数
  void button_callback(void) {
      // 按钮按下处理
      printf("Button pressed!\r\n");
  }
  
  // 3. 配置初始化参数
  GPIO_EXTI_Init_Config config = {
      .pin = GPIO_PIN_0,
      .callback = button_callback
  };
  
  // 4. 注册设备
  GPIO_EXTI_Device* device = BSP_GPIO_EXTI_Register(&config);
  if (device == NULL) {
      // 错误处理
      return;
  }
  
  // 5. 使能中断
  if (BSP_GPIO_EXTI_Enable(device) != OSAL_SUCCESS) {
      // 错误处理
      return;
  }
  
  // 6. 等待中断事件（可选，也可以只使用回调函数）
  if (BSP_GPIO_EXTI_Wait(device, 5000) == OSAL_SUCCESS) {
      // 中断触发处理
      printf("Button interrupt received via event!\r\n");
  }
  
  // 7. 禁用中断
  BSP_GPIO_EXTI_Disable(device);
  
  // 8. 反注册设备
  BSP_GPIO_EXTI_Unregister(device);
  ```
  
  ### 多个GPIO中断处理
  
  ```c
  // 1. 定义多个设备结构
  static GPIO_EXTI_Device button1_device;
  static GPIO_EXTI_Device button2_device;
  
  // 2. 定义回调函数
  void button1_callback(void) {
      printf("Button 1 pressed!\r\n");
  }
  
  void button2_callback(void) {
      printf("Button 2 pressed!\r\n");
  }
  
  // 3. 注册多个设备
  GPIO_EXTI_Init_Config config1 = {
      .pin = GPIO_PIN_0,
      .callback = button1_callback
  };
  
  GPIO_EXTI_Init_Config config2 = {
      .pin = GPIO_PIN_1,
      .callback = button2_callback
  };
  
  GPIO_EXTI_Device* dev1 = BSP_GPIO_EXTI_Register(&config1);
  GPIO_EXTI_Device* dev2 = BSP_GPIO_EXTI_Register(&config2);
  
  if (dev1 == NULL || dev2 == NULL) {
      // 错误处理
      return;
  }
  
  // 4. 使能所有中断
  BSP_GPIO_EXTI_Enable(dev1);
  BSP_GPIO_EXTI_Enable(dev2);
  
  // 5. 在任务中等待事件
  while(1) {
      // 等待任意一个按钮中断
      if (BSP_GPIO_EXTI_Wait(dev1, 1000) == OSAL_SUCCESS) {
          printf("Button 1 event received!\r\n");
      } else if (BSP_GPIO_EXTI_Wait(dev2, 1000) == OSAL_SUCCESS) {
          printf("Button 2 event received!\r\n");
      }
  }
  ```
  
  ## 工作原理
  
  ### 中断处理流程
  1. **硬件中断触发**：当配置的GPIO引脚电平发生变化时，硬件触发EXTI中断
  2. **HAL库处理**：STM32 HAL库处理中断并向用户回调函数发送通知
  3. **BSP驱动处理**：用户在HAL回调中调用`BSP_GPIO_EXTI_Handle_IRQ`函数
  4. **事件通知**：BSP驱动设置事件标志并调用用户注册的回调函数
  5. **用户处理**：用户通过回调函数或等待事件来处理中断
  
  ### 设备管理
  
  - 驱动内部维护一个设备数组，支持最多GPIO_EXTI_DEVICE_NUM个设备
  - 每个设备都有独立的事件标志和回调函数
  - 支持动态使能/禁用中断，提高灵活性
  
  ## 注意事项
  
  1. **CubeMX配置**：需要在CubeMX中正确配置GPIO为外部中断模式，并设置触发边沿
  
  2. **中断使能**：设备注册后默认是禁用状态，需要调用BSP_GPIO_EXTI_Enable使能中断
  
  3. **内存管理**：驱动使用静态内存管理，避免动态内存分配，提高系统稳定性
  
  4. **事件等待**：BSP_GPIO_EXTI_Wait函数会调用osal_event_wait直到中断触发或超时
  
  ## 错误处理
  
  驱动通过osal_status_t返回值通知操作结果：
  
  ```c
  
  ```
  
  




