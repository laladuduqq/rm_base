# 创建静态库
set(name BSP)

add_library(${name} STATIC
    UART/bsp_uart.c
    SPI/bsp_spi.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/UART
    ${CMAKE_CURRENT_SOURCE_DIR}/SPI

)

# 链接必要的库
target_link_libraries(${name} stm32cubemx OSAL)

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})