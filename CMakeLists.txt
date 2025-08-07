
cmake_minimum_required(VERSION 3.10)
project(boot_time_report_parser C)

set(CMAKE_C_STANDARD 99)

add_executable(boot_time_report_parser
    boot_time_report.c
)

install(TARGETS boot_time_report_parser DESTINATION bin)
install(FILES boot_time_report.h DESTINATION include)
