cmake_minimum_required(VERSION 3.20.0)

set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ZEPHYR_EXTRA_MODULES ${BOARD_ROOT}/drivers)

if (NOT BUILD_TYPE)
    set(BUILD_TYPE debug)
endif()

list(APPEND CONF_FILE
    ${CMAKE_CURRENT_LIST_DIR}/conf/prj.conf
    ${CMAKE_CURRENT_LIST_DIR}/conf/prj_${BUILD_TYPE}.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf52840_smartwatch)

target_sources(app PRIVATE src/main.c)
