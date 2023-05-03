#ifndef HEADER_PARSER_PE_HEADER_PARSER_H
#define HEADER_PARSER_PE_HEADER_PARSER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "../Globals.h"
#include "../PEHeaderData.h"
#include "PEHeader.h"
#include "PEHeaderOffsets.h"
#include "PEOptionalHeaderSignature.h"
#include "PEImageDirectoryParser.h"
#include "PEHeaderPrinter.h"



#define MAX_NR_OF_RVA_TO_READ (128)
#define MAX_CERT_TABLE_SIZE (10)




int parsePEHeaderData(
    PGlobalParams gp
);
static int parsePEHeader(
    PEHeaderData* pehd,
    PGlobalParams gp
);

static int PE_readImageDosHeader(PEImageDosHeader* idh,
                                 size_t file_offset,
                                 size_t file_size,
                                 unsigned char* block_l);

static unsigned char PE_checkDosHeader(const PEImageDosHeader* idh,
                                       size_t file_size);

static uint8_t PE_checkPESignature(
    uint32_t e_lfanew,
    PGlobalParams gp
);

static int PE_readCoffHeader(size_t offset,
                                 PECoffFileHeader* ch,
                                 size_t start_file_offset,
                                 size_t* abs_file_offset,
                                 size_t file_size,
                                 FILE* fp,
                                 unsigned char* block_l);

static int PE_readOptionalHeader(size_t offset,
                                     PE64OptHeader* oh,
                                     size_t start_file_offset,
                                     size_t* abs_file_offset,
                                     size_t file_size,
                                     FILE* fp,
                                     unsigned char* block_l);

static int PE_readSectionHeader(
    size_t header_start,
    PECoffFileHeader* ch,
    PGlobalParams gp,
    int parse_svas,
    SVAS** svas
);

static void PE_fillSectionHeader(const unsigned char* ptr, PEImageSectionHeader* sh);
static int PE_isNullSectionHeader(const PEImageSectionHeader* sh);

static void PE_cleanUp(
    PEHeaderData* pehd
);



// The PE file header consists of a
//  - Microsoft MS-DOS stub,
//  - the PE signature,
//  - the COFF file header,
//  - and an optional header.
// A COFF object file header consists of
//  - a COFF file header
//  - and an optional header.
// In both cases, the file headers are followed immediately by section headers.
//
// HeaderData
//   .bitness is received by analysing the target machine and optional header
//   .endian defaults to 1 (le), because the determining Coff header flags (characteristics) are deprecated
//
// Each row of the section table is, in effect, a section header.
// This table immediately follows the optional header, if any.
// This positioning is required because the file header does not contain a direct pointer to the section table.
// Instead, the location of the section table is determined by calculating the location of the first byte after the headers.
// Make sure to use the size of the optional header as specified in the file header.
// 40 bytes per entry
int parsePEHeaderData(
    PGlobalParams gp
)
{
    int s = 0;

    PEHeaderData pehd;
    PEImageDosHeader image_dos_header_l;
    PECoffFileHeader coff_header_l;
    PE64OptHeader opt_header_l;

    memset(&image_dos_header_l, 0, sizeof(PEImageDosHeader));
    memset(&coff_header_l, 0, sizeof(PECoffFileHeader));
    memset(&opt_header_l, 0, sizeof(PE64OptHeader));

    memset(&pehd, 0, sizeof(PEHeaderData));

    pehd.image_dos_header = &image_dos_header_l;
    pehd.coff_header = &coff_header_l;
    pehd.opt_header = &opt_header_l;

    s = parsePEHeader(&pehd, gp);

    PE_cleanUp(&pehd);

    return s;
}

