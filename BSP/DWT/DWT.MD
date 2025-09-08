# BSP DWT 驱动文档

## 概述

BSP DWT 是一个基于 Cortex-M 内核的 DWT (Data Watchpoint and Trace) 单元的高精度定时驱动程序。该驱动提供了微秒级甚至更高精度的时间测量和延时功能，相比传统的 SysTick 定时器具有更高的精度和更广泛的应用场景。

## 特性

- 高精度时间测量（CPU 时钟周期级）
- 支持浮点数和双精度浮点数时间间隔计算
- 提供秒、毫秒、微秒三种时间单位
- 高精度延时功能，不受中断开关影响
- 自动处理计数器溢出问题
- 支持时间轴获取和增量时间计算
  
  ## 数据结构
  
  ### DWT_Time_t
  
  ```c
  typedef struct
  {
      uint32_t s;   // 秒
      uint16_t ms;  // 毫秒
      uint16_t us;  // 微秒
  } DWT_Time_t;
  ```
  
  ## API 接口
  
  ```c
  void DWT_Init(uint32_t CPU_Freq_mHz);
  ```
  
  ```c
  float DWT_GetDeltaT(uint32_t *cnt_last);
  ```
  
  ```c
  double DWT_GetDeltaT64(uint32_t *cnt_last);
  ```
  
  ```c
  float DWT_GetTimeline_s(void);
  ```
  
  ```c
  float DWT_GetTimeline_ms(void);
  ```
  
  ```c
  uint64_t DWT_GetTimeline_us(void);
  ```
  
  ```c
  void DWT_Delay(float Delay);
  ```
  
  ```c
  void DWT_SysTimeUpdate(void);
  ```
  
  ## 使用流程
  
  ### 1. 初始化 DWT
  
  ```c
  // 初始化DWT，传入CPU频率(MHz)
  // C板为168MHz，A板为180MHz
  DWT_Init(168);  // 对于C板
  // 或
  DWT_Init(180);  // 对于A板
  ```
  
  ### 2. 时间间隔测量
  
  ```c
  //使用时间戳方式测量代码执行时间：
  uint32_t timestamp;
  
  // 记录起始时间戳
  timestamp = DWT->CYCCNT;
  
  // 执行需要测量的代码
  some_function();
  
  // 计算执行时间（秒）
  float execution_time = DWT_GetDeltaT(&timestamp);
  printf("Execution time: %f seconds\n", execution_time);
  ```
  
  ### 3. 获取系统时间轴
  
  ```c
  //获取自初始化以来的系统运行时间：
  // 获取系统运行时间（秒）
  float time_s = DWT_GetTimeline_s();
  
  // 获取系统运行时间（毫秒）
  float time_ms = DWT_GetTimeline_ms();
  
  // 获取系统运行时间（微秒）
  uint64_t time_us = DWT_GetTimeline_us();
  ```
  
  ### 4. 高精度延时
  
  ```c
  // 延时1毫秒
  DWT_Delay(0.001f);
  
  // 延时100微秒
  DWT_Delay(0.0001f);
  
  // 在中断关闭期间也能正常工作的延时
  __disable_irq();
  DWT_Delay(0.001f);  // 正常工作
  __enable_irq();
  ```
  
  ## 工作原理
  
  ### DWT 硬件原理
  
  DWT (Data Watchpoint and Trace) 是 Cortex-M 内核的一个调试单元，其中包含一个 32 位的周期计数器 (CYCCNT)。该计数器以 CPU 主频运行，提供 CPU 时钟周期级的计时精度。
  
  ### 计数器溢出处理
  
  由于 CYCCNT 是 32 位寄存器，在高频 CPU 下会频繁溢出：
  - 168MHz 下约每 25.5 秒溢出一次
  - 180MHz 下约每 23.8 秒溢出一次
  
  驱动通过 `CYCCNT_RountCount` 变量跟踪溢出次数，提供 64 位的计数能力。
  
  ### 时间计算
  
  驱动内部维护一个 `SysTime` 结构体，将 64 位的周期计数转换为秒、毫秒、微秒格式：
  
  ```c
  CYCCNT64 = (uint64_t)CYCCNT_RountCount * (uint64_t)UINT32_MAX + (uint64_t)cnt_now;
  SysTime.s = CYCCNT64 / CPU_FREQ_Hz;
  SysTime.ms = (CYCCNT64 % CPU_FREQ_Hz) / CPU_FREQ_Hz_ms;
  SysTime.us = ((CYCCNT64 % CPU_FREQ_Hz) % CPU_FREQ_Hz_ms) / CPU_FREQ_Hz_us;
  ```
  
  ## 注意事项
  
  1. **初始化要求**：使用任何 DWT 功能前必须调用 DWT_Init 函数
  
  2. **CPU 频率设置**：必须正确设置 CPU 频率，否则时间计算会出现错误
  
  3. **计数器溢出**：虽然驱动已处理溢出问题，但在长时间不调用时间函数时仍需手动更新时间轴
  
  4. **中断安全**：DWT 延时函数可以在中断关闭时使用，但时间测量函数在中断关闭时不会更新
  
  5. **精度说明**：
     
     - 时间间隔测量精度：CPU 时钟周期级
     - 浮点数精度：约 6 位有效数字
     - 双精度浮点数精度：约 15 位有效数字
