@echo off
set root=%cd%
set freetype_root=%root%\thirdparty\freetype-2.13.3

set src_files=
set src_files=%src_files% %freetype_root%\src\base\ftsystem.c
set src_files=%src_files% %freetype_root%\src\base\ftinit.c
set src_files=%src_files% %freetype_root%\src\base\ftdebug.c
set src_files=%src_files% %freetype_root%\src\base\ftbase.c
set src_files=%src_files% %freetype_root%\src\base\ftmm.c
set src_files=%src_files% %freetype_root%\src\base\ftbitmap.c

set src_files=%src_files% %freetype_root%\src\sfnt\sfnt.c
set src_files=%src_files% %freetype_root%\src\truetype\truetype.c

set src_files=%src_files% %freetype_root%\src\smooth\smooth.c 

set src_files=%src_files% %freetype_root%\src\psnames\psnames.c


set inc_files=
set inc_files=%inc_files% -I%freetype_root%\custom
set inc_files=%inc_files% -I%freetype_root%\include

if not exist "build/libfreetype" mkdir "build/libfreetype" 

pushd "build/libfreetype"
del *.obj
cl -c -nologo -DFT2_BUILD_LIBRARY %inc_files% %src_files%
lib -nologo -out:libfreetype.lib *.obj
popd