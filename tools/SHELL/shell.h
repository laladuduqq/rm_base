/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-04 12:51:36
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-10 18:22:06
 * @FilePath: /rm_base/tools/SHELL/shell.h
 * @Description: 
 */
#ifndef _SHELL_H_
#define _SHELL_H_

#include "tx_api.h"
#include "bsp_uart.h"
#include <stdbool.h>
#include "tools_config.h"


// 命令处理函数指针
typedef void (*shell_cmd_func_t)(int argc, char **argv);

// 命令结构体
typedef struct {
    const char *name;           // 命令名称
    shell_cmd_func_t func;      // 处理函数
    const char *description;    // 命令描述
} shell_cmd_t;

// shell上下文结构体
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

#if SHELL_ENABLE
#define SHELL_INIT() shell_init()
#define SHELL_PRINTF(...) shell_printf(__VA_ARGS__)
#define SHELL_UART_SEND(data, len) shell_uart_send(data, len)
#define SHELL_REGISTER_FUNCTION(name, func, description) shell_register_function(name, func, description)
#else
#define SHELL_INIT() do {} while(0);
#define SHELL_PRINTF(...) do {} while(0);
#define SHELL_UART_SEND(data, len) do {} while(0);
#define SHELL_REGISTER_FUNCTION(name, func, description) -1
#endif


void shell_init();
void shell_printf(const char *fmt, ...);
void shell_send(const uint8_t *data, uint16_t len);
// 函数注册接口
int shell_register_function(const char* name, shell_cmd_func_t func, const char* description);


void shell_help_cmd(int argc, char **argv);
void shell_clear_cmd(int argc, char **argv);
void shell_version_cmd(int argc, char **argv);
void shell_ps_cmd(int argc, char **argv);

#endif // _SHELL_H_