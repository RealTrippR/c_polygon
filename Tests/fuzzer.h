#ifndef DEBUG_FUZZER_H
#define DEBUG_FUZZER_H

#include <stdlib.h>
#include <time.h>

#include "ISAAC/isaac64.h"
#include <assert.h>

void clearAndWriteToFile(const char* filename, const char* data, uint64_t dataLen)
{
    FILE* fptr;
    fopen_s(&fptr, filename, "wb");


    if (fptr) {
        fwrite(data, dataLen, 1, fptr);
        fclose(fptr);
    }

}

void l4tohexstr(ub4 value, char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size <= 1) {
        if (buffer != NULL && buffer_size == 1 )
        {
            buffer[0] = '\0';
        }
        return;
    }

    // buffer is assured to be at least 2 bytes
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
       
        return;
    }

    size_t i = buffer_size - 1;
    buffer[i] = '\0';
    i--;

    // Convert digits in reverse order
    while (value > 0) {
        int digit = value % 16;
        if (digit < 10) {
            buffer[i--] = (char)(digit + (int)'0');
        }
        else {
            buffer[i--] = (char)(digit - 10 + 'a');
        }
        value /= 16;
    }

    // Shift characters to the beginning if necessary
    // This handles cases where the number of digits is less than buffer_size - 1
    size_t len = (buffer_size - 1) - (i + 1); // Calculate actual length
    for (size_t j = 0; j < len; ++j) {
        buffer[j] = buffer[i + 1 + j];
    }
    buffer[len] = '\0'; // Re-null-terminate at the correct position
}


// Zero-extend version of l4tohexstr
void l4tohexstrZX(ub4 value, char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size <= 1) {
        if (buffer_size == 1 && buffer)
        {
            buffer[0] = '\0';
        }
        return;
    }

    // buffer is assured to be at least 2 bytes
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';

        return;
    }

    size_t i = buffer_size - 1;
    buffer[i] = '\0';
    i--;

    // Convert digits in reverse order
    while (value > 0) {
        int digit = value % 16;
        if (digit < 10) {
            buffer[i] = (char)(digit + (int)'0');
        }
        else {
            buffer[i] = (char)(digit - 10 + (int)'a');
        }
        i--;
        value /= 16;
    }

    // This handles cases where the number of digits is less than buffer_size - 1
    size_t zxLen = i+1;
    for (size_t j = 0; j < zxLen; ++j) {
        buffer[j] = '0';
    }
}

size_t generateRandomULL(size_t min, size_t max)
{
    static size_t last = 0;
    last = rand() + isc_rand();

    size_t range = max - min + 1;
    return ((last) % (range) + min);
}

int64_t generateRandomLL(int64_t min, int64_t max)
{
    static int64_t last = 0;
    last = rand() + isc_rand();

    int64_t range = max - min + 1;
    if (min < 0) {
        int64_t offset = (int64_t)((uint64_t)llabs(last) % (uint64_t)range);
        return min + offset;
    }
    else {
        int64_t offset = last % range;
        if (offset < 0) offset += range;
        return min + offset;
    }
}


void generateRandomBytes(char* dst, const size_t dstSize)
{
    if (dstSize % 8 != 0) {
        assert(0&&"generateRandomBytes: dstSize must be a multiple of 8");
        return;
    }
    static    ub8 aa = 0, bb = 0, cc = 0;

    size_t dstWritePos = 0x0;
    ub8 i, j;
    aa = bb = cc = (ub8)0;
    isc_randinit(true);

    for (i = 0; i < 2; ++i)
    {
        isaac64();

        for (j = 0; j < RANDSIZ; ++j)
        {
            char buff[9];

            const size_t r = isc_rand();
            // l4tohexstrZX((ub4)(randrsl[j] >> 32), &buff, sizeof(buff));
            l4tohexstrZX((ub4)(r >> 32), buff, sizeof(buff));

            //printf("%s ", buff);
            for (size_t d = 0; d < 8; ++d) {
                dst[dstWritePos++] = buff[d];
                if (dstWritePos == dstSize)
                {
                    return;
                }
            }


            l4tohexstrZX((ub4)r, buff, sizeof(buff));
            //printf("%s \n", buff);


            for (size_t d = 0; d < 8; ++d) {
                dst[dstWritePos++] = buff[d];
                if (dstWritePos == dstSize)
                {
                    return;
                }
            }


            srand((unsigned int)(clock()+ dstWritePos+j*105-i * 5 + i + r));
            uint8_t num = (uint8_t)rand();
            if (num <= 128) {
                dst[dstWritePos++] = '\n';

                if (dstWritePos == dstSize)
                {
                    return;
                }
            }
       }
    }

}
void generateRandomBytesNoNL(char* dst, const size_t dstSize)
{
    generateRandomBytes(dst, dstSize);

    for (char* c = dst; c < dst + dstSize; c++)
    {
        if (*c == '\n' || *c == '\r')
        {
            *c = generateRandomLL('a', 'z');
        }
    }
}

