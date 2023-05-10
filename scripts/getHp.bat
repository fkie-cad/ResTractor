:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: clone headerParser                                              ::
:: compile it                                                      ::
:: and place a copy in res\lib\headerParser.lib                    ::
::                                                                 ::
:: vs: 1.0.0                                                       ::
:: changed: 10.05.2023                                             ::
::                                                                 ::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
@echo off 
setlocal enabledelayedexpansion

for /F %%a in ('echo prompt $E ^| cmd') do (
  set "ESC=%%a"
)

set my_name=%~n0
set my_dir="%~dp0"
set "my_dir=%my_dir:~1,-2%"

CALL :normalizePath "%my_dir%\.."
SET "root_dir=%RETVAL%"
:: FOR %%A IN ("%~dp0.") DO SET root_dir=%%~dpA
:: echo root_dir: %root_dir%


set "inc_dir=%root_dir%\res\inc"
set "lib_dir=%root_dir%\res\lib"
set "tmp_dir=%root_dir%\tmp"
set repo_url=https://github.com/fkie-cad/headerParser.git
set repo_name=headerParser
set bin_name=headerParser
set bin_ext=.lib
set pdb_ext=.pdb
set build_path=build
set build_cmd=winbuild

set /a DP_FLAG=1
set /a EP_FLAG=2

set /a dp=%EP_FLAG%

set /a lib=1
set /a cln=0

set /a bitness=64
set platform=x64

set /a debug=0
set /a release=0

set /a rtl=0
set /a pdb=0

set verbose=0

:: adjust this path, if you're using another version or path.
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"
set pts=v142

:: default
if [%1]==[] goto main

:ParseParams

    REM IF "%~1"=="" GOTO Main
    if [%1]==[/?] goto help
    if [%1]==[/h] goto help
    if [%1]==[/help] goto help
    
    IF /i "%~1"=="/cln" (
        SET /a cln=1
        goto reParseParams
    )

    IF /i "%~1"=="/b" (
        SET /a bitness="%~2"
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/d" (
        SET debug=1
        goto reParseParams
    )
    IF /i "%~1"=="/r" (
        SET release=1
        goto reParseParams
    )
    IF /i "%~1"=="/bt" (
        SET buildTools="%~2"
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/pts" (
        SET pts="%~2"
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/url" (
        SET repo_url="%~2"
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/rtl" (
        SET /a rtl=1
        goto reParseParams
    )
    IF /i "%~1"=="/pdb" (
        SET /a pdb=1
        goto reParseParams
    )
    IF /i "%~1"=="/dp" (
        SET /a dp="%~2"
        SHIFT
        goto reParseParams
    )
    
    IF /i "%~1"=="/v" (
        SET verbose=1
        goto reParseParams
    ) ELSE (
        echo Unknown option : "%~1"
    )
    
    :reParseParams
    SHIFT
    if [%1]==[] goto main

GOTO :ParseParams



:main

    if %verbose% == 1 (
        echo lib_dir: %lib_dir%
        echo tmp_dir: %tmp_dir%
        echo root_dir: %root_dir%
        echo repo_url: %repo_url%
        echo repo_name: %repo_name%
        echo bitness=%bitness%
        echo platform=%platform%
        echo debug=%debug%
        echo release=%release%
        echo buildTools=%buildTools%
        echo rtlib=%rtlib%
        echo pts=%pts%
        echo dp=%dp%
    )

    :: test valid targets
    set /a "valid=%lib%+%cln%"
    if %valid% == 0 (
        set /a exe=1
    )

    set /a "valid=%debug%+%release%"
    if %valid% == 0 (
        set /a release=1
    )
    if %valid% == 2 (
        set /a debug=0
        set /a release=1
    )

    if %cln% == 1 (
        echo removing "%tmp_dir%"
        rmdir /s /q "%tmp_dir%" >nul 2>&1 
        echo removing "%lib_dir%\%bin_name%%bin_ext%"
        del "%lib_dir%\%bin_name%%bin_ext%" >nul 2>&1
    )
    
    if %lib% == 1 (
        call :build
    )
    
    :clean
        set /a error_code=%errorlevel%
        echo Cleaning up...
        cd "%root_dir%"
        rmdir /s /q "%tmp_dir%" 
        echo done

    :end
        echo finished with code %error_code%
        endlocal
        exit /b %error_code%


:normalizePath
    SET RETVAL=%~f1
    EXIT /B %errorlevel%