int parsePEHeader(
    PEHeaderData* pehd,
    PGlobalParams gp
)
{
    PEImageDosHeader* image_dos_header = NULL;
    PECoffFileHeader* coff_header = NULL;
    PE64OptHeader* opt_header = NULL;

    size_t optional_header_offset = 0;
    size_t section_header_offset = 0;

    uint8_t pe_header_type = 0;
    int parse_svas = 1;
    int s = 0;

    image_dos_header = pehd->image_dos_header;
    coff_header = pehd->coff_header;
    opt_header = pehd->opt_header;

    s = PE_readImageDosHeader(image_dos_header, gp->file.start_offset, gp->file.size, gp->data.block_main);
    if ( s != 0 )
        return -2;
    
    if ( !PE_checkDosHeader(image_dos_header, gp->file.size) )
    {
        EPrint("DOS header is invalid!\n");
        return -3;
    }

    pe_header_type = PE_checkPESignature(
        image_dos_header->e_lfanew, 
        gp
    );
    if ( pe_header_type != 1 )
    {
        EPrint("No valid PE00 section signature found!\n");
        return -4;
    }

    s = PE_readCoffHeader((size_t)image_dos_header->e_lfanew + SIZE_OF_MAGIC_PE_SIGNATURE, coff_header, gp->file.start_offset,
                       &gp->file.abs_offset, gp->file.size, gp->file.handle, gp->data.block_main);
    if ( s != 0 )
        return -5;
    
    optional_header_offset = (size_t)image_dos_header->e_lfanew + SIZE_OF_MAGIC_PE_SIGNATURE + PE_COFF_FILE_HEADER_SIZE;
    s = PE_readOptionalHeader(optional_header_offset, opt_header, gp->file.start_offset, &gp->file.abs_offset, gp->file.size, gp->file.handle, gp->data.block_main);
    if ( s != 0 )
        return -7;
    
    section_header_offset = (size_t)image_dos_header->e_lfanew + SIZE_OF_MAGIC_PE_SIGNATURE + PE_COFF_FILE_HEADER_SIZE + coff_header->SizeOfOptionalHeader;
    s = PE_readSectionHeader(
        section_header_offset, 
        coff_header, 
        gp,
        parse_svas, 
        &pehd->svas
    );
    if ( s != 0 )
    {
        EPrint("reading section header failed! (0x%x)\n", s);
        return s;
    }


    s = PE_parseImageResourceTable(
        opt_header, 
        coff_header->NumberOfSections, 
        gp, 
        pehd->svas
    );

    return s;
}

int PE_readImageDosHeader(PEImageDosHeader* idh,
                          size_t file_offset,
                          size_t file_size,
                          unsigned char* block_l)
{
//	uint16_t *ss, *sp; // 2 byte value
//	uint16_t *ip, *cs; // 2 byte value
    unsigned char *ptr;

//    DPrint("readImageDosHeader()\n");
//    DPrint(" - file_offset: %zx\n", file_offset);

    if ( !checkFileSpace(0, file_offset, sizeof(PEImageDosHeader), file_size) )
        return -1;

    ptr = &block_l[0];

    idh->signature[0] = (char)ptr[PEImageDosHeaderOffsets.signature];
    idh->signature[1] = (char)ptr[PEImageDosHeaderOffsets.signature+1];
    idh->lastsize = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.lastsize);
    idh->nblocks = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.nblocks);
    idh->nreloc = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.nreloc);
    idh->hdrsize = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.hdrsize);
    idh->minalloc = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.minalloc);
    idh->maxalloc = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.maxalloc);
    idh->checksum = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.checksum);
    idh->relocpos = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.relocpos);
    idh->noverlay = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.noverlay);
    idh->oem_id = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.oem_id);
    idh->oem_info = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.oem_info);
    idh->ss = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.ss);
    idh->sp = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.sp);
    idh->ip = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.ip);
    idh->cs = GetIntXValueAtOffset(uint16_t, ptr, PEImageDosHeaderOffsets.cs);
    idh->e_lfanew = GetIntXValueAtOffset(uint32_t, ptr, PEImageDosHeaderOffsets.e_lfanew);

    return 0;
}

unsigned char PE_checkDosHeader(const PEImageDosHeader *idh,
                                size_t file_size)
{
    return idh->e_lfanew != 0 && idh->e_lfanew + SIZE_OF_MAGIC_PE_SIGNATURE < file_size;
}

/**
 * This signature shows that
 * a) this file is a legitimate PE file,
 * b) this is a NE, LE, LX file
 *
 * @param e_lfanew
 * @return
 */
