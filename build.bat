cd /d %~dp0
@echo off
set path=%path%;D:\MinGW\bin
premake5 gmake
mingw32-make config=release
