#ifndef HEADER_PARSER_PE_IMAGE_RESSOURCE_TABLE_H
#define HEADER_PARSER_PE_IMAGE_RESSOURCE_TABLE_H



#define PE_MAX_RES_DIR_LEVEL (0x20)

#define BASE_NAME_MAX_SIZE (0x200)

typedef struct _NAME {
    uint32_t Length;
    uint32_t MaxSize;
    char Buffer[BASE_NAME_MAX_SIZE];
} NAME, *PNAME;

size_t res_count = 0;
size_t file_count = 0;

int PE_parseImageResourceTable(
    PE64OptHeader* oh,
    uint16_t nr_of_sections,
    PGlobalParams gp,
    SVAS* svas
);

int PE_fillImageResourceDirectory(
    PE_IMAGE_RESOURCE_DIRECTORY* rd,
    size_t offset,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
);

//int PE_recurseImageResourceDirectory(
//    size_t offset,
//    size_t table_fo,
//    uint16_t nr_of_named_entries,
//    uint16_t nr_of_id_entries,
//    uint16_t level,
//    PGlobalParams gp,
//    SVAS* svas,
//    uint16_t nr_of_sections,
//    PNAME res_base_name
//);

//int PE_parseResourceDirectoryEntry(
//    uint16_t id, 
//    size_t offset, 
//    size_t table_fo, 
//    uint16_t nr_of_entries, 
//    uint16_t level,
//    PGlobalParams gp,
//    SVAS* svas,
//    uint16_t nr_of_sections,
//    PNAME res_base_name
//);

int PE_iterateImageResourceDirectory(
    size_t offset,
    size_t table_fo,
    uint16_t nr_of_named_entries,
    uint16_t nr_of_id_entries,
    uint16_t level,
    PGlobalParams gp,
    SVAS* svas,
    uint16_t nr_of_sections
);

int PE_parseResourceDirectoryEntryI(
    uint16_t id,
    size_t offset,
    size_t table_fo,
    uint16_t nr_of_entries,
    uint16_t level,
    PGlobalParams gp,
    SVAS* svas,
    uint16_t nr_of_sections, PFifo fifo,
    PNAME res_base_name
);

int PE_fillImageResourceDirectoryEntry(
    PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
    size_t offset,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
);

int PE_fillImageResourceDataEntry(
    PE_IMAGE_RESOURCE_DATA_ENTRY* de,
    size_t offset,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
);

void PE_saveResource(
    const PE_IMAGE_RESOURCE_DATA_ENTRY* de, 
    uint32_t fo, 
    PGlobalParams gp,
    PNAME res_base_name
);

char* getFileType(
    uint8_t* buffer, 
    uint32_t nr_bytes
);


/**
 * Parse ImageResourceTable, i.e. DataDirectory[RESOURCE]
 *
 * @param oh
 * @param nr_of_sections
 */
int PE_parseImageResourceTable(
    PE64OptHeader* oh,
    uint16_t nr_of_sections,
    PGlobalParams gp,
    SVAS* svas
)
{
    PE_IMAGE_RESOURCE_DIRECTORY rd;
    size_t table_fo;
    int s = 0;
    
    size_t start_file_offset = gp->file.start_offset;
    size_t file_size = gp->file.size;
    FILE* fp = gp->file.handle;
    
    uint8_t* block_s = gp->data.block_sub;

//    NAME res_base_name = {0};
//    res_base_name.MaxSize = PATH_MAX;

    
    if ( oh->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_RESOURCE )
    {
        EPrint("Data Directory too small for RESOURCE entry!\n");
        return -1;
    }

    table_fo = PE_getDataDirectoryEntryFileOffset(oh->DataDirectory, IMAGE_DIRECTORY_ENTRY_RESOURCE, nr_of_sections, "Resource", svas);
    if ( table_fo == 0 )
    {
        printf("No resource table found!\n");
        return -2;
    }

    // fill root PE_IMAGE_RESOURCE_DIRECTORY info
    s = PE_fillImageResourceDirectory(&rd, table_fo, start_file_offset, file_size, fp, block_s);
    if ( s != 0 )
        return s;
    
#ifdef DEBUG_PRINT
    PE_printImageResourceDirectory(&rd, 0);
#endif

    //PE_recurseImageResourceDirectory(
    //    table_fo + PE_RESOURCE_DIRECTORY_SIZE, 
    //    table_fo, 
    //    rd.NumberOfNamedEntries,
    //    rd.NumberOfIdEntries, 
    //    0, 
    //    gp, 
    //    svas, 
    //    nr_of_sections,
    //    &res_base_name
    //);
    
    s = PE_iterateImageResourceDirectory(
        table_fo + PE_RESOURCE_DIRECTORY_SIZE,
        table_fo,
        rd.NumberOfNamedEntries,
        rd.NumberOfIdEntries, 
        0,
        gp,
        svas, 
        nr_of_sections
    );

    printf("Resource count: 0x%zx\n", res_count);
    printf("File count: 0x%zx\n", file_count);

    return s;
}

