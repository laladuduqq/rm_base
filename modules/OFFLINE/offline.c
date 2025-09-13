/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 19:45:50
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-13 10:08:13
 * @FilePath: /rm_base/modules/OFFLINE/offline.c
 * @Description: 
 */
#include "offline.h"
#include "beep.h"
#include "bsp_dwt.h"
#include "iwdg.h"
#include "modules_config.h"
#include "osal_def.h"
#include "rgb.h"
#include <stdint.h>
#include "shell.h"

#define log_tag  "offline"
#include "log.h"

#if OFFLINE_MODULE_ENABLE

// 静态变量
static OfflineManager_t offline_manager;
static osal_thread_t offline_thread;
static uint8_t current_beep_times;
OFFLINE_THREAD_STACK_SECTION static uint8_t offline_thread_stack[OFFLINE_THREAD_STACK_SIZE];
static void beep_ctrl_times(ULONG timer);
static void shell_offline_cmd(int argc, char **argv);

void offline_task(ULONG thread_input)
{    
    (void)(thread_input);
    #if OFFLINE_WATCHDOG_ENABLE
        __HAL_DBGMCU_FREEZE_IWDG();
        MX_IWDG_Init();
    #endif
    for (;;)
    {
        static uint8_t highest_error_level = 0;
        static uint8_t alarm_device_index = OFFLINE_INVALID_INDEX;
        uint32_t current_time = tx_time_get();
        
        // 重置错误状态
        highest_error_level = 0;
        alarm_device_index = OFFLINE_INVALID_INDEX;
        bool any_device_offline = false;
    
        // 检查所有设备状态
        for (uint8_t i = 0; i < offline_manager.device_count; i++) {
            OfflineDevice_t* device = &offline_manager.devices[i];
            
            if (!device->enable) {continue;}
    
            if (current_time - device->last_time > device->timeout_ms) {
                device->is_offline = STATE_OFFLINE;
                any_device_offline = true;
                
                // 更新最高优先级设备
                if (device->level > highest_error_level) {
                    highest_error_level = device->level;
                    alarm_device_index = i;
                    current_beep_times = device->beep_times;
                }
                // 相同优先级时的处理
                else if (device->level == highest_error_level) {
                    // 如果当前设备不需要蜂鸣（beep_times=0），保持原来的设备
                    if (device->beep_times == 0) {continue;}
                    // 如果之前选中的设备不需要蜂鸣，或者当前设备蜂鸣次数更少
                    if (current_beep_times == 0 || (device->beep_times > 0 && device->beep_times < current_beep_times)) {
                        alarm_device_index = i;
                        current_beep_times = device->beep_times;
                    }
                }
            } else {
                device->is_offline = STATE_ONLINE;
            }
        }
    
        // 触发报警或清除报警
        if (alarm_device_index != OFFLINE_INVALID_INDEX && any_device_offline) {
            // 已在上面设置了current_beep_times
        } else {
            // 所有设备都在线，清除报警
            current_beep_times = 0;
            RGB_show(LED_Green);              // 表示所有设备都在线
        }
        #if OFFLINE_WATCHDOG_ENABLE
            HAL_IWDG_Refresh(&hiwdg);
        #endif

        tx_thread_sleep(10);
    }
}


void offline_init(void)
{
    // 初始化管理器
    memset(&offline_manager, 0, sizeof(offline_manager)); 
    osal_status_t status = osal_thread_create(&offline_thread, "offlineTask", offline_task, 
    0,offline_thread_stack, OFFLINE_THREAD_STACK_SIZE,OFFLINE_THREAD_PRIORITY);

    if(status != TX_SUCCESS) {
        LOG_ERROR("Failed to create offline task!");
        return;
    }
    status = osal_thread_start(&offline_thread);

    beep_init(2000, 10, beep_ctrl_times);
    
    shell_register_function("offline", shell_offline_cmd, "Show offline device information");

    LOG_INFO("Offline task initialized successfully.");
}

uint8_t offline_device_register(const OfflineDeviceInit_t* init)
{
    if (init == NULL || offline_manager.device_count >= MAX_OFFLINE_DEVICES) {
        return OFFLINE_INVALID_INDEX;
    }
    
    uint8_t index = offline_manager.device_count;
    OfflineDevice_t* device = &offline_manager.devices[index];
    
    strncpy(device->name, init->name, sizeof(device->name) - 1);
    device->name[sizeof(device->name) - 1] = '\0'; // 确保字符串结束符
    device->timeout_ms = init->timeout_ms;
    device->level = init->level;
    device->beep_times = init->beep_times;
    device->is_offline = STATE_OFFLINE;
    device->last_time = tx_time_get();
    device->index = index;
    device->enable = init->enable;
    
    offline_manager.device_count++;

    LOG_INFO("offline device register: %s", device->name);

    return index;
}

void offline_device_update(uint8_t device_index)
{
        if (device_index < offline_manager.device_count) {
            offline_manager.devices[device_index].last_time = tx_time_get();
            offline_manager.devices[device_index].dt = DWT_GetDeltaT(&offline_manager.devices[device_index].dt_cnt);
        }
}

void offline_device_enable(uint8_t device_index)
{
        if (device_index < offline_manager.device_count) {
            offline_manager.devices[device_index].enable = OFFLINE_ENABLE;
        }
}

