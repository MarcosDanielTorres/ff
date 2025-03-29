#pragma once
#include "os/win32/os_win32.h"


struct OS_SystemInfo
{
    size_t page_size;
    size_t large_page_size;
};

struct OS_FileReadResult 
{
    u8* data;
    size_t size;
};

OS_FileReadResult os_file_read(Arena* arena, const char* filename); 