int PE_fillImageResourceDirectory(PE_IMAGE_RESOURCE_DIRECTORY* rd,
                                  size_t offset,
                                  size_t start_file_offset,
                                  size_t file_size,
                                  FILE* fp,
                                  uint8_t* block_s)
{
    size_t size;
    uint8_t* ptr = NULL;
    struct Pe_Image_Resource_Directory_Offsets offsets = PeImageResourceDirectoryOffsets;

    if ( !checkFileSpace(offset, start_file_offset, PE_RESOURCE_DIRECTORY_SIZE, file_size))
        return -1;

    offset = offset + start_file_offset;
    size = readFile(fp, offset, BLOCKSIZE_SMALL, block_s);
    if ( size == 0 )
        return -2;
    offset = 0;

    ptr = &block_s[offset];
    
    memset(rd, 0, PE_RESOURCE_DIRECTORY_SIZE);
    rd->Characteristics = GetIntXValueAtOffset(uint32_t, ptr, offsets.Characteristics);
    rd->TimeDateStamp = GetIntXValueAtOffset(uint32_t, ptr, offsets.TimeDateStamp);
    rd->MajorVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MajorVersion);
    rd->MinorVersion = GetIntXValueAtOffset(uint16_t, ptr, offsets.MinorVersion);
    rd->NumberOfNamedEntries = GetIntXValueAtOffset(uint16_t, ptr, offsets.NumberOfNamedEntries);
    rd->NumberOfIdEntries = GetIntXValueAtOffset(uint16_t, ptr, offsets.NumberOfIdEntries);
    // follows immediately and will be iterated on its own.
//	rd->DirectoryEntries[0].Name = *((uint32_t*) &ptr[offsets.DirectoryEntries + PeImageResourceDirectoryEntryOffsets.Name]);
//	rd->DirectoryEntries[0].OffsetToData = *((uint32_t*) &ptr[offsets.DirectoryEntries + PeImageResourceDirectoryEntryOffsets.OffsetToData]);

    return 0;
}

//int PE_recurseImageResourceDirectory(
//    size_t offset,
//    size_t table_fo,
//    uint16_t nr_of_named_entries,
//    uint16_t nr_of_id_entries,
//    uint16_t level,
//    PGlobalParams gp,
//    SVAS* svas,
//    uint16_t nr_of_sections,
//    PNAME res_base_name
//)
//{
//    uint16_t i;
//    int s;
//
//    if ( level >= PE_MAX_RES_DIR_LEVEL )
//    {
//        EPrint("Maximum ressource directory level reached!\n");
//        return -1;
//    }
//    DPrint("offset: 0x%zx\n", offset);
//    DPrint("table_fo: 0x%zx\n", table_fo);
//    DPrint("file_size: 0x%zx\n", gp->file.size);
//    DPrint("level: 0x%x\n", level);
//
//    //PE_printImageResourceDirectoryEntryHeader(0, nr_of_named_entries, level);
//    for ( i = 0; i < nr_of_named_entries; i++)
//    {
//        if ( level == 0 )
//        {
//            res_base_name->Length = 0;
//            res_base_name->Buffer[0] = 0;
//        }
//
//        s = PE_parseResourceDirectoryEntry(i, offset, table_fo, nr_of_named_entries, level, gp, svas, nr_of_sections, res_base_name);
//        // break on error or try next ??
//        if ( s != 0 )
//            break;
//        
//        DPrint("offset: 0x%zx\n", offset);
//        offset += PE_RESOURCE_ENTRY_SIZE;
//        if ( offset > gp->file.size )
//        {
//            return -2;
//        }
//    }
//
//    PE_printImageResourceDirectoryEntryHeader(1, nr_of_id_entries, level);
//    for ( i = 0; i < nr_of_id_entries; i++)
//    {
//        if ( level == 0 )
//        {
//            res_base_name->Length = 0;
//            res_base_name->Buffer[0] = 0;
//        }
//
//        s = PE_parseResourceDirectoryEntry(i, offset, table_fo, nr_of_id_entries, level, gp, svas, nr_of_sections, res_base_name);
//        // break on error or try next ??
//        if ( s != 0 )
//            break;
//        
//        DPrint("offset: 0x%zx\n", offset);
//        offset += PE_RESOURCE_ENTRY_SIZE;
//        if ( offset > gp->file.size )
//        {
//            return -3;
//        }
//    }
//
//    return 0;
//}

