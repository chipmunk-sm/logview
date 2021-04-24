
@echo off
@echo.
@echo ******************************************************
@echo *** Setup environment ***
@echo ******************************************************
@echo.


if [%QTDIR%] == [] set QTDIR=D:\Qt\5.15.2\msvc2019_64

if %QTDIR:_64=%==%QTDIR% (set BUILD_PLATFORMID=x86) else set BUILD_PLATFORMID=x64
@echo BUILD_PLATFORMID=%BUILD_PLATFORMID%

powershell.exe -noprofile -executionpolicy bypass -file version.ps1 -platformId %BUILD_PLATFORMID%
if %ERRORLEVEL% NEQ 0 GOTO error

if [%APPVEYOR_BUILD_FOLDER%] == [] set APPVEYOR_BUILD_FOLDER=%cd%

if [%CONFIGURATION%] == [] set CONFIGURATION=Release

@echo CONFIGURATION = %CONFIGURATION%
@echo APPVEYOR_BUILD_FOLDER = %APPVEYOR_BUILD_FOLDER%
@echo QTDIR = %QTDIR%

@echo.
@echo ******************************************************
@echo *** Remove temporary folders
@echo ******************************************************
@echo.

rmdir /Q /S "%APPVEYOR_BUILD_FOLDER%-build"
rmdir /Q /S "%APPVEYOR_BUILD_FOLDER%\Installer\logview"
mkdir "%APPVEYOR_BUILD_FOLDER%-build"

@echo.
@echo ******************************************************
@echo *** Setup 7z ***
@echo ******************************************************
@echo.

where 7z.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 set PATH=%PATH%;C:\Program Files\7-Zip

@echo PATH=%PATH%

where 7z.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    @echo 7z.exe not found in path!
    GOTO error
)

@echo.
@echo ******************************************************
@echo *** Set Qt environment
@echo ******************************************************
@echo.

call "%QTDIR%\bin\qtenv2.bat"
if %ERRORLEVEL% NEQ 0 GOTO error

@echo.
qmake -v
if %ERRORLEVEL% NEQ 0 GOTO error

@echo.
@echo ******************************************************
@echo *** Test for VS or mingw
@echo ******************************************************
@echo.

if %QTDIR:msvc=%==%QTDIR% g++ --version
if %QTDIR:msvc=%==%QTDIR% set make=mingw32-make.exe
if %QTDIR:msvc=%==%QTDIR% %make% --version

if %QTDIR:msvc2015=%==%QTDIR% goto skip14
if EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %BUILD_PLATFORMID%
if EXIST "D:\MVS2015\VC\vcvarsall.bat" call "D:\MVS2015\VC\vcvarsall.bat" %BUILD_PLATFORMID%
:skip14

if %QTDIR:msvc2017=%==%QTDIR% goto skip15
if EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" %BUILD_PLATFORMID%
if EXIST "D:\MVS2017\VC\Auxiliary\Build\vcvarsall.bat" call "D:\MVS2017\VC\Auxiliary\Build\vcvarsall.bat" %BUILD_PLATFORMID%
:skip15

if %QTDIR:msvc2019=%==%QTDIR% goto skip16
if EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" %BUILD_PLATFORMID%
if EXIST "D:\MVS2019\VC\Auxiliary\Build\vcvarsall.bat" call "D:\MVS2019\VC\Auxiliary\Build\vcvarsall.bat" %BUILD_PLATFORMID%
:skip16


if not %QTDIR:msvc=%==%QTDIR% set make=nmake.exe
if not %QTDIR:msvc=%==%QTDIR% %make% /? > nul
if %ERRORLEVEL% NEQ 0 GOTO error

@echo. 
@echo ******************************************************
@echo *** Start build                                    ***
@echo ******************************************************
@echo.

qmake -o "%APPVEYOR_BUILD_FOLDER%-build" -r -Wall -Wlogic -Wparser CONFIG+=%CONFIGURATION% %APPVEYOR_BUILD_FOLDER%
if %ERRORLEVEL% NEQ 0 GOTO error

cd "%APPVEYOR_BUILD_FOLDER%-build"

%make%
if %ERRORLEVEL% NEQ 0 GOTO error

@echo. 
@echo ******************************************************
@echo *** Collect files to "%APPVEYOR_BUILD_FOLDER%\Installer\logview\"
@echo ******************************************************
@echo.

cd "%APPVEYOR_BUILD_FOLDER%"

windeployqt "%APPVEYOR_BUILD_FOLDER%-build\%CONFIGURATION%\logview.exe" --dir Installer\logview
if %ERRORLEVEL% NEQ 0 GOTO error

copy "%APPVEYOR_BUILD_FOLDER%-build\%CONFIGURATION%\logview.exe" "%APPVEYOR_BUILD_FOLDER%\Installer\logview\"
if %ERRORLEVEL% NEQ 0 GOTO error

copy "%APPVEYOR_BUILD_FOLDER%\data\*.*" "%APPVEYOR_BUILD_FOLDER%\Installer\logview\"
if %ERRORLEVEL% NEQ 0 GOTO error

copy "%APPVEYOR_BUILD_FOLDER%\releaseNote.txt" "%APPVEYOR_BUILD_FOLDER%\Installer\logview\"
if %ERRORLEVEL% NEQ 0 GOTO error

powershell.exe -noprofile -executionpolicy bypass -file "%APPVEYOR_BUILD_FOLDER%\installer.ps1" -platformId %BUILD_PLATFORMID% -cultureId "en-us"
if %ERRORLEVEL% NEQ 0 GOTO error

cd "%APPVEYOR_BUILD_FOLDER%\Installer"

@echo. 
@echo ******************************************************
@echo *** Create archive ***
@echo ******************************************************
@echo.

7z.exe a "%APPVEYOR_BUILD_FOLDER%\logview.%WINVER%.%BUILD_PLATFORMID%.%APPVEYOR_BUILD_VERSION%.zip" "logview\*"
if %ERRORLEVEL% NEQ 0 GOTO error

7z.exe a "%APPVEYOR_BUILD_FOLDER%\logview.%WINVER%.%BUILD_PLATFORMID%.%APPVEYOR_BUILD_VERSION%.Installer.zip" logview.msi
if %ERRORLEVEL% NEQ 0 GOTO error
  
goto exit

:error
    @echo Failed!.
	cd "%APPVEYOR_BUILD_FOLDER%"
rem pause
        exit 1
:exit  
cd "%APPVEYOR_BUILD_FOLDER%"
rem pause
