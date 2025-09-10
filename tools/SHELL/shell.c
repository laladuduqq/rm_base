/*
 * @Author: Lingma 
 * @Date: 2025-09-04
 * @Description: ThreadX Shell System implementation
 */

#include "shell.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "SEGGER_RTT.h"
#include "bsp_uart.h"
#include "osal_def.h"
#include "tools_config.h"

// 全局shell上下文
static shell_context_t g_shell_ctx;
static uint8_t shell_rx_buffer[SHELL_BUFFER_SIZE][2]; // 双缓冲区，每个缓冲区2字节
SHELL_THREAD_STACK_SECTION static uint8_t shell_thread_stack[SHELL_THREAD_STACK_SIZE];

// 内置命令表
static const shell_cmd_t g_shell_cmds[] = {
    {"help", shell_help_cmd, "显示帮助信息 (Show help)"},
    {"clear", shell_clear_cmd, "清屏 (Clear screen)"},
    {"version", shell_version_cmd, "显示版本信息 (Version info)"},
    {"ps", shell_ps_cmd, "显示系统信息 (Show system info)"},
    {NULL, NULL, NULL} // 结束标记
};

// 动态注册命令表
static shell_cmd_t g_dynamic_shell_cmds[SHELL_MAX_DYNAMIC_COMMANDS];
static int g_dynamic_cmd_count = 0;

// 内部函数声明
static void shell_thread_entry(ULONG input);
static void shell_parse_cmd(void);
static void shell_execute_cmd(void);
static void shell_print_prompt(void);
static int shell_split_args(char *cmdline);
static const shell_cmd_t* shell_find_cmd(const char *name);
static void shell_handle_char(char ch);

// 跟踪按键序列
static uint8_t g_escape_sequence = 0;  // 跟踪是否收到ESC字符