void writeNL(FILE* fptr)
{
    if (generateRandomULL(0, 100) != 99)
    {
        fprintf(fptr, "\n");
    }
}

void writeSpace(FILE* fptr)
{
    if (generateRandomULL(0, 75) != 45)
    {
        fprintf(fptr, " ");
    }
}

void writeRandomScalarType(FILE* fptr)
{
    const uint16_t t = generateRandomULL(1, 9);
    if (t == PLY_SCALAR_TYPE_CHAR)
    {
        fprintf(fptr,"char");
        return;
    }
    if (t == PLY_SCALAR_TYPE_UCHAR)
    {
        fprintf(fptr, "uchar");  return;
    }
    if (t == PLY_SCALAR_TYPE_USHORT)
    {
        fprintf(fptr, "ushort"); return;
    }
    if (t == PLY_SCALAR_TYPE_SHORT)
    {
        fprintf(fptr, "short"); return;
    }
    if (t == PLY_SCALAR_TYPE_UINT)
    {
        fprintf(fptr, "uint"); return;
    }
    if (t == PLY_SCALAR_TYPE_INT)
    {
        fprintf(fptr, "int"); return;
    }
    if (t == PLY_SCALAR_TYPE_FLOAT)
    {
        fprintf(fptr, "float"); return;
    }
    if (t == PLY_SCALAR_TYPE_DOUBLE)
    {
        fprintf(fptr, "double"); return;
    }
}

void fwriteRandomElement(FILE* fptr)
{
    if (generateRandomULL(0, 55) != 5)
        fprintf(fptr, "element");
    writeSpace(fptr);

    char name[256] = { 0 };
    generateRandomBytesNoNL(name, generateRandomULL(1, 20)*8);
    fprintf(fptr,"%s ", name);

    if (generateRandomLL(1, 20) > 15)
    {
        fprintf(fptr, "%llu", generateRandomLL(-10, INT32_MAX));
    } else {
        fprintf(fptr,"%llu", generateRandomLL(0, 200));
    }

    writeNL(fptr);
}

void fwriteRandomProperty(FILE* fptr)
{
    if (generateRandomULL(0, 55) != 35)
        fprintf(fptr, "property");
        writeSpace(fptr);

    writeRandomScalarType(fptr);
    fprintf(fptr, " ");
    char name[256] = { 0 };
    generateRandomBytesNoNL(name, generateRandomULL(1, 10) * 8);
    fprintf(fptr, "%s", name);
    writeSpace(fptr);
}

void fuzzStructuredRandom(const char* filename, const size_t lineCount)
{
    
try_again:

    FILE* fptr;
    fopen_s(&fptr, filename, "w");
    if (!fptr) {
        goto try_again;
    }
    if (generateRandomULL(0,15)!=1)
        fprintf(fptr, "ply");
    writeNL(fptr);
    if (generateRandomULL(0, 15) != 1)
        fprintf(fptr, "format ascii 1.0");
    writeNL(fptr);


    uint16_t ct = 0;
    ct = generateRandomULL(0, 32);
    for (uint32_t i = 0; i < ct; i++)
    {
        fwriteRandomElement(fptr);

        uint16_t ct2 = generateRandomULL(1, 8);
        for (uint32_t j = 0; j < ct2; j++)
        {
            fwriteRandomProperty(fptr);
            writeNL(fptr);
        }
        writeNL(fptr);
    }

    fclose(fptr);

    if (generateRandomULL(0, 25) != 5)
        fprintf(fptr, "header_end");
    writeSpace(fptr);

}








// writes maxDataLen characters in the range of ASCII 0.....F to the specified file.
void fuzzFullRandom(const char* filename, const size_t maxDataLen)
{
    const size_t len = maxDataLen;
    char* c = (char*)malloc(len);
    if (!c) return;

    generateRandomBytes(c, len);

    clearAndWriteToFile(filename, (const char*)c, len);

    if (c) free(c);
}

#endif // !DEBUG_FUZZER_H