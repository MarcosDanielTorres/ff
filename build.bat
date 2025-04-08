@echo off
setlocal enabledelayedexpansion
set root=%cd%

set freetype_lib_path=%root%\build\libfreetype
set freetype_root=%root%\thirdparty\freetype-2.13.3

set inc_files=
set inc_files=%inc_files% -I%freetype_root%\custom
set inc_files=%inc_files% -I%freetype_root%\include
set inc_files=%inc_files% -I%root%\src

set main=0
set ui=0
set opengl=0
set vulkan=1
set cmdline=0

@rem TODO combine them together!
if "%vulkan%"=="1" set inc_files=%inc_files% -I%VULKAN_SDK%\Include
if "%vulkan%"=="1" set vulkan_lib_path=%VULKAN_SDK%\Lib

set compile_flags=
set compile_flags=-std:c++20 -nologo /EHsc %inc_files% -Zi

set compile=call cl %compile_flags% 

if not exist build mkdir build

pushd build
del *.obj
if "%main%"=="1"                %compile% ..\src\main.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib
if "%cmdline%"=="1"             %compile% ..\src\samples\cmdline.cpp /link user32.lib gdi32.lib
if "%opengl%"=="1"              %compile% ..\src\samples\opengl.cpp /link user32.lib gdi32.lib opengl32.lib
if "%ui%"=="1"                  %compile% ..\src\samples\ui\ui.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib
@rem if "%vulkan%"=="1"              %compile% ..\src\samples\vulkan.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib /LIBPATH:%vulkan_lib_path% vulkan-1.lib
if "%vulkan%"=="1"              %compile% ..\src\samples\vulkan.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib 
popd