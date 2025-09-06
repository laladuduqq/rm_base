/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-06 09:58:09
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-06 15:03:58
 * @FilePath: /rm_base/OSAL/osal_def.h
 * @Description: 
 */
#ifndef __OSAL_DEF_H__
#define __OSAL_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* OSAL支持的RTOS类型 */
#define OSAL_BARE_METAL    (0)
#define OSAL_THREADX       (2)
#define OSAL_FREERTOS      (3)

/* 配置当前使用的RTOS类型，默认为裸机模式 */
#ifndef OSAL_RTOS_TYPE
#define OSAL_RTOS_TYPE     OSAL_THREADX
#endif

#ifdef OSAL_RTOS_TYPE
    #if OSAL_RTOS_TYPE == OSAL_THREADX
    #include "tx_api.h"
    #elif OSAL_RTOS_TYPE == OSAL_FREERTOS
    #include "FreeRTOS.h"
    #include "task.h"
    #elif OSAL_RTOS_TYPE != OSAL_BARE_METAL
    #error "OSAL_RTOS_TYPE is not defined correctly"
    #endif
    #else
    #error "OSAL_RTOS_TYPE is not defined"
#endif

/* OSAL通用数据类型定义 */
typedef enum {
    OSAL_SUCCESS = 0,
    OSAL_ERROR,
    OSAL_INVALID_PARAM,
    OSAL_NO_MEMORY,
    OSAL_TIMEOUT
} osal_status_t;

/* 线程相关类型定义 */
#if (OSAL_RTOS_TYPE == OSAL_BARE_METAL)
typedef void (*osal_thread_entry_t)(void *argument);
typedef void * osal_thread_t;
typedef unsigned int osal_thread_priority_t;
#elif (OSAL_RTOS_TYPE == OSAL_THREADX)
typedef TX_THREAD osal_thread_t;
typedef UINT osal_thread_priority_t;
typedef VOID (*osal_thread_entry_t)(ULONG);
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
// FreeRTOS下，osal_thread_t需要包含StaticTask_t结构体
typedef struct {
    StaticTask_t task_buffer;  // 任务控制块缓冲区
    TaskHandle_t task_handle;  // 任务句柄
} osal_thread_t;
typedef UBaseType_t osal_thread_priority_t;
typedef void (*osal_thread_entry_t)(void *);
#endif

/* 时间类型定义 */
#if (OSAL_RTOS_TYPE == OSAL_BARE_METAL)
typedef unsigned int osal_tick_t;
#elif (OSAL_RTOS_TYPE == OSAL_THREADX)
typedef ULONG osal_tick_t;
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
typedef TickType_t osal_tick_t;
#endif


/* 常量定义 */
#define OSAL_WAIT_FOREVER        ((osal_tick_t)-1)
#define OSAL_NO_WAIT             (0)

/* 函数声明 */
/**
 * @description: 创建线程
 * @param {osal_thread_t*} thread, 线程句柄指针
 * @param {const char*} name, 线程名称
 * @param {osal_thread_entry_t} entry, 线程入口函数
 * @param {void*} argument, 传递给线程入口函数的参数
 * @param {void*} stack_pointer, 线程栈指针
 * @param {unsigned int} stack_size, 线程栈大小
 * @param {osal_thread_priority_t} priority, 线程优先级
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_thread_create(osal_thread_t *thread,
                                 const char *name,
                                 osal_thread_entry_t entry,
                                 void *argument,
                                 void *stack_pointer,
                                 unsigned int stack_size,
                                 osal_thread_priority_t priority);
/**
 * @description: 启动线程
 * @param {osal_thread_t*} thread, 线程句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_thread_start(osal_thread_t *thread);
/**
 * @description: 终止线程
 * @param {osal_thread_t*} thread, 线程句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_thread_stop(osal_thread_t *thread);
/**
 * @description:  线程删除
 * @param {osal_thread_t*} thread， 线程句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_thread_delete(osal_thread_t *thread);

// 通用延时函数
/**
 * @description: 毫秒延时
 * @param {unsigned int} ms
 * @return {*}
 */
void osal_delay_ms(unsigned int ms);
/**
 * @description: 微秒延时
 * @param {unsigned int} us
 * @note: 该函数是nop延时，注意rtos下，时间过长会阻塞系统调度
 * @return {*}
 */
void osal_delay_us(unsigned int us);

#ifdef __cplusplus
}
#endif

#endif /* __OSAL_DEF_H__ */