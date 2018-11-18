cmake_minimum_required(VERSION 2.0)
add_definitions(-Wall -std=c++11)
add_executable(raspi-echonet
    main.cpp
    serial/serial.cpp
)
