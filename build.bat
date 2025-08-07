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
set includes=-I/src/ -I../external/
set linker_flags=-luser32 -lgdi32 -lwinmm -ld3d11 -ldxgi -lopengl32
:: set defines=

echo %assembly% compiling...

clang++ %compiler_flags% %app_flags% main.cpp %code_files% -o %outdir%\%assembly%.exe %defines% %includes% %linker_flags%

popd 

if %ERRORLEVEL% neq 0 (
    echo Build failed with errors!
    exit /b %ERRORLEVEL%
)

echo Building %assembly% complete