// 处理单个字符输入
static void shell_handle_char(char ch) {
    // 处理转义序列（箭头键）
    if (g_escape_sequence == 1) {
        if (ch == '[') {
            g_escape_sequence = 2;
            return;
        } else {
            g_escape_sequence = 0;  // 重置状态
        }
    } else if (g_escape_sequence == 2) {
        g_escape_sequence = 0;  // 重置状态
        switch (ch) {
            case 'A': // 上箭头键
                if (g_shell_ctx.history_count > 0) {
                    if (g_shell_ctx.history_index > 0) {
                        g_shell_ctx.history_index--;
                    } else {
                        return; // 已经在最顶部的历史记录
                    }
                    
                    // 清除当前行
                    for (int i = 0; i < g_shell_ctx.cmd_len; i++) {
                        shell_printf("\b \b");
                    }
                    
                    // 显示历史命令
                    strcpy(g_shell_ctx.cmd_buffer, g_shell_ctx.history[g_shell_ctx.history_index]);
                    g_shell_ctx.cmd_len = strlen(g_shell_ctx.cmd_buffer);
                    shell_printf("%s", g_shell_ctx.cmd_buffer);
                }
                return;
                
            case 'B': // 下箭头键
                if (g_shell_ctx.history_count > 0) {
                    if (g_shell_ctx.history_index < g_shell_ctx.history_count - 1) {
                        g_shell_ctx.history_index++;
                    } else if (g_shell_ctx.history_index == g_shell_ctx.history_count - 1) {
                        g_shell_ctx.history_index++; // 移动到"当前编辑行"
                    } else {
                        return; // 已经在最底部
                    }
                    
                    // 清除当前行
                    for (int i = 0; i < g_shell_ctx.cmd_len; i++) {
                        shell_printf("\b \b");
                    }
                    
                    // 如果是"当前编辑行"则清空，否则显示历史命令
                    if (g_shell_ctx.history_index == g_shell_ctx.history_count) {
                        g_shell_ctx.cmd_len = 0;
                        g_shell_ctx.cmd_buffer[0] = '\0';
                    } else {
                        strcpy(g_shell_ctx.cmd_buffer, g_shell_ctx.history[g_shell_ctx.history_index]);
                        g_shell_ctx.cmd_len = strlen(g_shell_ctx.cmd_buffer);
                        shell_printf("%s", g_shell_ctx.cmd_buffer);
                    }
                }
                return;
                
            default:
                return;
        }
    }
    
    switch (ch) {
        case '\033': // ESC字符
            g_escape_sequence = 1;
            return;
            
        case '\r': // 回车
        case '\n': // 换行
            if (g_shell_ctx.cmd_len > 0) {
                g_shell_ctx.cmd_buffer[g_shell_ctx.cmd_len] = '\0';
                shell_printf("\r\n");
                shell_parse_cmd();
                shell_execute_cmd();
                // 添加到历史命令
                if (g_shell_ctx.history_count < SHELL_HISTORY_MAX) {
                    strcpy(g_shell_ctx.history[g_shell_ctx.history_count], g_shell_ctx.cmd_buffer);
                    g_shell_ctx.history_count++;
                } else {
                    // 循环替换历史命令
                    for (int i = 0; i < SHELL_HISTORY_MAX - 1; i++) {
                        strcpy(g_shell_ctx.history[i], g_shell_ctx.history[i+1]);
                    }
                    strcpy(g_shell_ctx.history[SHELL_HISTORY_MAX-1], g_shell_ctx.cmd_buffer);
                }
                g_shell_ctx.history_index = g_shell_ctx.history_count;
            } else {
                // 即使命令为空也要换行
                shell_printf("\r\n");
            }
            g_shell_ctx.cmd_len = 0;
            shell_print_prompt();
            break;
            
        case '\b': // 退格
        case 127:  // 删除
            if (g_shell_ctx.cmd_len > 0) {
                g_shell_ctx.cmd_len--;
                // 发送退格、空格、再退格的序列来正确删除字符
                shell_printf("\b \b");
            }
            break;
            
        case '\t': // 制表符自动补全
            // TODO: 实现自动补全功能
            break;
            
        case 0x03: // Ctrl+C
            g_shell_ctx.cmd_len = 0;
            shell_printf("^C\r\n");
            shell_print_prompt();
            break;
            
        default:
            if (g_shell_ctx.cmd_len < SHELL_CMD_MAX_LENGTH - 1) {
                if (ch >= 32 && ch <= 126) { // 可打印字符
                    g_shell_ctx.cmd_buffer[g_shell_ctx.cmd_len++] = ch;
                    shell_printf("%c", ch);
                }
            }
            break;
    }
}


// 初始化shell
void shell_init() {
    // 初始化上下文
    memset(&g_shell_ctx, 0, sizeof(g_shell_ctx));
    // 初始化动态命令表
    memset(g_dynamic_shell_cmds, 0, sizeof(g_dynamic_shell_cmds));
    g_dynamic_cmd_count = 0;

    if (SHELL_RTT)
    {
        SEGGER_RTT_Init();
    }
    else
    {
        // 初始化UART设备用于shell
        UART_Device_init_config uart_config = {
            .huart = &SHELL_COM,          
            .expected_rx_len = 0,         
            .rx_buf = (uint8_t (*)[2])shell_rx_buffer,
            .rx_buf_size = SHELL_BUFFER_SIZE,
            .rx_mode = UART_MODE_IT,
            .tx_mode = UART_MODE_IT,
        };
        
        g_shell_ctx.uart_dev = BSP_UART_Device_Init(&uart_config);        
    }
    g_shell_ctx.running = false;
    g_shell_ctx.initialized = true;
    
    // 创建互斥锁
    osal_mutex_create(&g_shell_ctx.mutex, "Shell Mutex");
    
    // 创建shell线程
    osal_thread_create(&g_shell_ctx.thread, "Shell Thread",shell_thread_entry,
                      0,shell_thread_stack,
                      SHELL_THREAD_STACK_SIZE,SHELL_THREAD_PRIORITY); 
    osal_thread_start(&g_shell_ctx.thread);
    g_shell_ctx.running = true;
    shell_printf("\r\n");
    shell_printf("  __  __    _    ____\r\n");
    shell_printf(" |  \\/  |  / \\  / ___|\r\n");
    shell_printf(" | |\\/| | / _ \\ \\___ \\\r\n");
    shell_printf(" | |  | |/ ___ \\ ___) |\r\n");
    shell_printf(" |_|  |_/_/   \\_\\____/\r\n");
    shell_printf("\r\n");
    shell_print_prompt();
}

