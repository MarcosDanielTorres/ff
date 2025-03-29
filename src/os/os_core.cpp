
#include "os/win32/os_win32.cpp"

OS_FileReadResult os_file_read(Arena* arena, const char* filename) 
{
    OS_FileReadResult result = {0};

    FILE* file = fopen(filename, "rb");
    if (file)  
    {
        fseek(file, 0, SEEK_END);
        size_t filesize = ftell(file);
        fseek(file, 0,SEEK_SET);

        u8* buffer = (u8*) arena_push_size(arena, u8, filesize + 1);
        fread(buffer, 1, filesize, file);

        buffer[filesize] = '\0';

        result.data = buffer;
        result.size = filesize;
        fclose(file);
    }
    return result;
}
