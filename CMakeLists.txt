cmake_minimum_required(VERSION 3.16)

project(easy_bsp
    VERSION 0.1.0
    LANGUAGES C
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(EZBSP_IS_TOP_LEVEL ON)
else()
    set(EZBSP_IS_TOP_LEVEL OFF)
endif()

option(EZBSP_BUILD_TESTS "Build easy_bsp tests" ${EZBSP_IS_TOP_LEVEL})
option(EZBSP_BUILD_EXAMPLES "Build easy_bsp examples" ${EZBSP_IS_TOP_LEVEL})

add_library(easy_bsp STATIC)

# User-editable source selection.
# Change this path to select your backend/platform source folder.
file(GLOB_RECURSE EZBSP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mock/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dev/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drv_template/*.c
)

target_sources(easy_bsp
    PRIVATE
        ${EZBSP_SOURCES}
)

target_include_directories(easy_bsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# ============================================================
# External dependencies
#
# Add chip SDK / HAL / platform package dependencies here.
#
# Example:
#
# target_include_directories(easy_bsp
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/Firmware/CMSIS
#         ${PROJECT_SOURCE_DIR}/Firmware/GD32E23x_standard_peripheral/Include
# )
#
# target_sources(easy_bsp
#     PRIVATE
#         ${PROJECT_SOURCE_DIR}/Firmware/GD32E23x_standard_peripheral/Source/gd32e23x_gpio.c
#         ${PROJECT_SOURCE_DIR}/Firmware/GD32E23x_standard_peripheral/Source/gd32e23x_rcu.c
#         ${PROJECT_SOURCE_DIR}/Firmware/GD32E23x_standard_peripheral/Source/gd32e23x_usart.c
# )
#
# target_compile_definitions(easy_bsp
#     PUBLIC
#         GD32E230
# )
#
# target_link_libraries(easy_bsp
#     PUBLIC
#         some_external_sdk_target
# )
# ============================================================

target_compile_features(easy_bsp
    PUBLIC
        c_std_99
)

if(EZBSP_IS_TOP_LEVEL)
    if(MSVC)
        target_compile_options(easy_bsp PRIVATE /W4)
    else()
        target_compile_options(easy_bsp PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(EZBSP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()