

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#define VERBOSE_MODE 1

#ifdef _WIN32
#include "warnings.h"
#endif
#include "print.h"
#include "utils/env.h"
#include "errorCodes.h"
#include "Globals.h"

#include "utils/Converter.h"
#include "utils/common_fileio.h"
#include "utils/Files.h"
#include "utils/blockio.h"
#include "utils/Helper.h"

#include "parser.h"


#define BIN_NAME "ResTractor"
#define BIN_VS "1.0.0"
#define BIN_DATE "17.08.2022"

#define LIN_PARAM_IDENTIFIER ('-')
#define WIN_PARAM_IDENTIFIER ('/')
#ifdef _WIN32
#define PARAM_IDENTIFIER WIN_PARAM_IDENTIFIER
#else
#define PARAM_IDENTIFIER LIN_PARAM_IDENTIFIER
#endif

int initGpData(
    PGlobalParams gp
);

int cleanGpData(
    PGlobalParams gp
);

static void printUsage();
static void printHelp();
static bool isCallForHelp(const char* arg1);
static int parseArgs(
    int argc,
    char** argv,
    PGlobalParams gp,
    char* file_name
    );
static void sanitizeArgs(PGlobalParams gp);
static uint8_t isArgOfType(const char* arg, char* type);
static uint8_t hasValue(char* type, int i, int end_i);



int
#ifdef _WIN32
__cdecl
#endif
main(int argc, char** argv)
{
    size_t n = 0;
    int errsv = 0;
    char file_name[PATH_MAX];

    HeaderData* hd = NULL;
    GlobalParams gp;

    memset(file_name, 0, PATH_MAX);
    memset(&gp, 0, sizeof(GlobalParams));

    int s = 0;

    if ( argc < 2 )
    {
        printUsage();
        return 0;
    }
    

    if ( isCallForHelp(argv[1]) )
    {
        printHelp();
        return 1;
    }

    if ( parseArgs(argc, argv, &gp, file_name) != 0 )
        return 0;

    errno = 0;
    gp.file.handle = fopen(file_name, "rb");
    errsv = errno;
    if ( gp.file.handle == NULL)
    {
        EPrint("Could not open file: \"%s\" (0x%x)\n", file_name, errsv);
        return -1;
    }

    gp.file.size = getSizeFP(gp.file.handle);
    if ( gp.file.size == 0 )
    {
        EPrint("File \"%s\" is zero.\n", file_name);
        s = -2;
        goto exit;
    }

    sanitizeArgs(&gp);

    DPrint("file_name: %s\n", file_name);
    DPrint("abs_file_offset: 0x%zx\n", gp.file.abs_offset);
    DPrint("start_file_offset: 0x%zx\n", gp.file.start_offset);

    hd = (HeaderData*) malloc(sizeof(HeaderData));
    if ( hd == NULL )
    {
        EPrint("Malloc HeaderData failed.\n");
        s = -3;
        goto exit;
    }
    s =  initHeaderData(hd, DEFAULT_CODE_REGION_CAPACITY);
    if ( s != 0 )
        goto exit;
    
    s = initGpData(&gp);
    if ( s != 0 )
        goto exit;

    parseHeader(hd, &gp);


exit:
    cleanGpData(&gp);
    freeHeaderData(hd);
    hd = NULL;
    
    if ( gp.file.handle != NULL )
        fclose(gp.file.handle);

    return s;
}

int initGpData(PGlobalParams gp)
{
    int s = 0;
    int errsv = 0;

    gp->data.block_main_size = BLOCKSIZE_LARGE;
    errno = 0;
    gp->data.block_main = (uint8_t*)malloc(gp->data.block_main_size);
    errsv = errno;
    if ( !gp->data.block_main )
    {
        s = (errsv!=0)?errsv:-1;
        EPrint("No memory for block main! (0x%x)\n", s);
        return -1;
    }

    gp->data.block_sub_size = BLOCKSIZE_SMALL;
    errno = 0;
    gp->data.block_sub = (uint8_t*)malloc(gp->data.block_sub_size);
    errsv = errno;
    if ( !gp->data.block_sub )
    {
        s = (errsv!=0)?errsv:-1;
        EPrint("No memory for block sub! (0x%x)\n", s);
        return -1;
    }

    return s;
}

