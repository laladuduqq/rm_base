# 创建静态库
set(name OSAL)

add_library(${name} STATIC
    osal_thread.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接必要的库，根据是否存在ThreadX库决定链接哪个RTOS库
if(TARGET ThreadX)
    # 如果存在ThreadX库，则链接ThreadX
    target_link_libraries(${name} stm32cubemx ThreadX)
    message(STATUS "OSAL: Using ThreadX RTOS")
else()
    # 否则使用裸机模式
    target_link_libraries(${name} stm32cubemx)
    message(STATUS "OSAL: Using Bare Metal mode")
endif()

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})