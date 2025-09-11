# algorithms

## 总览和使用

module层的algorithm提供了一些供其他模块以及app的应用层使用的算法，包括：

1. PID控制器[controller.h](file:///home/pan/code/rm_base/modules/algorithm/controller.h)
2. crc8 crc16循环冗余校验
3. 卡尔曼滤波器[kalman_filter.h](file:///home/pan/code/rm_base/modules/algorithm/kalman_filter.h)，可以通过用户自定义函数配置为扩展卡尔曼滤波
4. [LQR.h](file:///home/pan/code/rm_base/modules/algorithm/LQR.h)，线性二次型调节器
5. [QuaternionEKF.h](file:///home/pan/code/rm_base/modules/algorithm/QuaternionEKF.h)，用于`ins_task`的四元数姿态解算和扩展卡尔曼滤波融合
6. [user_lib.h](file:///home/pan/code/rm_base/modules/algorithm/user_lib.h)，一些通用的函数，包括限幅、数据类型转换、角度弧度转换、快速符号判断以及优化开方等功能。多个模块都会使用的、不好区分的函数可以放置于此
7. [compensation.h](file:///home/pan/code/rm_base/modules/algorithm/compensation.h)，提供重力补偿和摩擦补偿等前馈补偿函数

### PID控制器 (controller.h)

PID控制器实现了标准的位置式PID控制算法，支持多种优化功能：

- 积分限幅 ([PID_Integral_Limit](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L27-L27))
- 微分先行 ([PID_Derivative_On_Measurement](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L28-L28))
- 梯形积分 ([PID_Trapezoid_Intergral](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L29-L29))
- 变速积分 ([PID_ChangingIntegrationRate](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L32-L32))
- 输出滤波 ([PID_OutputFilter](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L31-L31))
- 微分滤波 ([PID_DerivativeFilter](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L33-L33))
- 死区控制
- 电机堵转检测 ([PID_ErrorHandle](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L34-L34))

初始化参数：

- [Kp](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L98-L98), [Ki](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L99-L99), [Kd](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L100-L100)：比例、积分、微分系数
- [MaxOut](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L101-L101)：输出限幅值
- [DeadBand](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L102-L102)：死区范围
- [Improve](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L105-L105)：优化功能标志位组合
- [IntegralLimit](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L106-L106)：积分限幅值
- [CoefA](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L107-L107), [CoefB](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L108-L108)：变速积分参数
- [Output_LPF_RC](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L109-L109)：输出滤波器系数
- [Derivative_LPF_RC](file:///home/pan/code/rm_base/modules/algorithm/controller.h#L110-L110)：微分滤波器系数

使用示例：

```c
PID_Init_Config_s config = {
    .Kp = 10.0f,
    .Ki = 0.1f,
    .Kd = 0.01f,
    .MaxOut = 1000.0f,
    .DeadBand = 0.0f,
    .Improve = PID_Integral_Limit | PID_DerivativeFilter,
    .IntegralLimit = 100.0f,
    .Derivative_LPF_RC = 0.01f
};
PIDInstance pid;
PIDInit(&pid, &config);
float output = PIDCalculate(&pid, measure, ref);
```

### 卡尔曼滤波器 (kalman_filter.h)

卡尔曼滤波器实现了标准的卡尔曼滤波算法，具有以下特点：

- 支持动态调整观测矩阵H、R和增益K的维度
- 可通过用户自定义函数扩展为EKF/UKF等
- 支持多传感器融合
- 包含防止滤波器过度收敛的机制

初始化参数：

- xhatSize：状态变量维度
- uSize：控制变量维度（0表示无控制输入）
- zSize：观测量维度

使用示例：

```c
KalmanFilter_t kf;
Kalman_Filter_Init(&kf, 3, 0, 3); // 3个状态，无控制输入，3个观测量
// 设置状态转移矩阵F、过程噪声协方差Q等参数
float* result = Kalman_Filter_Update(&kf);
```

### 四元数扩展卡尔曼滤波 (QuaternionEKF.h)

专门用于姿态解算的扩展卡尔曼滤波器：

- 使用四元数表示姿态，避免万向节死锁
- 估计陀螺仪零偏
- 集成卡方检验提高鲁棒性
- 结合加速度计进行姿态修正

初始化参数：

- `init_quaternion`：初始四元数
- `process_noise1`：四元数更新过程噪声
- `process_noise2`：陀螺仪零偏过程噪声
- `measure_noise`：加速度计量测噪声
- lambda：渐消因子
- `lpf`：加速度计低通滤波系数

使用示例：

```c
float init_q[4] = {1.0f, 0.0f, 0.0f, 0.0f};
IMU_QuaternionEKF_Init(init_q, 10.0f, 0.001f, 1000000.0f, 0.9996f, 0.0f);
IMU_QuaternionEKF_Update(gx, gy, gz, ax, ay, az, dt);
```

### 线性二次型调节器 (LQR.h)

实现LQR最优控制算法：

- 支持单状态和双状态控制
- 可配置前馈补偿函数
- 包含电机堵转检测

初始化参数：

- `K[2]`：LQR增益矩阵
- state_dim：状态维度（1或2）
- output_max：输出最大值
- output_min：输出最小值
- feedforward_func：前馈函数指针

使用示例：

```c
LQR_Init_Config_s config = {
    .K = {10.0f, 5.0f},
    .state_dim = 2,
    .output_max = 1000.0f,
    .output_min = -1000.0f,
    .feedforward_func = NULL
};
LQRInstance lqr;
LQRInit(&lqr, &config);
float output = LQRCalculate(&lqr, degree, angular_velocity, ref);
```

### 补偿函数 (compensation.h)

提供常用的前馈补偿函数：

- 重力补偿
- 摩擦补偿
- 无补偿函数（空函数）

使用示例：

```c
// 重力补偿
FeedforwardFunc gravity_func = create_gravity_compensation_wrapper(9.8f, 0.2f);
float gravity_comp = gravity_func(ref, degree, angular_velocity);

// 摩擦补偿
FeedforwardFunc friction_func = create_friction_compensation_wrapper(0.5f);
float friction_comp = friction_func(ref, degree, angular_velocity);
```

### 通用函数库 (user_lib.h)

包含各种常用的数学和工具函数：

- 限幅函数：abs_limit, float_constrain, int16_constrain
- 死区处理：float_deadline, int16_deadline
- 斜波函数：ramp_init, ramp_calc
- 一阶低通滤波器：first_order_filter_init, first_order_filter_cali
- 快速开方：`Sqrt`
- 向量运算：Dot3d, Cross3d, Norm3d, NormOf3d
- 角度处理函数：theta_format, rad_format
- 平均滤波：AverageFilter
- 矩阵初始化：MatInit