//int PE_parseResourceDirectoryEntry(
//    uint16_t id, 
//    size_t offset, 
//    size_t table_fo, 
//    uint16_t nr_of_entries, 
//    uint16_t level,
//    PGlobalParams gp,
//    SVAS* svas,
//    uint16_t nr_of_sections,
//    PNAME res_base_name
//)
//{
//    PE_IMAGE_RESOURCE_DIRECTORY rd;
//    PE_IMAGE_RESOURCE_DIRECTORY_ENTRY re;
//    PE_IMAGE_RESOURCE_DATA_ENTRY de;
//    
//    int s;
//    size_t dir_offset = 0;
//    uint32_t fotd;
//
//    size_t start_file_offset = gp->file.start_offset;
//    size_t file_size = gp->file.size;
//    FILE* fp = gp->file.handle;
//
//    uint8_t* block_s = gp->data.block_sub;
//    
//    s = PE_fillImageResourceDirectoryEntry(&re, offset, start_file_offset, file_size, fp, block_s);
//    if ( s != 0 ) 
//        return s;
//
//    PE_printImageResourceDirectoryEntry(&re, table_fo, level, id, nr_of_entries, start_file_offset, file_size, fp, block_s);
//
//    dir_offset = table_fo + re.OFFSET_UNION.DATA_STRUCT.OffsetToDirectory;
//    if ( dir_offset > file_size )
//    {
//        EPrint("Dir offset (0x%zx) beyond file size (0x%zx)!\n", dir_offset, file_size);
//        return ERROR_DATA_BEYOND_FILE_SIZE;
//    }
//
//    if ( re.NAME_UNION.NAME_STRUCT.NameIsString )
//    {
//        if ( res_base_name->Length < res_base_name->MaxSize - 5 );
//        {
//            sprintf(&res_base_name->Buffer[res_base_name->Length], "%04x.", re.NAME_UNION.NAME_STRUCT.NameOffset);
//            res_base_name->Length += 5;
//        }
//    }
//    else
//    {
//        if ( res_base_name->Length < res_base_name->MaxSize - 3 );
//        {
//            sprintf(&res_base_name->Buffer[res_base_name->Length], "%02x.", re.NAME_UNION.Id);
//            res_base_name->Length += 3;
//        }
//    }
//
//    if ( re.OFFSET_UNION.DATA_STRUCT.DataIsDirectory )
//    {
//        s = PE_fillImageResourceDirectory(&rd, dir_offset, start_file_offset, file_size, fp, block_s);
//        if ( s != 0 )
//            return -2;
//        PE_printImageResourceDirectory(&rd, level+1);
//
//        dir_offset = (size_t)dir_offset + PE_RESOURCE_DIRECTORY_SIZE;
//        if ( dir_offset > file_size )
//        {
//            EPrint("next dir offset (0x%zx) beyond file size (0x%zx)!\n", dir_offset, file_size);
//            return ERROR_DATA_BEYOND_FILE_SIZE;
//        }
//
//        s = PE_recurseImageResourceDirectory(dir_offset, table_fo, rd.NumberOfNamedEntries,
//                                        rd.NumberOfIdEntries, level + 1, gp, svas, nr_of_sections, res_base_name);
//        if ( s != 0 )
//            return s;
//    }
//    else
//    {
//        s = PE_fillImageResourceDataEntry(&de, dir_offset, start_file_offset, file_size, fp, block_s);
//        if ( s != 0 ) 
//            return s;
//        fotd = (uint32_t)PE_Rva2Foa(de.OffsetToData, svas, nr_of_sections);
//        fotd += (uint32_t)start_file_offset;
//        PE_printImageResourceDataEntry(&de, fotd, dir_offset, level);
//        
//        printf("res_base_name: %s\n", res_base_name->Buffer);
//        PE_saveResource(&re, &de, fotd, gp, res_base_name);
//    }
//    
//    return 0;
//}