void shell_rtt_process_no_lock(void) {
    uint8_t len =0;
    // 无锁版本
    while ((len = SEGGER_RTT_ReadNoLock(0, shell_rx_buffer[0], sizeof(uint8_t)* SHELL_BUFFER_SIZE)) > 0) {
        for (uint32_t i = 0; i < len; i++) {
            shell_handle_char(shell_rx_buffer[0][i]);
        }
    }
}

// shell线程入口函数
static void shell_thread_entry(ULONG input) {
    while (g_shell_ctx.running && g_shell_ctx.initialized) {
        if (SHELL_RTT)
        {
            shell_rtt_process_no_lock();
            osal_delay_ms(100);
        }
        else
        {
            uint8_t* rx_data = BSP_UART_Read(g_shell_ctx.uart_dev);
            if (rx_data != NULL) {
                uint8_t len = g_shell_ctx.uart_dev->real_rx_len;
                if (len > 0) {
                    for (uint32_t i = 0; i < len; i++) {
                        shell_handle_char(rx_data[i]);
                    }
                }
            }
        }
    }
}

// 打印提示符
static void shell_print_prompt(void) {
    if (!g_shell_ctx.initialized) {
        return;
    }
    
    shell_printf(SHELL_PROMPT);
}

// 解析命令
static void shell_parse_cmd(void) {
    if (!g_shell_ctx.initialized) {
        return;
    }
    
    g_shell_ctx.argc = shell_split_args(g_shell_ctx.cmd_buffer);
}

// 执行命令
static void shell_execute_cmd(void) {
    if (!g_shell_ctx.initialized || g_shell_ctx.argc == 0) {
        return;
    }
    
    const shell_cmd_t *cmd = shell_find_cmd(g_shell_ctx.args[0]);
    if (cmd != NULL) {
        cmd->func(g_shell_ctx.argc, g_shell_ctx.args);
    } else {
        shell_printf("Unknown command: %s\r\n", g_shell_ctx.args[0]);
    }
}

// 分割参数
static int shell_split_args(char *cmdline) {
    int argc = 0;
    char *token;
    char *saveptr;
    
    if (!cmdline) {
        return 0;
    }
    
    // 处理命令行参数
    token = strtok_r(cmdline, " ", &saveptr);
    while (token != NULL && argc < SHELL_MAX_ARGS) {
        g_shell_ctx.args[argc++] = token;
        token = strtok_r(NULL, " ", &saveptr);
    }
    
    return argc;
}

// 查找命令（包括内置命令和动态注册命令）
static const shell_cmd_t* shell_find_cmd(const char *name) {
    if (!name) {
        return NULL;
    }
    
    // 先在内置命令表中查找
    for (int i = 0; g_shell_cmds[i].name != NULL; i++) {
        if (strcmp(g_shell_cmds[i].name, name) == 0) {
            return &g_shell_cmds[i];
        }
    }
    
    // 再在动态注册命令表中查找
    for (int i = 0; i < g_dynamic_cmd_count; i++) {
        if (g_dynamic_shell_cmds[i].name && strcmp(g_dynamic_shell_cmds[i].name, name) == 0) {
            return &g_dynamic_shell_cmds[i];
        }
    }
    
    return NULL;
}

