@echo off

pushd %~dp0..\
set origin_path=%cd%\
popd

pushd %~dp1
set GP=%cd%\
popd

pushd %origin_path%..\scripts
IF "%2"=="shell" CALL shell.bat
popd

pushd %GP%

SET compiler_flags= -MT -nologo -EHa- -GR- -Oi -WX -W4 -wd4201 -wd4127 -wd4211 -wd4238 -wd4459 -wd4996 -wd4456 -wd4281 -wd4100 -wd4530 -FC /std:c++14 -Z7

SET defines= -DSV_PLATFORM_WIN=1 -DSV_SLOW=0 -DSV_EDITOR=1 -DSV_GFX=0

SET linker_flags= /incremental:no
 
SET output_dir=%GP%

IF NOT EXIST %output_dir%int\ MKDIR %output_dir%int\

SET include_paths= /I %origin_path%include\
SET link_libs= SilverEngine.lib
SET link_flags= /DLL %linker_flags% /LIBPATH:"%origin_path%system\bin\"

pushd %output_dir%\int\

CALL cl %compiler_flags% %defines% %include_paths% %GP%src\build_unit.cpp /link %link_flags% /DLL %link_libs% /out:..\Game.dll /PDB:Game.pdb > build_output.txt

popd
popd
