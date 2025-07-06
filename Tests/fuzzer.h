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

void generateRandomBytes(char* dst, const size_t dstSize)
{
    if (dstSize % 8 != 0) {
        assert("generateRandomBytes: dstSize must be a multiple of 8");
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

size_t generateRandomNumber(size_t min, size_t max)
{
    static size_t last = 0;
    srand((unsigned int)(clock() + last + time(NULL)));
    last = rand() + isc_rand();

    return ((last) % (max-min)+min);
}


void fuzzStructuredRandom(const char* filename, const size_t lineCount)
{

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