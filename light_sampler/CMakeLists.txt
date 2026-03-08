cmake_minimum_required(VERSION 3.10)
project(light_sampler C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -D_POSIX_C_SOURCE=200809L")

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/hal
    ${CMAKE_SOURCE_DIR}/src
)

# HAL library
add_library(hal STATIC
    ${CMAKE_SOURCE_DIR}/hal/adc_hal.c
    ${CMAKE_SOURCE_DIR}/hal/rotary_hal.c
    ${CMAKE_SOURCE_DIR}/hal/pwm_hal_linux.c
)
target_link_libraries(hal gpiod pthread)

# Rotary control module
add_library(rotary_control STATIC
    ${CMAKE_SOURCE_DIR}/src/rotary_control.c
)
target_link_libraries(rotary_control hal pthread)

# Sampler library
add_library(sampler STATIC
    ${CMAKE_SOURCE_DIR}/src/sampler.c
    ${CMAKE_SOURCE_DIR}/src/periodTimer.c
    ${CMAKE_SOURCE_DIR}/src/dip_detector.c
)
target_link_libraries(sampler hal pthread)

# UDP server library
add_library(udp_server STATIC
    ${CMAKE_SOURCE_DIR}/src/udp_server.c
)
target_link_libraries(udp_server sampler)

# Test programs
add_executable(test_adc ${CMAKE_SOURCE_DIR}/test/test_adc.c)
target_link_libraries(test_adc hal)

add_executable(test_sampler_udp ${CMAKE_SOURCE_DIR}/test/test_sampler_udp.c)
target_link_libraries(test_sampler_udp udp_server sampler hal pthread)

# FIXED: Link against hal library (which includes rotary_hal.c)
add_executable(test_rotary ${CMAKE_SOURCE_DIR}/test/test_rotary_working.c)
target_link_libraries(test_rotary hal pthread)

# FIXED: Link against rotary_control and hal
add_executable(test_rotary_led ${CMAKE_SOURCE_DIR}/test/test_rotary_led.c)
target_link_libraries(test_rotary_led rotary_control hal pthread)
