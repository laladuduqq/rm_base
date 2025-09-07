# 创建静态库
set(name OSAL)

add_library(${name} STATIC
    osal_thread.c
    osal_sem.c
    osal_mutex.c
    osal_event.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接必要的库，链接哪个RTOS库
if(TARGET ThreadX)
    # 如果存在ThreadX库，则链接ThreadX
    target_link_libraries(${name} stm32cubemx ThreadX)
    message(STATUS "OSAL: Using ThreadX RTOS")
elseif(EXISTS ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS)
    # 如果存在FreeRTOS，则链接FreeRTOS
    target_link_libraries(${name} stm32cubemx FreeRTOS)
    message(STATUS "OSAL: Using FreeRTOS")
else()
    # 否则报错，要求必须使用RTOS
    message(FATAL_ERROR "OSAL: No RTOS found. Please provide either ThreadX or FreeRTOS.")
endif()

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})