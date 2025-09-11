/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-11 13:43:09
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-09-11 14:48:42
 * @FilePath: /rm_base/modules/BMI088/bmi088.c
 * @Description: 
 */
#include "bmi088.h"
#include "BMI088_reg.h"
#include "bsp_dwt.h"
#include "osal_def.h"
#include <string.h>

#define log_tag "BMI088"
#include "log.h"

// 常量定义
static BMI088_Instance_t bmi088_instance={0};
#define BMI088REG 0
#define BMI088DATA 1
#define BMI088ERROR 2
// BMI088初始化配置数组for accel,第一列为reg地址,第二列为写入的配置值,第三列为错误码(如果出错)
static uint8_t BMI088_Accel_Init_Table[BMI088_WRITE_ACCEL_REG_NUM][3] =
    {
        {BMI088_ACC_PWR_CTRL, BMI088_ACC_ENABLE_ACC_ON, BMI088_ACC_PWR_CTRL_ERROR},
        {BMI088_ACC_PWR_CONF, BMI088_ACC_PWR_ACTIVE_MODE, BMI088_ACC_PWR_CONF_ERROR},
        {BMI088_ACC_CONF, BMI088_ACC_NORMAL | BMI088_ACC_800_HZ | BMI088_ACC_CONF_MUST_Set, BMI088_ACC_CONF_ERROR},
        {BMI088_ACC_RANGE, BMI088_ACC_RANGE_6G, BMI088_ACC_RANGE_ERROR},
        {BMI088_INT1_IO_CTRL, BMI088_ACC_INT1_IO_ENABLE | BMI088_ACC_INT1_GPIO_PP | BMI088_ACC_INT1_GPIO_LOW, BMI088_INT1_IO_CTRL_ERROR},
        {BMI088_INT_MAP_DATA, BMI088_ACC_INT1_DRDY_INTERRUPT, BMI088_INT_MAP_DATA_ERROR}};
// BMI088初始化配置数组for gyro,第一列为reg地址,第二列为写入的配置值,第三列为错误码(如果出错)
static uint8_t BMI088_Gyro_Init_Table[BMI088_WRITE_GYRO_REG_NUM][3] =
    {
        {BMI088_GYRO_RANGE, BMI088_GYRO_2000, BMI088_GYRO_RANGE_ERROR},
        {BMI088_GYRO_BANDWIDTH, BMI088_GYRO_1000_116_HZ | BMI088_GYRO_BANDWIDTH_MUST_Set, BMI088_GYRO_BANDWIDTH_ERROR},
        {BMI088_GYRO_LPM1, BMI088_GYRO_NORMAL_MODE, BMI088_GYRO_LPM1_ERROR},
        {BMI088_GYRO_CTRL, BMI088_DRDY_ON, BMI088_GYRO_CTRL_ERROR},
        {BMI088_GYRO_INT3_INT4_IO_CONF, BMI088_GYRO_INT3_GPIO_PP | BMI088_GYRO_INT3_GPIO_LOW, BMI088_GYRO_INT3_INT4_IO_CONF_ERROR},
        {BMI088_GYRO_INT3_INT4_IO_MAP, BMI088_GYRO_DRDY_IO_INT3, BMI088_GYRO_INT3_INT4_IO_MAP_ERROR}};

/* 内部使用的bmi088读写函数 */
static inline osal_status_t _bmi088_writedata(SPI_Device *device  , const uint8_t addr, const uint8_t *data, const uint8_t data_len){
    osal_status_t status = OSAL_SUCCESS;
    uint8_t ptdata = addr & BMI088_SPI_WRITE_CODE;
    status = BSP_SPI_TransAndTrans(device,&ptdata,1,data,data_len);
    return status;
}

