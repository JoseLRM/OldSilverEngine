@echo off

pushd %~dp0\..
set origin_path=%cd%
popd

pushd %origin_path%

SET compiler_flags= -MT -nologo -EHa- -GR- -Oi -WX -W4 -wd4201 -wd4127 -wd4211 -wd4238 -wd4459 -wd4996 -wd4456 -wd4281 -wd4100 -wd4530 -FC /std:c++14 -Z7

SET defines= -DSV_PLATFORM_WIN=1 -DSV_SLOW=0 -DSV_DEV=1 -DSV_GFX=0

SET linker_flags= /incremental:no
 
SET output_dir=%origin_path%\
IF NOT EXIST %output_dir%game_int\ MKDIR %output_dir%game_int\

ECHO.

ECHO Output directory: %output_dir%

cls

REM Game path
SET GP= %origin_path%\gamecode\

SET include_paths= /I %GP%SilverEngine\
SET link_libs= SilverEngine.lib
SET link_flags= /DLL %linker_flags% /LIBPATH:"%origin_path%\gamecode\lib\"

pushd %output_dir%\game_int\

ECHO.
ECHO.
ECHO.
ECHO -- Compiling Game! --
CALL cl %compiler_flags% %defines% %include_paths% %GP%src\build_unit.cpp /link %link_flags% /DLL %link_libs% /out:..\system\game_bin\Game.dll /PDB:Game.pdb
)

ECHO.
ECHO.

popd
popd
