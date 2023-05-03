# ResTractor
Resource extractor tool for PE files.

This tool reads the resource table of a PE file and saves found resources to a directory.
The files will be named after their id and have a `.res` file type.
Some types are recognized and will be given a different type.
- AVI files are typed `.avi`
- Binary MOF Data files are typed `.bmf`
- Compound File Binary Format are typed as `.cbf`. Could possibly be .doc, .xls, .ppt, .msi, .msg.
- GIFs are typed as `.gif`.
- Icons are typed as `.ico`. Icons seem to be split in multiple parts though.
- MCSV files are typed `.mcsv`.
- PE files are typed `.pe`. Could possibly .exe, .sys, dll.
- PNG files are typed `.png`
- <STYLE> files are typed `.style`
- VS Version Info files are typed `.vsi`
- WAVE files are typed `.wav`
- Windows event tempate files are typed `.wevt`
- XML files are typed `.xml`
- Default type is `.res`

The source is deduced from the famous `HeaderParser`.
Some more cleaning may be done sometime.


POSIX compliant.  
Compiles and runs under
- Linux 
- Windows (x86/x64)  
- OsX may work too
- Android in Termux



## Version ##
1.0.6  
Last changed: 03.05.2023

## REQUIREMENTS ##
- Linux
   - Gcc
   - Building with cmake requires cmake.
- Windows
   - msbuild

## BUILD ##
### Linux (gcc) & cmake
```bash
$ ./linuxBuild.sh [-t exe] [-m Release|Debug] [-h]  
```

### Linux (gcc)
```bash
$ mkdir build
$ gcc -o build/resTractor -Wl,-z,relro,-z,now -D_FILE_OFFSET_BITS=64 -Ofast src/resTractor.c  
```

Use `clang` instead of `gcc` in Termux on Android.

### Windows (MsBuild) ###
```bash
$ winBuild.bat [/exe] [/m <Release|Debug>] [/b <32|64>] [/rtl] [/pdb] [/bt <path>] [/pts <PlatformToolset>] [/h]
```
This will run in a normal cmd.  

The correct path to your build tools may be passed  with the `/bt` parameter or changed in the script [winBuild.bat](winBuild.bat) itself.  

The PlatformToolset defaults to "v142", but may be changed with the `/pts` option.
"v142" is used for VS 2019, "v143" would be used in VS 2022.

In a developer cmd you can also type:
```bash
$devcmd> msbuild ResTractor.vcxproj /p:Configuration=<Release|Debug> /p:Platform=<x64|x86> [/p:PlatformToolset=<v142|v143>]
```

### Runtime Errors (Windows)
If a "VCRUNTIMExxx.dll not found Error" occurs on the target system, statically including runtime libs is a solution.  
This is done by using the `/p:RunTimeLib=Debug|Release` (msbuild) or `[/rtl]` (winBuild) flags.


## USAGE ##
```bash
$ ./ResTractor a/file/name [options]
$ ./ResTractor [options] a/file/name
```
Options:  
 * -h Print help.
 * -o:string Out directory, where the resource files will be saved.
 * -p Print the resource directory structure.
 
## EXAMPLE ##
```bash
$ ResTractor C:\Windows\System32\mspaint.exe -o %tmp%
```

#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 
