cmake_minimum_required(VERSION 3.5)
project(wrapper_api)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
set(CMAKE_CXX_STANDARD 17)
# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
include_directories(${CMAKE_SOURCE_DIR}/crow/include)
include_directories(${CMAKE_SOURCE_DIR}/asio/1.30.2/include)
# executable definition and properties
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(wrapper_api main.cpp)
# Include and Link
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(wrapper_api ${Boost_LIBRARIES})