int PE_fillImageResourceDirectoryEntry(
    PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
    size_t offset,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
)
{
    struct Pe_Image_Resource_Directory_Entry_Offsets entry_offsets = PeImageResourceDirectoryEntryOffsets;
    uint8_t* ptr = NULL;
    size_t size;

    if ( !checkFileSpace(offset, start_file_offset, PE_RESOURCE_ENTRY_SIZE, file_size))
        return -1;

    offset += start_file_offset;
    size = readFile(fp, offset, BLOCKSIZE_SMALL, block_s);
    if ( size == 0 )
        return -2;

    ptr = block_s;

    memset(re, 0, PE_RESOURCE_ENTRY_SIZE);
    re->NAME_UNION.Name = GetIntXValueAtOffset(uint32_t, ptr, entry_offsets.Name);
    re->OFFSET_UNION.OffsetToData = GetIntXValueAtOffset(uint32_t, ptr, entry_offsets.OffsetToData);

    return 0;
}

int PE_fillImageResourceDataEntry(PE_IMAGE_RESOURCE_DATA_ENTRY* de,
                                  size_t offset,
                                  size_t start_file_offset,
                                  size_t file_size,
                                  FILE* fp,
                                  uint8_t* block_s)
{
    uint8_t* ptr;
    size_t size;
    
    if ( !checkFileSpace(offset, start_file_offset, PE_RESOURCE_DATA_ENTRY_SIZE, file_size) )
        return -1;
    
    offset += start_file_offset;
    size = readFile(fp, offset, BLOCKSIZE_SMALL, block_s);
    if ( size == 0 )
        return -2;
    
    ptr = block_s;

    memset(de, 0, PE_RESOURCE_ENTRY_SIZE);
    de->OffsetToData = GetIntXValueAtOffset(uint32_t, ptr, PeImageResourceDataEntryOffsets.OffsetToData);
    de->Size = GetIntXValueAtOffset(uint32_t, ptr, PeImageResourceDataEntryOffsets.Size);
    de->CodePage = GetIntXValueAtOffset(uint32_t, ptr, PeImageResourceDataEntryOffsets.CodePage);
    de->Reserved = GetIntXValueAtOffset(uint32_t, ptr, PeImageResourceDataEntryOffsets.Reserved);

    return 0;
}

typedef struct _RDI_DATA {
    size_t Offset;
    uint16_t NumberOfNamedEntries;
    uint16_t NumberOfIdEntries;
    uint16_t Level;
    NAME ResName;
} RDI_DATA, *PRDI_DATA;

int PE_iterateImageResourceDirectory(
    size_t offset,
    size_t table_fo,
    uint16_t
    nr_of_named_entries,
    uint16_t nr_of_id_entries,
    uint16_t level,
    PGlobalParams gp,
    SVAS* svas,
    uint16_t nr_of_sections
)
{
    uint16_t i;
    int s;
    Fifo fifo;
    RDI_DATA rdid;
    PRDI_DATA act;
    PFifoEntryData act_e;

    Fifo_init(&fifo);

    memset(&rdid, 0, sizeof(rdid));

    rdid.Offset = (size_t)offset;
    rdid.NumberOfNamedEntries = nr_of_named_entries;
    rdid.NumberOfIdEntries = nr_of_id_entries;
    rdid.Level = level;
    rdid.ResName.MaxSize = BASE_NAME_MAX_SIZE;

    Fifo_push(&fifo, &rdid, sizeof(RDI_DATA));

    while ( !Fifo_empty(&fifo) )
    {
        act_e = Fifo_front(&fifo);
        act = (PRDI_DATA)act_e->bytes;

        offset = act->Offset;
        nr_of_named_entries = act->NumberOfNamedEntries;
        nr_of_id_entries = act->NumberOfIdEntries;
        level = act->Level;

        //printf("act.name->Length: 0x%x\n", act->ResName.Length);
        //printf("act.name->MaxSize: 0x%x\n", act->ResName.MaxSize);
        //printf("act.name->Buffer: %s (%p)\n", act->ResName.Buffer, act->ResName.Buffer);
        
#ifdef DEBUG_PRINT
        PE_printImageResourceDirectoryEntryHeader(0, nr_of_named_entries, level);
#endif
        for ( i = 0; i < nr_of_named_entries; i++ )
        {
            s = PE_parseResourceDirectoryEntryI(i, offset, table_fo, nr_of_named_entries, level, gp, svas, nr_of_sections, &fifo, &act->ResName);
            if ( s != 0 )
                continue;

            offset += PE_RESOURCE_ENTRY_SIZE;
        }
        
#ifdef DEBUG_PRINT
        PE_printImageResourceDirectoryEntryHeader(1, nr_of_id_entries, level);
#endif
        for ( i = 0; i < nr_of_id_entries; i++ )
        {
            s = PE_parseResourceDirectoryEntryI(i, offset, table_fo, nr_of_id_entries, level, gp, svas, nr_of_sections, &fifo, &act->ResName);
            if ( s != 0 )
                continue;

            offset += PE_RESOURCE_ENTRY_SIZE;
        }

        Fifo_pop_front(&fifo);
    }

    return 0;
}

