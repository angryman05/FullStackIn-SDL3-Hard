cmake_minimum_required(VERSION 3.20)
project(SDL3_Project_SelfContained)

set(CMAKE_CXX_STANDARD 20)

include_directories(libs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(SDL3_Project_SelfContained
        main.cpp
        rectangle.cpp
        circle.cpp)

if(WIN32)
    include_directories(SDL3/include)
    link_directories(SDL3/lib)
    target_link_libraries(SDL3_Project_SelfContained mingw32 SDL3)
else()
    find_package(SDL3 REQUIRED CONFIG)
    target_link_libraries(SDL3_Project_SelfContained SDL3::SDL3)
endif()
