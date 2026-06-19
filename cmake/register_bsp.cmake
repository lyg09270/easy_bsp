macro(register_easy_bsp)

    # ============================================================
    # 【核心修复】精准判定环境，采用各自最稳妥的根路径变量
    # ============================================================
    if(COMMAND idf_component_register)
        # ESP-IDF 官方标准变量，无论在脚本模式还是常规模式下都绝对安全、精准
        set(EZBSP_ROOT_DIR "${COMPONENT_DIR}")
    else()
        # 标准 CMake / 跨平台裸机环境下的标准根目录
        set(EZBSP_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # 1. 统一收集源文件 (现在路径绝对 100% 能抓到文件了)
    file(GLOB_RECURSE EZBSP_SOURCES
        "${EZBSP_ROOT_DIR}/src/mock/*.c"
        "${EZBSP_ROOT_DIR}/src/dev/*.c"
        "${EZBSP_ROOT_DIR}/src/drv/*.c"
        "${EZBSP_ROOT_DIR}/src/hal/*.c"
        "${EZBSP_ROOT_DIR}/src/drv_template/*.c"
        "${EZBSP_ROOT_DIR}/src/espressif/*.c"
    )
    
    set(EZBSP_INC_DIR "${EZBSP_ROOT_DIR}/inc")

    # 2. 环境路由分流
    if(COMMAND idf_component_register)
        message(STATUS "[easy_bsp] Registering as ESP-IDF Component")
        
        # 打印一下抓到的源文件数量，方便你在后台肉眼直接校验是不是空的
        message(STATUS "[easy_bsp] Found source files: ${EZBSP_SOURCES}")
        
        idf_component_register(
            SRCS "${EZBSP_SOURCES}"
            INCLUDE_DIRS "${EZBSP_INC_DIR}"
            # 添加了 log 组件依赖
            REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi soc log esp_timer
        )
        set(EZBSP_TARGET ${COMPONENT_LIB})

    else()
        message(STATUS "[easy_bsp] Registering as Standard CMake Target")
        
        set(EZBSP_TARGET easy_bsp)
        add_library(${EZBSP_TARGET} STATIC)
        
        target_sources(${EZBSP_TARGET} PRIVATE ${EZBSP_SOURCES})
        target_include_directories(${EZBSP_TARGET} PUBLIC ${EZBSP_INC_DIR})
    endif()

    # 3. 统一配置公共 Target 属性
    if(NOT COMMAND idf_component_register)
        target_compile_features(${EZBSP_TARGET} PUBLIC c_std_99)
    endif()

    if(EZBSP_IS_TOP_LEVEL)
        if(MSVC)
            target_compile_options(${EZBSP_TARGET} PRIVATE /W4)
        else()
            target_compile_options(${EZBSP_TARGET} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()

    # Platform Specific (平台/芯片特定) 配置区
    if(NOT COMMAND idf_component_register)
        # 如果不是 ESP-IDF（比如是 GD32 或本地 Mock 仿真），在这里添加特定的 SDK 依赖
        # target_include_directories(${EZBSP_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/Firmware/CMSIS)
        # target_compile_definitions(${EZBSP_TARGET} PUBLIC GD32E230)
    endif()

endmacro()