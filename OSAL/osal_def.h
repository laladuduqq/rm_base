/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-06 09:58:09
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-07 10:42:07
 * @FilePath: /rm_base/OSAL/osal_def.h
 * @Description: 
 */
#ifndef __OSAL_DEF_H__
#define __OSAL_DEF_H__

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* OSAL支持的RTOS类型 */
#define OSAL_THREADX       (1)
#define OSAL_FREERTOS      (2)

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
    #include "semphr.h"
    #include "event_groups.h"
    #else
    #error "OSAL_RTOS_TYPE is not defined"
    #endif
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
#if (OSAL_RTOS_TYPE == OSAL_THREADX)
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

/* 信号量相关类型定义 */
#if (OSAL_RTOS_TYPE == OSAL_THREADX)
typedef TX_SEMAPHORE osal_sem_t;
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
// FreeRTOS下，osal_sem_t需要包含StaticSemaphore_t结构体
typedef struct {
    SemaphoreHandle_t sem_handle;
    StaticSemaphore_t sem_buffer;
} osal_sem_t;
#endif

/* 互斥量相关类型定义 */
#if (OSAL_RTOS_TYPE == OSAL_THREADX)
typedef TX_MUTEX osal_mutex_t;
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
// FreeRTOS下，osal_mutex_t需要包含StaticSemaphore_t结构体
typedef struct {
    SemaphoreHandle_t mutex_handle;
    StaticSemaphore_t mutex_buffer;
} osal_mutex_t;
#endif

/* 事件相关类型定义 */
#if (OSAL_RTOS_TYPE == OSAL_THREADX)
typedef TX_EVENT_FLAGS_GROUP osal_event_t;
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
// FreeRTOS下，osal_event_t需要包含StaticEventGroup_t结构体
typedef struct {
    EventGroupHandle_t handle;
    StaticEventGroup_t buffer;
} osal_event_t;
#endif
/* 事件等待选项 */
#define OSAL_EVENT_WAIT_FLAG_AND            0x01U  /* 等待所有指定的事件标志都被设置 */
#define OSAL_EVENT_WAIT_FLAG_OR             0x02U  /* 等待任何指定的事件标志被设置 */
#define OSAL_EVENT_WAIT_FLAG_CLEAR          0x04U  /* 在等待完成后清除所等待的事件标志 */


/* 定时器类型定义 */
#if (OSAL_RTOS_TYPE == OSAL_THREADX)
typedef TX_TIMER osal_timer_t;
typedef VOID (*osal_timer_callback_t)(ULONG);
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
typedef struct {
    TimerHandle_t handle;
    StaticTimer_t buffer;
} osal_timer_t;
// 严格按照FreeRTOS标准定义回调函数类型
typedef void (*osal_timer_callback_t)(TimerHandle_t xTimer);
#endif

/* 定时器模式 */
typedef enum {
    OSAL_TIMER_MODE_ONCE    = 0,  /* 单次触发模式 */
    OSAL_TIMER_MODE_PERIODIC = 1  /* 周期触发模式 */
} osal_timer_mode_t;

/* 时间类型定义 */
#if (OSAL_RTOS_TYPE == OSAL_THREADX)
typedef ULONG osal_tick_t;
#elif (OSAL_RTOS_TYPE == OSAL_FREERTOS)
typedef TickType_t osal_tick_t;
#endif


/* 常量定义 */
#define OSAL_WAIT_FOREVER        ((osal_tick_t)-1)
#define OSAL_NO_WAIT             (0)

/* 函数声明 */

// 线程部分
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
// 信号量部分
/**
 * @description: 创建信号量
 * @param {osal_sem_t*} sem, 信号量句柄指针
 * @param {const char*} name, 信号量名称
 * @param {unsigned int} initial_count, 信号量初始计数值
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_sem_create(osal_sem_t *sem, const char *name, unsigned int initial_count);
/**
 * @description: 等待(获取)信号量
 * @param {osal_sem_t*} sem, 信号量句柄指针
 * @param {osal_tick_t} timeout, 超时时间，OSAL_WAIT_FOREVER表示永久等待，OSAL_NO_WAIT表示不等待
 * @return {osal_status_t} OSAL_SUCCESS - 成功获取信号量, OSAL_TIMEOUT - 超时, OSAL_ERROR - 错误, OSAL_INVALID_PARAM - 参数无效
 */
osal_status_t osal_sem_wait(osal_sem_t *sem, osal_tick_t timeout);
/**
 * @description: 释放(发送)信号量
 * @param {osal_sem_t*} sem, 信号量句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功释放信号量, OSAL_ERROR - 错误, OSAL_INVALID_PARAM - 参数无效
 */
