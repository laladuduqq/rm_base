# OSAL (操作系统抽象层) 文档

## 概述

OSAL（操作系统抽象层）是一个为嵌入式系统设计的抽象层，提供了一套统一的API接口，使得应用程序可以在不同的RTOS（实时操作系统）之间移植，而无需修改应用程序代码。

## 支持的RTOS类型

```c
#define OSAL_THREADX       (1)
#define OSAL_FREERTOS      (2)
```

## 数据类型定义

### 通用状态码

```c
typedef enum {
    OSAL_SUCCESS = 0,      // 成功
    OSAL_ERROR,            // 错误
    OSAL_INVALID_PARAM,    // 无效参数
    OSAL_NO_MEMORY,        // 内存不足
    OSAL_TIMEOUT           // 超时
} osal_status_t;
```

### 时间类型

```c
typedef ULONG osal_tick_t;  // ThreadX
typedef TickType_t osal_tick_t;  // FreeRTOS
```

### 常量定义

```c
#define OSAL_WAIT_FOREVER        ((osal_tick_t)-1)  // 永久等待
#define OSAL_NO_WAIT             (0)               // 不等待
```

## 线程管理

### 数据类型

```c
// ThreadX
typedef TX_THREAD osal_thread_t;

// FreeRTOS
typedef struct {
    StaticTask_t task_buffer;  // 任务控制块缓冲区
    TaskHandle_t task_handle;  // 任务句柄
} osal_thread_t;
```

### API接口

```c
osal_status_t osal_thread_create(osal_thread_t *thread,
                                 const char *name,
                                 osal_thread_entry_t entry,
                                 void *argument,
                                 void *stack_pointer,
                                 unsigned int stack_size,
                                 osal_thread_priority_t priority);
```

```c
osal_status_t osal_thread_start(osal_thread_t *thread);
```

```c
osal_status_t osal_thread_stop(osal_thread_t *thread);
```

```c
osal_status_t osal_thread_delete(osal_thread_t *thread);
```

### 使用示例

```c
#include "osal_def.h"

// 线程入口函数
void thread_entry(void *argument) {
    while(1) {
        // 线程工作代码
        osal_delay_ms(1000); // 延时1秒
    }
}

// 线程栈空间
static uint8_t thread_stack[1024];

// 线程句柄
static osal_thread_t my_thread;

void example_thread_usage(void) {
    // 创建线程
    osal_status_t status = osal_thread_create(&my_thread, 
                                              "MyThread", 
                                              thread_entry, 
                                              NULL, 
                                              thread_stack, 
                                              sizeof(thread_stack), 
                                              15);

    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 启动线程
    status = osal_thread_start(&my_thread);
    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 线程现在开始运行

    // 停止线程
    osal_thread_stop(&my_thread);

    // 删除线程
    osal_thread_delete(&my_thread);
}
```

## 信号量管理

### 数据类型

```c
// ThreadX
typedef TX_SEMAPHORE osal_sem_t;

// FreeRTOS
typedef struct {
    SemaphoreHandle_t sem_handle;
    StaticSemaphore_t sem_buffer;
} osal_sem_t;
```

### API接口

```c
osal_status_t osal_sem_create(osal_sem_t *sem, const char *name, unsigned int initial_count);
```

```c
osal_status_t osal_sem_wait(osal_sem_t *sem, osal_tick_t timeout);
```

```c
osal_status_t osal_sem_post(osal_sem_t *sem);
```

```c
osal_status_t osal_sem_delete(osal_sem_t *sem);
```

### 使用示例

```c
#include "osal_def.h"

// 信号量句柄
static osal_sem_t my_sem;

void example_semaphore_usage(void) {
    // 创建信号量，初始计数值为2
    osal_status_t status = osal_sem_create(&my_sem, "MySemaphore", 2);
    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 获取信号量，等待时间为1000个tick
    status = osal_sem_wait(&my_sem, 1000);
    if (status == OSAL_SUCCESS) {
        // 成功获取信号量
        // 执行需要同步保护的代码

        // 释放信号量
        osal_sem_post(&my_sem);
    } else if (status == OSAL_TIMEOUT) {
        // 等待超时
    } else {
        // 其他错误
    }

    // 删除信号量
    osal_sem_delete(&my_sem);
}

// 在中断服务例程中释放信号量的示例
void some_interrupt_handler(void) {
    // 处理中断

    // 释放信号量
    osal_sem_post(&my_sem);
}
```

## 互斥量管理

### 数据类型

```c
// ThreadX
typedef TX_MUTEX osal_mutex_t;

// FreeRTOS
typedef struct {
    SemaphoreHandle_t mutex_handle;
    StaticSemaphore_t mutex_buffer;
} osal_mutex_t;
```

### API接口

```c
osal_status_t osal_mutex_create(osal_mutex_t *mutex, const char *name);
```

```c
osal_status_t osal_mutex_lock(osal_mutex_t *mutex, osal_tick_t timeout);
```

