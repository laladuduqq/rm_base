#ifndef __LQR_H
#define __LQR_H
#include "stdint.h"

// 前馈补偿函数指针类型
typedef float (*FeedforwardFunc)(float ref, float degree , float angular_velocity);

// LQR错误类型枚举
typedef enum {
    LQR_ERROR_NONE = 0,
    LQR_MOTOR_BLOCKED_ERROR
} LQRErrorType;

typedef struct {
    float K[2]; // LQR增益矩阵 K
    uint8_t state_dim; // 状态维度
    float output_max;           // 输出最大值
    float output_min;           // 输出最小值
    FeedforwardFunc feedforward_func; // 前馈函数指针
} LQR_Init_Config_s;

typedef struct {
    float K[2]; // LQR增益矩阵 K
    uint8_t state_dim; // 状态维度
    float err;
    float output;
    float state[2];
    float output_max;           // 输出最大值
    float output_min;           // 输出最小值
    FeedforwardFunc feedforward_func; // 前馈函数指针
    
    // 错误处理相关
    struct {
        uint32_t ERRORCount;     // 错误计数
        LQRErrorType ERRORType;  // 错误类型
    } ErrorHandler;
    
    // 输入输出值保存
    float Ref;      // 参考值
    float Measure;  // 测量值
} LQRInstance;

/**
 * @brief 初始化LQR实例
 * @param lqr    LQR实例指针
 * @param config LQR初始化配置
 */
void LQRInit(LQRInstance *lqr, LQR_Init_Config_s *config);

/**
 * @brief 计算LQR输出
 * @param lqr     LQR实例指针
 * @param degree  （角度）
 * @param angular_velocity  （角速度）
 * @param ref     参考值
 * @return float  LQR计算输出
 */
float LQRCalculate(LQRInstance *lqr, float degree, float angular_velocity, float ref);

#endif