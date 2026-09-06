# qr
二维码生成器

1、下载https://github.com/fltk/fltk/releases/download/release-1.4.5/fltk-1.4.5-source.tar.gz

解压后将fltk-1.4.5重命名文件夹为fltk

2、下载stb_image_write.h与main.cpp同目录

https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h?spm=a2ty_o01.29997173.0.0.535855fb0zGQW9&file=stb_image_write.h


3、克隆 libqrencode 及其子模块

git clone --depth 1 --recurse-submodules -b v4.1.1 https://github.com/fukuchi/libqrencode.git

4、编译

:: 1. 配置 CMake (自动创建 build 目录并生成 VS 工程)

cmake -G "Visual Studio 17 2022" -A x64 -S . -B build

:: 2. 开始编译 

cmake --build build --config Release