```c
osal_status_t osal_mutex_unlock(osal_mutex_t *mutex);
```

```c
osal_status_t osal_mutex_delete(osal_mutex_t *mutex);
```

### 使用示例

```c
#include "osal_def.h"

// 互斥量句柄
static osal_mutex_t my_mutex;

// 共享资源
static int shared_resource = 0;

void example_mutex_usage(void) {
    // 创建互斥量
    osal_status_t status = osal_mutex_create(&my_mutex, "MyMutex");
    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 获取互斥量
    status = osal_mutex_lock(&my_mutex, OSAL_WAIT_FOREVER);
    if (status == OSAL_SUCCESS) {
        // 成功获取互斥量，可以安全访问共享资源
        shared_resource++;

        // 释放互斥量
        osal_mutex_unlock(&my_mutex);
    }

    // 删除互斥量
    osal_mutex_delete(&my_mutex);
}
```

## 事件管理

### 数据类型

```c
// ThreadX
typedef TX_EVENT_FLAGS_GROUP osal_event_t;

// FreeRTOS
typedef struct {
    EventGroupHandle_t handle;
    StaticEventGroup_t buffer;
} osal_event_t;
```

### 事件等待选项

```c
#define OSAL_EVENT_WAIT_FLAG_AND    0x01U  // 等待所有指定的事件标志都被设置
#define OSAL_EVENT_WAIT_FLAG_OR     0x02U  // 等待任何指定的事件标志被设置
#define OSAL_EVENT_WAIT_FLAG_CLEAR  0x04U  // 在等待完成后清除所等待的事件标志
```

### API接口

```c
osal_status_t osal_event_create(osal_event_t *event, const char *name);
```

```c
osal_status_t osal_event_set(osal_event_t *event, unsigned int flags);
```

```c
osal_status_t osal_event_clear(osal_event_t *event, unsigned int flags);
```

```c
osal_status_t osal_event_delete(osal_event_t *event);
```

### 使用示例

```c
#include "osal_def.h"

// 事件组句柄
static osal_event_t my_event;

// 定义事件标志
#define EVENT_FLAG_TASK_1_READY    0x01
#define EVENT_FLAG_TASK_2_READY    0x02
#define EVENT_FLAG_DATA_READY      0x04

void example_event_usage(void) {
    // 创建事件组
    osal_status_t status = osal_event_create(&my_event, "MyEvent");
    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 设置事件标志
    osal_event_set(&my_event, EVENT_FLAG_TASK_1_READY);

    // 等待单个事件标志
    unsigned int actual_flags;
    status = osal_event_wait(&my_event, 
                             EVENT_FLAG_DATA_READY, 
                             OSAL_EVENT_WAIT_FLAG_OR, 
                             OSAL_WAIT_FOREVER, 
                             &actual_flags);

    if (status == OSAL_SUCCESS) {
        // 成功等待到事件
        if (actual_flags & EVENT_FLAG_DATA_READY) {
            // 处理数据
        }
    }

    // 等待多个事件标志（AND逻辑）
    status = osal_event_wait(&my_event, 
                             EVENT_FLAG_TASK_1_READY | EVENT_FLAG_TASK_2_READY,
                             OSAL_EVENT_WAIT_FLAG_AND,
                             1000,
                             &actual_flags);

    if (status == OSAL_SUCCESS) {
        // 两个事件标志都已设置
    }

    // 等待事件并自动清除
    status = osal_event_wait(&my_event,
                             EVENT_FLAG_DATA_READY,
                             OSAL_EVENT_WAIT_FLAG_OR | OSAL_EVENT_WAIT_FLAG_CLEAR,
                             OSAL_WAIT_FOREVER,
                             &actual_flags);

    // 删除事件组
    osal_event_delete(&my_event);
}

// 在中断中设置事件标志
void some_interrupt_handler(void) {
    // 处理中断

    // 设置事件标志
    osal_event_set(&my_event, EVENT_FLAG_DATA_READY);
}
```

## 定时器管理

### 数据类型

```c
// ThreadX
typedef TX_TIMER osal_timer_t;
typedef VOID (*osal_timer_callback_t)(ULONG);

// FreeRTOS
typedef struct {
    TimerHandle_t handle;
    StaticTimer_t buffer;
} osal_timer_t;
typedef void (*osal_timer_callback_t)(TimerHandle_t xTimer);
```

### 定时器模式

```c
typedef enum {
    OSAL_TIMER_MODE_ONCE    = 0,  // 单次触发模式
    OSAL_TIMER_MODE_PERIODIC = 1  // 周期触发模式
} osal_timer_mode_t;
```

### API接口

```c
osal_status_t osal_timer_create(osal_timer_t *timer, 
                                const char *name,
                                osal_timer_callback_t callback,
                                void *argument,
                                unsigned int timeout_ms,
                                osal_timer_mode_t mode);
```

```c
osal_status_t osal_timer_start(osal_timer_t *timer);
```

