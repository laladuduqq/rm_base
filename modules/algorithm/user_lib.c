#include "user_lib.h"
#include "kalman_filter.h"
#include "stdlib.h"

////快速开方
//float invSqrt(float num)
//{
//    float halfnum = 0.5f * num;
//    float y = num;
//    long i = *(long *)&y;
//    i = 0x5f3759df - (i >> 1);
//    y = *(float *)&i;
//    y = y * (1.5f - (halfnum * y * y));
//    return y;
//}

/**
  * @brief          斜波函数初始化
  * @author         RM
  * @param[in]      斜波函数结构体
  * @param[in]      间隔的时间，单位 s
  * @param[in]      最大值
  * @param[in]      最小值
  * @retval         返回空
  */
void ramp_init(ramp_function_source_t *ramp_source_type, float frame_period, float max, float min)
{
    ramp_source_type->frame_period = frame_period;
    ramp_source_type->max_value = max;
    ramp_source_type->min_value = min;
    ramp_source_type->input = 0.0f;
    ramp_source_type->out = 0.0f;
}

/**
  * @brief          斜波函数计算，根据输入的值进行叠加， 输入单位为 /s 即一秒后增加输入的值
  * @author         RM
  * @param[in]      斜波函数结构体
  * @param[in]      输入值
  * @param[in]      滤波参数
  * @retval         返回空
  */
void ramp_calc(ramp_function_source_t *ramp_source_type, float input)
{
    ramp_source_type->input = input;
    ramp_source_type->out += ramp_source_type->input * ramp_source_type->frame_period;
    if (ramp_source_type->out > ramp_source_type->max_value)
    {
        ramp_source_type->out = ramp_source_type->max_value;
    }
    else if (ramp_source_type->out < ramp_source_type->min_value)
    {
        ramp_source_type->out = ramp_source_type->min_value;
    }
}
/**
  * @brief          一阶低通滤波初始化
  * @author         RM
  * @param[in]      一阶低通滤波结构体
  * @param[in]      间隔的时间，单位 s
  * @param[in]      滤波参数
  * @retval         返回空
  */
void first_order_filter_init(first_order_filter_type_t *first_order_filter_type, float frame_period, const float num[1])
{
    first_order_filter_type->frame_period = frame_period;
    first_order_filter_type->num[0] = num[0];
    first_order_filter_type->input = 0.0f;
    first_order_filter_type->out = 0.0f;
}

/**
  * @brief          一阶低通滤波计算
  * @author         RM
  * @param[in]      一阶低通滤波结构体
  * @param[in]      间隔的时间，单位 s
  * @retval         返回空
  */
void first_order_filter_cali(first_order_filter_type_t *first_order_filter_type, float input)
{
    first_order_filter_type->input = input;
    first_order_filter_type->out =
        first_order_filter_type->num[0] / (first_order_filter_type->num[0] + first_order_filter_type->frame_period) * first_order_filter_type->out + first_order_filter_type->frame_period / (first_order_filter_type->num[0] + first_order_filter_type->frame_period) * first_order_filter_type->input;
}

//绝对限制
void abs_limit(float *num, float Limit)
{
    if (*num > Limit)
    {
        *num = Limit;
    }
    else if (*num < -Limit)
    {
        *num = -Limit;
    }
}

//判断符号位
float sign(float value)
{
    if (value >= 0.0f)
    {
        return 1.0f;
    }
    else
    {
        return -1.0f;
    }
}

//浮点死区
float float_deadline(float Value, float minValue, float maxValue)
{
    if (Value < maxValue && Value > minValue)
    {
        Value = 0.0f;
    }
    return Value;
}

//int26死区
int16_t int16_deadline(int16_t Value, int16_t minValue, int16_t maxValue)
{
    if (Value < maxValue && Value > minValue)
    {
        Value = 0;
    }
    return Value;
}

//限幅函数
float float_constrain(float Value, float minValue, float maxValue)
{
    if (Value < minValue)
    {
        return minValue;
    }
    else if (Value > maxValue)
    {
        return maxValue;
    }
    else
    {
        return Value;
    }
}

//限幅函数
int16_t int16_constrain(int16_t Value, int16_t minValue, int16_t maxValue)
{
    if (Value < minValue)
    {
        return minValue;
    }
    else if (Value > maxValue)
    {
        return maxValue;
    }
    else
    {
        return Value;
    }
}

//循环限幅函数
float loop_float_constrain(float Input, float minValue, float maxValue)
{
    if (maxValue < minValue)
    {
        return Input;
    }

    if (Input > maxValue)
    {
        float len = maxValue - minValue;
        while (Input > maxValue)
        {
            Input -= len;
        }
    }
    else if (Input < minValue)
    {
        float len = maxValue - minValue;
        while (Input < minValue)
        {
            Input += len;
        }
    }
    return Input;
}

//弧度格式化为-PI~PI

//角度格式化为-180~180
float theta_format(float Ang)
{
    return loop_float_constrain(Ang, -180.0f, 180.0f);
}





/**
  * @brief     ramp filter initialize
  * @param[in]
  * @retval    void
  */
void ramp_v0_init(ramp_v0_t *ramp, int32_t scale)
{
    ramp->count = 0;
    ramp->scale = scale;
}

/**
  * @brief     caculate output of ramp filter
  * @param[in] ramp: a ramp filter pointer
  * @retval    output
  */
float ramp_v0_calculate(ramp_v0_t *ramp)
{
    if (ramp->scale <= 0)
    {
        return 0;
    }

    if (ramp->count++ >= ramp->scale)
    {
        ramp->count = ramp->scale;
    }

    ramp->out = ramp->count / ((float)ramp->scale);
    return ramp->out;
}

void *zmalloc(size_t size)
{
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

// 快速开方
float Sqrt(float x)
{
    float y;
    float delta;
    float maxError;

    if (x <= 0)
    {
        return 0;
    }

    // initial guess
    y = x / 2;

    // refine
    maxError = x * 0.001f;

    do
    {
        delta = (y * y) - x;
        y -= delta / (2 * y);
    } while (delta > maxError || delta < -maxError);

    return y;
}




int float_rounding(float raw)
{
    static int integer;
    static float decimal;
    integer = (int)raw;
    decimal = raw - integer;
    if (decimal > 0.5f)
        integer++;
    return integer;
}

// 三维向量归一化
float *Norm3d(float *v)
{
    float len = Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
    return v;
}

// 计算模长
float NormOf3d(float *v)
{
    return Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

// 三维向量叉乘v1 x v2
void Cross3d(float *v1, float *v2, float *res)
{
    res[0] = v1[1] * v2[2] - v1[2] * v2[1];
    res[1] = v1[2] * v2[0] - v1[0] * v2[2];
    res[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

// 三维向量点乘
float Dot3d(float *v1, float *v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

// 均值滤波,删除buffer中的最后一个元素,填入新的元素并求平均值
float AverageFilter(float new_data, float *buf, uint8_t len)
{
    float sum = 0;
    for (uint8_t i = 0; i < len - 1; i++)
    {
        buf[i] = buf[i + 1];
        sum += buf[i];
    }
    buf[len - 1] = new_data;
    sum += new_data;
    return sum / len;
}

void MatInit(mat *m, uint8_t row, uint8_t col)
{
    m->numCols = col;
    m->numRows = row;
    m->pData = (float *)zmalloc(row * col * sizeof(float));
}
