@echo off
setlocal enabledelayedexpansion
set root=%cd%

set freetype_lib_path=%root%\build\libfreetype
set freetype_root=%root%\thirdparty\freetype-2.13.3

set inc_files=
set inc_files=%inc_files% -I%freetype_root%\custom
set inc_files=%inc_files% -I%freetype_root%\include
set inc_files=%inc_files% -I%root%\src

set compile_flags=
set compile_flags=-nologo %inc_files% -Z7

set compile=call cl %compile_flags% 

set main=0
set ui=1
set opengl=0
set vulkan=0

if not exist build mkdir build

pushd build
del *.obj
if "%main%"=="1"                %compile% ..\src\main.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib
if "%samples_cmdline%"=="1"     %compile% ..\src\samples\samples_cmdline.cpp /link user32.lib gdi32.lib
if "%opengl%"=="1"              %compile% ..\src\samples\opengl.cpp /link user32.lib gdi32.lib opengl32.lib
if "%ui%"=="1"                  %compile% ..\src\samples\ui\ui.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib
popd

