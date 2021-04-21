@ECHO OFF

pushd %~dp0..\

CALL scripts\build.bat win64 dev

ECHO.
ECHO.

SET SVP=build\win64\
SET RP=release\win64\

IF NOT EXIST %SVP% (
   ECHO Build folder not found!
   EXIT
)

IF NOT EXIST release (
   MKDIR release\win64
) ELSE (
   IF EXIST release\win64 RMDIR release\win64 /Q /S
   MKDIR release\win64
)

XCOPY %SVP% %RP% /E /I /Q /Y > NUL

RMDIR %RP%int\ /Q /S > NUL

RMDIR %RP%assets\ /Q /S > NUL
MKDIR %RP%assets\ > NUL

RMDIR %RP%bin\ /Q /S > NUL
MKDIR %RP%bin\ > NUL

DEL %RP%gamecode\src\* /Q /S > NUL

XCOPY Game\default_code.cpp %RP%gamecode\src\ /I /Q /Y > NUL
RENAME %RP%gamecode\src\default_code.cpp build_unit.cpp

CALL %RP%system\build_game.bat

popd