:build
setlocal
    mkdir "%tmp_dir%" >nul 2>&1 
    if not %errorlevel% EQU 0 (
    if not %errorlevel% EQU 1 (
        echo [e] making %tmp_dir% failed! ^(%errorlevel%^)
        goto build_end
    ))

    cd "%tmp_dir%"
    if not %errorlevel% EQU 0 (
        echo [e] changing diretory failed! ^(%errorlevel%^)
        goto build_end
    )

    if not exist %repo_name% (
        git clone %repo_url%
        if not !errorlevel! EQU 0 (
            echo [e] cloning %repo_url% failed!
            goto build_end
        )
    ) else (
        git pull %repo_url%
        if not !errorlevel! EQU 0 (
            echo [e] pulling %repo_url% failed!
            goto build_end
        )
    )

    cd %repo_name%
    if not %errorlevel% EQU 0 (
        echo [e] changing diretory failed!
        goto build_end
    )
    
    if %release% == 1 ( set r=/r ) else ( set r= )
    if %debug% == 1 ( set d=/d ) else ( set d= )
    if %verbose% == 1 ( set v=/v ) else ( set v= )
    if %cln% == 1 ( set cln=/cln ) else ( set cln= )
    if %pdb% == 1 ( set pdb=/pdb ) else ( set pdb= )
    if %rtl% == 1 ( set rtl=/rtl ) else ( set rtl= )
    
    cmd /k "%build_cmd% /dp %dp% /lib !r! !d! !v! !pdb! !rtl! !cln! /b %bitness% /bt "%buildTools:~1,-1%" /pts %pts% & exit"
    if not %errorlevel% EQU 0 (
        echo [e] building %bin_name%%bin_ext% failed!
        goto build_end
    )
    
    mkdir "%lib_dir%" >nul 2>&1 
    if not %errorlevel% EQU 0 (
    if not %errorlevel% EQU 1 (
        echo [e] making %lib_dir% failed! ^(%errorlevel%^)
        goto build_end
    ))

    if %release% == 1 (
        set "build_path=%build_path%\%bitness%"
    ) else (
        set "build_path=%build_path%\debug\%bitness%"
    )
    echo build_path = %build_path%
    copy /y "%build_path%\%bin_name%%bin_ext%" "%lib_dir%"
    if not %errorlevel% EQU 0 (
        echo [e] copying %bin_name%%bin_ext% failed! ^(%errorlevel%^)
        goto build_end
    )
    if %debug% == 1 (
        set "rt_build_dir=%root_dir%\build\debug\%bitness%"
        mkdir "!rt_build_dir!" >nul 2>&1 
        if not %errorlevel% EQU 0 (
        if not %errorlevel% EQU 1 (
            echo [e] making %lib_dir% failed! ^(%errorlevel%^)
            goto build_end
        ))
        copy /y "%build_path%\%bin_name%%pdb_ext%" "!rt_build_dir!"
    )
    
    mkdir "%inc_dir%\pe" >nul 2>&1 
    if not %errorlevel% EQU 0 (
    if not %errorlevel% EQU 1 (
        echo [e] making "%lib_dir%\pe" failed! ^(%errorlevel%^)
        goto build_end
    ))
    set files=(exp.h Globals.h HeaderData.h headerParserLib.h headerParserLibPE.h PEHeaderData.h pe\PEHeader.h pe\PEHeaderOffsets.h)
    set src_dir=%tmp_dir%\%repo_name%\src
    for /d %%f in %files% do (
        copy "%src_dir%\%%f" "%inc_dir%\%%f" >nul 2>&1 
        if not !errorlevel! EQU 0 (
            echo [e] copying %%f failed!
            goto build_end
        )
    )
    
    :build_end
    endlocal
    exit /B !errorlevel!

:usage
    echo Usage: %my_name% [/b ^<bitness^>] [/m ^<mode^>] [/rtl] [/dp ^<value^>] [/pdb] [/pts ^<toolset^>] [/bt ^<path^>] [/v] [/h]
    echo Default: %my_name% [/b %bitness% /m %mode% /pts %pts% /bt %buildTools%]
    exit /B 0
    
:help
    call :usage
    echo.
    echo Options:
    echo /b Target bitness: 32^|64. Default: 64.
    echo /m Build mode: Debug^|Release. Default: Release.
    echo /rtl Statically include runtime libs. May be needed if a "VCRUNTIMExxx.dll not found Error" occurs on the target system.
    echo /pdb Include pdb symbols into release build. Default in debug mode. 
    echo /bt Custom path to Microsoft Visual Studio BuildTools.
    echo /dp Debug print value. 1: Debug print flag, 2: Error print flag.
    echo /pts Platformtoolset. Defaults to "v142".
    echo /url Custom url to headerParser repo.
    echo.
    echo /v more verbose output
    echo /h print this
    
    endlocal
    exit /B 0