void offline_device_disable(uint8_t device_index)
{
        if (device_index < offline_manager.device_count) {
            offline_manager.devices[device_index].enable = OFFLINE_DISABLE;
        }
}
uint8_t get_device_status(uint8_t device_index){
        if(device_index < offline_manager.device_count){
            return offline_manager.devices[device_index].is_offline;
        }
        else {return STATE_ONLINE;}
}

uint8_t get_system_status(void){
    uint8_t status = 0;
    for (uint8_t i = 0; i < offline_manager.device_count; i++) {
        if (offline_manager.devices[i].is_offline) {
            status |= (1 << i);
        }
    }
    return status;
}

void beep_ctrl_times(ULONG timer)
{
    static uint32_t beep_period_start_time;
    static uint32_t beep_cycle_start_time;
    static uint8_t remaining_beep_cycles;

    if (DWT_GetTimeline_ms() - beep_period_start_time > OFFLINE_BEEP_PERIOD)
    {
        remaining_beep_cycles = current_beep_times;
        beep_period_start_time = DWT_GetTimeline_ms();
        beep_cycle_start_time = DWT_GetTimeline_ms();
    }
    else if (remaining_beep_cycles != 0)
    {
        if (DWT_GetTimeline_ms() - beep_cycle_start_time < OFFLINE_BEEP_ON_TIME)
        {
            #if OFFLINE_BEEP_ENABLE == 1
            beep_set_tune(OFFLINE_BEEP_TUNE_VALUE, OFFLINE_BEEP_CTRL_VALUE);
            #else
            beep_set_tune(0, 0);
            #endif
            RGB_show(LED_Red);
        }
        else if (DWT_GetTimeline_ms() - beep_cycle_start_time < OFFLINE_BEEP_ON_TIME + OFFLINE_BEEP_OFF_TIME)
        {
            beep_set_tune(0, 0);
            RGB_show(LED_Black);
        }
        else
        {
            remaining_beep_cycles--;
            beep_cycle_start_time = DWT_GetTimeline_ms();
        }
    }
}

// shell命令处理函数

// 添加获取设备信息的函数，供shell命令使用
const OfflineManager_t* offline_get_manager_info(void)
{
    return &offline_manager;
}
void shell_offline_cmd(int argc, char **argv)
{
    const OfflineManager_t* manager = offline_get_manager_info();
    
    if (argc < 2) {
        // 显示帮助信息
        shell_printf("Usage: offline <command>\r\n");
        shell_printf("Commands:\r\n");
        shell_printf("  list     - List all registered devices and their status\r\n");
        shell_printf("  status   - Show overall system offline status\r\n");
        shell_printf("\r\n");
        return;
    }
    
    if (strcmp(argv[1], "list") == 0) {
        // 显示所有注册设备及其状态
        shell_printf("Offline Device List:\r\n");
        shell_printf("Index %-20s %-10s %-10s %-8s %-10s\r\n", "Name", "Status", "Timeout", "Level", "BeepTimes");
        shell_printf("-------------------------------------------------------------------------\r\n");
        
        if (manager->device_count == 0) {
            shell_printf("No devices registered.\r\n");
            shell_printf("\r\n");
            return;
        }
        
        for (uint8_t i = 0; i < manager->device_count; i++) {
            const OfflineDevice_t* device = &manager->devices[i];
            const char* status_str = device->is_offline ? "OFFLINE" : "ONLINE";
            const char* level_str = "";
            
            switch (device->level) {
                case OFFLINE_LEVEL_LOW:
                    level_str = "LOW";
                    break;
                case OFFLINE_LEVEL_MEDIUM:
                    level_str = "MEDIUM";
                    break;
                case OFFLINE_LEVEL_HIGH:
                    level_str = "HIGH";
                    break;
                default:
                    level_str = "UNKNOWN";
                    break;
            }
            
            shell_printf("%-5d %-20s %-10s %-10lu %-8s %-10d\r\n", 
                        device->index,
                        device->name,
                        status_str,
                        device->timeout_ms,
                        level_str,
                        device->beep_times);
        }
        
        shell_printf("\r\nTotal devices: %d\r\n", manager->device_count);
        shell_printf("\r\n");
    } 
    else if (strcmp(argv[1], "status") == 0) {
        // 显示系统整体离线状态
        uint8_t system_status = get_system_status();
        bool any_offline = false;
        
        for (uint8_t i = 0; i < manager->device_count; i++) {
            if (manager->devices[i].is_offline) {
                any_offline = true;
                break;
            }
        }
        
        shell_printf("System Offline Status:\r\n");
        shell_printf("----------------------\r\n");
        if (any_offline) {
            shell_printf("Status: OFFLINE (0x%02X)\r\n", system_status);
            shell_printf("Some devices are offline!\r\n");
        } else {
            shell_printf("Status: ALL ONLINE\r\n");
            shell_printf("All devices are online.\r\n");
        }
        shell_printf("\r\n");
    }
    else {
        shell_printf("Unknown command: %s\r\n", argv[1]);
        shell_printf("Use 'offline' without arguments to see help.\r\n");
        shell_printf("\r\n");
    }
}

#else   
void offline_init(void){}
void offline_device_update(uint8_t device_index){}
void offline_device_enable(uint8_t device_index){}
void offline_device_disable(uint8_t device_index){}
uint8_t get_device_status(uint8_t device_index){return OFFLINE_INVALID_INDEX;}
uint8_t get_system_status(void) { return 0;}

#endif

