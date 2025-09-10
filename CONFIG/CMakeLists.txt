set(CONFIG_NAME CONFIG)

# 创建一个接口库，这样其他库可以链接它并获得包含路径
add_library(${CONFIG_NAME} INTERFACE)

# 为接口库设置包含目录
target_include_directories(${CONFIG_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)