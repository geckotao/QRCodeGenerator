# qr
二维码生成器

1、克隆 wxWidgets 及其所有子模块 
git clone --depth 1 --recurse-submodules -b v3.2.4 https://github.com/wxWidgets/wxWidgets.git

2、克隆 libqrencode 及其子模块
git clone --depth 1 --recurse-submodules -b v4.1.1 https://github.com/fukuchi/libqrencode.git

3、编译

:: 1. 配置 CMake (自动创建 build 目录并生成 VS 工程)
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build

:: 2. 开始编译 
cmake --build build --config Release
