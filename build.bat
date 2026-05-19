@echo off
setlocal

REM 1. Set up the compiler environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM 2. Prepare directories
if not exist build mkdir build

REM 3. Copy dependencies and shaders
REM Copy DLL to build folder
copy /y "SDL3-3.4.8\lib\x64\SDL3.dll" "build\"
REM Copy shaders folder to build folder
xcopy /s /y "shaders" "build\shaders\"

REM 4. Compile and Link
REM Note: Using the paths visible in your screenshot
cl /EHsc /Zi ^
 /I "SDL3-3.4.8\include" ^
 /I "glm" ^
 /I "imgui" ^
 /I "imgui\backends" ^
 /I "glad\include" ^
 src\Main.cpp ^
 glad\src\glad.c ^
 imgui\imgui.cpp ^
 imgui\imgui_draw.cpp ^
 imgui\imgui_widgets.cpp ^
 imgui\imgui_tables.cpp ^
 imgui\backends\imgui_impl_sdl3.cpp ^
 imgui\backends\imgui_impl_opengl3.cpp ^
 /Fe:build\HT_Game_Engine.exe ^
 /link /LIBPATH:"SDL3-3.4.8\lib\x64" SDL3.lib opengl32.lib user32.lib shell32.lib

echo Build Complete!