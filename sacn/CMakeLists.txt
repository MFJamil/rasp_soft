cmake_minimum_required(VERSION 3.16)
project(sacn_client_01) #Project name

set(CMAKE_CXX_STANDARD 17) 
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

add_subdirectory("lib")



#building target executable
add_executable(${PROJECT_NAME} sacn_client.cpp)


target_link_libraries(${PROJECT_NAME} PRIVATE sACN)