@ECHO OFF

pushd %~dp0..\

CALL scripts\build.bat win64 dev

ECHO.
ECHO.

SET SVP=build\win64\
SET RP=release\win64_dev\

IF NOT EXIST %SVP% (
   ECHO Build folder not found!
   EXIT
)

IF NOT EXIST release (
   MKDIR release\win64_dev
) ELSE (
   IF EXIST release\win64_dev RMDIR release\win64_dev /Q /S
   MKDIR release\win64_dev
)

IF EXIST %SVP%SilverEngine.exe XCOPY %SVP%SilverEngine.exe %RP% /I /Q /Y > NUL
XCOPY %SVP%*.dll %RP% /I /Q /Y > NUL
IF EXIST %RP%Game.dll DEL %RP%Game.dll
XCOPY %SVP%system\ %RP%system /E /I /Q /Y > NUL
XCOPY %SVP%SilverEngine.lib %RP% /I /Q /Y > NUL
XCOPY %SVP%LICENSE %RP% /I /Q /Y > NUL


popd
