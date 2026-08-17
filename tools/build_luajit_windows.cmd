@echo off
setlocal
if not exist third_party\LuaJIT\.git git clone --depth 1 --branch v2.1 https://github.com/LuaJIT/LuaJIT.git third_party\LuaJIT
pushd third_party\LuaJIT\src
call msvcbuild.bat
if errorlevel 1 exit /b 1
popd
if not exist luajit-windows mkdir luajit-windows
copy /y third_party\LuaJIT\src\luajit.exe luajit-windows\ >nul
copy /y third_party\LuaJIT\src\lua51.dll luajit-windows\ >nul
copy /y third_party\LuaJIT\src\lua51.lib luajit-windows\ >nul
echo luajit-windows
