#include <stdlib.h>
#include <stdio.h>
#include <io.h>

const char* getReadableSize(unsigned long long bytes) {
    static char output[64]; // static buffer (safe if not used concurrently)

    const char* sizes[] = { "Bytes", "KB", "MB", "GB", "TB" };
    int unit = 0;
    double size = (double)bytes;

    while (size >= 1024 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    snprintf(output, sizeof(output), "%.2f %s", size, sizes[unit]);
    return output;
}

unsigned long long getFileSize(const char* file)
{

    FILE* fptr = NULL;
    fopen_s(&fptr, file, "rb");
    if (fptr == NULL) {
        return 0u;
    }

    /* get file size */
    _fseeki64(fptr, 0, SEEK_END);
    const long long fsze = _ftelli64(fptr);
    fclose(fptr);

    return fsze;
}
