/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-10 09:54:06
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-10 13:35:27
 * @FilePath: /rm_base/CONFIG/tools_config.h
 * @Description: 
 */
#ifndef _TOOLS_CONFIG_H_
#define _TOOLS_CONFIG_H_

/* shell 配置 */
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

#endif // _TOOLS_CONFIG_H_