:: Build script for application layer
@ECHO OFF
SetLocal EnableDelayedExpansion

:: Build directory
set outdir=%cd%\bin
if not exist %outdir% mkdir %outdir%

:: Mode
:: Should be _DEBUG or _PRODUCTION
set mode=_DEBUG

if %mode% == _PRODUCTION (
    :: Build shaders
    echo building shaders...
    pushd "shaders"

    set windows_sdk="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64"
    set hlsl_flags=/nologo /T
    set cfg=/Od /Zi

    %windows_sdk%\fxc.exe %hlsl_flags% vs_5_0 /E vertex_shader /Fo "%outdir%\tri_vs.cso" %cfg% "tri.hlsl"
    %windows_sdk%\fxc.exe %hlsl_flags% ps_5_0 /E pixel_shader  /Fo "%outdir%\tri_ps.cso" %cfg% "tri.hlsl"

    if ERRORLevel 1 (
        echo ERROR: Shaders failed to compile
        popd
        exit /b 1
    )
    popd
    echo shaders complete!!!
    echo:
)


:: Go into source code directory
pushd "src"

set assembly=application
set app_flags=-D_D3D -D%mode%
set compiler_flags=-g -std=c++20 -Wvarargs -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-deprecated
set includes=-I\src\ -I..\external\
set linker_flags=-luser32 -lgdi32 -lwinmm -ld3d11 -ldxgi -lopengl32 -ld3dcompiler
:: set defines=


:: Build platform libraries

:: Windows platform lib
set plat_assembly=platform
set plat_defines=-D%mode% -D_EXPORT
set plat_includes=-I\src\ -I..\external\
set plat_flags_comp=-g -std=c++20 -shared -Wvarargs -Wall -Werror -Wno-deprecated -Wno-unused-function
set plat_flags_link=-luser32 -lgdi32 -lwinmm -lopengl32

echo %plat_assembly% compiling...
clang++ %plat_flags_comp% %plat_defines% platform_win32.cpp core.cpp -o %OUTDIR%\%plat_assembly%.dll %plat_includes% %plat_flags_link%

if %ERRORLEVEL% neq 0 (
    popd 
    echo Build failed with errors!
    exit /b %ERRORLEVEL%
)
echo Building %plat_assembly% complete
echo:


:: Build application
echo %assembly% compiling...
:: clang++ %compiler_flags% %app_flags% maintest.cpp %code_files% -o %outdir%\%assembly%.exe %defines% %includes% %linker_flags% -L%OUTDIR% -l%plat_assembly%.lib
clang++ %compiler_flags% %app_flags% main.cpp render_dx11.cpp %code_files% -o %outdir%\%assembly%.exe %defines% %includes% %linker_flags% -L%OUTDIR% -l%plat_assembly%.lib

popd 

if %ERRORLEVEL% neq 0 (
    echo Build failed with errors!
    exit /b %ERRORLEVEL%
)

echo Building %assembly% complete
