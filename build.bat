@echo off
REM ============================================================
REM   随机抽签工具 - Windows 源码构建脚本
REM   需要预先安装 MSYS2 + MinGW-w64 UCRT64
REM ============================================================

setlocal enabledelayedexpansion

REM 切换到脚本所在目录
cd /d "%~dp0\.."

REM 检测 MSYS2 安装路径
if exist "C:\msys64\msys64\ucrt64.exe" (
    set "MSYS2=C:\msys64\msys64"
) else if exist "D:\msys64\msys64\ucrt64.exe" (
    set "MSYS2=D:\msys64\msys64"
) else (
    echo [错误] 未找到 MSYS2。请先安装 MSYS2 到 C:\msys64
    echo 下载地址: https://www.msys2.org/
    exit /b 1
)

echo [信息] 使用 MSYS2: %MSYS2%
echo [信息] 项目目录: %CD%

REM 检查必要组件
"%MSYS2%\ucrt64\bin\g++.exe" --version >nul 2>&1
if errorlevel 1 (
    echo [信息] 正在通过 pacman 安装编译依赖...
    "%MSYS2%\usr\bin\bash.exe" -lc "pacman -S --noconfirm --needed mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,pkgconf,libzip,zlib}"
    if errorlevel 1 (
        echo [错误] pacman 安装失败
        exit /b 1
    )
)

REM 配置 CMake
echo [信息] 配置 CMake ...
"%MSYS2%\usr\bin\bash.exe" -lc "cd '%CD%' && export PATH=/ucrt64/bin:/usr/bin:\$PATH && rm -rf build && cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release"
if errorlevel 1 exit /b 1

REM 构建
echo [信息] 编译中 ...
"%MSYS2%\usr\bin\bash.exe" -lc "cd '%CD%' && export PATH=/ucrt64/bin:/usr/bin:\$PATH && cmake --build build"
if errorlevel 1 exit /b 1

REM 复制运行时 DLL
echo [信息] 复制运行时 DLL ...
set "OUTDIR=%CD%\dist\Windows"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"
copy /Y "%CD%\build\random_picker.exe" "%OUTDIR%\" >nul
for %%f in (libwinpthread-1.dll libzip.dll libbz2-1.dll zlib1.dll libzstd.dll liblzma-5.dll) do (
    if exist "%MSYS2%\ucrt64\bin\%%f" copy /Y "%MSYS2%\ucrt64\bin\%%f" "%OUTDIR%\" >nul
)

echo.
echo ============================================================
echo  构建完成！可执行文件位于:
echo   %OUTDIR%\random_picker.exe
echo ============================================================
endlocal