uint8_t PE_checkPESignature(
    const uint32_t e_lfanew,
    PGlobalParams gp
)
{
    unsigned char *ptr;
    unsigned char is_pe = 0;
    unsigned char is_ne = 0;
    unsigned char is_le = 0;
    unsigned char is_lx = 0;
    
    size_t size;
    size_t file_size = gp->file.size;
    size_t file_offset = gp->file.start_offset;
    size_t abs_file_offset = gp->file.abs_offset;
    FILE* fp = gp->file.handle;

    uint8_t* block_s = gp->data.block_sub;
    uint8_t* block_l = gp->data.block_main;

    if ( !checkFileSpace(e_lfanew, file_offset, SIZE_OF_MAGIC_PE_SIGNATURE , file_size) )
        return 0;

    if ( e_lfanew + SIZE_OF_MAGIC_PE_SIGNATURE > BLOCKSIZE_LARGE )
    {
        abs_file_offset = file_offset + e_lfanew;
        size = readFile(fp, abs_file_offset, BLOCKSIZE_SMALL, block_s);
        if ( !size )
        {
            EPrint("Reading PE Signature block failed.\n");
            return 0;
        }
        ptr = block_s;
    }
    else
    {
        ptr = &block_l[e_lfanew];
    }

    if ( checkBytes(MAGIC_PE_SIGNATURE, SIZE_OF_MAGIC_PE_SIGNATURE, ptr) )
        is_pe = 1;

    if ( checkBytes(MAGIC_NE_SIGNATURE, SIZE_OF_MAGIC_NE_SIGNATURE, ptr) )
        is_ne = 1;

    if ( checkBytes(MAGIC_LE_SIGNATURE, SIZE_OF_MAGIC_LE_SIGNATURE, ptr) )
        is_le = 1;

    if ( checkBytes(MAGIC_LX_SIGNATURE, SIZE_OF_MAGIC_LX_SIGNATURE, ptr) )
        is_lx = 1;
    
//    DPrint("checkPESignature()\n");
//    DPrint(" - pe_signature: %2X %2X %2X %2X\n", ptr[0], ptr[1], ptr[2], ptr[3]);
//    DPrint(" - is_pe: %d\n", is_pe);
//    DPrint(" - is_ne: %d\n", is_ne);

    if ( is_pe == 1 ) return 1;
    if ( is_ne == 1 ) return 2;
    if ( is_le == 1 ) return 3;
    if ( is_lx == 1 ) return 4;

    return 0;
}

int PE_readCoffHeader(size_t offset,
                          PECoffFileHeader* ch,
                          size_t start_file_offset,
                          size_t* abs_file_offset,
                          size_t file_size,
                          FILE* fp,
                          unsigned char* block_l)
{
//    DPrint("readCoffHeader()\n");
    unsigned char *ptr;

    if ( !checkFileSpace(offset, start_file_offset, sizeof(PECoffFileHeader), file_size) )
        return -1;

    *abs_file_offset = start_file_offset;
    if ( !checkLargeBlockSpace(&offset, abs_file_offset, sizeof(PECoffFileHeader), block_l, fp) )
        return -1;

    ptr = &block_l[offset];

    ch->Machine = GetIntXValueAtOffset(uint16_t, ptr, PECoffFileHeaderOffsets.Machine);
    ch->NumberOfSections = GetIntXValueAtOffset(uint16_t, ptr, PECoffFileHeaderOffsets.NumberOfSections);
    ch->TimeDateStamp = GetIntXValueAtOffset(uint32_t, ptr, PECoffFileHeaderOffsets.TimeDateStamp);
    ch->PointerToSymbolTable = GetIntXValueAtOffset(uint32_t, ptr, PECoffFileHeaderOffsets.PointerToSymbolTable);
    ch->NumberOfSymbols = GetIntXValueAtOffset(uint32_t, ptr, PECoffFileHeaderOffsets.NumberOfSymbols);
    ch->SizeOfOptionalHeader = GetIntXValueAtOffset(uint16_t, ptr, PECoffFileHeaderOffsets.SizeOfOptionalHeader);
    ch->Characteristics = GetIntXValueAtOffset(uint16_t, ptr, PECoffFileHeaderOffsets.Characteristics);

    return 0;
}

/**
 * Read the optional header.
 * Just the magic is filled right now, to provide a fallback for bitness determination.
 *
 * @param offset
 * @param oh
 */