int PE_parseResourceDirectoryEntryI(
    uint16_t id,
    size_t offset,
    size_t table_fo,
    uint16_t nr_of_entries,
    uint16_t level,
    PGlobalParams gp,
    SVAS* svas,
    uint16_t nr_of_sections,
    PFifo fifo,
    PNAME res_base_name
)
{
    PE_IMAGE_RESOURCE_DIRECTORY rd;
    PE_IMAGE_RESOURCE_DIRECTORY_ENTRY re;
    PE_IMAGE_RESOURCE_DATA_ENTRY de;
    RDI_DATA rdid;

    int s;
    uint32_t dir_offset = 0;
    uint32_t fotd;
    
    size_t start_file_offset = gp->file.start_offset;
    size_t file_size = gp->file.size; 
    FILE* fp = gp->file.handle; 
    uint8_t* block_s = gp->data.block_sub;

    (id);(nr_of_entries);

    PE_fillImageResourceDirectoryEntry(&re, offset, start_file_offset, file_size, fp, block_s);
#ifdef DEBUG_PRINT
    PE_printImageResourceDirectoryEntry(&re, table_fo, level, id, nr_of_entries, start_file_offset, file_size, fp, block_s);
#endif
    
    //memset(&rdid, 0, sizeof(rdid));

    //printf("res_base_name->Length: 0x%x\n", res_base_name->Length);
    //printf("res_base_name->MaxSize: 0x%x\n", res_base_name->MaxSize);
    //printf("res_base_name->Buffer: %s (%p)\n", res_base_name->Buffer, res_base_name->Buffer);

    memset(&rdid, 0, sizeof(rdid));
    rdid.ResName.Length = res_base_name->Length;
    rdid.ResName.MaxSize = res_base_name->MaxSize;
    if ( re.NAME_UNION.NAME_STRUCT.NameIsString )
    {
        if ( rdid.ResName.Length + 10 < rdid.ResName.MaxSize )
        {
            sprintf(rdid.ResName.Buffer, "%s%08x.", res_base_name->Buffer, re.NAME_UNION.NAME_STRUCT.NameOffset);
            rdid.ResName.Length += 5;
        }
    }
    else
    {
        if ( rdid.ResName.Length + 5 < rdid.ResName.MaxSize )
        {
            sprintf(rdid.ResName.Buffer, "%s%04x.", res_base_name->Buffer, re.NAME_UNION.Id);
            rdid.ResName.Length += 3;
        }
    }
    //printf("rdid.ResName.Length: 0x%x\n", rdid.ResName.Length);
    //printf("rdid.ResName.MaxSize: 0x%x\n", rdid.ResName.MaxSize);
    //printf("rdid.ResName.Buffer: %s (%p)\n", rdid.ResName.Buffer, rdid.ResName.Buffer);

    dir_offset = (uint32_t)table_fo + re.OFFSET_UNION.DATA_STRUCT.OffsetToDirectory;

    if ( re.OFFSET_UNION.DATA_STRUCT.DataIsDirectory )
    {
        s = PE_fillImageResourceDirectory(&rd, dir_offset, start_file_offset, file_size, fp, block_s);
        if ( s != 0 )
            return 1;
#ifdef DEBUG_PRINT
        PE_printImageResourceDirectory(&rd, level+1);
#endif
        rdid.Offset = (size_t)dir_offset + PE_RESOURCE_DIRECTORY_SIZE;
        rdid.NumberOfNamedEntries = rd.NumberOfNamedEntries;
        rdid.NumberOfIdEntries = rd.NumberOfIdEntries;
        rdid.Level = level + 1;

        Fifo_push(fifo, &rdid, sizeof(RDI_DATA));

    }
    else
    {
        s = PE_fillImageResourceDataEntry(&de, dir_offset, start_file_offset, file_size, fp, block_s);
        if ( s != 0 ) 
            return s;
        fotd = (uint32_t)PE_Rva2Foa(de.OffsetToData, svas, nr_of_sections);
        fotd += (uint32_t)start_file_offset;
#ifdef DEBUG_PRINT
        PE_printImageResourceDataEntry(&de, fotd, level);
#endif
        
        res_count++;

        //printf("final res base name: %s\n", rdid.ResName.Buffer);
        PE_saveResource(&de, fotd, gp, &rdid.ResName);
    }

    return 0;
}


