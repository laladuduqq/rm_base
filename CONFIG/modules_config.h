/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 10:28:00
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 18:36:56
 * @FilePath: /rm_base/CONFIG/modules_config.h
 * @Description: 
 */
#ifndef _MODULES_CONFIG_H_
#define _MODULES_CONFIG_H_

/* 模块配置文件 */

/* IST8310 磁力计模块 */
#define IST8310_ENABLE                 0                 // 启用IST8310模块        
/* BMI088 模块 */
#define BMI088_ENABLE                  1                 // 启用BMI088模块
#if BMI088_ENABLE
   #define BMI088_TEMP_ENABLE          1                 // 启用BMI088模块的温度控制
   #define BMI088_TEMP_SET             35.0f             // BMI088的设定温度
#endif

#endif // _MODULES_CONFIG_H_