// 格式化打印函数
void shell_printf(const char *fmt, ...) {
    static char buffer[256];
    va_list args;
    int len;
    
    if (!g_shell_ctx.initialized || !fmt) {
        return;
    }
    
    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    if (SHELL_RTT)
    {
        if (len > 0) {
            tx_mutex_get(&g_shell_ctx.mutex, TX_WAIT_FOREVER);
            RTT_WriteDataSkip(0, (uint8_t*)buffer, (uint16_t)len);
            tx_mutex_put(&g_shell_ctx.mutex);
        }
    }
    else
    {
        if (len > 0 && g_shell_ctx.uart_dev) {
            tx_mutex_get(&g_shell_ctx.mutex, TX_WAIT_FOREVER);
            BSP_UART_Send(g_shell_ctx.uart_dev, (uint8_t*)buffer, (uint16_t)len);
            tx_mutex_put(&g_shell_ctx.mutex);
        }
    }
}

void shell_send(const uint8_t *data, uint16_t len) {
    if (!g_shell_ctx.initialized) {
        return;
    }
    
    if (SHELL_RTT)
    {
        if (len > 0) {
            tx_mutex_get(&g_shell_ctx.mutex, TX_WAIT_FOREVER);
            RTT_WriteDataSkip(0, (uint8_t*)data, (uint16_t)len);
            tx_mutex_put(&g_shell_ctx.mutex);
        }
    }
    else
    {
        if (len > 0 && g_shell_ctx.uart_dev) {
            tx_mutex_get(&g_shell_ctx.mutex, TX_WAIT_FOREVER);
            BSP_UART_Send(g_shell_ctx.uart_dev, (uint8_t*)data, (uint16_t)len);
            tx_mutex_put(&g_shell_ctx.mutex);
        }
    }
}


// help命令实现
void shell_help_cmd(int argc, char **argv) {
    shell_printf("Available commands:\r\n");
    
    // 显示内置命令
    for (int i = 0; g_shell_cmds[i].name != NULL; i++) {
        shell_printf("  %-10s - %s\r\n", g_shell_cmds[i].name, g_shell_cmds[i].description);
    }
    
    // 显示动态注册命令
    for (int i = 0; i < g_dynamic_cmd_count; i++) {
        if (g_dynamic_shell_cmds[i].name) {
            shell_printf("  %-10s - %s\r\n", g_dynamic_shell_cmds[i].name, 
                         g_dynamic_shell_cmds[i].description ? g_dynamic_shell_cmds[i].description : "No description");
        }
    }
    
    shell_printf("\r\n");
}

// clear命令实现
void shell_clear_cmd(int argc, char **argv) {
    // 发送ANSI清屏序列
    shell_printf("\033[2J\033[H");
}

// version命令实现
void shell_version_cmd(int argc, char **argv) {
    shell_printf("osal Shell System v1.0\r\n");
    shell_printf("Built with osal\r\n");
}

// 注册函数接口
int shell_register_function(const char* name, shell_cmd_func_t func, const char* description) {
    // 检查参数
    if (!name || !func) {
        return -1;
    }
    
    // 检查是否已达到最大命令数
    if (g_dynamic_cmd_count >= SHELL_MAX_DYNAMIC_COMMANDS) {
        shell_printf("Error: Maximum number of dynamic commands reached.\r\n");
        return -1;
    }
    
    // 检查命令是否已存在
    if (shell_find_cmd(name) != NULL) {
        shell_printf("Error: Command '%s' already exists.\r\n", name);
        return -1;
    }
    
    // 添加命令到动态命令表
    g_dynamic_shell_cmds[g_dynamic_cmd_count].name = name;
    g_dynamic_shell_cmds[g_dynamic_cmd_count].func = func;
    g_dynamic_shell_cmds[g_dynamic_cmd_count].description = description;
    g_dynamic_cmd_count++;
    
    return 0;
}

