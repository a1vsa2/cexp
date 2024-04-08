@echo off


setlocal
set exedir=bin
:run
if "%~1" neq "" (%exedir%\%1.exe)
goto end


:: ======================= discard
setlocal


pushd bin
set exename=%1
call :getArgsAfter 2 "%*"

if "%objnames%"=="" set objnames=default.o
if "%exename%"=="" set exename=default.exe

set objs=
setlocal enabledelayedexpansion 
for /f  %%i in ('dir /b /s %objnames%') do (
    set objs= !objs! %%i
)
endlocal disabledelayedexpansion && set objs=%objs%

popd

if "%objs%" neq "" (make exe exename="%exename%" ts="%objs%")




:getArgsAfter
:: %~1：删除参数两侧的引号（如果有的话）
setlocal enabledelayedexpansion
set /a idx=0
for %%i in (%~2) do (
 set /a idx+=1
 if !idx! gtr 1 (
    set args=!args! %%i
 )
)
endlocal disabledelayedexpansion && set objnames=%args%
exit /b


:end
endlocal