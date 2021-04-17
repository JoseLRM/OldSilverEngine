@echo off
pushd %~dp0\..
XCOPY Game\src build\win64\gamecode\src /E /I /Q /Y > NUL
popd
