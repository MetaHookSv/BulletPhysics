@echo off
setlocal
set "Configuration=Debug"
call "%~dp0build-BulletPhysics-x86.bat" %*
exit /b %errorlevel%
