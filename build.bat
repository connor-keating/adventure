:: Build script for application layer
@ECHO OFF
SetLocal EnableDelayedExpansion

:: Build directory
set outdir=%cd%\bin
if not exist %outdir% mkdir %outdir%

pushd "src"

set assembly=application
set app_flags=-D_OPENGL -D_DEBUG
set compiler_flags=-g -std=c++20 -Wvarargs -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-deprecated
set includes=-I\src\ -I..\external\
set linker_flags=-luser32 -lgdi32 -lwinmm -ld3d11 -ldxgi -lopengl32
:: set defines=


:: Build platform libraries

:: Windows platform lib
set plat_assembly=platform
set plat_defines=-D_DEBUG -D_EXPORT
set plat_includes=-I\src\ -I..\external\
set plat_flags_comp=-g -std=c++20 -shared -Wvarargs -Wall -Werror -Wno-deprecated -Wno-unused-function
set plat_flags_link=-luser32 -lgdi32 -lwinmm

echo %plat_assembly% compiling...
clang++ %plat_flags_comp% %plat_defines% platform_win32.cpp core.cpp -o %OUTDIR%\%plat_assembly%.dll %plat_includes% %plat_flags_link%

if %ERRORLEVEL% neq 0 (
    popd 
    echo Build failed with errors!
    exit /b %ERRORLEVEL%
)
echo Building %plat_assembly% complete

:: Build application
echo %assembly% compiling...
clang++ %compiler_flags% %app_flags% main.cpp %code_files% -o %outdir%\%assembly%.exe %defines% %includes% %linker_flags% -L%OUTDIR% -l%plat_assembly%.lib

popd 

if %ERRORLEVEL% neq 0 (
    echo Build failed with errors!
    exit /b %ERRORLEVEL%
)

echo Building %assembly% complete
