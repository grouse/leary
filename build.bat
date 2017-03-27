@ECHO off

SET ROOT=%~dp0
SET BUILD_DIR=%ROOT%\build

SET INCLUDE_DIR=-I%ROOT%\src -I%ROOT%
SET WARNINGS=/W4 /wd4996 /wd4577 /wd4065 /wd4800
SET DEFINITIONS=-DWIN32_LEAN_AND_MEAN -DNOMINMAX

SET FLAGS=%WARNINGS% %DEFINITIONS%
SET FLAGS=%FLAGS% /Zi /EHsc /permissive-

SET OPTIMIZED=/O2
SET UNOPTIMIZED=/Od

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %BUILD_DIR%\data mkdir %BUILD_DIR%\data
if not exist %BUILD_DIR%\data\shaders mkdir %BUILD_DIR%\data\shaders

SET INCLUDE_DIR=%INCLUDE_DIR% -I%VULKAN_LOCATION%\Include

SET LIBS=User32.lib Shlwapi.lib Shell32.lib %VULKAN_LOCATION%\Bin\vulkan-1.lib

PUSHD %BUILD_DIR%
REM TODO(jesper): move this into a separate build script; we won't need to and
REM don't want to rebuild all the assets every build, it'll become real slow
REM real fast
glslangValidator -V %ROOT%\src\render\shaders\generic.frag -o %BUILD_DIR%\data\shaders\generic.frag.spv
glslangValidator -V %ROOT%\src\render\shaders\generic.vert -o %BUILD_DIR%\data\shaders\generic.vert.spv

glslangValidator -V %ROOT%\src\render\shaders\font.frag -o %BUILD_DIR%\data\shaders\font.frag.spv
glslangValidator -V %ROOT%\src\render\shaders\font.vert -o %BUILD_DIR%\data\shaders\font.vert.spv

glslangValidator -V %ROOT%\src\render\shaders\mesh.frag -o %BUILD_DIR%\data\shaders\mesh.frag.spv
glslangValidator -V %ROOT%\src\render\shaders\mesh.vert -o %BUILD_DIR%\data\shaders\mesh.vert.spv

glslangValidator -V %ROOT%\src\render\shaders\terrain.frag -o %BUILD_DIR%\data\shaders\terrain.frag.spv
glslangValidator -V %ROOT%\src\render\shaders\terrain.vert -o %BUILD_DIR%\data\shaders\terrain.vert.spv

xcopy /i /y %ROOT%\assets\fonts %BUILD_DIR%\data\fonts
xcopy /i /y %ROOT%\assets\models %BUILD_DIR%\data\models

cl %FLAGS% %UNOPTIMIZED% %INCLUDE_DIR% %ROOT%\src\platform\win32_leary.cpp /link %LIBS% /DLL /OUT:game.dll
cl %FLAGS% %UNOPTIMIZED% %INCLUDE_DIR% /Feleary.exe %ROOT%\src\platform/win32_main.cpp /SUBSYSTEM:WINDOWS
POPD
