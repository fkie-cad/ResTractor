#ifndef RESTRACTOR_HP_H
#define RESTRACTOR_HP_H

// 
// implemented in HeaderParser.lib
// 

// PEImageDirectoryParser.h
size_t PE_getDataDirectoryEntryFileOffset(PEDataDirectory* data_directory,
                                            enum ImageDirectoryEntries entry_id,
                                            uint16_t nr_of_sections,
                                            const char* label,
                                            SVAS* svas);
size_t PE_Rva2Foa(uint32_t va, SVAS* svas, uint16_t svas_size);

// PEHeaderPrinter.h
void PE_printImageResourceDirectory(const PE_IMAGE_RESOURCE_DIRECTORY* rd, size_t offset, uint16_t level);
void PE_printImageResourceDirectoryEntryHeader(int type, uint16_t n, uint16_t level);
void PE_printImageResourceDirectoryEntry(
    const PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
    size_t table_fo,
    size_t offset,
    uint16_t level,
    uint16_t id,
    uint16_t n,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
);
void PE_printImageResourceDataEntry(
    const PE_IMAGE_RESOURCE_DATA_ENTRY* de, 
    uint32_t fotd, 
    size_t offset, 
    uint16_t level
);

// blockio.h
uint8_t checkFileSpace(size_t rel_offset,
                       size_t abs_offset,
                       size_t needed,
                       size_t file_size);

// common_fileio.h
size_t readFile(FILE* fi, size_t begin, size_t size, uint8_t* data);
size_t getSizeFP(FILE* fi);

// Files.h
void cropTrailingSlash(char* path);
int fileExists(const char* path);
int dirExists(const char* path);

// Helper.h
void expandFilePath(const char* src, char* dest);

#endif
