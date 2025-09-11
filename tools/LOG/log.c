/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 08:34:41
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 09:09:16
 * @FilePath: /rm_base/tools/LOG/log.c
 * @Description: 
 */
#include "log.h"
#include "osal_def.h"
#include "bsp_dwt.h"

#include "shell.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// 默认日志级别
static int g_log_level = LOG_OUTPUT_LEVEL;

// 多线程保护的互斥锁
static osal_mutex_t log_mutex;
static int mutex_initialized = 0;

#if !SHELL_ENABLE 
    #if LOG_RTT
    #include "SEGGER_RTT.h"
    #else
    #include "bsp_uart.h"
    // UART设备指针
    static UART_Device *log_uart_dev = NULL;
    static uint8_t log_uart_rx_buf[2][32];
    #endif
#endif

// 日志级别名称
static const char* level_names[] = {
    "VERBOSE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

#if LOG_COLOR_ENABLE
// 日志级别颜色
static const char* level_colors[] = {
    LOG_COLOR_VERBOSE,
    LOG_COLOR_DEBUG,
    LOG_COLOR_INFO,
    LOG_COLOR_WARN,
    LOG_COLOR_ERROR,
    LOG_COLOR_FATAL
};
#endif

// 使用DWT实现时间戳函数
static char* get_dwt_timestamp(void) {
    static char timestamp_str[32];
    uint64_t us = DWT_GetTimeline_us();
    // 格式化为 [秒.微秒] 格式，例如 [123.456789]
    snprintf(timestamp_str, sizeof(timestamp_str), "[%lu.%06lu]", 
             (unsigned long)(us / 1000000),
             (unsigned long)(us % 1000000));
    return timestamp_str;
}

void log_init(void)
{
    if (!mutex_initialized) {
        osal_status_t status = osal_mutex_create(&log_mutex, "log_mutex");
        if (status == OSAL_SUCCESS) {
            mutex_initialized = 1;
        }
    }
    
#if !SHELL_ENABLE
    #if LOG_RTT
        // 初始化RTT
        SEGGER_RTT_Init();
    #else
        UART_Device_init_config uart_config = {
            .huart = &LOG_COM,
            .rx_buf = (uint8_t (*)[2])log_uart_rx_buf,
            .rx_buf_size = 32,
            .expected_rx_len = 0,
            .rx_mode = UART_MODE_IT,
            .tx_mode = UART_MODE_IT
        };
        log_uart_dev = BSP_UART_Device_Init(&uart_config);
    #endif
#endif

}

void log_set_level(int level)
{
    g_log_level = level;
}

static void lock_log(void)
{
    if (mutex_initialized) {
        osal_mutex_lock(&log_mutex, OSAL_WAIT_FOREVER);
    }
}

static void unlock_log(void)
{
    if (mutex_initialized) {
        osal_mutex_unlock(&log_mutex);
    }
}

static void log_output(const char* str, size_t len)
{
#if SHELL_ENABLE
    // 使用shell输出
    shell_send((const uint8_t*)str, (uint16_t)len);
#elif LOG_RTT
    // 使用RTT输出
    SEGGER_RTT_Write(0, str, len);
#else
    // 使用UART输出
    if (log_uart_dev) {
        BSP_UART_Send(log_uart_dev, (uint8_t*)str, (uint16_t)len);
    }
#endif
}

void log_write(int level, const char* tag, const char* format, ...)
{
    if (level < g_log_level) {
        return;
    }

    // 多线程保护上锁
    lock_log();

    static char buffer[256];
    char *p = buffer;
    size_t remaining = sizeof(buffer);
    size_t len;

#if LOG_COLOR_ENABLE
    // 添加颜色代码
    if (level < sizeof(level_colors) / sizeof(level_colors[0])) {
        len = snprintf(p, remaining, "%s", level_colors[level]);
        if (len >= remaining) goto end;
        p += len;
        remaining -= len;
    }
#endif
    
#if LOG_TIMSTAMP_ENABLE
    // 获取时间戳
    char* timestamp = get_dwt_timestamp();
    len = snprintf(p, remaining, "%s", timestamp);
    if (len >= remaining) goto end;
    p += len;
    remaining -= len;
#endif

    // 添加日志级别和标签
    len = snprintf(p, remaining, "[%s][%s]:", level_names[level], tag ? tag : "NO_TAG");
    if (len >= remaining) goto end;
    p += len;
    remaining -= len;

    // 添加日志内容
    va_list args;
    va_start(args, format);
    len = vsnprintf(p, remaining, format, args);
    va_end(args);
    if (len >= remaining) goto end;
    p += len;
    remaining -= len;

#if LOG_COLOR_ENABLE
    // 重置颜色
    len = snprintf(p, remaining, "%s", LOG_COLOR_RESET);
    if (len >= remaining) goto end;
    p += len;
    remaining -= len;
#endif
    
    // 换行
    len = snprintf(p, remaining, "\r\n");
    if (len >= remaining) goto end;
    p += len;

end:
    // 输出日志
    log_output(buffer, p - buffer);

    // 多线程保护解锁
    unlock_log();
}