int PE_readOptionalHeader(size_t offset,
                              PE64OptHeader* oh,
                              size_t start_file_offset,
                              size_t* abs_file_offset,
                              size_t file_size,
                              FILE* fp,
                              unsigned char* block_l)
{
    PEOptionalHeaderOffsets offsets = PEOptional64HeaderOffsets;
    unsigned char *ptr;
    size_t size;
    uint32_t i;
    uint8_t size_of_data_entry = sizeof(PEDataDirectory);
    size_t data_entry_offset;
    uint8_t nr_of_rva_to_read;
//    DPrint("readPEOptionalHeader()\n");

    if ( !checkFileSpace(offset, start_file_offset, sizeof(oh->Magic), file_size) )
        return 1;

    *abs_file_offset = offset + start_file_offset;
    // read new large block, to ease up offsetting
    size = readFile(fp, *abs_file_offset, BLOCKSIZE_LARGE, block_l);
    if ( size == 0 )
        return 2;

    offset = 0;
    ptr = &block_l[offset];

    oh->Magic = *((uint16_t*) &ptr[offsets.Magic]);
    if ( oh->Magic == PeOptionalHeaderSignature.IMAGE_NT_OPTIONAL_HDR32_MAGIC )
        offsets = PEOptional32HeaderOffsets;

    if ( !checkFileSpace(offset, *abs_file_offset, sizeof(PE64OptHeader), file_size) )
    {
        EPrint("PE Optional Header beyond file size!\n");
        return 1;
    }

    if ( oh->Magic == PeOptionalHeaderSignature.IMAGE_NT_OPTIONAL_HDR32_MAGIC )
    {
        oh->ImageBase = GetIntXValueAtOffset(uint32_t, ptr, offsets.ImageBase);
        oh->BaseOfData = GetIntXValueAtOffset(uint32_t, ptr, offsets.BaseOfData);
        oh->SizeOfStackReserve = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfStackReserve);
        oh->SizeOfStackCommit = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfStackCommit);
        oh->SizeOfHeapReserve = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfHeapReserve);
        oh->SizeOfHeapCommit = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfHeapCommit);
    }
    else
    {
        oh->ImageBase = GetIntXValueAtOffset(uint64_t, ptr, offsets.ImageBase);
        oh->SizeOfStackReserve = GetIntXValueAtOffset(uint64_t, ptr, offsets.SizeOfStackReserve);
        oh->SizeOfStackCommit = GetIntXValueAtOffset(uint64_t, ptr, offsets.SizeOfStackCommit);
        oh->SizeOfHeapReserve = GetIntXValueAtOffset(uint64_t, ptr, offsets.SizeOfHeapReserve);
        oh->SizeOfHeapCommit = GetIntXValueAtOffset(uint64_t, ptr, offsets.SizeOfHeapCommit);
    }
    oh->MajorLinkerVersion = GetIntXValueAtOffset(uint8_t, ptr, offsets.MajorLinkerVersion);
    oh->MinorLinkerVersion = GetIntXValueAtOffset(uint8_t, ptr, offsets.MinorLinkerVersion);
    oh->SizeOfCode = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfCode);
    oh->SizeOfInitializedData = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfInitializedData);
    oh->SizeOfUninitializedData = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfUninitializedData);
    oh->AddressOfEntryPoint = GetIntXValueAtOffset(uint32_t, ptr, offsets.AddressOfEntryPoint);
    oh->BaseOfCode = GetIntXValueAtOffset(uint32_t, ptr, offsets.BaseOfCode);
    oh->SectionAlignment = GetIntXValueAtOffset(uint32_t, ptr, offsets.SectionAlignment);
    oh->FileAlignment = GetIntXValueAtOffset(uint32_t, ptr, offsets.FileAlignment);
    oh->MajorOSVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MajorOperatingSystemVersion);
    oh->MinorOSVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MinorOperatingSystemVersion);
    oh->MajorImageVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MajorImageVersion);
    oh->MinorImageVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MinorImageVersion);
    oh->MajorSubsystemVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MajorSubsystemVersion);
    oh->MinorSubsystemVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MinorSubsystemVersion);
    oh->Win32VersionValue = GetIntXValueAtOffset(uint32_t, ptr, offsets.Win32VersionValue);
    oh->SizeOfImage = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfImage);
    oh->SizeOfHeaders = GetIntXValueAtOffset(uint32_t, ptr, offsets.SizeOfHeaders);
    oh->Checksum = GetIntXValueAtOffset(uint32_t, ptr, offsets.CheckSum);
    oh->Subsystem = GetIntXValueAtOffset(uint16_t, ptr, offsets.Subsystem);
    oh->DLLCharacteristics = GetIntXValueAtOffset(uint16_t, ptr, offsets.DllCharacteristics);
    oh->LoaderFlags = GetIntXValueAtOffset(uint32_t, ptr, offsets.LoaderFlags);
    oh->NumberOfRvaAndSizes = GetIntXValueAtOffset(uint32_t, ptr, offsets.NumberOfRvaAndSizes);

    data_entry_offset = offsets.DataDirectories;

