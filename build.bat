:: Build script for application layer
@ECHO OFF
SetLocal EnableDelayedExpansion

:: Build directory
set outdir=%cd%\bin
if not exist %outdir% mkdir %outdir%

:: What are you building? Ex. apps/file.cpp -> app2build=file
set app2build=scratch

:: Mode - _DEBUG or _PRODUCTION
set mode=_DEBUG

:: Go into source code directory
pushd "src"

set assembly=main
set app_src_dir=..\apps
set app_flags=-D_D3D -D%mode%
set compiler_flags=-g -std=c++20 -Wvarargs -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-deprecated
set includes=-I. -I..\external -I%app_src_dir%
set linker_flags=-luser32 -lgdi32 -lwinmm -ld3d11 -ldxgi -lopengl32 -ld3dcompiler

:: Build application
echo %assembly% compiling...
:: clang++ %compiler_flags% %app_flags% maintest.cpp %code_files% -o %outdir%\%assembly%.exe %defines% %includes% %linker_flags% -L%OUTDIR% -l%plat_assembly%.lib

clang++ ^
%compiler_flags% ^
%app_flags% ^
main.cpp core.cpp platform_win32.cpp render_dx11.cpp %app_src_dir%\%app2build%.cpp ^
-o ^
%outdir%\%assembly%.exe ^
%defines% ^
%includes% ^
%linker_flags% 

popd 

if %ERRORLEVEL% neq 0 (
    echo Build failed with errors!
    exit /b %ERRORLEVEL%
)

echo Building %assembly% complete
