@echo off

pushd %~dp0

CALL system\shell.bat
CLS
CALL SilverEngine.exe

popd
