#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
//#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
#include "warnings.h"
#else
#include "errno.h"
#endif
#include "Args.h"

#include "Globals.h"
#include "HeaderData.h"
#include "headerParserLib.h"
#include "headerParserLibPE.h"
#include "pe/PEHeaderOffsets.h"

#include "flags.h"
#include "structs.h"
#include "hp.h"
#include "pe/idp/PEImageResourceTable.h"


#define BIN_NAME "ResTractor"
#define BIN_VS "1.1.2"
#define BIN_DATE "29.02.2024"



typedef struct _CmdParams {
    char* out_dir;
    char file_name[PATH_MAX];
    uint32_t flags;
} CmdParams, *PCmdParams;


static void printVersion();
static void printUsage();
static void printHelp();
static int parseArgs(
    int argc,
    char** argv,
    PCmdParams Params
);
int checkArgs(
    PCmdParams Params
);

int openFile(PRTParams rtp, const char* file_name);
int initRtp(
    PRTParams rtp
);

int cleanRtp(
    PRTParams rtp
);



int main(int argc, char** argv)
{
    int s = 0;

    size_t offset = 0;
    
    CmdParams params = { 0 };
    RTParams rtp = { 0 };
    PEHeaderData* peHd = NULL;
    


    if ( argc < 2 )
    {
        printUsage();
        return 0;
    }
    if ( isAskForHelp(argc, argv) )
    {
        printHelp();
        return 0;
    }

    s = parseArgs(argc, argv, &params);
    if ( s != 0 )
        return s;
    s = checkArgs(&params);
    if ( s != 0 )
        return s;

    DPrint("file_name: %s\n", params.file_name);
    DPrint("flags: 0x%x\n", params.flags);
    
    s = openFile(&rtp, params.file_name);
    if ( s != 0 )
        goto clean;

    s = initRtp(&rtp);
    if ( s != 0 )
        goto clean;

    peHd = getPEHeaderData(params.file_name, offset);
    if ( !peHd || !peHd->opt_header || !peHd->coff_header || !peHd->svas )
    {
        EPrint("No PE header data found!\n");
        goto clean;
    }
    
    rtp.flags = params.flags;
    rtp.out_dir = params.out_dir;

    s = parseImageResourceTable(
        peHd->opt_header, 
        peHd->coff_header->NumberOfSections, 
        &rtp, 
        peHd->svas
    );

clean:
    freePEHeaderData(peHd);
    cleanRtp(&rtp);

    return s;
}

int openFile(PRTParams rtp, const char* file_name)
{
    int s = 0;
    int errsv = 0;

    errno = 0;
    rtp->file.handle = fopen(file_name, "rb");
    errsv = errno;
    if ( rtp->file.handle == NULL)
    {
        EPrint("Could not open file \"%s\" (0x%x)\n", file_name, errsv);
        return errsv;
    }

    rtp->file.size = getSizeFP(rtp->file.handle);
    if ( rtp->file.size == 0 )
    {
        s = ERROR_FILE_NOT_FOUND;
        EPrint("File \"%s\" is zero.\n", file_name);
        return s;
    }

    return s;
}

int initRtp(PRTParams rtp)
{
    int s = 0;
    int errsv = 0;

    rtp->data.block_main_size = BLOCKSIZE_LARGE;
    errno = 0;
    rtp->data.block_main = (uint8_t*)malloc(rtp->data.block_main_size);
    errsv = errno;
    if ( !rtp->data.block_main )
    {
        s = (errsv!=0)?errsv:-1;
        EPrint("No memory for block main! (0x%x)\n", s);
        return s;
    }

    rtp->data.block_sub_size = BLOCKSIZE_SMALL;
    errno = 0;
    rtp->data.block_sub = (uint8_t*)malloc(rtp->data.block_sub_size);
    errsv = errno;
    if ( !rtp->data.block_sub )
    {
        s = (errsv!=0)?errsv:-1;
        EPrint("No memory for block sub! (0x%x)\n", s);
        return s;
    }

    return s;
}

int cleanRtp(PRTParams rtp)
{
    int s = 0;

    if ( rtp->data.block_main )
        free(rtp->data.block_main);

    if ( rtp->data.block_sub )
        free(rtp->data.block_sub);
    
    if ( rtp->file.handle != NULL )
        fclose(rtp->file.handle);
    
    memset(rtp, 0, sizeof(*rtp));

    return s;
}

int parseArgs(int argc, char** argv, PCmdParams Params)
{
    int s = 0;

    int start_i = 1;
    int end_i = argc;
    int i;
    char* arg = NULL;
    char *val1 = NULL;

    for ( i = start_i; i < end_i; i++ )
    {
        arg = argv[i];
        val1 = GET_ARG_VALUE(argc, argv, i, 1);

        if ( IS_1C_ARG(arg, 'o') )
        {
            BREAK_ON_NOT_A_VALUE(val1, s, "ERROR: No out dir set!\n");

            Params->out_dir = val1;
            cropTrailingSlash((char*)Params->out_dir);
            
            i++;
        }
        else if ( IS_1C_ARG(arg, 'p') )
        {
            Params->flags |= FLAG_PRINT;
        }
        else if ( IS_VALUE(arg) )
        {
            expandFilePath(arg, Params->file_name);
        }
        else
        {
            IPrint("Unknown Option \"%s\"\n", arg);
        }
    }

    return s;
}

int checkArgs(PCmdParams Params)
{
    int s = 0;
    
    if ( Params->file_name[0] == 0 )
    {
        EPrint("No file set!\n");
        s = ERROR_INVALID_PARAMETER;
    }
    else if ( !fileExists(Params->file_name) )
    {
        EPrint("File not found!\n");
        s = ERROR_FILE_NOT_FOUND;
    }

    if ( Params->out_dir != NULL )
    {
        if ( !dirExists(Params->out_dir) )
        {
            EPrint("Output directory \"%.*s\" does not exist!\n", PATH_MAX, Params->out_dir);
            Params->out_dir = NULL;
            s = ERROR_PATH_NOT_FOUND;
        }
        else if ( strnlen(Params->out_dir, PATH_MAX) >= PATH_MAX-20 )
        {
            EPrint("Output directory path \"%.*s\" too long!\n", PATH_MAX, Params->out_dir);
            Params->out_dir = NULL;
            s = ERROR_BUFFER_OVERFLOW;
        }
    }

    if ( Params->out_dir == NULL && !(Params->flags&FLAG_PRINT) )
    {
        EPrint("No out dir or printing mode set!\n");
        s = ERROR_INVALID_PARAMETER;
    }
#ifdef ERROR_PRINT
    printf("\n");
#endif

    return s;
}

void printVersion()
{
    printf("Version: %s\n", BIN_VS);
    printf("Last changed: %s\n", BIN_DATE);
    printf("Build date: %s %s\n", __DATE__, __TIME__);
}

void printUsage()
{
    printf("Usage: %s%s [options] <file> [options]\n", BIN_PREF, BIN_NAME);
}

void printHelp()
{
    printVersion();
    printf("\n");
    printUsage();
    printf("\n"
        "Options:\n"
            " * -o:string Out directory, where the resource files will be saved.\n"
            " * -p Print the resource directory structure.\n"
            " * -h Print this.\n"
    );
    printf("\n");
    printf("Examples:\n");
#ifdef _WIN32
    printf("%s C:\\interesting.exe -o %%tmp%%\n", BIN_NAME);
#else
    printf("./%s /tmp/interesting.exe -o /tmp\n", BIN_NAME);
#endif
}
