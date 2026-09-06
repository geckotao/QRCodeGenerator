# 二维码生成器

使用libqrencode及fltk库生成二维码

项目结构如下：

\qr

├── [文件]app.rc 

├── [文件]CMakeLists.txt

├── [文件]favicon.ico

├── [目录]fltk（由fltk.zip解压）

├── [目录]libqrencode

├── [文件]main.cpp

└── [文件]stb_image_write.h


编译
:: 1. 配置 CMake (自动创建 build 目录并生成 VS 工程)

cmake -G "Visual Studio 17 2022" -A x64 -S . -B build

:: 2. 开始编译 

cmake --build build --config Release
