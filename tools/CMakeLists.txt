# 创建静态库
set(name TOOLS)

enable_language(C ASM)

add_library(${name} STATIC
    RTT/SEGGER_RTT.c
    RTT/SEGGER_RTT_printf.c
    RTT/SEGGER_RTT_ASM_ARMv7M.S
    SHELL/shell.c
    SHELL/shell_ps.c
    LOG/log.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/RTT
    ${CMAKE_CURRENT_SOURCE_DIR}/SHELL
    ${CMAKE_CURRENT_SOURCE_DIR}/LOG
)

# 链接必要的库
target_link_libraries(${name} stm32cubemx OSAL BSP CONFIG)

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})