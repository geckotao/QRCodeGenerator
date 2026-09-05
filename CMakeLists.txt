cmake_minimum_required(VERSION 3.16)
project(QRCodeGenerator LANGUAGES CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# 强制 MSVC 使用 UTF-8 编码处理源文件
# ==========================================
if(MSVC)
    add_compile_options("/utf-8")
endif()

# 兼容 CMake 策略
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# 1. 配置 wxWidgets 编译选项
set(wxBUILD_SHARED OFF CACHE BOOL "Build shared libraries" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "Build samples" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "Build demos" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)

# 2. 配置 libqrencode 编译选项
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(WITH_TOOLS OFF CACHE BOOL "Build utility tools" FORCE)

# 3. 引入本地源码
add_subdirectory(wxWidgets)
add_subdirectory(libqrencode)

# 4. 配置主程序
add_executable(QRCodeGenerator WIN32 main.cpp)

if(WIN32)
    target_sources(QRCodeGenerator PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
endif()

target_include_directories(QRCodeGenerator PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/libqrencode
)

target_link_libraries(QRCodeGenerator PRIVATE 
    wxcore 
    wxbase
    qrencode
)