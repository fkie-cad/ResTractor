#ifndef RESTRACTOR_PE_IMAGE_RESOURCE_TABLE_H
#define RESTRACTOR_PE_IMAGE_RESOURCE_TABLE_H

#include "../../utils/Strings.h"
#include "../../utils/fifo/Fifo.h"



#define PE_MAX_RES_DIR_LEVEL (0x20)

#define BASE_NAME_MAX_SIZE (0x200)

typedef struct _NAME {
    uint32_t Length;
    uint32_t MaxSize;
    char Buffer[BASE_NAME_MAX_SIZE];
} NAME, *PNAME;

typedef struct _RDI_DATA {
    size_t Offset;
    uint16_t NumberOfNamedEntries;
    uint16_t NumberOfIdEntries;
    uint16_t Level;
    NAME ResName;
} RDI_DATA, *PRDI_DATA;

size_t res_count = 0;
size_t file_count = 0;

int parseImageResourceTable(
    PE64OptHeader* oh,
    uint16_t nr_of_sections,
    PRTParams rtp,
    SVAS* svas
);

int fillImageResourceDirectory(
    PE_IMAGE_RESOURCE_DIRECTORY* rd,
    size_t offset,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
);

int iterateImageResourceDirectory(
    size_t offset,
    size_t table_fo,
    uint16_t nr_of_named_entries,
    uint16_t nr_of_id_entries,
    uint16_t level,
    PRTParams rtp,
    SVAS* svas,
    uint16_t nr_of_sections
);

int parseResourceDirectoryEntryI(
    uint16_t id,
    size_t offset,
    size_t table_fo,
    uint16_t nr_of_entries,
    uint16_t level,
    PRTParams rtp,
    SVAS* svas,
    uint16_t nr_of_sections, PFifo fifo,
    PNAME res_base_name
);

int fillImageResourceDirectoryEntry(
    PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
    size_t offset,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
);

int fillImageResourceDataEntry(
    PE_IMAGE_RESOURCE_DATA_ENTRY* de,
    size_t offset,
    size_t start_file_offset,
    size_t file_size,
    FILE* fp,
    uint8_t* block_s
);

int getResName(
    const PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
    size_t table_fo,
    PRTParams rtp,
    PE_IMAGE_RESOURCE_DIR_STRING_U_PTR* name
);

void saveResource(
    const PE_IMAGE_RESOURCE_DATA_ENTRY* de, 
    uint32_t fo, 
    PRTParams rtp,
    PNAME res_base_name
);

char* getFileType(
    uint8_t* buffer, 
    uint32_t buffer_size
);


/**
 * Parse ImageResourceTable, i.e. DataDirectory[RESOURCE]
 *
 * @param oh
 * @param nr_of_sections
 */
int parseImageResourceTable(
    PE64OptHeader* oh,
    uint16_t nr_of_sections,
    PRTParams rtp,
    SVAS* svas
)
{
    PE_IMAGE_RESOURCE_DIRECTORY rd;
    size_t table_fo;
    int s = 0;
    
    size_t start_file_offset = rtp->file.start_offset;
    size_t file_size = rtp->file.size;
    FILE* fp = rtp->file.handle;
    
    uint8_t* block_s = rtp->data.block_sub;

    if ( oh->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_RESOURCE )
    {
        EPrint("Data Directory too small for RESOURCE entry!\n");
        return -1;
    }

    table_fo = PE_getDataDirectoryEntryFileOffset(oh->DataDirectory, IMAGE_DIRECTORY_ENTRY_RESOURCE, nr_of_sections, "Resource", svas);
    if ( table_fo == 0 )
    {
        printf("No resource table found!\n");
        return 1;
    }

    // fill root PE_IMAGE_RESOURCE_DIRECTORY info
    s = fillImageResourceDirectory(&rd, table_fo, start_file_offset, file_size, fp, block_s);
    if ( s != 0 )
        return s;

    if ( rtp->flags&FLAG_PRINT )
        PE_printImageResourceDirectory(&rd, rtp->offset, 0);

    s = iterateImageResourceDirectory(
        table_fo + PE_RESOURCE_DIRECTORY_SIZE,
        table_fo,
        rd.NumberOfNamedEntries,
        rd.NumberOfIdEntries, 
        0,
        rtp,
        svas, 
        nr_of_sections
    );

    printf("Resource count: 0x%zx\n", res_count);
    printf("File count: 0x%zx\n", file_count);

    return s;
}

