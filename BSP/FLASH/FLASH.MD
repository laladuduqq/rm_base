# BSP FLASH 驱动文档

## 概述

BSP FLASH 是一个基于 STM32 HAL 库的 FLASH 存储驱动程序，提供了统一的接口来操作 STM32F4 系列微控制器内部的 FLASH 存储器。该驱动支持扇区擦除、数据读写等操作，并提供了完整的错误检查和地址验证机制。

## 特性

- 支持 STM32F4 系列微控制器 FLASH 操作
- 完整的扇区擦除功能
- 支持字(32位)、半字(16位)、字节(8位)数据写入
- 支持批量数据读写操作
- 自动处理 FLASH 解锁/锁定
- 完善的参数检查和错误处理机制
- 地址有效性验证
- 扇区信息查询功能
  
  ## 数据结构
  
  ### BSP_FLASH_Status
  
  ```c
  typedef enum {
      BSP_FLASH_OK       = 0x00U,           // 操作成功
      BSP_FLASH_ERROR    = 0x01U,           // 操作失败
      BSP_FLASH_BUSY     = 0x02U,           // FLASH忙
      BSP_FLASH_TIMEOUT  = 0x03U,           // 操作超时
      BSP_FLASH_INVALID_ADDRESS = 0x04U,    // 无效地址
      BSP_FLASH_INVALID_SIZE    = 0x05U     // 无效大小
  } BSP_FLASH_Status;
  ```
  
  ### BSP_FLASH_Sector
  
  ```c
  typedef enum {
      BSP_FLASH_SECTOR_0  = 0x00U,  /*!< Sector Number 0   */
      BSP_FLASH_SECTOR_1  = 0x01U,  /*!< Sector Number 1   */
      BSP_FLASH_SECTOR_2  = 0x02U,  /*!< Sector Number 2   */
      BSP_FLASH_SECTOR_3  = 0x03U,  /*!< Sector Number 3   */
      BSP_FLASH_SECTOR_4  = 0x04U,  /*!< Sector Number 4   */
      BSP_FLASH_SECTOR_5  = 0x05U,  /*!< Sector Number 5   */
      BSP_FLASH_SECTOR_6  = 0x06U,  /*!< Sector Number 6   */
      BSP_FLASH_SECTOR_7  = 0x07U,  /*!< Sector Number 7   */
      BSP_FLASH_SECTOR_8  = 0x08U,  /*!< Sector Number 8   */
      BSP_FLASH_SECTOR_9  = 0x09U,  /*!< Sector Number 9   */
      BSP_FLASH_SECTOR_10 = 0x0AU,  /*!< Sector Number 10  */
      BSP_FLASH_SECTOR_11 = 0x0BU   /*!< Sector Number 11  */
  } BSP_FLASH_Sector;
  ```
  
  ## API 接口
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Erase_Sector(BSP_FLASH_Sector sector);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Write_Word(uint32_t address, uint32_t data);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Write_HalfWord(uint32_t address, uint16_t data);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Write_Byte(uint32_t address, uint8_t data);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Write_Buffer(uint32_t address, uint8_t* data, uint32_t size);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Read_Word(uint32_t address, uint32_t* data);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Read_HalfWord(uint32_t address, uint16_t* data);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Read_Byte(uint32_t address, uint8_t* data);
  ```
  
  ```c
  BSP_FLASH_Status BSP_FLASH_Read_Buffer(uint32_t address, uint8_t* data, uint32_t size);
  ```
  
  ```c
  uint32_t BSP_FLASH_Get_Sector_Address(BSP_FLASH_Sector sector);
  ```
  
  ```c
  uint8_t BSP_FLASH_Is_Address_Valid(uint32_t address);
  ```
  
  ## 使用示例
  
  ```c
  #include "bsp_flash.h"
  
  void flash_basic_operations_example(void)
  {
      BSP_FLASH_Status status;
      uint32_t data_word;
      uint16_t data_halfword;
      uint8_t data_byte;
      uint8_t buffer[100];
      
      // 1. 擦除扇区5
      status = BSP_FLASH_Erase_Sector(BSP_FLASH_SECTOR_5);
      if (status != BSP_FLASH_OK) {
          printf("Erase sector failed: %d\r\n", status);
          return;
      }
      
      // 2. 写入32位数据
      status = BSP_FLASH_Write_Word(0x08020000, 0x12345678);
      if (status != BSP_FLASH_OK) {
          printf("Write word failed: %d\r\n", status);
          return;
      }
      
      // 3. 读取32位数据
      status = BSP_FLASH_Read_Word(0x08020000, &data_word);
      if (status != BSP_FLASH_OK) {
          printf("Read word failed: %d\r\n", status);
          return;
      }
      printf("Read data: 0x%08X\r\n", data_word);
      
      // 4. 写入半字数据
      status = BSP_FLASH_Write_HalfWord(0x08020004, 0xABCD);
      if (status != BSP_FLASH_OK) {
          printf("Write halfword failed: %d\r\n", status);
          return;
      }
      
      // 5. 读取半字数据
      status = BSP_FLASH_Read_HalfWord(0x08020004, &data_halfword);
      if (status != BSP_FLASH_OK) {
          printf("Read halfword failed: %d\r\n", status);
          return;
      }
      printf("Read data: 0x%04X\r\n", data_halfword);
      
      // 6. 写入字节数据
      status = BSP_FLASH_Write_Byte(0x08020006, 0xEF);
      if (status != BSP_FLASH_OK) {
          printf("Write byte failed: %d\r\n", status);
          return;
      }
      
      // 7. 读取字节数据
      status = BSP_FLASH_Read_Byte(0x08020006, &data_byte);
      if (status != BSP_FLASH_OK) {
          printf("Read byte failed: %d\r\n", status);
          return;
      }
      printf("Read data: 0x%02X\r\n", data_byte);
      
      // 8. 写入数据块
      for (int i = 0; i < sizeof(buffer); i++) {
          buffer[i] = i;
      }
      status = BSP_FLASH_Write_Buffer(0x08020100, buffer, sizeof(buffer));
      if (status != BSP_FLASH_OK) {
          printf("Write buffer failed: %d\r\n", status);
          return;
      }
      
      // 9. 读取数据块
      memset(buffer, 0, sizeof(buffer));
      status = BSP_FLASH_Read_Buffer(0x08020100, buffer, sizeof(buffer));
      if (status != BSP_FLASH_OK) {
          printf("Read buffer failed: %d\r\n", status);
          return;
      }
      
      // 10. 验证数据
      for (int i = 0; i < sizeof(buffer); i++) {
          if (buffer[i] != i) {
              printf("Data verification failed at index %d\r\n", i);
              return;
          }
      }
      printf("Data verification passed\r\n");
  }
  ```
  
  ### FLASH配置参数存储
  
  ```c
  #include "bsp_flash.h"
  
  // 定义配置参数结构体
  typedef struct {
      uint32_t device_id;
      uint16_t config_version;
      uint8_t  motor_speed;
      uint8_t  sensor_threshold;
      float    kp, ki, kd;
  } SystemConfig;
  
  #define CONFIG_FLASH_ADDR 0x080E0000  // 扇区11的起始地址
  
  void save_system_config(SystemConfig* config)
  {
      BSP_FLASH_Status status;
      
      // 1. 擦除扇区
      status = BSP_FLASH_Erase_Sector(BSP_FLASH_SECTOR_11);
      if (status != BSP_FLASH_OK) {
          printf("Erase sector failed\r\n");
          return;
      }
      
      // 2. 写入配置参数
      status = BSP_FLASH_Write_Buffer(CONFIG_FLASH_ADDR, 
                                     (uint8_t*)config, 
                                     sizeof(SystemConfig));
      if (status != BSP_FLASH_OK) {
          printf("Write config failed\r\n");
          return;
      }
      
      printf("System config saved successfully\r\n");
  }
  
  void load_system_config(SystemConfig* config)
  {
      BSP_FLASH_Status status;
      
      // 读取配置参数
      status = BSP_FLASH_Read_Buffer(CONFIG_FLASH_ADDR, 
                                    (uint8_t*)config, 
                                    sizeof(SystemConfig));
      if (status != BSP_FLASH_OK) {
          printf("Read config failed\r\n");
          return;
      }
      
      printf("System config loaded successfully\r\n");
  }
  ```
  
  ## 工作原理
  
  ### FLASH组织结构
  
  STM32F4系列FLASH存储器被划分为多个扇区，每个扇区有不同的大小：
  
  | 扇区  | 大小    | 起始地址       | 结束地址       |
  | --- | ----- | ---------- | ---------- |
  | 0   | 16KB  | 0x08000000 | 0x08003FFF |
  | 1   | 16KB  | 0x08004000 | 0x08007FFF |
  | 2   | 16KB  | 0x08008000 | 0x0800BFFF |
  | 3   | 16KB  | 0x0800C000 | 0x0800FFFF |
  | 4   | 64KB  | 0x08010000 | 0x0801FFFF |
  | 5   | 128KB | 0x08020000 | 0x0803FFFF |
  | 6   | 128KB | 0x08040000 | 0x0805FFFF |
  | 7   | 128KB | 0x08060000 | 0x0807FFFF |
  | 8   | 128KB | 0x08080000 | 0x0809FFFF |
  | 9   | 128KB | 0x080A0000 | 0x080BFFFF |
  | 10  | 128KB | 0x080C0000 | 0x080DFFFF |
  | 11  | 128KB | 0x080E0000 | 0x080FFFFF |
  
  ### 操作流程
  1. **解锁FLASH**：在执行任何写入/擦除操作前，必须先解锁FLASH
  2. **执行操作**：调用相应的写入或擦除函数
  3. **锁定FLASH**：操作完成后立即锁定FLASH以保护数据
  
  ### 地址对齐要求
  
  - Word写入(32位)：地址必须4字节对齐
  - HalfWord写入(16位)：地址必须2字节对齐
  - Byte写入(8位)：地址无对齐要求
  
  ## 注意事项
  
  1. **擦除操作**：FLASH写入前必须先擦除，且只能按扇区擦除
  
  2. **地址范围**：操作地址必须在有效的FLASH地址范围内(0x08000000 - 0x080FFFFF)
  
  3. **写入次数**：FLASH有写入次数限制，应避免频繁写入
  
  4. **数据保持**：FLASH具有数据保持特性，掉电后数据不会丢失
  
  5. **操作原子性**：FLASH操作期间不应被中断打断
  
  6. **错误处理**：所有操作都应检查返回状态，确保操作成功
  
  7. **内存保护**：避免在执行代码的FLASH区域进行写入操作
  
  ## 错误处理
  
  驱动通过BSP_FLASH_Status枚举类型返回操作状态：
  
  ```c
  // 检查操作结果
  BSP_FLASH_Status status = BSP_FLASH_Write_Word(address, data);
  switch (status) {
      case BSP_FLASH_OK:
          // 操作成功
          break;
      case BSP_FLASH_ERROR:
          // 操作失败
          break;
      case BSP_FLASH_INVALID_ADDRESS:
          // 地址无效
          break;
      case BSP_FLASH_INVALID_SIZE:
          // 数据大小无效
          break;
      default:
          // 其他错误
          break;
  }
  
  // 地址有效性检查
  if (!BSP_FLASH_Is_Address_Valid(address)) {
      printf("Invalid flash address: 0x%08X\r\n", address);
      return;
  }
  ```

## 性能说明

1. **擦除时间**：扇区擦除时间通常在几十毫秒到几百毫秒之间
2. **编程时间**：字节/半字/字编程时间通常在几微秒到几十微秒之间
3. **读取时间**：FLASH读取速度与系统时钟相同，无额外延迟
4. **耐久性**：FLASH通常支持至少10万次擦写操作