void PE_saveResource(
    const PE_IMAGE_RESOURCE_DATA_ENTRY* de, 
    uint32_t fo, 
    PGlobalParams gp,
    PNAME res_base_name
)
{
    uint8_t* buffer = gp->data.block_main;
    uint32_t buffer_size = gp->data.block_main_size;
    uint32_t i;
    uint32_t parts;
    uint32_t rest;
    size_t nr_bytes;
    char path[PATH_MAX];
    FILE* out_file = NULL;
    char* file_type = NULL;
    
    FPrint();
    DPrint("  fo: 0x%x\n", fo);
    DPrint("  Size: 0x%x\n", de->Size);

    if ( fo > gp->file.size || fo + de->Size > gp->file.size )
    {
        EPrint("Resource data invalid!\n");
        return;
    }
    
    nr_bytes = readFile(gp->file.handle, fo, buffer_size, buffer);
    if ( !nr_bytes )
    {
        EPrint("Reading resource failed.\n");
        return;
    }
    file_type = getFileType(buffer, (uint32_t)nr_bytes);
    DPrint("file_type: %s\n", file_type);

    if ( strlen(gp->outDir) + 1 + res_base_name->Length + strlen(file_type) >= PATH_MAX )
    {
        EPrint("path to long!\n");
        return;
    }
    sprintf(path, "%s%c%s%s", gp->outDir, PATH_SEPARATOR, res_base_name->Buffer, file_type);
    
    out_file = fopen(path, "wb");
    if ( !out_file )
    {
        EPrint("Creating \"%s\" failed!\n", path);
        return;
    }
    DPrint("Created res file \"%s\"\n", path);
    file_count++;

    parts = de->Size / buffer_size;
    rest = de->Size % buffer_size;

    for ( i = 0; i < parts; i++ )
    {
        nr_bytes = readFile(gp->file.handle, fo, buffer_size, buffer);
        if ( !nr_bytes )
        {
            EPrint("Reading resource failed.\n");
            goto clean;
        }

        nr_bytes = fwrite(buffer, 1, nr_bytes, out_file);
        if ( !nr_bytes )
        {
            EPrint("Writing resource failed.\n");
            goto clean;
        }
        DPrint("0x%zx bytes written\n", nr_bytes);

        fo += buffer_size;
    }

    if ( rest > 0 )
    {
        nr_bytes = readFile(gp->file.handle, fo, rest, buffer);
        if ( !nr_bytes )
        {
            EPrint("Reading resource failed.\n");
            goto clean;
        }

        nr_bytes = fwrite(buffer, 1, nr_bytes, out_file);
        if ( !nr_bytes )
        {
            EPrint("Writing resource failed.\n");
            goto clean;
        }
        DPrint("0x%zx bytes written\n", nr_bytes);
    }
    printf("\"%s\" written.\n", path);

clean:
    if ( out_file )
        fclose(out_file);
}

char* getFileType(uint8_t* buffer, uint32_t buffer_size)
{
    FPrint();
    if ( buffer_size > 0x100 && *(uint16_t*)&buffer[0] == *(uint16_t*)&MAGIC_PE_BYTES[0] )
    {
        return "pe";
    }
    else if ( 
              ( buffer_size > 0x10 && *(uint32_t*)&buffer[1] == 0x6c6d783f ) // [<]?xml
            || ( buffer_size > 0x10 && *(uint32_t*)&buffer[0] == 0x7373613c && *(uint32_t*)&buffer[4] == 0x6c626d65 ) // <assembl[y]
            )
    {
        return "xml";
    }
    else if ( buffer_size > 0x10 && *(uint32_t*)&buffer[0] == 0x00010000)
    {
        return "ico";
    }
    else if ( buffer_size > 0x30 
              && *(uint64_t*)&buffer[0x06] == 0x0056005f00530056 
              && *(uint64_t*)&buffer[0x0E] == 0x0049005300520045 
              && *(uint64_t*)&buffer[0x16] == 0x0049005f004e004f 
              && *(uint32_t*)&buffer[0x1E] == 0x0046004e )
    {
        return "vsi";
    }
    else
    {
        return "res";
    }
}

#endif
