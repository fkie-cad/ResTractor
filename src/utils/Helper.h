#ifndef HEADER_PARSER_UTILS_HELPER_H
#define HEADER_PARSER_UTILS_HELPER_H

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <unistd.h>
#endif

#include "../Globals.h"
#include "../utils/env.h"

void expandFilePath(const char* src, char* dest);
int checkBytes(const unsigned char* bytes, const uint8_t size, const unsigned char* block);



/**
 * Expand the file path:
 * 	on linux: if it starts with a '~'. If the src is passed as cmd line param, this is done automatically.
 * 	on windows: not implemented yet
 *
 * @param src char* the source string
 * @param dest char* the preallocated destination buffer
 */
void expandFilePath(const char* src, char* dest)
{
    const char* env_home;

    if ( !src || src[0] == 0 )
        return;
    size_t cch = strlen(src);
    if ( cch >= PATH_MAX )
        return;

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__APPLE__)
    if ( src[0] == '~' && src[1] == '/' && src[2] != 0 )
    {
        env_home = getenv("HOME");
        if ( env_home != NULL )
        {
            cch = snprintf(dest, PATH_MAX, "%s/%s", env_home, &src[2]);
            if ( cch >= PATH_MAX )
                snprintf(dest, PATH_MAX, "%s", src);
        }
        else
        {
            snprintf(dest, PATH_MAX, "%s", src);
        }
    }
    else if ( src[0] != '/' )
    {
        char cwd[PATH_MAX] = {0};
        if ( getcwd(cwd, PATH_MAX) != NULL )
        {
            cch = snprintf(dest, PATH_MAX, "%s/%s", cwd, src);
            if ( cch >= PATH_MAX )
                snprintf(dest, PATH_MAX, "%s", src);
        }
        else
        {
            snprintf(dest, PATH_MAX, "%s", src);
        }
    }
    else
#endif
    {
        snprintf(dest, PATH_MAX, "%s", src);
    }
    dest[PATH_MAX-1] = 0;
}

/**
 * Check bytes in block.
 * 
 * @param bytes unsigned char* the expected bytes
 * @param size size_t size of expected bytes
 * @param block the block to search in 
 * @return 
 */
int checkBytes(const unsigned char* bytes, const uint8_t size, const unsigned char* block)
{
    size_t i;

    for ( i = 0; i < size; i++ )
    {
        if ( block[i] != bytes[i] )
            return 0;
    }

    return 1;
}

#endif
