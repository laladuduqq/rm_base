# 创建静态库
set(name MODULES)


add_library(${name} STATIC
    RGB/rgb.c
    IST8310/ist8310.c
    BMI088/bmi088.c
    BMI088/bmi088_cali.c
    algorithm/compensation.c
    algorithm/controller.c
    algorithm/kalman_filter.c
    algorithm/LQR.c
    algorithm/QuaternionEKF.c
    algorithm/user_lib.c
    BEEP/beep.c
    OFFLINE/offline.c
)

# 设置包含目录
target_include_directories(${name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/RGB
    ${CMAKE_CURRENT_SOURCE_DIR}/IST8310
    ${CMAKE_CURRENT_SOURCE_DIR}/BMI088
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithm
    ${CMAKE_CURRENT_SOURCE_DIR}/BEEP
    ${CMAKE_CURRENT_SOURCE_DIR}/OFFLINE
)

# 链接必要的库
target_link_libraries(${name} stm32cubemx OSAL BSP CONFIG TOOLS)
target_link_libraries(MODULES -l:libarm_cortexM4lf_math.a) #链接dsp
target_link_libraries(MODULES m) #链接数学库

# 添加到主项目
target_link_libraries(${PROJECT_NAME} ${name})