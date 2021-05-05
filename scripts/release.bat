@ECHO OFF

pushd %~dp0..\

REM CALL scripts\build.bat win64 dev

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

XCOPY %SVP%SilverEngine.exe %RP% /I /Q /Y > NUL
XCOPY SilverEngine\system %RP%system /E /I /Q /Y > NUL
XCOPY SilverEngine\*.dll %RP% /I /Q /Y > NUL

RMDIR %RP%int\ /Q /S > NUL

XCOPY SilverEngine\include %RP%include /E /I /Q /Y > NUL
XCOPY scripts\shell.bat %RP%system /I /Q /Y > NUL

XCOPY misc\SilverEditor.bat %RP% /I /Q /Y > NUL

popd
