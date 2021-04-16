@echo off

pushd %~dp0\..
set origin_path=%cd%
popd

pushd %origin_path%

SET platform=win64

SET compile_entry=false
SET compile_engine=false

SET option_slow=false
SET option_dev=false
SET option_gfx=false

SET run=false

:arg_loop
IF "%1"=="" GOTO end_arg_loop

IF "%1"=="win64" SET platform=win64

IF "%1"=="engine" SET compile_engine=true
IF "%1"=="entry" SET compile_entry=true

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

IF "%compile_entry%"=="false" (
   IF "%compile_engine%"=="false" (
      	 SET compile_entry=true
   	 SET compile_engine=true
      )
)

SET common_compiler_flags= -MT -nologo -EHa- -GR- -Oi -WX -W4 -wd4127 -wd4211 -wd4238 -wd4459 -wd4996 -wd4456 -wd4281 -wd4100 -wd4530 -FC /std:c++14

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
   SET common_compiler_flags= %common_compiler_flags% -Z7
   SET output_dir=%output_dir%_debug
) ELSE (
   SET common_defines=%common_defines% -DSV_SLOW=0 -DNDEBUG
   SET common_compiler_flags= %common_compiler_flags% -GL
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

REM Silver Engine args
SET sv_defines= -DSV_SILVER_ENGINE=1 %common_defines%
SET sv_compiler_flags= %common_compiler_flags%
SET sv_include_paths= /I %SVP%include /I %SVP%src\ /I %SVP%src\external\ /I %VULKAN_SDK%\Include\
SET sv_link_libs= user32.lib %VULKAN_SDK%\Lib\vulkan-1.lib assimp.lib sprv.lib
SET sv_link_flags= /DLL %common_linker_flags% /LIBPATH:"%origin_path%\SilverEngine\lib\" /out:..\SilverEngine.dll /PDB:SilverEngine.pdb /LTCG

pushd %output_dir%int

IF "%compile_entry%"=="true" (
   ECHO.
   ECHO.
   ECHO.
   ECHO -- Compiling entry point! --
   CALL cl %common_compiler_flags% %SVP%src\entrypoint\win64_entrypoint.cpp /link %common_linker_flags% user32.lib /out:..\EntryPoint.exe /PDB:EntryPoint.pdb
)

IF "%compile_engine%"=="true" (
   ECHO.
   ECHO.
   ECHO.
   ECHO -- Compiling SilverEngine! --
   CALL cl %sv_compiler_flags% %sv_defines% %sv_include_paths% %SVP%src\build_unit.cpp /link %sv_link_flags% %sv_link_libs% 
)

ECHO.
ECHO.

popd

pushd %output_dir%

SET SVP= %origin_path%SilverEngine\
SET GP= %origin_path%Game\
 
IF EXIST EntryPoint.exe (

   IF EXIST SilverEngine.exe DEL SilverEngine.exe -Q
   RENAME EntryPoint.exe SilverEngine.exe
      
)
IF EXIST  SilverEngine.exp DEL SilverEngine.exp -Q


ECHO Creating system data

IF EXIST system (
   RMDIR system /Q /S
)
XCOPY %SVP%system\ system /E /I /Q /Y > NUL

IF NOT EXIST bin MKDIR bin

ECHO Moving DLLs

IF EXIST SilverEngine.dll RENAME SilverEngine.dll SilverEngineTemp
IF EXIST Game.dll RENAME Game.dll GameTemp
DEL *.dll > NUL 2> NUL
IF EXIST SilverEngineTemp RENAME SilverEngineTemp SilverEngine.dll
IF EXIST GameTemp RENAME GameTemp Game.dll
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

CALL system/build_game.bat

IF "%run%"=="true" SilverEngine.exe

popd

popd
