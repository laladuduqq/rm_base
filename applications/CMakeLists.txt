# 创建静态库
set(name APPLICATIONS)


add_library(${name} STATIC
    robot_init/robot_init.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/robot_init
)

# 链接必要的库
target_link_libraries(${name} stm32cubemx OSAL BSP CONFIG TOOLS MODULES)

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})