# 创建静态库
set(name MODULES)


add_library(${name} STATIC
    RGB/rgb.c
    IST8310/ist8310.c
    BMI088/bmi088.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/RGB
    ${CMAKE_CURRENT_SOURCE_DIR}/IST8310
    ${CMAKE_CURRENT_SOURCE_DIR}/BMI088
)

# 链接必要的库
target_link_libraries(${name} stm32cubemx OSAL BSP CONFIG TOOLS)

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})