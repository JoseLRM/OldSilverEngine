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
SET option_editor=false
SET option_gfx=false

SET run=false

:arg_loop
IF "%1"=="" GOTO end_arg_loop

IF "%1"=="win64" SET platform=win64

IF "%1"=="platform" SET module_platform=true
IF "%1"=="core" SET module_core=true
IF "%1"=="debug" SET module_debug=true

IF "%1"=="slow" SET option_slow=true
IF "%1"=="editor" SET option_editor=true
IF "%1"=="gfx" SET option_gfx=true
IF "%1"=="all" (
   SET option_slow=true
   SET option_editor=true
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

SET common_compiler_flags= -MP -nologo -EHa- -GR- -Oi -WX -W4 -wd4390 -wd4201 -wd4127 -wd4211 -wd4238 -wd4459 -wd4996 -wd4456 -wd4281 -wd4100 -wd4530 -FC /std:c++14

SET common_defines=
SET physx_option=
SET lib_option=

SET output_dir=%origin_path%build\

IF "%platform%"=="win64" (
   SET common_defines=%common_defines% -DSV_PLATFORM_WIN=1
   SET output_dir=%output_dir%win64
) ELSE (
  SET common_defines=%common_defines% -DSV_PLATFORM_WIN=0
)

IF "%option_slow%"=="true" (
   SET common_defines=%common_defines% -DSV_SLOW=1 -DNDEBUG
   SET common_compiler_flags= %common_compiler_flags% -Z7 -Od
   SET physx_option=checked
   SET lib_option=debug
) ELSE (
   SET common_defines=%common_defines% -DSV_SLOW=0 -DNDEBUG
   SET common_compiler_flags= %common_compiler_flags% -GL -Ox
   SET physx_option=release
   SET lib_option=release
)

IF "%option_editor%"=="true" (
   SET common_defines=%common_defines% -DSV_EDITOR=1
) ELSE (
   SET common_defines=%common_defines% -DSV_EDITOR=0
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
SET sv_include_paths= /I %SVP%include /I %SVP%src\ /I %SVP%system\shaders\ /I %SVP%src\external\ /I %VULKAN_SDK%\Include\ /I C:\physx\physx\include\ /I C:\physx\pxshared\include\
SET sv_link_libs= user32.lib Comdlg32.lib Shell32.lib %VULKAN_SDK%\Lib\vulkan-1.lib sprv.lib PhysXCommon_64.lib PhysXFoundation_64.lib PhysX_64.lib PhysXExtensions_static_64.lib
SET sv_link_flags= %common_linker_flags% /LIBPATH:"C:\physx\physx\bin\win.x86_64.vc142.mt\%physx_option%\" /LIBPATH:"%origin_path%\SilverEngine\lib\" /out:..\SilverEngine.exe /PDB:SilverEngine.pdb /LTCG
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
popd

pushd %origin_path%

XCOPY %output_dir%SilverEngine.lib SilverEngine\system\bin /I /Q /Y > NUL
XCOPY %output_dir%SilverEngine.exe SilverEngine\ /I /Q /Y > NUL

pushd SilverEngine

XCOPY lib\%lib_option%\*.dll . /I /Q /Y > NUL

IF "%run%"=="true" CALL SilverEngine.exe
popd
popd