int cleanGpData(PGlobalParams gp)
{
    int s = 0;

    if ( gp->data.block_main )
        free(gp->data.block_main);

    if ( !gp->data.block_sub )
        free(gp->data.block_sub);

    memset(gp, 0, sizeof(*gp));

    return s;
}



void printUsage()
{
#ifdef _WIN32
    char* pref = "";
#else
    char* pref = "./";
#endif
    printf("Usage: %s%s [options] <file> [options]\n", pref, BIN_NAME);
    printf("\n");
    printf("Version: %s\n", BIN_VS);
    printf("Last changed: %s\n", BIN_DATE);
}

bool isCallForHelp(const char* arg1)
{
    return isArgOfType(arg1, "/h") || 
           isArgOfType(arg1, "/?");
}

void printHelp()
{
    printUsage();
    printf("\n"
        "Options:\n"
            " * -o:string Out directory, where the resource files will be saved.\n"
            " * -h Print this.\n"
    );
    printf("\n");
    printf("Examples:\n");
#ifdef _WIN32
    printf("%s path/to/a.file -o %%tmp%%\n", BIN_NAME);
#else
    printf("./%s path/to/a.file -o /tmp\n", BIN_NAME);
#endif
}

int parseArgs(int argc, char** argv, PGlobalParams gp, char* file_name)
{
    int start_i = 1;
    int end_i = argc;
    int i;
    int s = 0;
    char* arg = NULL;

    for ( i = start_i; i < end_i; i++ )
    {
        arg = argv[i];
        //if ( arg[0] != LIN_PARAM_IDENTIFIER &&  )
        //    break;

        if ( isArgOfType(arg, "-o") )
        {
            if ( hasValue("-o", i, end_i) )
            {
                gp->outDir = argv[i + 1];
                cropTrailingSlash((char*)gp->outDir);
                i++;
            }
        }
        else if ( arg[0] != '-' )
        {
            expandFilePath(arg, file_name);
        }
        else
        {
            IPrint("Unknown Option \"%s\"\n", arg);
        }
    }


    if ( file_name[0] == 0 )
    {
        EPrint("No file set!\n");
        s = -1;
    }
    else if ( !fileExists(file_name) )
    {
        EPrint("File not found!\n");
        s = -1;
    }

    if ( gp->outDir!=NULL )
    {
        if ( strnlen(gp->outDir, PATH_MAX) >= PATH_MAX-10 )
        {
            EPrint("Output directory path \"%.*s\" too long!\n", PATH_MAX, gp->outDir);
            gp->outDir = NULL;
            s = -2;
        }
        if ( !dirExists(gp->outDir) )
        {
            EPrint("Output directory \"%.*s\" does not exist!\n", PATH_MAX, gp->outDir);
            gp->outDir = NULL;
            s = -3;
        }
    }

    return s;
}

void sanitizeArgs(PGlobalParams gp)
{
    (gp);
}

uint8_t isArgOfType(const char* arg, char* type)
{
    size_t i;
    size_t type_ln;
    if ( arg[0] != LIN_PARAM_IDENTIFIER && arg[0] != WIN_PARAM_IDENTIFIER )
        return 0;

    type_ln = strlen(type);

    for ( i = 1; i < type_ln; i++ )
    {
        if ( arg[i] != type[i] )
            return 0;
    }
    return arg[i] == 0;
}

uint8_t hasValue(char* type, int i, int end_i)
{
    (type);
    if ( i >= end_i - 1 )
    {
        IPrint("Arg \"%s\" has no value! Skipped!\n", type);
        return 0;
    }

    return 1;
}
