#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stddef.h>
#include <limits.h>



#if defined(Win64) || defined(_WIN64)
#define fseek(f, o, t) _fseeki64(f, o, t)
#define ftell(s) _ftelli64(s)
#endif

#ifndef PATH_MAX
    #define PATH_MAX (0x1000)
#endif

#define BLOCKSIZE_SMALL (0x200u)
#define BLOCKSIZE_LARGE (0x400u)

#define getVarName(var)  #var
#define ERRORS_BUFFER_SIZE (512)

#define MIN_ASCII_INT (48)
#define MAX_ASCII_INT (57)

#define MIN_FILE_SIZE (0x10)

#include "print.h"


// _t_ type
// _p_ pointer
// _o_ offset
#define GetIntXValueAtOffset(_t_, _p_, _o_) (*((_t_*) &(_p_)[_o_]))



#ifndef __cplusplus
    typedef uint8_t bool;
    #define true 1
    #define false 0
#endif
    


#define MAX_SIZE_OF_SECTION_NAME (128)


// mostly const after init
typedef struct HPFile
{
    //char file_name[PATH_MAX];
    FILE* handle;
    size_t size;
    size_t start_offset;
    size_t abs_offset; // dynamic
} HPFile, *PHPFile;

typedef struct GlobalParams
{
    // dynamic
	struct data {
        uint8_t* block_main;
        uint32_t block_main_size;
        uint8_t* block_sub;
        uint32_t block_sub_size;
    } data;
    
    const char* outDir;

    HPFile file;
    uint32_t flags;
} GlobalParams, *PGlobalParams;

#define FLAG_PRINT  (0x1)

#endif
