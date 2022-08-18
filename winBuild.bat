@echo off
setlocal

set my_name=%~n0
set my_dir="%~dp0"

set name=ResTractor

set /a exe=0
set /a cln=0

set /a bitness=64
set platform=x64
set /a debug=0
set /a release=0

set /a rtl=0
set /a dp=0
set /a pdb=0
set verbose=0

:: adjust this path, if you're using another version or path.
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\"
set pts=v142


:: default
if [%1]==[] goto main


GOTO :ParseParams

:ParseParams

    REM IF "%~1"=="" GOTO Main
    if [%1]==[/?] goto help
    if [%1]==[/h] goto help
    if [%1]==[/help] goto help

    IF /i "%~1"=="/exe" (
        SET /a exe=1
        goto reParseParams
    )
    IF /i "%~1"=="/cln" (
        SET /a cln=1
        goto reParseParams
    )

    IF /i "%~1"=="/b" (
        SET /a bitness=%~2
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
        SET buildTools=%~2
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/pts" (
        SET pts=%~2
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
        SET /a dp=%~2
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

    :: set platform
    set /a valid=0
    if %bitness% == 32 (
        set platform=x86
        set /a valid=1
    ) else (
        if %bitness% == 64 (
            set platform=x64
            set /a valid=1
        )
    )
    if %valid% == 0 (
        goto help
    )

    :: test valid targets
    set /a "valid=%exe%+%cln%"
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


    :: set runtime lib
    set rtlib=No
    if %debug% == 1 (
        if %rtl% == 1 (
            set rtlib=Debug
        )
        set /a pdb=1
    ) else (
        if %release% == 1 (
            if %rtl% == 1 (
                set rtlib=Release
            )
        )
    )
    
    :: verbose print
    if %verbose% == 1 (
        echo exe=%exe%
        echo bitness=%bitness%
        echo platform=%platform%
        echo debug=%debug%
        echo release=%release%
        echo buildTools=%buildTools%
        echo rtlib=%rtlib%
        echo pts=%pts%
        echo dp=%dp%
    )

    :: set vcvars, if necessary
    :: pseudo nop command to prevent if else bug in :build
    set vcvars=call
    if [%VisualStudioVersion%] EQU [] (
        if not exist %buildTools% (
            echo [e] No build tools found in %buildTools%!
            echo     Please set the correct path in this script or with the /bt option.
            exit /b -1
        )
        set vcvars="%buildTools:~1,-1%\VC\Auxiliary\Build\vcvars%bitness%.bat"
    )
    
    :: build targets
    if %cln% == 1 (
        rmdir /s /q build
    )
    if %exe% == 1 (
        call :build %name%.vcxproj Application
    ) 
    
    endlocal
    exit /b %errorlevel%


:build
    setlocal
        set proj=%1
        set ct=%2
        set conf=
        if %debug% EQU 1 (
            set conf=Debug
        ) else (
            if %release% EQU 1 set conf=Release
        )
        cmd /k "%vcvars% & msbuild %proj% /p:Platform=%platform% /p:PlatformToolset=%pts% /p:Configuration=%conf% /p:RuntimeLib=%rtlib% /p:PDB=%pdb% /p:ConfigurationType=%ct%  /p:DebugPrint=%dp%  & exit"

    endlocal
    exit /B %errorlevel%


:usage
    echo Usage: %my_name% [/exe] [/b ^<bitness^>] [/m ^<mode^>] [/rtl] [/dp ^<value^>] [/pdb] [/pts ^<toolset^>] [/bt ^<path^>] [/v] [/h]
    echo Default: %my_name% [/exe /b %bitness% /m %mode% /pts %pts% /bt %buildTools%]
    exit /B 0
    
:help
    call :usage
    echo.
    echo Targets:
    echo /exe Build ResTractor.exe application.
    echo.
    echo Options:
    echo /b Target bitness: 32^|64. Default: 64.
    echo /m Build mode: Debug^|Release. Default: Release.
    echo /rtl Statically include runtime libs. May be needed if a "VCRUNTIMExxx.dll not found Error" occurs on the target system.
    echo /pdb Include pdb symbols into release build. Default in debug mode. 
    echo /bt Custom path to Microsoft Visual Studio BuildTools.
    echo /pts Platformtoolset. Defaults to "v142".
    echo /dp Debug print value. 1: Debug print.
    echo.
    echo /v more verbose output
    echo /h print this
    
    endlocal
    exit /B 0