#include <stdlib.h>
#include <stdio.h>
#include <io.h>

const char* getReadableSize(unsigned long long bytes) {
    static char output[64]; /* static buffer (safe if not used concurrently) */

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


const char* getFilename(const char* filename) {
    static char output[64]; /* static buffer (safe if not used concurrently) */
    
    uint32_t len = strlen(filename);
    if (len>=sizeof(output)) {
        return "";
    }
    int64_t i = len-1;
    uint32_t s = 0;
    for (; i >= 0; i--) {
        if (filename[i]=='/' || filename[i]=='\\') {
            s = i+1;
            break;
        }
    }
    i = s;
    s = 0;
    for (; i < len; ++i) {
        output[s] = filename[i];
        ++s;
    }
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

void loadFile(const char* fileName, unsigned char** dataOut, size_t* fileSizeOut) {
	FILE* fptr = NULL;
	fopen_s(&fptr, fileName, "rb");
	if (fptr == NULL) {
		return;
	}

    /* get file size */
    _fseeki64(fptr, 0, SEEK_END);
    *fileSizeOut= _ftelli64(fptr);
    rewind(fptr);

    U8* fileData = NULL;

    if (*fileSizeOut <= 0) {
        return;
    }

    fileData = malloc(*fileSizeOut + 1);

    if (fileData == NULL) {
        fclose(fptr);
        return;
    }


	fread_s(fileData, *fileSizeOut, *fileSizeOut, 1, fptr);
    if (fptr)
        fclose(fptr);

    fileData[*fileSizeOut] = '\0';

    *dataOut = fileData;
}