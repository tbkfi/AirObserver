cmake_minimum_required(VERSION 4.00)

set(BOARD nrf7002dk/nrf5340/cpuapp)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(application)

# can be passed as CLI arg -b to west

target_sources(app PRIVATE
    src/main.c
)

