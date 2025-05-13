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
- \<STYLE\> files are typed `.style`
- VS Version Info files are typed `.vsi`
- WAVE files are typed `.wav`
- Windows event tempate files are typed `.wevt`
- XML files are typed `.xml`
- Default type is `.res`

It uses the famous `HeaderParser` included as a static lib.


POSIX compliant.  
Compiles and runs under
- Linux 
- Windows (x86/x64)  
- OsX may work too
- Android in Termux



## Version ##
1.1.3  
Last changed: 13.05.2025

## REQUIREMENTS ##
- Linux
   - Gcc
   - Building with cmake requires cmake.
   - a static libheaderparser.a in `res\lib\libheaderparser.a`
- Windows
   - msbuild
   - a static headerParser.lib in `res\lib\headerParser.lib`

## BUILD ##
### Linux (gcc) & cmake
```bash
$ ./linuxBuild.sh [-t app] [-m Release|Debug] [-h]  
```
This will build `libheaderparser.a` automatically, if not present.
The cloning process requires an active internet connection, though.

### Linux (gcc)
Compile a static headerparser library and put it into `res/lib/libheaderparser.a` and make sure the `res\inc` files are present and up to date.
```bash
$ copy src/print.h res/inc
$ mkdir build
$ gcc -o build/resTractor -Wl,-z,relro,-z,now -D_FILE_OFFSET_BITS=64 -Ofast src/main.c src/utils/fifo/Fifo.c res/lib/libheaderparser.a -Ires/inc
```

Use `clang` instead of `gcc` in Termux on Android.

### Windows (MsBuild) ###
```bash
$ winBuild.bat [/exe] [/m <Release|Debug>] [/b <32|64>] [/rtl] [/pdb] [/bt <path>] [/pts <PlatformToolset>] [/h]
```
This will run in a normal cmd.  
It will build `headerParser.lib` automatically, if not present.
The cloning process requires an active internet connection, though.

The correct path to your build tools may be passed  with the `/bt` parameter or changed in the script [winBuild.bat](winBuild.bat) itself.  

The PlatformToolset defaults to "v142", but may be changed with the `/pts` option.
"v142" is used for VS 2019, "v143" would be used in VS 2022.

In a developer cmd you can also type:
```bash
$devcmd> msbuild ResTractor.vcxproj /p:Configuration=<Release|Debug> /p:Platform=<x64|x86> [/p:PlatformToolset=<v142|v143>]
```
But you have to build a `res/lib/headerParser.lib` on your own.

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
Saves all found resources of `mspaint.exe` into `%tmp%`.

#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](mailto:henning.braun@fkie.fraunhofer.de)) 
