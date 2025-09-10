/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-10 14:46:33
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-10 21:52:01
 * @FilePath: /rm_base/tools/SHELL/shell_ps.c
 * @Description: 
 */
#include "shell.h"

#if OSAL_RTOS_TYPE == OSAL_THREADX
#include "tx_block_pool.h"
#include "tx_byte_pool.h"
#include "tx_event_flags.h"
#include "tx_mutex.h"
#include "tx_queue.h"
#include "tx_semaphore.h"
#include "tx_thread.h"
#include "tx_timer.h"

// ps命令实现
void shell_ps_cmd(int argc, char **argv) {
    if (argc < 2) {
        // 显示基本帮助信息
        shell_printf("Usage: ps <object_type>\r\n");
        shell_printf("Object types: thread, timer, mutex, sem, event, queue, bytepool, blockpool\r\n");
        shell_printf("\r\n");
        return;
    }
    
    if (strcmp(argv[1], "thread") == 0) {
        // 显示线程信息
        shell_printf("Thread Information:\r\n");
        shell_printf("%-32s %-8s %-10s %-12s %-8s\r\n", "Name", "State", "Priority", "Stack Size", "Run Count");
        shell_printf("------------------------------------------------------------------------\r\n");
        
        TX_THREAD *thread_ptr = _tx_thread_created_ptr;
        if (thread_ptr) {
            do {
                CHAR *name;
                UINT state;
                ULONG run_count;
                UINT priority;
                UINT preempt_threshold; 
                ULONG time_slice;
                TX_THREAD *next_thread;
                
                // 获取线程信息
                UINT status = tx_thread_info_get(thread_ptr, &name, &state, &run_count, 
                                                &priority, &preempt_threshold, &time_slice, 
                                                TX_NULL, &next_thread);
                
                if (status == TX_SUCCESS) {
                    char state_str[16];
                    switch (state) {
                        case TX_READY:
                            strcpy(state_str, "READY");
                            break;
                        case TX_COMPLETED:
                            strcpy(state_str, "COMPLETED");
                            break;
                        case TX_TERMINATED:
                            strcpy(state_str, "TERMINATED");
                            break;
                        case TX_SUSPENDED:
                            strcpy(state_str, "SUSPENDED");
                            break;
                        case TX_SLEEP:
                            strcpy(state_str, "SLEEP");
                            break;
                        case TX_QUEUE_SUSP:
                            strcpy(state_str, "QUEUE_SUSP");
                            break;
                        case TX_SEMAPHORE_SUSP:
                            strcpy(state_str, "SEMAPHORE_SUSP");
                            break;
                        case TX_EVENT_FLAG:
                            strcpy(state_str, "EVENT_FLAG");
                            break;
                        case TX_BLOCK_MEMORY:
                            strcpy(state_str, "BLOCK_MEMORY");
                            break;
                        case TX_BYTE_MEMORY:
                            strcpy(state_str, "BYTE_MEMORY");
                            break;
                        case TX_IO_DRIVER:
                            strcpy(state_str, "IO_DRIVER");
                            break;
                        case TX_FILE:
                            strcpy(state_str, "FILE");
                            break;
                        case TX_TCP_IP:
                            strcpy(state_str, "TCP_IP");
                            break;
                        case TX_MUTEX_SUSP:
                            strcpy(state_str, "MUTEX_SUSP");
                            break;
                        case TX_PRIORITY_CHANGE:
                            strcpy(state_str, "PRIORITY_CHANGE");
                            break;
                        default:
                            strcpy(state_str, "UNKNOWN");
                            break;
                    }
                    
                    shell_printf("%-32s %-8s %-10d %-12lu %-8lu\r\n", 
                                name ? name : "N/A", 
                                state_str, 
                                priority, 
                                thread_ptr -> tx_thread_stack_size,
                                run_count);
                }
                
                thread_ptr = thread_ptr -> tx_thread_created_next;
            } while (thread_ptr != _tx_thread_created_ptr);
        } else {
            shell_printf("No threads created.\r\n");
        }
        shell_printf("\r\n");
        return;
    } 
    else if (strcmp(argv[1], "timer") == 0) {
        // 显示定时器信息
        shell_printf("Timer Information:\r\n");
        shell_printf("%-16s %-8s %-12s\r\n", "Name", "Active", "Reschedules");
        shell_printf("----------------------------------------------------------------\r\n");
        
        TX_TIMER *timer_ptr = _tx_timer_created_ptr;
        if (timer_ptr && (ULONG)timer_ptr != TX_CLEAR_ID) {
            do {
                CHAR *name = TX_NULL;
                UINT active = 0;
                ULONG remaining_ticks = 0;
                ULONG reschedule_ticks = 0;
                TX_TIMER *next_timer = TX_NULL;
                
                UINT status = tx_timer_info_get(timer_ptr, &name, &active, &remaining_ticks,
                                               &reschedule_ticks, &next_timer);
                
                if (status == TX_SUCCESS) {
                    shell_printf("%-16s %-8s %-12lu\r\n",
                                name ? name : "N/A",
                                active ? "YES" : "NO",
                                reschedule_ticks);
                }
                
                timer_ptr = next_timer;
                if (timer_ptr == TX_NULL || (ULONG)timer_ptr == TX_CLEAR_ID) break;
            } while (timer_ptr != _tx_timer_created_ptr);
        } else {
            shell_printf("No timers created.\r\n");
        }
        shell_printf("\r\n");
        return;
    }
    else if (strcmp(argv[1], "mutex") == 0) {
        // 显示互斥量信息
        shell_printf("Mutex Information:\r\n");
        shell_printf("%-16s %-8s %-16s\r\n", "Name", "Count", "Owner");
        shell_printf("----------------------------------------------------------------\r\n");
        
        TX_MUTEX *mutex_ptr = _tx_mutex_created_ptr;
        if (mutex_ptr && (ULONG)mutex_ptr != TX_CLEAR_ID) {
            do {
                CHAR *name = TX_NULL;
                ULONG count_val = 0;
                TX_THREAD *owner = TX_NULL;
                TX_THREAD *first_suspended = TX_NULL;
                ULONG suspended_count = 0;
                TX_MUTEX *next_mutex = TX_NULL;
                
                UINT status = tx_mutex_info_get(mutex_ptr, &name, &count_val, &owner,
                                               &first_suspended, &suspended_count, &next_mutex);
                
                if (status == TX_SUCCESS) {
                    const char* owner_name = "NONE";
                    if (owner) {
                        if (owner->tx_thread_name) {
                            owner_name = owner->tx_thread_name;
                        } else {
                            owner_name = "N/A";
                        }
                    }
                    shell_printf("%-16s %-8lu %-16s\r\n",
                                name ? name : "N/A",
                                count_val,
                                owner_name);
                }
                
                mutex_ptr = next_mutex;
                if (mutex_ptr == TX_NULL || (ULONG)mutex_ptr == TX_CLEAR_ID) break;
            } while (mutex_ptr != _tx_mutex_created_ptr);
        } else {
            shell_printf("No mutexes created.\r\n");
        }
        shell_printf("\r\n");
        return;
    }
    else if (strcmp(argv[1], "sem") == 0) {
        // 显示信号量信息
        shell_printf("Semaphore Information:\r\n");
        shell_printf("%-16s %-12s %-12s\r\n", "Name", "Current Count", "Suspended");
        shell_printf("----------------------------------------------------------------\r\n");
        
        TX_SEMAPHORE *semaphore_ptr = _tx_semaphore_created_ptr;
        if (semaphore_ptr && (ULONG)semaphore_ptr != TX_CLEAR_ID) {
            do {
                CHAR *name = TX_NULL;
                ULONG current_value = 0;
                TX_THREAD *first_suspended = TX_NULL;
                ULONG suspended_count = 0;
                TX_SEMAPHORE *next_semaphore = TX_NULL;
                
                UINT status = tx_semaphore_info_get(semaphore_ptr, &name, &current_value,
                                                   &first_suspended, &suspended_count, &next_semaphore);
                
                if (status == TX_SUCCESS) {
                    shell_printf("%-16s %-12lu %-12lu\r\n",
                                name ? name : "N/A",
                                current_value,
                                suspended_count);
                }
                
                semaphore_ptr = next_semaphore;
                if (semaphore_ptr == TX_NULL || (ULONG)semaphore_ptr == TX_CLEAR_ID) break;
            } while (semaphore_ptr != _tx_semaphore_created_ptr);
        } else {
            shell_printf("No semaphores created.\r\n");
        }
        shell_printf("\r\n");
        return;
    }
    else if (strcmp(argv[1], "event") == 0) {
        // 显示事件标志组信息
        shell_printf("Event Flags Information:\r\n");
        shell_printf("%-16s %-12s %-12s\r\n", "Name", "Current Flags", "Suspended");
        shell_printf("----------------------------------------------------------------\r\n");
        
        TX_EVENT_FLAGS_GROUP *event_ptr = _tx_event_flags_created_ptr;
        if (event_ptr && (ULONG)event_ptr != TX_CLEAR_ID) {
            do {
                CHAR *name = TX_NULL;
                ULONG current_flags = 0;
                TX_THREAD *first_suspended = TX_NULL;
                ULONG suspended_count = 0;
                TX_EVENT_FLAGS_GROUP *next_event = TX_NULL;
                
                UINT status = tx_event_flags_info_get(event_ptr, &name, &current_flags,
                                                     &first_suspended, &suspended_count, &next_event);
                
                if (status == TX_SUCCESS) {
                    shell_printf("%-16s 0x%-10lx %-12lu\r\n",
                                name ? name : "N/A",
                                current_flags,
                                suspended_count);
                }
                
                event_ptr = next_event;
                if (event_ptr == TX_NULL || (ULONG)event_ptr == TX_CLEAR_ID) break;
            } while (event_ptr != _tx_event_flags_created_ptr);
        } else {
            shell_printf("No event flags created.\r\n");
        }
        shell_printf("\r\n");
        return;
    }
    else if (strcmp(argv[1], "queue") == 0) {
        // 显示队列信息
        shell_printf("Queue Information:\r\n");
        shell_printf("%-16s %-10s %-10s %-10s\r\n", "Name", "Enqueued", "Available", "Suspended");
        shell_printf("----------------------------------------------------------------\r\n");
        
        TX_QUEUE *queue_ptr = _tx_queue_created_ptr;
        if (queue_ptr && (ULONG)queue_ptr != TX_CLEAR_ID) {
            do {
                CHAR *name = TX_NULL;
                ULONG enqueued = 0;
                ULONG available_storage = 0;
                TX_THREAD *first_suspended = TX_NULL;
                ULONG suspended_count = 0;
                TX_QUEUE *next_queue = TX_NULL;
                
                UINT status = tx_queue_info_get(queue_ptr, &name, &enqueued, &available_storage,
                                               &first_suspended, &suspended_count, &next_queue);
                
                if (status == TX_SUCCESS) {
                    shell_printf("%-16s %-10lu %-10lu %-10lu\r\n",
                                name ? name : "N/A",
                                enqueued,
                                available_storage,
                                suspended_count);
                }
                
                queue_ptr = next_queue;
                if (queue_ptr == TX_NULL || (ULONG)queue_ptr == TX_CLEAR_ID) break;
            } while (queue_ptr != _tx_queue_created_ptr);
        } else {
            shell_printf("No queues created.\r\n");
        }
        shell_printf("\r\n");
        return;
    }
    else if (strcmp(argv[1], "bytepool") == 0) {
        // 显示字节池信息
        shell_printf("Byte Pool Information:\r\n");
        shell_printf("%-16s %-12s %-12s\r\n", "Name", "Available", "Fragments");
        shell_printf("----------------------------------------------------------------\r\n");
        
        TX_BYTE_POOL *byte_pool_ptr = _tx_byte_pool_created_ptr;
        if (byte_pool_ptr && (ULONG)byte_pool_ptr != TX_CLEAR_ID) {
            do {
                CHAR *name = TX_NULL;
                ULONG available_bytes = 0;
                ULONG fragments = 0;
                TX_THREAD *first_suspended = TX_NULL;
                ULONG suspended_count = 0;
                TX_BYTE_POOL *next_pool = TX_NULL;
                
                UINT status = tx_byte_pool_info_get(byte_pool_ptr, &name, &available_bytes,
                                                   &fragments, &first_suspended, 
                                                   &suspended_count, &next_pool);
                
                if (status == TX_SUCCESS) {
                    shell_printf("%-16s %-12lu %-12lu\r\n",
                                name ? name : "N/A",
                                available_bytes,
                                fragments);
                }
                
                byte_pool_ptr = next_pool;
                if (byte_pool_ptr == TX_NULL || (ULONG)byte_pool_ptr == TX_CLEAR_ID) break;
            } while (byte_pool_ptr != _tx_byte_pool_created_ptr);
        } else {
            shell_printf("No byte pools created.\r\n");
        }
        shell_printf("\r\n");
        return;
    }
    else if (strcmp(argv[1], "blockpool") == 0) {
        // 显示块池信息
        shell_printf("Block Pool Information:\r\n");
        shell_printf("%-16s %-10s %-10s %-10s\r\n", "Name", "Available", "Total", "Suspended");
        shell_printf("----------------------------------------------------------------\r\n");
        
        TX_BLOCK_POOL *block_pool_ptr = _tx_block_pool_created_ptr;
        if (block_pool_ptr && (ULONG)block_pool_ptr != TX_CLEAR_ID) {
            do {
                CHAR *name = TX_NULL;
                ULONG available_blocks = 0;  
                ULONG total_blocks = 0;     
                TX_THREAD *first_suspended = TX_NULL;
                ULONG suspended_count = 0;
                TX_BLOCK_POOL *next_pool = TX_NULL;
                
                UINT status = tx_block_pool_info_get(block_pool_ptr, &name, &available_blocks,
                                                    &total_blocks, &first_suspended,
                                                    &suspended_count, &next_pool);
                
                if (status == TX_SUCCESS) {
                    shell_printf("%-16s %-10lu %-10lu %-10lu\r\n",
                                name ? name : "N/A",
                                available_blocks,
                                total_blocks,
                                suspended_count);
                }
                
                block_pool_ptr = next_pool;
                if (block_pool_ptr == TX_NULL || (ULONG)block_pool_ptr == TX_CLEAR_ID) break;
            } while (block_pool_ptr != _tx_block_pool_created_ptr);
        } else {
            shell_printf("No block pools created.\r\n");
        }
        shell_printf("\r\n");
        return;
    }
    else {
        shell_printf("Unknown object type: %s\r\n", argv[1]);
        shell_printf("Supported types: thread, timer, mutex, sem, event, queue, bytepool, blockpool\r\n");
        shell_printf("\r\n");
        return;
    }
}
#else
void shell_ps_cmd(int argc, char **argv) {
    
}
#endif

