#ifndef _COMPENSATION_H_
#define _COMPENSATION_H_

// 前馈补偿函数指针类型
typedef float (*FeedforwardFunc)(float ref, float degree , float angular_velocity);

// 用于存储补偿参数的结构体
typedef struct {
    float gravity_force;  // 重力补偿力(N)
    float arm_length;     // 杆长(m)
    float friction;       // 摩擦力(N)
} CompensationParams;

/**
 * @brief 重力补偿函数
 * @param ref       参考值（角度）
 * @param velocity  速度
 * @param gravity_force 重力补偿力(N)
 * @param arm_length 杆长(m)
 * @return 补偿值
 */
float gravity_compensation(float ref, float velocity, float gravity_force, float arm_length);

/**
 * @brief 摩擦补偿函数
 * @param ref       参考值（角度）
 * @param velocity  速度
 * @param friction  摩擦力(N)
 * @return 补偿值
 */
float friction_compensation(float ref, float velocity, float friction);

/**
 * @brief 无补偿函数
 * @param ref       参考值
 * @param velocity  速度
 * @return 补偿值（始终为0）
 */
float none_compensation(float ref, float velocity);

/**
 * @brief 创建带参数的重力补偿函数包装器
 * @param gravity_force 重力补偿力(N)
 * @param arm_length 杆长(m)
 * @return 指向包装函数的函数指针
 */
FeedforwardFunc create_gravity_compensation_wrapper(float gravity_force, float arm_length);

/**
 * @brief 创建带参数的摩擦补偿函数包装器
 * @param friction 摩擦力
 * @return 指向包装函数的函数指针
 */
FeedforwardFunc create_friction_compensation_wrapper(float friction);

#endif // _COMPENSATION_H_