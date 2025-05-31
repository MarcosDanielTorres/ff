@echo off
setlocal enabledelayedexpansion
set root=%cd%

set freetype_lib_path=%root%\build\libfreetype
set opengl_lib_path=%root%\src\samples\opengl\thirdpartya
set some_path=%root%\build
set freetype_root=%root%\thirdparty\freetype-2.13.3
set glm_root=%root%\thirdparty\glm

set pc="C:\Program Files"
set notebook="C:\Archivos de programa"
set assimp_root=%pc%

set assimp_root=%assimp_root%\assimp
set assimp_lib_path=%assimp_root%\lib
set assimp_lib_path_dll=%assimp_root%\lib-dll

set inc_files=
set inc_files=%inc_files% -I%freetype_root%\custom
set inc_files=%inc_files% -I%freetype_root%\include
set inc_files=%inc_files% -I%assimp_root%\include
set inc_files=%inc_files% -I%root%\src
@rem fucking fix this fuckkkkkkk. most reasonable thing is to just drop them with all the other files! i did this because the lib libs in build (i guess jolt or someother, not freetype)
set inc_files=%inc_files% -I%some_path%

set main=0
set ui=0
set opengl=0
set vulkan=0
set cmdline=0
set meta=0
set game=0
set openglrewrite=0
set skinning=0
set render_example=1

@rem TODO combine them together!
if "%vulkan%"=="1" set inc_files=%inc_files% -I%VULKAN_SDK%\Include
if "%vulkan%"=="1" set vulkan_lib_path=%VULKAN_SDK%\Lib

@rem TODO see why I have /EHsc
set compile_flags=
set compile_flags=-diagnostics:column -WL -std:c++20 -nologo -Gm- /EHa %inc_files% -Zi -Od
@ rem try with /Zc:nrvo instead of -Zi

set compile=call cl %compile_flags% 

if not exist build mkdir build

pushd build
del *.obj
if "%meta%"=="1"                %compile% ..\src\samples\metagen\meta.cpp /link user32.lib gdi32.lib
if "%meta%"=="1" (
    popd
    build\meta.exe > .\src\generated\generated.h
    build\meta.exe > .\src\samples\metagen\generated.h
    pushd build
)
if "%meta%"=="1"                %compile% ..\src\samples\metagen\program.cpp
if "%main%"=="1"                %compile% ..\src\main.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib
if "%cmdline%"=="1"             %compile% ..\src\samples\cmdline.cpp /link user32.lib gdi32.lib
if "%opengl%"=="1"              %compile% ..\src\samples\opengl.cpp /link user32.lib gdi32.lib opengl32.lib
if "%ui%"=="1"                  %compile% ..\src\samples\ui\ui.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib opengl32.lib
@rem if "%vulkan%"=="1"              %compile% ..\src\samples\vulkan.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib /LIBPATH:%vulkan_lib_path% vulkan-1.lib
if "%vulkan%"=="1"              %compile% -MDd ..\src\samples\vulkan\vulkan.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib glslangd.lib glslang-default-resource-limitsd.lib SPIRVd.lib spirv-reflect-staticd.lib
if "%game%"=="1"                %compile% ..\src\samples\2d_game\game.cpp /link /LIBPATH:%freetype_lib_path% libfreetype.lib user32.lib gdi32.lib comdlg32.lib

@rem if "%openglrewrite%"=="1"       %compile% /MTd -Fmmain.map ..\src\samples\opengl\maain.cpp -DJPH_CROSS_PLATFORM_DETERMINISTIC=1 -DJPH_OBJECT_LAYER_BITS=32 -DJPH_DEBUG_RENDERER=1 -DJPH_USE_AVX2=1 -DJPH_USE_AVX=1 -DJPH_USE_SSE4_1=1 -DJPH_USE_SSE4_2=1 -DJPH_USE_LZCNT=1 -DJPH_USE_TZCNT=1 -DJPH_USE_F16C=1 /I%VULKAN_SDK%\Include /I%opengl_lib_path%\JoltPhysics-5.0.0\include /I%opengl_lib_path%\assimp-5.2.5\include /link /LIBPATH:%opengl_lib_path%\assimp-5.2.5\lib\Debug assimp-vc143-mtd.lib /LIBPATH:%opengl_lib_path%\JoltPhysics-5.0.0\lib Joltd.lib shell32.lib msvcrtd.lib libcmtd.lib opengl32.lib user32.lib gdi32.lib
if "%openglrewrite%"=="1"       %compile% -MD -Fmmain.map ..\src\samples\opengl\maain.cpp /I%VULKAN_SDK%\Include /link /LIBPATH:%assimp_lib_path% opengl32.lib gdi32.lib user32.lib assimp.lib zlibstatic.lib

if "%skinning%"=="1"            %compile% -MD -Feopengl-skinning.exe -Fmopengl-skinning.map ..\src\samples\skinning\skinning.cpp /I%VULKAN_SDK%\Include /link /LIBPATH:%assimp_lib_path% opengl32.lib gdi32.lib user32.lib assimp.lib zlibstatic.lib
@rem NOTE I was previously using -MD for compilation (I guess for no particular reason, just found that it worked at the moment. But when adding freetype i had to moved back to the fault (whic is MT))
if "%render_example%"=="1"      %compile% -MT -Ferender_example.exe -Fmrender_example.map ..\src\samples\render_example\render_example.cpp /I%glm_root% /link -incremental:no -opt:ref /LIBPATH:%assimp_lib_path_dll% opengl32.lib gdi32.lib user32.lib assimp.lib /LIBPATH:%freetype_lib_path% libfreetype.lib 

popd