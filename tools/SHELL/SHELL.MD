# Shell 工具文档

## 概述

Shell 是一个基于 OSAL 的命令行接口工具，提供了统一的接口来与系统进行交互。该工具支持命令注册、历史命令、命令解析和执行等功能。

## 特性

- 支持命令行交互界面
- 命令历史记录功能
- 支持 ANSI 转义序列（清屏等）
- 双缓冲区机制提高接收效率
- 基于 OSAL 事件的异步通知机制
- 支持多个命令动态注册
- 支持 RTT 和 UART 两种通信方式
  
  ## 数据结构
  
  ### shell_cmd_func_t
  
  ```c
  typedef void (*shell_cmd_func_t)(int argc, char **argv);
  ```
  
  > 命令处理函数指针类型，所有命令处理函数都应遵循此原型。
  
  ### shell_context_t
  
  Shell上下文结构体，包含Shell运行所需的所有状态信息。
  
  ```c
  typedef struct {
      UART_Device *uart_dev;              // UART设备
      char cmd_buffer[SHELL_CMD_MAX_LENGTH]; // 命令缓冲区
      uint16_t cmd_len;                   // 命令长度
      char *args[SHELL_MAX_ARGS];         // 参数指针数组
      uint8_t argc;                       // 参数数量
      char history[SHELL_HISTORY_MAX][SHELL_CMD_MAX_LENGTH]; // 历史命令
      uint8_t history_count;              // 历史命令数量
      uint8_t history_index;              // 当前历史命令索引
      TX_THREAD thread;                   // shell线程
      TX_MUTEX mutex;                     // 互斥锁
      bool running;                       // 运行状态
      bool initialized;                   // 初始化状态
  } shell_context_t;
  ```
  
  ## 配置参数
  
  ```c
  #define SHELL_ENABLE                 1                                    // 启用shell功能
  #define SHELL_CMD_MAX_LENGTH         128                                  // 最大命令长度
  #define SHELL_MAX_ARGS               8                                    // 最大参数数量
  #define SHELL_HISTORY_MAX            10                                   // 历史命令数量
  #define SHELL_PROMPT                 "shell> "                            // 提示符
  #define SHELL_MAX_DYNAMIC_COMMANDS   8                                    // 注册命令的最大数量
  #define SHELL_RTT                    0                                    // 使用rtt作为shell通讯方式,当使能时,SHELL_COM无效
  #define SHELL_COM                    huart6                               // shell使用的通讯接口(uart)
  #define SHELL_BUFFER_SIZE            32                                   // shell缓冲区大小
  #define SHELL_THREAD_STACK_SIZE      1024                                 // shell线程栈大小
  #define SHELL_THREAD_PRIORITY        30                                   // shell线程优先级
  #define SHELL_THREAD_STACK_SECTION  __attribute__((section(".ccmram")))   //shell线程栈内存区域
  ```
  
  ## API 接口
  
  ```c
  void shell_init();
  ```
  
  ```c
  void shell_printf(const char *fmt, ...);
  ```
  
  ```c
  void shell_send(const uint8_t *data, uint16_t len);
  ```
  
  ```c
  int shell_register_function(const char* name, shell_cmd_func_t func, const char* description);
  ```
  
  ## 内置命令
  
  ### help 命令
  
  显示所有可用命令的帮助信息。
  
  ### clear 命令
  
  清屏命令，使用 ANSI 转义序列清除终端屏幕。
  
  ### version 命令
  
  显示 Shell 系统的版本信息。
  
  ### ps 命令
  
  显示系统信息，包括线程、定时器、互斥量、信号量、事件标志组、队列、字节池和块池等信息。
  
  > 目前只写了threadx的
  
  ## 使用示例
  
  ### 注册自定义命令
  
  ```c
  // 定义命令处理函数
  void my_command_handler(int argc, char **argv) {
      if (argc < 2) {
          shell_printf("Usage: mycmd <param>\r\n");
          return;
      }
      
      shell_printf("Received parameter: %s\r\n", argv[1]);
  }
  
  // 在适当位置注册命令
  void register_my_command(void) {
      shell_register_function("mycmd", my_command_handler, "My custom command");
  }
  ```
  
  ### 使用Shell进行调试
  
  ```c
  shell> help
  Available commands:
    help     - 显示帮助信息 (Show help)
    clear    - 清屏 (Clear screen)
    version  - 显示版本信息 (Version info)
    ps       - 显示系统信息 (Show system info)
    mycmd    - My custom command
  
  shell> version
  osal Shell System v1.0
  Built with osal
  
  shell> ps thread
  Thread Information:
  Name                             State    Priority   Stack Size   Run Count
  ------------------------------------------------------------------------
  tx_idle_thread                   READY    30         1024         1000    
  tx_timer_thread                  READY    30         1024         500     
  my_thread                        READY    30         1024         200     
  
  shell> mycmd test_parameter
  Received parameter: test_parameter
  ```
  
  ## 工作原理
  
  ### 线程模型
  
  Shell在独立的线程中运行，通过UART或RTT接收用户输入，解析命令并执行相应的处理函数。
  
  ### 命令处理流程
  1. 用户输入命令
  2. Shell解析命令和参数
  3. 查找匹配的命令处理函数
  4. 调用命令处理函数执行具体操作
  5. 返回结果通过shell_printf输出
  
  ### 历史命令
  
  Shell维护一个历史命令缓冲区，用户可以通过上下箭头键浏览之前输入的命令。
  
  ## 注意事项
  
  1. **内存管理**：Shell使用静态内存管理，避免动态内存分配，提高系统稳定性。
  
  2. **线程安全**：Shell内部使用互斥锁保护共享资源，确保线程安全。
  
  3. **缓冲区大小**：根据实际需求调整SHELL_CMD_MAX_LENGTH和SHELL_BUFFER_SIZE等参数。
  
  4. **命令数量**：动态注册的命令数量不能超过SHELL_MAX_DYNAMIC_COMMANDS定义的数量。
  
  5. **通信接口**：可以通过配置选择使用UART或RTT作为通信接口。
  
  ## 错误处理
  
  Shell内部处理常见的错误情况，如：
  
  - 未知命令
  - 命令参数错误
  - 命令注册失败（命令已存在或超出最大命令数量）
  
  对于这些情况，Shell会输出相应的错误信息提示用户。
  
  ```c
  
  ```
  
  


