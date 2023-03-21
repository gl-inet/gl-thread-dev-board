# SPDX-License-Identifier: Apache-2.0

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/drivers/hx3203)
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/drivers/spl0601)

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(GLNRF52840DK)

# FILE(GLOB app_sources src/*.c)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/ot app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/led app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/sensor app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/cjson app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/led_strip app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/qdec app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/smp app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/srp app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/coap app_sources)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/components/gpio app_sources)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/ot)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/led)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/sensor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/cjson)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/led_strip)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/qdec)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/smp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/srp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/coap)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/components/gpio)


# NORDIC SDK APP START
target_sources(app PRIVATE ${app_sources})
# NORDIC SDK APP END