static inline osal_status_t _bmi088_readdata(SPI_Device *device  , const uint8_t addr, uint8_t *data, const uint8_t data_len){
    osal_status_t status = OSAL_SUCCESS;
    
    static uint8_t tmp[8];
    static uint8_t tx[8]={0};

    // data_len 最大为6，保证 tmp 不越界
    if(data_len > 6){ return OSAL_ERROR; }

    if (device==bmi088_instance.acc_device)
    {
        tx[0] = BMI088_SPI_READ_CODE | addr;
        status = BSP_SPI_TransReceive(device, tx,tmp, data_len+2);
        memcpy(data, tmp+2, data_len);//前2字节dummy byte
        return status;
    }
    else if(device==bmi088_instance.gyro_device){
        tx[0] = BMI088_SPI_READ_CODE | addr;
        status = BSP_SPI_TransReceive(device, tx,tmp, data_len+1);
        memcpy(data, tmp+1, data_len);//前1字节dummy byte
        return status;
    }
    return OSAL_ERROR; 
}


/* acc gyro 初始化 */
/*加速度计部分*/
static inline void bmi088_acc_init(void){
    uint8_t whoami_check = 0;
    uint8_t tmp;
    // 加速度计以I2C模式启动,需要一次上升沿来切换到SPI模式,因此进行一次fake write
    _bmi088_readdata(bmi088_instance.acc_device, BMI088_ACC_CHIP_ID, &whoami_check, 1);
    DWT_Delay(0.001);

    tmp=BMI088_ACC_SOFTRESET_VALUE;
    _bmi088_writedata(bmi088_instance.acc_device, BMI088_ACC_SOFTRESET, &tmp, 1); // 软复位
    DWT_Delay(BMI088_COM_WAIT_SENSOR_TIME);

    _bmi088_readdata(bmi088_instance.acc_device, BMI088_ACC_CHIP_ID, &whoami_check, 1);
    DWT_Delay(0.001);
    
    uint8_t reg = 0, data = 0;
    BMI088_ERORR_CODE_e error = BMI088_NO_ERROR;
    for (uint8_t i = 0; i < sizeof(BMI088_Accel_Init_Table) / sizeof(BMI088_Accel_Init_Table[0]); i++)
    {
        reg = BMI088_Accel_Init_Table[i][BMI088REG];
        data = BMI088_Accel_Init_Table[i][BMI088DATA];
        _bmi088_writedata(bmi088_instance.acc_device, reg, &data, 1);// 写入寄存器
        DWT_Delay(0.001);
        _bmi088_readdata(bmi088_instance.acc_device, reg, &tmp, 1);// 写完之后立刻读回检查
        DWT_Delay(0.001);
        if (tmp != BMI088_Accel_Init_Table[i][BMI088DATA])
        {    
            error |= BMI088_Accel_Init_Table[i][BMI088ERROR];
            bmi088_instance.BMI088_ERORR_CODE=error;
            LOG_ERROR("ACCEL Init failed reg:%d",BMI088_Accel_Init_Table[i][BMI088REG]);
        }
    }
}

static inline void bmi088_gyro_init(void){
    uint8_t tmp;
    
    tmp=BMI088_GYRO_SOFTRESET_VALUE;
    _bmi088_writedata(bmi088_instance.gyro_device, BMI088_GYRO_SOFTRESET, &tmp, 1);// 软复位
    DWT_Delay(BMI088_COM_WAIT_SENSOR_TIME);

    // 检查ID,如果不是0x0F(bmi088 whoami寄存器值),则返回错误
    uint8_t whoami_check = 0;
    _bmi088_readdata(bmi088_instance.gyro_device, BMI088_GYRO_CHIP_ID, &whoami_check, 1);
    if (whoami_check != BMI088_GYRO_CHIP_ID_VALUE)
    {
        LOG_ERROR("No gyro Sensor!");
    }
    DWT_Delay(0.001);
    uint8_t reg = 0, data = 0;
    BMI088_ERORR_CODE_e error = BMI088_NO_ERROR;
    for (uint8_t i = 0; i < sizeof(BMI088_Gyro_Init_Table) / sizeof(BMI088_Gyro_Init_Table[0]); i++)
    {
        reg = BMI088_Gyro_Init_Table[i][BMI088REG];
        data = BMI088_Gyro_Init_Table[i][BMI088DATA];
        _bmi088_writedata(bmi088_instance.gyro_device, reg, &data, 1);
        DWT_Delay(0.001);
        _bmi088_readdata(bmi088_instance.gyro_device, reg, &tmp, 1);// 写完之后立刻读回对应寄存器检查是否写入成功
        DWT_Delay(0.001);
        if (tmp != BMI088_Gyro_Init_Table[i][BMI088DATA])
        {
            error |= BMI088_Gyro_Init_Table[i][BMI088ERROR];
            bmi088_instance.BMI088_ERORR_CODE=error;
            LOG_ERROR("GYRO Init failed reg:%d",BMI088_Gyro_Init_Table[i][BMI088REG]);
        }
    }
}

