cmake_minimum_required(VERSION 3.15)
project(QRCodeGenerator LANGUAGES C CXX)

# ================= 强制静态链接 C++ 运行库 =================
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# =================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 明确列出需要的 libqrencode 核心源文件 
set(QR_ENCODE_SOURCES
    libqrencode/bitstream.c
    libqrencode/mask.c
    libqrencode/mmask.c
    libqrencode/mqrspec.c
    libqrencode/qrencode.c
    libqrencode/qrinput.c
    libqrencode/qrspec.c
    libqrencode/rsecc.c
    libqrencode/split.c
)

# 2. 添加可执行文件
add_executable(QRCodeGenerator WIN32 
    main.cpp
    app.rc
    ${QR_ENCODE_SOURCES}
)

# 3. 添加头文件搜索路径
target_include_directories(QRCodeGenerator PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/libqrencode
)

# 4. 补充缺失的 config.h 宏定义 
target_compile_definitions(QRCodeGenerator PRIVATE
    STATIC_IN_RELEASE=
    MAJOR_VERSION=3
    MINOR_VERSION=4
    MICRO_VERSION=4
    VERSION="3.4.4"
    HAVE_SDL=0
    HAVE_PNG=0
    WITH_TESTS=0
    _CRT_NONSTDC_NO_DEPRECATE  # 消除 strdup 的警告
    _CRT_SECURE_NO_WARNINGS    # 消除 strcpy 等函数的安全警告
)

# 5. 链接 Windows 原生库
target_link_libraries(QRCodeGenerator PRIVATE
    user32.lib
    gdi32.lib
    comctl32.lib
    comdlg32.lib
)

# 6. MSVC 编译器设置：强制使用 UTF-8 编码
if(MSVC)
    target_compile_options(QRCodeGenerator PRIVATE "/utf-8")
endif()