#ifndef _STRUCTS
#define _STRUCTS

typedef struct _RTParams {
    // dynamic
	struct _data {
        uint8_t* block_main;
        uint32_t block_main_size;
        uint8_t* block_sub;
        uint32_t block_sub_size;
    } data;
    
    const char* out_dir;

    HPFile file;
    size_t offset;
    uint32_t flags;
} RTParams, *PRTParams;

#endif
