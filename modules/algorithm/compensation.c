#include "compensation.h"
#include "arm_math.h"
#include "user_lib.h"
#include <stdlib.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// 存储参数的静态变量
static float gravity_force_param = 0.0f;
static float arm_length_param = 0.0f;
static float friction_param = 0.0f;

static float degreesToRadians(float degrees) {
    return degrees * (PI / 180.0f);
}

/**
 * @brief 重力补偿函数
 * @param ref       参考值（角度）
 * @param degree    角度  
 * @param gravity_force 重力补偿力(N)
 * @param arm_length 杆长(m)
 * @return 补偿值
 */
float gravity_compensation(float ref, float degree, float gravity_force, float arm_length) {
    float rad_ref = degreesToRadians(ref);
    float compensation = 0.0f;
    
    if (rad_ref >= 0) {
        compensation = gravity_force * arm_length * cosf(rad_ref);
    } else {
        compensation = gravity_force * (arm_length + arm_length * sinf(-rad_ref)) * cosf(-rad_ref);
    }
    
    return compensation;
}

/**
 * @brief 摩擦补偿函数
 * @param ref       参考值（角度）
 * @param degree     角度  
 * @param friction  摩擦力(N)
 * @return 补偿值
 */
float friction_compensation(float ref, float degree, float friction) {
    // 根据速度方向确定摩擦力方向
    if (degree > 0) {
        return friction;
    } else if (degree < 0) {
        return -friction;
    } else {
        // 速度为0时，根据参考值判断
        if (ref > 0) {
            return friction;
        } else if (ref < 0) {
            return -friction;
        } else {
            return 0.0f;
        }
    }
}

/**
 * @brief 无补偿函数
 * @param ref       参考值
 * @param degree    角度  
 * @return 补偿值（始终为0）
 */
float none_compensation(float ref, float degree) {
    return 0.0f;
}

// 重力补偿包装函数
static float gravity_compensation_wrapper(float ref, float degree,float angular_velocity) {
    return gravity_compensation(ref, degree, gravity_force_param, arm_length_param);
}

// 摩擦补偿包装函数
static float friction_compensation_wrapper(float ref, float degree,float angular_velocity) {
    return friction_compensation(ref, degree, friction_param);
}

/**
 * @brief 创建带参数的重力补偿函数包装器
 * @param gravity_force 重力补偿力(N)
 * @param arm_length 杆长(m)
 * @return 指向包装函数的函数指针
 */
FeedforwardFunc create_gravity_compensation_wrapper(float gravity_force, float arm_length) {
    gravity_force_param = gravity_force;
    arm_length_param = arm_length;
    return gravity_compensation_wrapper;
}

/**
 * @brief 创建带参数的摩擦补偿函数包装器
 * @param friction 摩擦力
 * @return 指向包装函数的函数指针
 */
FeedforwardFunc create_friction_compensation_wrapper(float friction) {
    friction_param = friction;
    return friction_compensation_wrapper;
}