# OFFLINE 离线检测模块文档

## 概述

OFFLINE 离线检测模块是一个基于 OSAL（操作系统抽象层）的设备状态监控系统，用于检测和管理各种设备的在线/离线状态。该模块通过定期检查设备的心跳信号来判断设备是否在线，并根据设备优先级和配置触发相应的报警（如蜂鸣器和RGB灯）。

## 特性

- 支持多种设备的在线/离线状态检测
- 可配置的超时时间和报警策略
- 基于优先级的报警机制
- 支持蜂鸣器和RGB灯报警提示
- 提供Shell命令行接口查看设备状态
- 支持设备的动态启用/禁用
- 基于RTOS线程的后台检测机制
  
  ## 数据结构
  
  ### OfflineLevel_e
  
  ```c
  typedef enum {
      OFFLINE_LEVEL_LOW = 1,    // 低优先级
      OFFLINE_LEVEL_MEDIUM = 2, // 中优先级
      OFFLINE_LEVEL_HIGH = 3    // 高优先级
  } OfflineLevel_e;
  ```
  
  ### OfflineDeviceInit_t
  
  ```c
  typedef struct {
      const char* name;         // 设备名称
      uint32_t timeout_ms;      // 超时时间（毫秒）
      OfflineLevel_e level;     // 离线等级
      uint8_t beep_times;       // 蜂鸣次数
      uint8_t enable;           // 是否启用检测
  } OfflineDeviceInit_t;
  ```
  
  ### OfflineDevice_t
  
  ```c
  typedef struct {
      char name[32];            // 设备名称
      uint32_t timeout_ms;      // 超时时间（毫秒）
      OfflineLevel_e level;     // 离线等级
      uint8_t beep_times;       // 蜂鸣次数
      bool is_offline;          // 离线状态
      uint32_t last_time;       // 上次更新时间
      uint8_t index;            // 索引字段
      uint8_t enable;           // 是否启用检测
      float dt;                 // 时间间隔
      uint32_t dt_cnt;          // 时间计数器
  } OfflineDevice_t; 
  ```
  
  ### OfflineManager_t
  
  ```c
  typedef struct {
      OfflineDevice_t devices[MAX_OFFLINE_DEVICES]; // 设备数组
      uint8_t device_count;                         // 设备数量
  } OfflineManager_t;
  ```
  
  ## 状态定义
  
  ```c
  #define OFFLINE_INVALID_INDEX  0xFF  // 无效索引
  #define STATE_ONLINE           0     // 在线状态
  #define STATE_OFFLINE          1     // 离线状态
  #define OFFLINE_ENABLE         1     // 启用检测
  #define OFFLINE_DISABLE        0     // 禁用检测
  ```
  
  ## 配置参数
  
  ```c
  #define MAX_OFFLINE_DEVICES      10     // 最大设备数量
  #define OFFLINE_THREAD_PRIORITY  10     // 离线检测线程优先级
  #define OFFLINE_THREAD_STACK_SIZE 512   // 离线检测线程栈大小
  #define OFFLINE_BEEP_ENABLE      1      // 启用蜂鸣器报警
  #define OFFLINE_BEEP_PERIOD      5000   // 蜂鸣器报警周期
  #define OFFLINE_BEEP_ON_TIME     100    // 蜂鸣器开启时间
  #define OFFLINE_BEEP_OFF_TIME    100    // 蜂鸣器关闭时间
  #define OFFLINE_BEEP_TUNE_VALUE  1000   // 蜂鸣器音调值
  #define OFFLINE_BEEP_CTRL_VALUE  500    // 蜂鸣器控制值
  ```
  
  
  
  ## API 接口
  
  ```c
  void offline_init(void);
  ```
  
  ```c
  uint8_t offline_device_register(const OfflineDeviceInit_t* init);
  ```
  
  ```c
  void offline_device_update(uint8_t device_index);
  ```
  
  ```c
  void offline_device_enable(uint8_t device_index);
  ```
  
  ```c
  void offline_device_disable(uint8_t device_index);
  ```
  
  ```c
  uint8_t get_device_status(uint8_t device_index);
  ```
  
  ```c
  uint8_t get_system_status(void);
  ```
  
  ## Shell 命令
  
  ```c
  模块提供了一个Shell命令接口，用于查看设备离线状态信息：
  offline list     # 列出所有注册设备及其状态
  offline status   # 显示系统整体离线状态
  ```
  
  ## 使用示例
  
  ```c
  offline_init();
  // 定义设备初始化配置
  OfflineDeviceInit_t gimbal_init = {
      .name = "Gimbal Motor",
      .timeout_ms = 1000,
      .level = OFFLINE_LEVEL_HIGH,
      .beep_times = 3,
      .enable = OFFLINE_ENABLE
  };
  
  // 注册设备
  uint8_t gimbal_index = offline_device_register(&gimbal_init);
  if (gimbal_index == OFFLINE_INVALID_INDEX) {
      // 注册失败处理
      return;
  }
  
  // 在设备通信正常时定期调用此函数更新设备状态
  offline_device_update(gimbal_index);
  
  // 检查特定设备状态
  if (get_device_status(gimbal_index) == STATE_OFFLINE) {
      // 设备离线处理
  }
  
  // 检查系统整体状态
  uint8_t system_status = get_system_status();
  if (system_status != 0) {
      // 系统中有设备离线
  }
  
  // 临时禁用设备检测
  offline_device_disable(gimbal_index);
  
  // 重新启用设备检测
  offline_device_enable(gimbal_index);
  ```
  
  ## 工作原理
  
  ### 设备注册与管理
  1. 使用 offline_device_register 函数注册设备，系统会为设备分配索引
  2. 注册的设备信息存储在内部管理器中
  3. 每个设备有唯一的索引用于后续操作
  
  ### 离线检测机制
  
  1. 模块在独立的RTOS线程中运行离线检测任务
  2. 定期检查所有已注册且启用的设备
  3. 通过比较当前时间和设备上次更新时间与超时时间判断设备状态
  4. 根据设备优先级和配置触发报警
  
  ### 报警机制
  
  1. 当检测到设备离线时，根据设备优先级选择最高优先级的设备报警
  2. 通过蜂鸣器和RGB灯提供视觉和听觉报警
  3. 蜂鸣次数根据设备配置确定
  
  ### Shell接口
  
  1. 提供 `offline list` 命令列出所有设备状态
  2. 提供 `offline status` 命令显示系统整体状态
  3. 通过shell接口可以实时监控设备状态
  
  ## 注意事项
  
  1. **设备索引管理**：注册设备时会返回设备索引，应用程序需要妥善保管该索引用于后续操作。
  
  2. **定期更新**：应用程序需要定期调用 offline_device_update 函数更新设备状态，否则设备会被判定为离线。
  
  3. **内存管理**：模块使用静态内存管理，最大设备数量由 MAX_OFFLINE_DEVICES 宏定义。
  
  4. **线程安全**：模块内部使用RTOS线程和同步机制保证线程安全。
  
  5. **报警配置**：报警行为可以通过配置文件中的宏定义进行定制。
  
  ## 错误处理
  
  模块通过以下方式处理错误：
  
  1. **设备注册失败**：当设备注册参数无效或设备数量达到上限时，返回 `OFFLINE_INVALID_INDEX`。
  2. **无效索引操作**：对无效索引的操作将被忽略。
  3. **系统错误**：通过日志系统记录错误信息，便于调试和问题排查。
  
  
