# ResTractor
Resource extractor tool for PE files.

This tool reads the resource table of a PE file and saves found resources to a directory.
The files will be named after their id and have a `.res` file type.
Some types are recognized and will be given a different type.
- PE files are typed `.pe`. 
- XML files are typed `.xml`
- VS Version Info files are typed `.vsi`

The source is deduced from the famous `HeaderParser`.
Some cleaning may be done sometime.


POSIX compliant.  
Compiles and runs under
- Linux 
- Windows (x86/x64)  
- OsX may work too
- Android in Termux



## Version ##
1.0.0  
Last changed: 18.08.2022

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
$devcmd> msbuild ResTractor.vcxproj /p:Configuration=<Release|Debug> /p:Platform=<x64|x86> [/p:PlatformToolset=<v142|v143|WindowsApplicationForDrivers10.0>]
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
 
## EXAMPLE ##
```bash
$ ResTractor C:\Windows\System32\mspaint.exe -o %tmp%
```

#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 