//    DPrint(" - NumberOfRvaAndSizes: %u\n", oh->NumberOfRvaAndSizes);

    if ( oh->NumberOfRvaAndSizes == 0 )
        return 0;

    nr_of_rva_to_read = (oh->NumberOfRvaAndSizes > MAX_NR_OF_RVA_TO_READ) ? MAX_NR_OF_RVA_TO_READ : (uint8_t)oh->NumberOfRvaAndSizes;
    if ( oh->NumberOfRvaAndSizes != NUMBER_OF_RVA_AND_SIZES )
    {
        IPrint("Unusual value of NumberOfRvaAndSizes: %u\n", oh->NumberOfRvaAndSizes);
    }

    if ( nr_of_rva_to_read > 0 )
    {
        oh->DataDirectory = (PEDataDirectory*) malloc(sizeof(PEDataDirectory) * nr_of_rva_to_read);
        if ( !oh->DataDirectory )
        {
            IPrint("Allocation of DataDirectory with %u entries failed!\n", nr_of_rva_to_read);

            if ( nr_of_rva_to_read > NUMBER_OF_RVA_AND_SIZES )
            {
                IPrint("Fallback to standard size of %u!\n", NUMBER_OF_RVA_AND_SIZES);

                oh->NumberOfRvaAndSizes = NUMBER_OF_RVA_AND_SIZES;
                oh->DataDirectory = (PEDataDirectory*) malloc(sizeof(PEDataDirectory) * oh->NumberOfRvaAndSizes);

                if ( !oh->DataDirectory )
                {
                    EPrint("Allocation of DataDirectory with %u entries failed!\n", oh->NumberOfRvaAndSizes);
                    oh->NumberOfRvaAndSizes = 0;
                    return 1;
                }
                nr_of_rva_to_read = NUMBER_OF_RVA_AND_SIZES;
            }
            else
            {
                oh->NumberOfRvaAndSizes = 0;
                return -1;
            }
        }

        for ( i = 0; i < nr_of_rva_to_read; i++ )
        {
            if ( !checkFileSpace(data_entry_offset, *abs_file_offset, size_of_data_entry, file_size) )
                break;

            if ( !checkLargeBlockSpace(&data_entry_offset, abs_file_offset, size_of_data_entry, block_l, fp) )
                break;

            ptr = &block_l[0];

            oh->DataDirectory[i].VirtualAddress = *((uint32_t*) &ptr[data_entry_offset]);
            oh->DataDirectory[i].Size = *((uint32_t*) &ptr[data_entry_offset + 4]);

            data_entry_offset += size_of_data_entry;
        }
    }

    return 0;
}

/**
 * Read the section table.
 *
 * @param header_start
 * @param ch
 * @param finame
 */