osal_status_t bmi088_get_accel(BMI088_Instance_t *ist)
{
    if (ist == NULL)
    {
        return OSAL_ERROR;
    }
    osal_status_t status = OSAL_SUCCESS;
    // 读取accel的x轴数据首地址,bmi088内部自增读取地址 // 3* sizeof(int16_t)
    status = _bmi088_readdata(ist->acc_device, BMI088_ACCEL_XOUT_L, ist->buf, 6);
    for (uint8_t i = 0; i < 3; i++){
        bmi088_instance.BMI088_Raw_Data.acc[i] = (BMI088_ACCEL_6G_SEN) * (float)((int16_t)((ist->buf[2 * i + 1]) << 8) | ist->buf[2 * i]);
    }
    return status;
}

osal_status_t bmi088_get_temp(BMI088_Instance_t *ist){
    if (ist == NULL)
    {
        return OSAL_ERROR;
    }
    osal_status_t status = OSAL_SUCCESS;
    status = _bmi088_readdata(bmi088_instance.acc_device, BMI088_TEMP_M, ist->buf, 2);// 读温度,温度传感器在accel上
    int16_t tmp = (((ist->buf[0] << 3) | (ist->buf[1] >> 5)));
    if (tmp > 1023){tmp-=2048;}
    bmi088_instance.BMI088_Raw_Data.temperature = (float)(int16_t)tmp*BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET;
    return status;
}

osal_status_t bmi088_get_gyro(BMI088_Instance_t *ist)
{
    if (ist == NULL)
    {
        return OSAL_ERROR;
    }
    osal_status_t status = OSAL_SUCCESS;
    status = _bmi088_readdata(ist->gyro_device, BMI088_GYRO_X_L, ist->buf, 6); // 连续读取3个(3*2=6)轴的角速度
    for (uint8_t i = 0; i < 3; i++){
        ist->BMI088_Raw_Data.gyro[i] = BMI088_GYRO_2000_SEN * (float)((int16_t)((ist->buf[2 * i + 1]) << 8) | ist->buf[2 * i]);
    }
    return status;
}

BMI088_Instance_t* BMI088_init(void){
    static SPI_Device_Init_Config gyro_cfg = {
        .hspi       = &hspi1,
        .cs_port    = GPIOB,
        .cs_pin     = GPIO_PIN_0,
        .tx_mode    = SPI_MODE_BLOCKING,
        .rx_mode    = SPI_MODE_BLOCKING,
    };
    bmi088_instance.gyro_device = BSP_SPI_Device_Init(&gyro_cfg);
    static SPI_Device_Init_Config acc_cfg = {
        .hspi       = &hspi1,
        .cs_port    = GPIOA,
        .cs_pin     = GPIO_PIN_4,
        .tx_mode    = SPI_MODE_BLOCKING,
        .rx_mode    = SPI_MODE_BLOCKING,
      };
    bmi088_instance.acc_device = BSP_SPI_Device_Init(&acc_cfg);
    if (!bmi088_instance.acc_device || !bmi088_instance.gyro_device){
        LOG_ERROR("bmi088 init failed");
        return NULL;
    }else{
        bmi088_acc_init();
        bmi088_gyro_init();
        if (bmi088_instance.BMI088_ERORR_CODE == BMI088_NO_ERROR)
        {
            LOG_INFO("bmi088 init success");
            return &bmi088_instance;
        }
        return NULL;
    }
}

