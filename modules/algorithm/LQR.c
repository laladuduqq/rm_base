#include "LQR.h"
#include "user_lib.h"

#ifndef abs
#define abs(x) ((x) > 0 ? (x) : -(x))
#endif

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#include <arm_math.h>

// 角度转弧度
static float degreesToRadians(float degrees) {
    return degrees * (PI / 180.0f);
}

/**
 * @brief LQR堵转检测处理
 * @param lqr LQR实例指针
 */
static void LQR_ErrorHandle(LQRInstance *lqr)
{
    // 如果输出很小或参考值接近0，则不进行堵转检测
    if (abs(lqr->output) < lqr->output_max * 0.001f || abs(lqr->Ref) < 0.0001f)
        return;

    // 如果误差与参考值的比值大于阈值（95%），则认为可能发生了堵转
    if ((abs(lqr->Ref - lqr->Measure) / abs(lqr->Ref)) > 0.95f)
    {
        // 增加错误计数
        lqr->ErrorHandler.ERRORCount++;
    }
    else
    {
        // 误差恢复正常，重置错误计数和错误类型
        lqr->ErrorHandler.ERRORCount = 0;
        lqr->ErrorHandler.ERRORType = LQR_ERROR_NONE;
    }

    // 如果错误计数超过阈值（500次），则标记为电机堵转错误
    if (lqr->ErrorHandler.ERRORCount > 500)
    {
        lqr->ErrorHandler.ERRORType = LQR_MOTOR_BLOCKED_ERROR;
    }
}

/**
 * @brief 初始化LQR实例
 * @param lqr    LQR实例指针
 * @param config LQR初始化配置
 */
void LQRInit(LQRInstance *lqr, LQR_Init_Config_s *config) {
    // 参数检查
    if (lqr == NULL || config == NULL) {
        return;
    }
    // 清零整个结构体
    memset(lqr, 0, sizeof(LQRInstance));
    // 初始化基本参数
    lqr->state_dim = config->state_dim;
    // 检查状态维度有效性
    if (lqr->state_dim < 1 || lqr->state_dim > 2) {
        return;
    }
    // 复制增益矩阵
    for (uint8_t i = 0; i < lqr->state_dim; i++) {
        lqr->K[i] = config->K[i];
    }
    // 初始化输出相关参数
    lqr->output_max = config->output_max;
    lqr->output_min = config->output_min;
    // 初始化错误处理参数
    lqr->ErrorHandler.ERRORCount = 0;
    lqr->ErrorHandler.ERRORType = LQR_ERROR_NONE;
    lqr->feedforward_func = config->feedforward_func;
}

static float calculateOutput(LQRInstance *lqr, float degree, float angular_velocity, float ref) {
    if (lqr == NULL) {
        return 0.0f;
    }

    // 保存输入值
    lqr->Ref = ref;
    // 保存状态值
    lqr->state[0] = degree;
    lqr->state[1] = angular_velocity;
    
    // 将参考值转换为弧度
    float rad_ref = degreesToRadians(ref);
    
    if (lqr->state_dim == 1) {
        // 单状态情况：角速度控制
        float rad_angular_velocity = degreesToRadians(angular_velocity);
        lqr->err = rad_angular_velocity - rad_ref;
        lqr->output = -(lqr->K[0] * lqr->err);
    }
    else if (lqr->state_dim == 2) {
        // 双状态情况：角度和角速度控制
        float rad_degree= degreesToRadians(degree);
        //float rad_angular_velocity = degreesToRadians(angular_velocity);  
        lqr->err = rad_degree - rad_ref;
        lqr->output = -(lqr->K[0] * lqr->err) - lqr->K[1] * angular_velocity;
    }

    // 调用前馈函数（如果不为NULL）
    if (lqr->feedforward_func != NULL) {
        float feedforward = lqr->feedforward_func(ref,degree,angular_velocity);
        lqr->output += feedforward;
    }

    // 输出限幅
    VAL_LIMIT(lqr->output, lqr->output_min, lqr->output_max);
    
    return lqr->output;
}

float LQRCalculate(LQRInstance *lqr, float degree, float angular_velocity, float ref) {
    // 参数检查
    if (lqr == NULL || lqr->state_dim < 1 || lqr->state_dim > 2) {
        return 0.0f;
    }

    if (lqr->state_dim == 1)
    {
        lqr->Measure = angular_velocity;
    }
    else if (lqr->state_dim == 2) {
        lqr->Measure = degree;
    }
    
    // 执行堵转检测
    LQR_ErrorHandle(lqr);

    float output = calculateOutput(lqr, degree, angular_velocity, ref);
    
    // 如果检测到堵转错误，则输出为0
    if (lqr->ErrorHandler.ERRORType == LQR_MOTOR_BLOCKED_ERROR)
    {
        output = 0.0f;
    }
    
    return output;
}