osal_status_t osal_sem_post(osal_sem_t *sem);
/**
 * @description: 删除信号量
 * @param {osal_sem_t*} sem, 信号量句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功删除信号量, OSAL_ERROR - 错误, OSAL_INVALID_PARAM - 参数无效
 */
osal_status_t osal_sem_delete(osal_sem_t *sem); 
/**
 * @description: 创建互斥量
 * @param {osal_mutex_t*} mutex, 互斥量句柄指针
 * @param {const char*} name, 互斥量名称
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_mutex_create(osal_mutex_t *mutex, const char *name);
/**
 * @description: 获取互斥量
 * @param {osal_mutex_t*} mutex, 互斥量句柄指针
 * @param {osal_tick_t} timeout, 超时时间
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败, OSAL_TIMEOUT - 超时
 */
osal_status_t osal_mutex_lock(osal_mutex_t *mutex, osal_tick_t timeout);
/**
 * @description: 释放互斥量
 * @param {osal_mutex_t*} mutex, 互斥量句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_mutex_unlock(osal_mutex_t *mutex);
/**
 * @description: 删除互斥量
 * @param {osal_mutex_t*} mutex, 互斥量句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_mutex_delete(osal_mutex_t *mutex);
// 事件部分
/**
 * @description: 创建事件组
 * @param {osal_event_t*} event, 事件组句柄指针
 * @param {const char*} name, 事件组名称
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_event_create(osal_event_t *event, const char *name);
/**
 * @description: 设置事件标志
 * @param {osal_event_t*} event, 事件组句柄指针
 * @param {unsigned int} flags, 要设置的事件标志
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_event_set(osal_event_t *event, unsigned int flags);
/**
 * @description: 等待事件标志
 * @param {osal_event_t*} event, 事件组句柄指针
 * @param {unsigned int} requested_flags, 要等待的事件标志
 * @param {unsigned int} options, 等待选项
  *        OSAL_EVENT_WAIT_FLAG_AND - 等待所有指定的事件标志都被设置
  *        OSAL_EVENT_WAIT_FLAG_OR - 等待任何指定的事件标志被设置
  *        OSAL_EVENT_WAIT_FLAG_CLEAR - 在等待完成后清除所等待的事件标志
  *        可以组合使用，例如：OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR
 * @param {osal_tick_t} timeout, 超时时间
 * @param {unsigned int*} actual_flags, 实际返回的事件标志
 * @return {osal_status_t} OSAL_SUCCESS - 获取成功, OSAL_TIMEOUT - 超时, OSAL_ERROR - 获取失败, OSAL_INVALID_PARAM - 参数无效
 */
osal_status_t osal_event_wait(osal_event_t *event, unsigned int requested_flags, 
                              unsigned int options, osal_tick_t timeout, unsigned int *actual_flags);
/**
 * @description: 清除事件标志
 * @param {osal_event_t*} event, 事件组句柄指针
 * @param {unsigned int} flags, 要清除的事件标志
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_event_clear(osal_event_t *event, unsigned int flags);
/**
 * @description: 删除事件组
 * @param {osal_event_t*} event, 事件组句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_event_delete(osal_event_t *event);
// 定时器部分
/**
 * @description: 创建定时器
 * @param {osal_timer_t*} timer - 定时器句柄指针
 * @param {const char*} name - 定时器名称
 * @param {osal_timer_callback_t} callback - 定时器回调函数
 * @param {void*} argument - 传递给回调函数的参数
 * @param {unsigned int} timeout_ms - 定时器超时时间(毫秒)
 * @param {osal_timer_mode_t} mode - 定时器模式
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_timer_create(osal_timer_t *timer, 
                                const char *name,
                                osal_timer_callback_t callback,
                                void *argument,
                                unsigned int timeout_ms,
                                osal_timer_mode_t mode);

/**
 * @description: 启动定时器
 * @param {osal_timer_t*} timer - 定时器句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_timer_start(osal_timer_t *timer);

/**
 * @description: 停止定时器
 * @param {osal_timer_t*} timer - 定时器句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_timer_stop(osal_timer_t *timer);

/**
 * @description: 修改定时器超时时间
 * @param {osal_timer_t*} timer - 定时器句柄指针
 * @param {unsigned int} timeout_ms - 新的超时时间(毫秒)
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_timer_change_period(osal_timer_t *timer, unsigned int timeout_ms);

/**
 * @description: 删除定时器
 * @param {osal_timer_t*} timer - 定时器句柄指针
 * @return {osal_status_t} OSAL_SUCCESS - 成功, OSAL_ERROR - 失败
 */
osal_status_t osal_timer_delete(osal_timer_t *timer);

/**
 * @description: 检查定时器是否正在运行
 * @param {osal_timer_t*} timer - 定时器句柄指针
 * @return {uint8_t} 1 - 正在运行, 0 - 未运行
 */
uint8_t osal_timer_is_active(osal_timer_t *timer);

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