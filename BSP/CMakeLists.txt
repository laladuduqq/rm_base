# 创建静态库
set(name BSP)

add_library(${name} STATIC
    UART/bsp_uart.c
    SPI/bsp_spi.c
    PWM/bsp_pwm.c
    I2C/bsp_i2c.c
    GPIO/bsp_gpio.c
    DWT/bsp_dwt.c
    FLASH/bsp_flash.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/UART
    ${CMAKE_CURRENT_SOURCE_DIR}/SPI
    ${CMAKE_CURRENT_SOURCE_DIR}/PWM
    ${CMAKE_CURRENT_SOURCE_DIR}/I2C
    ${CMAKE_CURRENT_SOURCE_DIR}/GPIO
    ${CMAKE_CURRENT_SOURCE_DIR}/DWT
    ${CMAKE_CURRENT_SOURCE_DIR}/FLASH
    
)

# 链接必要的库
target_link_libraries(${name} stm32cubemx OSAL)

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})