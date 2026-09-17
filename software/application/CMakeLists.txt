cmake_minimum_required(VERSION 4.00)

set(BOARD nrf7002dk/nrf5340/cpuapp) # can be passed as CLI arg -b to west

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(application)

target_sources(app PRIVATE
    src/main.cpp
    src/wifictl.hpp
    src/wifictl.cpp
    src/bme690.hpp
    src/bme690.cpp
)

