@echo off

pushd %~dp0\..
set origin_path=%cd%
popd

pushd %origin_path%

SET platform=win64

SET module_platform=false
SET module_core=false
SET module_debug=false

SET option_slow=false
SET option_dev=false
SET option_gfx=false

SET run=false

:arg_loop
IF "%1"=="" GOTO end_arg_loop

IF "%1"=="win64" SET platform=win64

IF "%1"=="platform" SET module_platform=true
IF "%1"=="core" SET module_core=true
IF "%1"=="debug" SET module_debug=true

IF "%1"=="slow" SET option_slow=true
IF "%1"=="dev" SET option_dev=true
IF "%1"=="gfx" SET option_gfx=true
IF "%1"=="all" (
   SET option_slow=true
   SET option_dev=true
   SET option_gfx=true
)

IF "%1"=="run" SET run=true

SHIFT
GOTO arg_loop
:end_arg_loop

IF "%module_platform%"=="false" (
IF "%module_core%"=="false" (
IF "%module_debug%"=="false" (
   SET module_platform=true
   SET module_core=true
   SET module_debug=true
)
)
)

SET common_compiler_flags= -MP -nologo -EHa- -GR- -Oi -WX -W4 -wd4201 -wd4127 -wd4211 -wd4238 -wd4459 -wd4996 -wd4456 -wd4281 -wd4100 -wd4530 -FC /std:c++14

SET common_defines=

SET output_dir=%origin_path%build\

IF "%platform%"=="win64" (
   SET common_defines=%common_defines% -DSV_PLATFORM_WIN=1
   SET output_dir=%output_dir%win64
) ELSE (
  SET common_defines=%common_defines% -DSV_PLATFORM_WIN=0
)

IF "%option_slow%"=="true" (
   SET common_defines=%common_defines% -DSV_SLOW=1
   SET common_compiler_flags= %common_compiler_flags% -Z7 -Od
) ELSE (
   SET common_defines=%common_defines% -DSV_SLOW=0 -DNDEBUG
   SET common_compiler_flags= %common_compiler_flags% -GL -Ox
)

IF "%option_dev%"=="true" (
   SET common_defines=%common_defines% -DSV_DEV=1
) ELSE (
   SET common_defines=%common_defines% -DSV_DEV=0
)

IF "%option_gfx%"=="true" (
   SET common_defines=%common_defines% -DSV_GFX=1
) ELSE (
   SET common_defines=%common_defines% -DSV_GFX=0
)


SET output_dir=%output_dir%\

ECHO.

ECHO Output directory: %output_dir%

IF NOT EXIST %output_dir%\int mkdir %output_dir%\int

cls


SET common_linker_flags= /incremental:no

REM Silver Engine path
SET SVP= %origin_path%\SilverEngine\

SET sv_defines= -DSV_SILVER_ENGINE=1 %common_defines%
SET sv_compiler_flags= %common_compiler_flags%
SET sv_include_paths= /I %SVP%include /I %SVP%src\ /I %SVP%src\external\ /I %VULKAN_SDK%\Include\
SET sv_link_libs= user32.lib %VULKAN_SDK%\Lib\vulkan-1.lib assimp.lib sprv.lib
SET sv_link_flags= %common_linker_flags% /LIBPATH:"%origin_path%\SilverEngine\lib\" /out:..\SilverEngine.exe /PDB:SilverEngine.pdb /LTCG
SET sv_build_units=

IF "%module_platform%"=="true" (
   SET sv_build_units=%sv_build_units% %SVP%src\platform_unit.cpp
) ELSE (
   SET sv_link_libs=%sv_link_libs% platform_unit.obj
)

IF "%module_core%"=="true" (
   SET sv_build_units=%sv_build_units% %SVP%src\core_unit.cpp
) ELSE (
   SET sv_link_libs=%sv_link_libs% core_unit.obj
)

IF "%module_debug%"=="true" (
   SET sv_build_units=%sv_build_units% %SVP%src\debug_unit.cpp
) ELSE (
   SET sv_link_libs=%sv_link_libs% debug_unit.obj
)

pushd %output_dir%int

ECHO.
ECHO.
ECHO.
ECHO -- Compiling SilverEngine! --
CALL cl %sv_compiler_flags% %sv_defines% %sv_include_paths% %sv_build_units% /link %sv_link_flags% %sv_link_libs% 

ECHO.
ECHO.

popd

pushd %output_dir%

SET SVP= %origin_path%SilverEngine\
SET GP= %origin_path%Game\
 
IF EXIST  SilverEngine.exp DEL SilverEngine.exp -Q

ECHO Creating system data

IF EXIST system (
   RMDIR system /Q /S
)
XCOPY %SVP%system\ system /E /I /Q /Y > NUL

IF NOT EXIST bin MKDIR bin

ECHO Moving DLLs

DEL *.dll > NUL 2> NUL
XCOPY %SVP%lib\*.dll /I /Q /Y > NUL

IF EXIST  Game.exp DEL Game.exp -Q
IF EXIST  Game.lib DEL Game.lib -Q

ECHO Creating assets

IF EXIST assets\ RMDIR assets\ /S /Q
     
XCOPY %GP%assets\ assets /E /I /Q /Y > NUL

IF EXIST gamecode RMDIR gamecode /S /Q

MKDIR gamecode\lib
MKDIR gamecode\src
MKDIR gamecode\SilverEngine

XCOPY %SVP%\include gamecode\SilverEngine /E /I /Q /Y > NUL
XCOPY %GP%\src gamecode\src /E /I /Q /Y > NUL

IF EXIST SilverEngine.lib (
   XCOPY SilverEngine.lib gamecode\lib /I /Q /Y > NUL
   DEL SilverEngine.lib
)

XCOPY %origin_path%scripts\shell.bat system\ /I /Q /Y > NUL

CALL system/build_game.bat

IF "%run%"=="true" SilverEngine.exe

popd

popd
