#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "utils/Helper.h"
#include "utils/blockio.h"

#include "Globals.h"

#include "pe/PEHeaderParser.h"

static void parseHeader(
    PGlobalParams gp
);

int isPE(
    unsigned char* block
);



void parseHeader(PGlobalParams gp)
{
    size_t n;
    int s = 0;

    n = readFile(gp->file.handle, gp->file.abs_offset, BLOCKSIZE_LARGE, gp->data.block_main);
    if ( !n )
    {
        EPrint("Reading file failed.\n");
        return;
    }

    if ( gp->file.abs_offset + MIN_FILE_SIZE > gp->file.size )
    {
        EPrint("filesize (0x%zx) is too small for a start offset of 0x%zx!\n",
                     gp->file.size, gp->file.abs_offset);
    }
    else if ( isPE(gp->data.block_main) )
    {
        s = parsePEHeaderData(gp);
        if ( s != 0 )
        {
            EPrint("Parsing PE header failed!\n");
        }
    }
    else
    {
        printf("Not a PE file!\n");
    }
}

int isPE(unsigned char* block)
{
    return checkBytes(MAGIC_PE_BYTES, MAGIC_PE_BYTES_LN, block);
}
