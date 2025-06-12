cmake_minimum_required(VERSION 3.10.0)
project(LandingMoon VERSION 0.1.0 LANGUAGES C CXX)

add_executable(LandingMoon src/main.cpp)

# 设置 LandingMoon 可执行文件输出目录（适配 VS 多配置生成器）
set_target_properties(LandingMoon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin/relwithdebinfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin/minsizerel"
)