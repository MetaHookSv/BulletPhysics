@echo off
setlocal
set "Configuration=Release"
call "%~dp0build-BulletPhysics-x86.bat" %*
exit /b %errorlevel%
