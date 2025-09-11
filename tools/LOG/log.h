/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 08:34:46
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 08:59:08
 * @FilePath: /rm_base/tools/LOG/log.h
 * @Description: 日志系统，支持tag、彩色输出、时间戳和多线程保护
 */
#ifndef _LOG_H_
#define _LOG_H_

#include "tools_config.h"
#include <stdarg.h>

// ANSI颜色代码
#define LOG_COLOR_BLACK   "\033[30m"
#define LOG_COLOR_RED     "\033[31m"
#define LOG_COLOR_GREEN   "\033[32m"
#define LOG_COLOR_YELLOW  "\033[33m"
#define LOG_COLOR_BLUE    "\033[34m"
#define LOG_COLOR_PURPLE  "\033[35m"
#define LOG_COLOR_CYAN    "\033[36m"
#define LOG_COLOR_WHITE   "\033[37m"
#define LOG_COLOR_RESET   "\033[0m"

// 日志级别
#define LOG_LEVEL_VERBOSE 0
#define LOG_LEVEL_DEBUG   1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_WARN    3
#define LOG_LEVEL_ERROR   4
#define LOG_LEVEL_FATAL   5

// 默认日志级别
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_VERBOSE
#endif

// 日志级别对应的颜色
#define LOG_COLOR_VERBOSE LOG_COLOR_CYAN
#define LOG_COLOR_DEBUG   LOG_COLOR_BLUE
#define LOG_COLOR_INFO    LOG_COLOR_GREEN
#define LOG_COLOR_WARN    LOG_COLOR_YELLOW
#define LOG_COLOR_ERROR   LOG_COLOR_RED
#define LOG_COLOR_FATAL   LOG_COLOR_PURPLE

// 默认tag名称
#ifndef log_tag
#define log_tag "DEFAULT"
#endif

#if LOG_ENABLED
#define LOG_INIT() log_init()
// 日志宏定义
#define LOG_VERBOSE(fmt, ...) do { \
    if (LOG_LEVEL <= LOG_LEVEL_VERBOSE) { \
        log_write(LOG_LEVEL_VERBOSE, log_tag, fmt, ##__VA_ARGS__); \
    } \
} while(0)
#define LOG_DEBUG(fmt, ...) do { \
    if (LOG_LEVEL <= LOG_LEVEL_DEBUG) { \
        log_write(LOG_LEVEL_DEBUG, log_tag, fmt, ##__VA_ARGS__); \
    } \
} while(0)
#define LOG_INFO(fmt, ...) do { \
    if (LOG_LEVEL <= LOG_LEVEL_INFO) { \
        log_write(LOG_LEVEL_INFO, log_tag, fmt, ##__VA_ARGS__); \
    } \
} while(0)

#define LOG_WARN(fmt, ...) do { \
    if (LOG_LEVEL <= LOG_LEVEL_WARN) { \
        log_write(LOG_LEVEL_WARN, log_tag, fmt, ##__VA_ARGS__); \
    } \
} while(0)
#define LOG_ERROR(fmt, ...) do { \
    if (LOG_LEVEL <= LOG_LEVEL_ERROR) { \
        log_write(LOG_LEVEL_ERROR, log_tag, fmt, ##__VA_ARGS__); \
    } \
} while(0)
#define LOG_FATAL(fmt, ...) do { \
    if (LOG_LEVEL <= LOG_LEVEL_FATAL) { \
        log_write(LOG_LEVEL_FATAL, log_tag, fmt, ##__VA_ARGS__); \
    } \
} while(0)
#else
#define LOG_INIT()            do {} while(0)
#define LOG_VERBOSE(fmt, ...) do {} while(0)
#define LOG_DEBUG(fmt, ...)   do {} while(0)
#define LOG_INFO(fmt, ...)    do {} while(0)
#define LOG_WARN(fmt, ...)    do {} while(0)
#define LOG_ERROR(fmt, ...)   do {} while(0)
#define LOG_FATAL(fmt, ...)   do {} while(0)
#endif

/**
 * @brief 初始化日志系统
 */
void log_init(void);

/**
 * @brief 写入日志
 * @param level 日志级别
 * @param tag 日志标签
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_write(int level, const char* tag, const char* format, ...);

/**
 * @brief 设置日志输出级别
 * @param level 日志级别
 */
void log_set_level(int level);

#endif // _LOG_H_