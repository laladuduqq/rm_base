

#  LOG 驱动文档

## 概述

LOG 是一个功能丰富的日志系统，支持多级别日志输出、彩色显示、时间戳和多线程保护。该模块提供了统一的日志接口，可以将日志信息输出到多种设备，包括 UART、RTT 和 Shell。通过配置选项，可以灵活地控制日志的输出方式和详细程度。

## 特性

- 支持6个日志级别：VERBOSE、DEBUG、INFO、WARN、ERROR、FATAL
- 支持ANSI彩色输出，便于区分不同级别的日志
- 支持时间戳功能，记录日志产生的时间
- 多线程安全，使用互斥锁保护日志输出
- 支持多种输出方式：UART、RTT、Shell
- 可配置的日志输出级别，灵活控制日志详细程度
- 使用宏定义简化日志输出调用
  
  ## 数据结构
  
  ### 日志级别定义
  
  ```c
  #define LOG_LEVEL_VERBOSE 0
  #define LOG_LEVEL_DEBUG   1
  #define LOG_LEVEL_INFO    2
  #define LOG_LEVEL_WARN    3
  #define LOG_LEVEL_ERROR   4
  #define LOG_LEVEL_FATAL   5
  ```
  
  ## API 接口
  
  ```c
  void log_init(void);
  ```
  
  ```c
  void log_set_level(int level);
  ```
  
  ```c
  void log_write(int level, const char* tag, const char* format, ...);
  ```
  
  ### 日志宏定义
  
  ```c
  #define LOG_VERBOSE(fmt, ...)  // 输出VERBOSE级别日志
  #define LOG_DEBUG(fmt, ...)    // 输出DEBUG级别日志
  #define LOG_INFO(fmt, ...)     // 输出INFO级别日志
  #define LOG_WARN(fmt, ...)     // 输出WARN级别日志
  #define LOG_ERROR(fmt, ...)    // 输出ERROR级别日志
  #define LOG_FATAL(fmt, ...)    // 输出FATAL级别日志
  ```
  
  ## 配置选项
  
  ```c
  #define LOG_ENABLED              1              // 启用日志功能
  #define LOG_OUTPUT_LEVEL         LOG_LEVEL_VERBOSE  // 日志的输出等级
  #define LOG_COLOR_ENABLE         1              // 启用彩色输出
  #define LOG_TIMSTAMP_ENABLE      1              // 启用时间戳 
  #if SHELL_ENABLE
  #define LOG_OUTPUT_SHELL                        // 使用shell输出
  #else
  #define LOG_RTT              1                  // 是否使用rtt输出，0为使用uart输出          
  #define LOG_COM              huart6             // 使用的串口
  #endif
  ```
  
  ## 使用示例
  
  ```c
  // 在需要使用日志的文件中定义tag
  #define log_tag "MY_MODULE"
  #include "log.h"
  
  int main(void) {
      // 初始化日志系统
      LOG_INIT();
      
      // 输出不同级别的日志
      LOG_VERBOSE("This is a verbose message");
      LOG_DEBUG("This is a debug message with value: %d", 123);
      LOG_INFO("System initialized successfully");
      LOG_WARN("This is a warning message");
      LOG_ERROR("An error occurred: %s", "file not found");
      LOG_FATAL("Fatal error, system will terminate");
      
      return 0;
  }
  ```
  
  ## 工作原理
  
  ### 输出方式选择
  
  日志系统支持三种输出方式，优先级从高到低为：
  1. Shell输出：当SHELL_ENABLE使能时，默认使用Shell输出
  2. RTT输出：当LOG_RTT配置为1时，使用RTT输出
  3. UART输出：当以上两种方式都未使能时，使用UART输出
  
  ### 线程安全机制
  
  日志系统通过互斥锁保证线程安全：
  
  - 在写入日志前获取互斥锁
  - 日志输出完成后释放互斥锁
  - 防止多线程同时写入导致日志混乱
  
  ### 格式化输出
  
  日志输出格式为：
  
  ```c
  [时间戳][日志级别][TAG]:日志内容\r\n
  ```
  
  ## 注意事项
  
  1. **初始化**：使用日志功能前必须调用LOG_INIT()进行初始化
  2. **Tag定义**：建议在每个源文件中定义log_tag，便于识别日志来源
  3. **性能影响**：日志输出可能影响系统实时性，建议在发布版本中适当提高日志级别或禁用日志
  4. **缓冲区大小**：单条日志最大长度受内部缓冲区限制（256字节），超长日志会被截断
  5. **多线程**：日志系统是线程安全的，可以在多个线程中同时使用
  6. **输出阻塞**：某些输出方式（如UART）可能是阻塞的，会影响程序执行
  
  ## 错误处理
  
  日志系统本身具有较强的容错能力：
  
  - 初始化失败时，日志功能会被禁用但不会导致系统崩溃
  - 输出设备不可用时，日志会被静默丢弃
  - 缓冲区溢出时，日志会被截断而不是导致系统异常
  
  在配置和使用日志系统时应注意：
  
  1. 确保所选的输出设备（UART/RTT/Shell）已正确初始化
  2. 合理设置日志级别，避免输出过多无用信息
  3. 在内存受限的系统中，注意日志缓冲区对内存的占用