int fillImageResourceDirectory(PE_IMAGE_RESOURCE_DIRECTORY* rd,
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

int fillImageResourceDirectoryEntry(
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

int fillImageResourceDataEntry(PE_IMAGE_RESOURCE_DATA_ENTRY* de,
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

int iterateImageResourceDirectory(
    size_t offset,
    size_t table_fo,
    uint16_t nr_of_named_entries,
    uint16_t nr_of_id_entries,
    uint16_t level,
    PRTParams rtp,
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
        
        if ( rtp->flags&FLAG_PRINT )
            PE_printImageResourceDirectoryEntryHeader(0, nr_of_named_entries, level);

        for ( i = 0; i < nr_of_named_entries; i++ )
        {
            s = parseResourceDirectoryEntryI(i, offset, table_fo, nr_of_named_entries, level, rtp, svas, nr_of_sections, &fifo, &act->ResName);
            if ( s != 0 )
                continue;

            offset += PE_RESOURCE_ENTRY_SIZE;
        }
        
        if ( rtp->flags&FLAG_PRINT )
            PE_printImageResourceDirectoryEntryHeader(1, nr_of_id_entries, level);

        for ( i = 0; i < nr_of_id_entries; i++ )
        {
            s = parseResourceDirectoryEntryI(i, offset, table_fo, nr_of_id_entries, level, rtp, svas, nr_of_sections, &fifo, &act->ResName);
            if ( s != 0 )
                continue;

            offset += PE_RESOURCE_ENTRY_SIZE;
        }

        Fifo_pop_front(&fifo);
    }

    return 0;
}

int parseResourceDirectoryEntryI(
    uint16_t id,
    size_t offset,
    size_t table_fo,
    uint16_t nr_of_entries,
    uint16_t level,
    PRTParams rtp,
    SVAS* svas,
    uint16_t nr_of_sections,
    PFifo fifo,
    PNAME res_base_name
)
{
    PE_IMAGE_RESOURCE_DIRECTORY rd;
    PE_IMAGE_RESOURCE_DIRECTORY_ENTRY re;
    PE_IMAGE_RESOURCE_DATA_ENTRY de;
    PE_IMAGE_RESOURCE_DIR_STRING_U_PTR name;
    RDI_DATA rdid;

    int s;
    uint32_t dir_offset = 0;
    uint32_t fotd;
    
    size_t start_file_offset = rtp->file.start_offset;
    size_t file_size = rtp->file.size; 
    FILE* fp = rtp->file.handle; 
    uint8_t* block_s = rtp->data.block_sub;

    (id);(nr_of_entries);

    fillImageResourceDirectoryEntry(&re, offset, start_file_offset, file_size, fp, block_s);
    if ( rtp->flags&FLAG_PRINT )
        PE_printImageResourceDirectoryEntry(&re, table_fo, rtp->offset, level, id, nr_of_entries, start_file_offset, file_size, fp, block_s);
    
    memset(&rdid, 0, sizeof(rdid));
    rdid.ResName.Length = res_base_name->Length;
    rdid.ResName.MaxSize = res_base_name->MaxSize;
    if ( re.NAME_UNION.NAME_STRUCT.NameIsString )
    {
        s = getResName(
                &re, 
                table_fo, 
                rtp,
                &name
            );
        if ( s != 0 )
            return -1;

        if ( rdid.ResName.Length + name.Length+1 < rdid.ResName.MaxSize )
        {
#ifdef _WIN32
            sprintf(rdid.ResName.Buffer, "%s%.*ws.", res_base_name->Buffer, name.Length, name.NameString);
#else
            int cch = sprintf(rdid.ResName.Buffer, "%s", res_base_name->Buffer);
            uint16_t i;
            char* nameBuffer = &rdid.ResName.Buffer[cch];
            for ( i = 0; i < name.Length; i++ )
                nameBuffer[i] = (char)name.NameString[i];
            nameBuffer[i] = '.';
#endif
            rdid.ResName.Length += name.Length+1;

            s = sanitizePathA(rdid.ResName.Buffer, rdid.ResName.Length, rdid.ResName.Buffer, rdid.ResName.Length);
            if ( s != 0 )
            {
                return -2;
            }
        }
        //if ( rdid.ResName.Length + 10 < rdid.ResName.MaxSize )
        //{
        //    sprintf(rdid.ResName.Buffer, "%s%08x.", res_base_name->Buffer, re.NAME_UNION.NAME_STRUCT.NameOffset);
        //    rdid.ResName.Length += 5;
        //}
    }
    else
    {
        if ( rdid.ResName.Length + 5 < rdid.ResName.MaxSize )
        {
            sprintf(rdid.ResName.Buffer, "%s%04x.", res_base_name->Buffer, re.NAME_UNION.Id);
            rdid.ResName.Length += 5;
        }
    }
    //printf("rdid.ResName.Length: 0x%x\n", rdid.ResName.Length);
    //printf("rdid.ResName.MaxSize: 0x%x\n", rdid.ResName.MaxSize);
    //printf("rdid.ResName.Buffer: %s (%p)\n", rdid.ResName.Buffer, rdid.ResName.Buffer);

    dir_offset = (uint32_t)table_fo + re.OFFSET_UNION.DATA_STRUCT.OffsetToDirectory;

    if ( re.OFFSET_UNION.DATA_STRUCT.DataIsDirectory )
    {
        s = fillImageResourceDirectory(&rd, dir_offset, start_file_offset, file_size, fp, block_s);
        if ( s != 0 )
            return 1;

        if ( rtp->flags&FLAG_PRINT )
            PE_printImageResourceDirectory(&rd, rtp->offset, level+1);

        rdid.Offset = (size_t)dir_offset + PE_RESOURCE_DIRECTORY_SIZE;
        rdid.NumberOfNamedEntries = rd.NumberOfNamedEntries;
        rdid.NumberOfIdEntries = rd.NumberOfIdEntries;
        rdid.Level = level + 1;

        Fifo_push(fifo, &rdid, sizeof(RDI_DATA));
    }
    else
    {
        s = fillImageResourceDataEntry(&de, dir_offset, start_file_offset, file_size, fp, block_s);
        if ( s != 0 ) 
            return s;
        fotd = (uint32_t)PE_Rva2Foa(de.OffsetToData, svas, nr_of_sections);
        fotd += (uint32_t)start_file_offset;

        if ( rtp->flags&FLAG_PRINT )
            PE_printImageResourceDataEntry(&de, fotd, rtp->offset, level);
        
        res_count++;

        //printf("final res base name: %s\n", rdid.ResName.Buffer);
        if ( rtp->out_dir )
            saveResource(&de, fotd, rtp, &rdid.ResName);
    }

    return 0;
}

int getResName(
    const PE_IMAGE_RESOURCE_DIRECTORY_ENTRY* re,
    size_t table_fo,
    PRTParams rtp,
    PE_IMAGE_RESOURCE_DIR_STRING_U_PTR* name
)
{
    size_t name_offset = 0;
    size_t bytes_read = 0;
    uint8_t* ptr = NULL;

    const struct Pe_Image_Resource_Dir_String_U_Offsets *name_offsets = &PeImageResourceDirStringUOffsets;

    memset(name, 0, sizeof(*name));

    name_offset = table_fo + re->NAME_UNION.NAME_STRUCT.NameOffset;
    if ( !checkFileSpace(name_offset, rtp->file.start_offset, 4, rtp->file.size) )
    {
        EPrint("Resource name offset beyond file bounds!\n");
        return -1;
    }

    name_offset = name_offset + rtp->file.start_offset;
    bytes_read = readFile(rtp->file.handle, (size_t)name_offset, BLOCKSIZE_SMALL, rtp->data.block_sub);
    if ( bytes_read <= 4 )
        return -1;

    ptr = rtp->data.block_sub;
    name->Length = GetIntXValueAtOffset(uint16_t, ptr, name_offsets->Length);
    name->NameString = ((uint16_t*) &ptr[name_offsets->NameString]);
    if ( name->Length > (uint16_t)bytes_read - 4 ) // minus length - L'0'
        name->Length = (uint16_t)bytes_read-4;
    ptr[bytes_read-2] = 0;
    ptr[bytes_read-1] = 0;

    if ( !checkFileSpace(name_offset, rtp->file.start_offset, name_offsets->Length+name->Length, rtp->file.size))
    {
        EPrint("Resource name beyond file bounds!\n");
        return -1;
    }

    return 0;
}

void saveResource(
    const PE_IMAGE_RESOURCE_DATA_ENTRY* de, 
    uint32_t fo, 
    PRTParams rtp,
    PNAME res_base_name
)
{
    uint8_t* buffer = rtp->data.block_main;
    uint32_t buffer_size = rtp->data.block_main_size;
    uint32_t i;
    uint32_t parts;
    uint32_t rest;
    size_t nr_bytes;
    char* path = NULL;
    size_t path_cb = 0;
    size_t path_buffer_size = 0;
    FILE* out_file = NULL;
    char* file_type = NULL;
    
    FPrint();
    DPrint("  fo: 0x%x\n", fo);
    DPrint("  Size: 0x%x\n", de->Size);

    if ( fo > rtp->file.size || fo + de->Size > rtp->file.size )
    {
        EPrint("Resource data invalid!\n");
        return;
    }
    
    nr_bytes = readFile(rtp->file.handle, fo, buffer_size, buffer);
    if ( !nr_bytes )
    {
        EPrint("Reading resource failed.\n");
        return;
    }
    file_type = getFileType(buffer, (uint32_t)nr_bytes);
    DPrint("file_type: %s\n", file_type);

    path_cb = strlen(rtp->out_dir) + 1 + res_base_name->Length + strlen(file_type);
    path_buffer_size = path_cb + 1 + 17; // terminating 0 + possible duplication index : ~ + sizeof(uint64)*2
    path = (char*)malloc(path_buffer_size);
    if ( !path )
    {
        EPrint("Allocating path buffer failed!");
        return;
    }

    sprintf(path, "%s%c%s%s", rtp->out_dir, PATH_SEPARATOR, res_base_name->Buffer, file_type);
    
    // check if file exists
    size_t index = 0;
    while ( fileExists(path) )
    {
        DPrint("File \"%s\" exists.\n", path);

        // add index to path and check again
        if ( index < SIZE_MAX )
            index++;
        else
        {
            EPrint("Maximum index reached!\n");
            goto clean;
        }
        sprintf(&path[path_cb], "~%zx", index);
        DPrint("New name: \"%s\"\n", path);
    }

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
        nr_bytes = readFile(rtp->file.handle, fo, buffer_size, buffer);
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
        nr_bytes = readFile(rtp->file.handle, fo, rest, buffer);
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
    printf("Extracted \"%s\".\n", path);

clean:
    if ( out_file )
        fclose(out_file);

    if ( path )
        free(path);
}

char* getFileType(uint8_t* buffer, uint32_t buffer_size)
{
    FPrint();

    if ( buffer_size > 0x20 && *(uint32_t*)&buffer[0] == 0x46464952 && *(uint32_t*)&buffer[8] == 0x20495641 )
    {
        return "avi";
    }
    else if ( buffer_size > 0x10 &&
             *(uint32_t*)&buffer[0] == 0x424D4F46 ) // B M O F
    {
        return "bmf"; // binary MOF Data file
    }
    else if ( buffer_size > 0x50 && *(uint64_t*)&buffer[0] == 0xE11AB1A1E011CFD0 )
    {
        // Compound File Binary Format, a container format defined by Microsoft COM. 
        // It can contain the equivalent of files and directories. 
        // It is used by Windows Installer and for documents in older versions of Microsoft Office.
        // It can be used by other programs as well that rely on the COM and OLE API's.
        // .doc, .xls, .ppt, .msi, .msg
        return "cfm";
    }
    else if ( buffer_size > 0x10 && 
             *(uint32_t*)&buffer[0] == 0x38464947 &&
            ( *(uint16_t*)&buffer[4] == 0x6137 || *(uint16_t*)&buffer[4] == 0x6139 ) )
    {
        return "gif";
    }
    else if ( buffer_size > 0x10 && *(uint32_t*)&buffer[0] == 0x00010000)
    {
        return "ico";
    }
    else if ( buffer_size > 0x10 && *(uint32_t*)&buffer[0] == 0x4643534D  )
    {
        return "mcsv";
    }
    else if ( buffer_size > 0x100 && *(uint16_t*)&buffer[0] == *(uint16_t*)&MAGIC_PE_BYTES[0] )
    {
        return "pe";
    }
    else if ( buffer_size > 0x10 && *(uint64_t*)&buffer[0] == 0x0A1A0A0D474E5089  ) // .png
    {
        return "png";
    }
    else if ( buffer_size > 0x10 
              && *(uint64_t*)&buffer[0x00] == 0x0D3E454C5954533C ) // '<STYLE> '
    {
        return "style";
    }
    else if ( buffer_size > 0x30 
              && *(uint64_t*)&buffer[0x06] == 0x0056005f00530056 
              && *(uint64_t*)&buffer[0x0E] == 0x0049005300520045 
              && *(uint64_t*)&buffer[0x16] == 0x0049005f004e004f 
              && *(uint32_t*)&buffer[0x1E] == 0x0046004e )
    {
        return "vsi";
    }
    else if ( buffer_size > 0x20 && *(uint32_t*)&buffer[0] == 0x46464952 && *(uint32_t*)&buffer[8] == 0x45564157 )
    {
        return "wav";
    }
    else if ( buffer_size > 0x10 && 
              *(uint32_t*)&buffer[0] == 0x4D495243 )
    {
        return "wevt";  // windows event template
    }
    else if (
              ( buffer_size > 0x10 && *(uint32_t*)&buffer[1] == 0x6c6d783f ) // [<]?xml
            || ( buffer_size > 0x10 && *(uint32_t*)&buffer[3] == 0x6c6d783f ) // [<]?xml
            || ( buffer_size > 0x10 && *(uint32_t*)&buffer[4] == 0x6c6d783f ) // [<]?xml
            || ( buffer_size > 0x10 && *(uint64_t*)&buffer[0] == 0x6c626d657373613c ) // <assembl[y]
//            || ( buffer_size > 0x10 && *(uint32_t*)&buffer[0] == 0x7373613c && *(uint32_t*)&buffer[4] == 0x6c626d65 ) // <assembl[y]
            )
    {
        return "xml";
    }
    else if (
              buffer_size > 0x10 
              && *(uint64_t*)&buffer[0x00] == 0x0078003f003cfeff // ..<.?.x.
              && *(uint64_t*)&buffer[0x08] == 0x00760020006c006d // m.l. .v.
            )
    {
        return "xsl";
    }
    else
    {
        return "res";
    }
}

#endif