int PE_readSectionHeader(size_t header_start,
                          PECoffFileHeader* ch,
                          PGlobalParams gp,
                          int parse_svas,
                          SVAS** svas)
{
    unsigned char *ptr = NULL;
    size_t offset;
    PEImageSectionHeader s_header;
//    CodeRegionData code_region_data;
    uint16_t nr_of_sections = ch->NumberOfSections;
    uint16_t i = 0;
    size_t size;

    FILE* fp = gp->file.handle;
    size_t start_file_offset = gp->file.start_offset;
    size_t abs_file_offset = gp->file.abs_offset;
    size_t file_size = gp->file.size;

    uint8_t* block_m = gp->data.block_main;

    if ( parse_svas == 1 )
    {
        errno = 0;
        *svas = (SVAS*) calloc(nr_of_sections, sizeof(SVAS));
        if ( *svas == NULL )
        {
            EPrint("Alloc failed! (0x%x)\n", errno);
            return -1;
        }
    }
    // read new large block to ease up offsetting
    if ( !checkFileSpace(header_start, start_file_offset, PE_SECTION_HEADER_SIZE, file_size) )
        return -2;

    abs_file_offset = header_start + start_file_offset;
    size = readFile(fp, abs_file_offset, BLOCKSIZE_LARGE, block_m);
    if ( size == 0 )
        return -3;
    offset = 0;
    
    for ( i = 0; i < nr_of_sections; i++ )
    {
//        DPrint(" - %u / %u\n", (i+1), nr_of_sections);

        if ( !checkFileSpace(offset, abs_file_offset, PE_SECTION_HEADER_SIZE, file_size) )
            return -4;

        if ( !checkLargeBlockSpace(&offset, &abs_file_offset, PE_SECTION_HEADER_SIZE, block_m, fp) )
            break;

        ptr = &block_m[offset];

        PE_fillSectionHeader(ptr, &s_header);
        
        if ( PE_isNullSectionHeader(&s_header) )
        {
            break;
        }

        if ( parse_svas )
        {
            (*svas)[i].PointerToRawData = s_header.PointerToRawData;
            (*svas)[i].SizeOfRawData = s_header.SizeOfRawData;
            (*svas)[i].VirtualAddress = s_header.VirtualAddress;
            (*svas)[i].VirtualSize = s_header.Misc.VirtualSize;
        }

        offset += PE_SECTION_HEADER_SIZE;
    }
    
    gp->file.abs_offset = abs_file_offset;

    return 0;
}

void PE_fillSectionHeader(const unsigned char* ptr,
                          PEImageSectionHeader* sh)
{
    // may not be zero terminated
    strncpy(sh->Name, (const char*)&ptr[PESectionHeaderOffsets.Name], IMAGE_SIZEOF_SHORT_NAME);
    sh->Misc.VirtualSize = GetIntXValueAtOffset(uint32_t, ptr, PESectionHeaderOffsets.VirtualSize);
    sh->VirtualAddress = GetIntXValueAtOffset(uint32_t, ptr, PESectionHeaderOffsets.VirtualAddress);
    sh->SizeOfRawData = GetIntXValueAtOffset(uint32_t, ptr, PESectionHeaderOffsets.SizeOfRawData);
    sh->PointerToRawData = GetIntXValueAtOffset(uint32_t, ptr, PESectionHeaderOffsets.PointerToRawData);
    sh->PointerToRelocations = GetIntXValueAtOffset(uint32_t, ptr, PESectionHeaderOffsets.PointerToRelocations);
    sh->PointerToLinenumbers = GetIntXValueAtOffset(uint32_t, ptr, PESectionHeaderOffsets.PointerToLinenumbers);
    sh->NumberOfRelocations = GetIntXValueAtOffset(uint16_t, ptr, PESectionHeaderOffsets.NumberOfRelocations);
    sh->NumberOfLinenumbers = GetIntXValueAtOffset(uint16_t, ptr, PESectionHeaderOffsets.NumberOfLinenumbers);
    sh->Characteristics = GetIntXValueAtOffset(uint32_t, ptr, PESectionHeaderOffsets.Characteristics);
}

int PE_isNullSectionHeader(const PEImageSectionHeader* sh)
{
    return sh->Name[0] == 0 &&
           sh->Misc.VirtualSize == 0 &&
           sh->VirtualAddress == 0 &&
           sh->SizeOfRawData == 0 &&
           sh->PointerToRawData == 0 &&
           sh->PointerToRelocations == 0 &&
           sh->PointerToLinenumbers == 0 &&
           sh->NumberOfRelocations == 0 &&
           sh->NumberOfLinenumbers == 0 &&
           sh->Characteristics == 0;
}

void PE_cleanUp(PEHeaderData* pehd)
{
    if ( pehd == NULL )
        return;

    if ( pehd->st.strings != NULL )
    {
        free(pehd->st.strings);
        pehd->st.strings = NULL;
    }

    if ( pehd->opt_header->NumberOfRvaAndSizes > 0 )
    {
        free(pehd->opt_header->DataDirectory);
        pehd->opt_header->DataDirectory = NULL;
    }

    if ( pehd->svas != NULL )
    {
        free(pehd->svas);
        pehd->svas = NULL;
    }
}

#endif
