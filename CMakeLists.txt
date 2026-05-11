cmake_minimum_required(VERSION 3.20)
project(XteinkFirmware LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
add_library(ha_bridge
  src/ha_bridge.cpp
  src/blind_control_view.cpp
)
target_include_directories(ha_bridge PUBLIC include)
target_link_libraries(ha_bridge PUBLIC CURL::libcurl)
add_executable(xteink_main src/main.cpp)
target_link_libraries(xteink_main PRIVATE ha_bridge Threads::Threads)
add_executable(test_ha_bridge tests/test_ha_bridge.cpp)
target_link_libraries(test_ha_bridge PRIVATE ha_bridge)
enable_testing()
add_test(NAME ha_bridge_unit COMMAND test_ha_bridge)
