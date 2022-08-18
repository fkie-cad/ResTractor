#ifndef HEADER_PARSER_PE_HEADER_PRINTER_H
#define HEADER_PARSER_PE_HEADER_PRINTER_H

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "../Globals.h"
#include "../utils/Converter.h"
#include "../utils/Helper.h"
#include "../utils/blockio.h"
#include "PEHeader.h"
#include "PEHeaderOffsets.h"
#include "PEOptionalHeaderSignature.h"
#include "PEHeaderSectionNameResolution.h"
#include "PEMachineTypes.h"
#include "PEWindowsSubsystem.h"
#include "PECharacteristics.h"

void PE_printImageResourceDirectoryEntryHeader(
    int type,
    uint16_t n,
    uint16_t level
);
void PE_printImageResourceDirectoryEntry(const PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
                                         size_t table_fo,
                                         uint16_t level,
                                         uint16_t id,
                                         uint16_t n,
                                         size_t start_file_offset,
                                         size_t file_size,
                                         FILE* fp,
                                         uint8_t* block_s);

#define MAX_SPACES (512)
void fillSpaces(char* buf, size_t n, uint16_t level);
//#define MAX_DASHES (512)
//void fillDashes(char* buf, size_t n, uint16_t level);


void fillSpaces(char* buf, size_t n, uint16_t level)
{
//	memset(&spaces, 0, n);
    if ( n == 0 || buf == NULL )
        return;
    size_t min = (n-1 < level*2u) ? n-1 : level*2u;
    memset(buf, ' ', min);
    buf[min] = 0;
//	size_t i;
//	spaces[0] = ' ';
//	for ( i = 0; i < n && i < level; i++ )
//	{
//		buf[i*2+1] = ' ';
//		buf[i*2+2] = ' ';
//	}
}

void PE_printImageResourceDirectory(const PE_IMAGE_RESOURCE_DIRECTORY* rd, uint16_t level)
{
    char spaces[MAX_SPACES];
    fillSpaces(spaces, MAX_SPACES, level);

    char date[32];
    date[0] = 0;
    if (rd->TimeDateStamp != (uint32_t)-1)
        formatTimeStampD(rd->TimeDateStamp, date, sizeof(date));

    printf("%sResource Directory:\n", spaces);
    printf("%s- Characteristics: 0x%x\n", spaces, rd->Characteristics);
    printf("%s- TimeDateStamp: %s (0x%x)\n", spaces, date, rd->TimeDateStamp);
    printf("%s- MajorVersion: %u\n", spaces, rd->MajorVersion);
    printf("%s- MinorVersion: %u\n", spaces, rd->MinorVersion);
    printf("%s- NumberOfNamedEntries: 0x%x\n", spaces, rd->NumberOfNamedEntries);
    printf("%s- NumberOfIdEntries: 0x%x\n", spaces, rd->NumberOfIdEntries);
}

void PE_printImageResourceDirectoryEntryHeader(int type, uint16_t n, uint16_t level)
{
    char spaces[MAX_SPACES];
    fillSpaces(spaces, MAX_SPACES, level);
    
    if ( type == 0 )
        printf("%s- Named Entries (%u):\n", spaces, n);
    else if ( type == 1 )
        printf("%s- ID Entries (%u):\n", spaces, n);
}

void PE_printImageResourceDirectoryEntry(
    const PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
    size_t table_fo,
    uint16_t level,
    uint16_t id,
    uint16_t n,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
)
{
    size_t name_offset = 0;
    size_t bytes_read = 0;
    size_t i = 0;
    uint8_t* ptr = NULL;
    PE_IMAGE_RESOURCE_DIR_STRING_U_PTR name;
    const struct Pe_Image_Resource_Dir_String_U_Offsets *name_offsets = &PeImageResourceDirStringUOffsets;

    char spaces[MAX_SPACES];
    fillSpaces(spaces, MAX_SPACES, level);
    
    printf("%s  %u/%u:\n", spaces, (id+1), n);
    
    if ( re->NAME_UNION.NAME_STRUCT.NameIsString )
    {
        name_offset = table_fo + re->NAME_UNION.NAME_STRUCT.NameOffset;
        if ( !checkFileSpace(name_offset, start_file_offset, 4, file_size) )
        {
            header_error("ERROR: ressource name offset beyond file bounds!\n");
            return;
        }

        name_offset = name_offset + start_file_offset;
        bytes_read = readFile(fp, (size_t)name_offset, BLOCKSIZE_SMALL, block_s);
        if ( bytes_read <= 4 )
            return;

        ptr = block_s;
        name.Length = GetIntXValueAtOffset(uint16_t, ptr, name_offsets->Length);
        name.NameString = ((uint16_t*) &ptr[name_offsets->NameString]);
        if ( name.Length > (uint16_t)bytes_read - 4 ) // minus length - L'0'
            name.Length = (uint16_t)bytes_read-4;
        ptr[bytes_read-2] = 0;
        ptr[bytes_read-1] = 0;

        if ( !checkFileSpace(name_offset, start_file_offset, 2+name_offsets->Length, file_size))
        {
            header_error("ERROR: ressource name beyond file bounds!\n");
            return;
        }

        // wchar on linux is uint32_t
        // TODO: convert windows wchar(uint16_t) to utf8 to print cross os without expecting it to be ascii
        // hack: considering it ascii
        printf("%s  - Name (%u): ", spaces, name.Length);
        for ( i = 0; i < name.Length; i++ )
            printf("%c", name.NameString[i]);
        printf("\n");
    }
    // id entries have ids
    else
    {
        printf("%s  - Id: 0x%x\n", spaces, re->NAME_UNION.Id);
    }
    printf("%s  - OffsetToData: 0x%x\n", spaces, re->OFFSET_UNION.OffsetToData);
    printf("%s    - OffsetToData.OffsetToDirectory: 0x%x\n", spaces, re->OFFSET_UNION.DATA_STRUCT.OffsetToDirectory);
    printf("%s    - OffsetToData.NameIsDirectory: 0x%x\n", spaces, re->OFFSET_UNION.DATA_STRUCT.DataIsDirectory);
}

void PE_printImageResourceDataEntry(
    const PE_IMAGE_RESOURCE_DATA_ENTRY* de, 
    uint32_t fotd, 
    uint16_t level
)
{
    char spaces[MAX_SPACES];
    fillSpaces(spaces, MAX_SPACES, level);
    
    printf("%s  - ResourceDataEntry:\n", spaces);
    //printf("%s    - OffsetToData rva: 0x%x, fo: 0x%x\n", spaces, de->OffsetToData, fotd);
    //printf("%s    - OffsetToData: 0x%x (rva), 0x%x (fo)\n", spaces, de->OffsetToData, fotd);
    printf("%s    - OffsetToData\n", spaces);
    printf("%s        rva: 0x%x\n", spaces, de->OffsetToData);
    printf("%s         fo: 0x%x\n", spaces, fotd);
    printf("%s    - Size: 0x%x\n", spaces, de->Size);
    printf("%s    - CodePage: 0x%x\n", spaces, de->CodePage);
    printf("%s    - Reserved: 0x%x\n", spaces, de->Reserved);
}

#endif