```c
osal_status_t osal_timer_stop(osal_timer_t *timer);
```

```c
osal_status_t osal_timer_change_period(osal_timer_t *timer, unsigned int timeout_ms);
```

```c
osal_status_t osal_timer_delete(osal_timer_t *timer);
```

```c
uint8_t osal_timer_is_active(osal_timer_t *timer);
```

### 使用示例

```c
#include "osal_def.h"

// 定时器句柄
static osal_timer_t my_timer;

// 定时器回调函数
void timer_callback(osal_timer_t *timer) {
    // 定时器到期处理


    // 可以设置事件标志、发送信号量等
}

void example_timer_usage(void) {
    // 创建周期性定时器，周期为1000ms
    osal_status_t status = osal_timer_create(&my_timer,
                                             "MyTimer",
                                             timer_callback,
                                             NULL,
                                             1000,
                                             OSAL_TIMER_MODE_PERIODIC);

    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 启动定时器
    status = osal_timer_start(&my_timer);
    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 检查定时器是否正在运行
    if (osal_timer_is_active(&my_timer)) {
        // 定时器正在运行
    }

    // 运行一段时间后修改定时器周期
    osal_timer_change_period(&my_timer, 2000); // 修改为2000ms

    // 停止定时器
    osal_timer_stop(&my_timer);

    // 删除定时器
    osal_timer_delete(&my_timer);
}
```

## 队列管理

### 数据类型

```c
// ThreadX
typedef TX_QUEUE osal_queue_t;

// FreeRTOS
typedef struct {
    QueueHandle_t handle;
    StaticQueue_t buffer;
} osal_queue_t;
```

### API接口

```c
osal_status_t osal_queue_create(osal_queue_t *queue, 
                                const char *name,
                                unsigned int msg_size,
                                unsigned int msg_count,
                                void *msg_buffer);
```

```c
osal_status_t osal_queue_send(osal_queue_t *queue, void *msg_ptr, osal_tick_t timeout);
```

```c
osal_status_t osal_queue_recv(osal_queue_t *queue, void *msg_ptr, osal_tick_t timeout);
```

```c
osal_status_t osal_queue_delete(osal_queue_t *queue);
```

### 使用示例

```c
#include "osal_def.h"

// 队列句柄
static osal_queue_t my_queue;

// 消息缓冲区
static uint8_t queue_buffer[10 * sizeof(uint32_t)]; // 可存储10个uint32_t的消息

void example_queue_usage(void) {
    // 创建队列，可存储10个uint32_t类型的消息
    osal_status_t status = osal_queue_create(&my_queue,
                                             "MyQueue",
                                             sizeof(uint32_t),
                                             10,
                                             queue_buffer);

    if (status != OSAL_SUCCESS) {
        // 处理错误
        return;
    }

    // 发送消息到队列
    uint32_t data_to_send = 12345;
    status = osal_queue_send(&my_queue, &data_to_send, OSAL_WAIT_FOREVER);
    if (status != OSAL_SUCCESS) {
        // 处理发送失败
        return;
    }

    // 从队列接收消息
    uint32_t received_data;
    status = osal_queue_recv(&my_queue, &received_data, OSAL_WAIT_FOREVER);
    if (status == OSAL_SUCCESS) {
        // 成功接收到消息
        // received_data 现在包含 12345
    }

    // 删除队列
    osal_queue_delete(&my_queue);
}

// 在中断服务例程中发送消息到队列
void some_interrupt_handler(void) {
    static uint32_t isr_data = 0;
    isr_data++;

    // 在中断中发送消息，不等待
    osal_queue_send(&my_queue, &isr_data, OSAL_NO_WAIT);
}
```

## 延时函数

### API接口

```c
void osal_delay_ms(unsigned int ms);
```

```c
void osal_delay_us(unsigned int us);
```

## 中断管理

### 数据类型

```c
// ThreadX
typedef UINT osal_critical_state_t;

// FreeRTOS
typedef BaseType_t osal_critical_state_t;
```

### API接口

```c
osal_status_t osal_enter_critical(osal_critical_state_t *crit);
```

```c
osal_status_t osal_exit_critical(osal_critical_state_t *crit);
```

### 使用示例

```c
#include "osal_def.h"

// 全局变量需要在中断和任务中访问
volatile uint32_t shared_counter = 0;

// 在任务中访问共享变量
void task_function(void *argument) {
    osal_critical_state_t crit;
    
    // 进入临界区
    osal_enter_critical(&crit);
    
    // 访问共享资源
    shared_counter++;
    
    // 退出临界区
    osal_exit_critical(&crit);
}

// 在中断服务例程中访问共享变量
void interrupt_handler(void) {
    osal_critical_state_t crit;
    
    // 进入临界区
    osal_enter_critical(&crit);
    
    // 访问共享资源
    shared_counter++;
    
    // 退出临界区
    osal_exit_critical(&crit);
}
```
