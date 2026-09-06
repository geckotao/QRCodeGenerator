cmake_minimum_required(VERSION 3.16)
project(QRCodeGenerator LANGUAGES CXX)

# 用 /MD ，EXE 会依赖 vcruntime140.dll，但 Win10/11 系统已自带，不影响用户直接运行。
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options("/utf-8")
    add_compile_options("$<$<CONFIG:Release>:/O1>")
    add_compile_options("$<$<CONFIG:Release>:/GL>")
    add_compile_options("$<$<CONFIG:Release>:/Z7>")
    
    add_link_options("$<$<CONFIG:Release>:/LTCG>")
    add_link_options("$<$<CONFIG:Release>:/OPT:REF>")
    add_link_options("$<$<CONFIG:Release>:/OPT:ICF>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG:NONE>")
endif()

set(OPTION_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(FLTK_BUILD_TEST OFF CACHE BOOL "Build tests" FORCE)
set(OPTION_USE_GL OFF CACHE BOOL "Use OpenGL" FORCE)

set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(WITH_TOOLS OFF CACHE BOOL "Build utility tools" FORCE)

add_subdirectory(fltk)
add_subdirectory(libqrencode)

add_executable(QRCodeGenerator WIN32 main.cpp)

if(WIN32)
    target_sources(QRCodeGenerator PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
endif()

target_include_directories(QRCodeGenerator PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/libqrencode
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_link_libraries(QRCodeGenerator PRIVATE 
    fltk
    fltk_images
    qrencode
)

if(WIN32)
    target_link_libraries(QRCodeGenerator PRIVATE 
        comctl32 ws2_32 gdi32 ole32 uuid
    )
endif()