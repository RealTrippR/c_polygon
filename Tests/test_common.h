#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <assert.h>

double getDataFromPropertyOfElement(const struct PlyElement* e, const struct PlyProperty* prop, const U64 dataLineIdx, U8* success)
{
    const U64 lineBegin = e->dataLineBegins[dataLineIdx];
    const U32 dataOffset = prop->dataLineOffsets[dataLineIdx];
    const U64 offset = lineBegin + dataOffset;
    if (offset >= e->dataSize || dataLineIdx >= e->dataLineCount) {
        if (success)
            *success = 0;
        return 0;
    }

    U8* f = ((U8*)e->data) + offset;
    if (success)
        *success = 1;
    return PlyScaleBytesToD64(f, prop->scalarType);
}


void getDataFromPropertyOfElementAsList(double* dstBuffer, const size_t dstBufferSize, size_t* elementCountOut,
    const struct PlyElement* e, const struct PlyProperty* prop, const U64 dataLineIdx, U8* success)
{

    U64 offset = e->dataLineBegins[dataLineIdx] + prop->dataLineOffsets[dataLineIdx];
    if (offset >= e->dataSize) { /* check for out of bounds read */
        if (success)
            *success = 0;
        return;
    }
    U8* f = ((U8*)e->data) + offset;
    U64 count = (U64)PlyScaleBytesToD64(f, prop->listCountType);
    if (elementCountOut != NULL) {
        *elementCountOut = count;
    }
    if (!dstBuffer) {
        return;
    }

    offset += PlyGetSizeofScalarType(prop->listCountType);

    U64 i = 0;
    for (; i < min(count, dstBufferSize / sizeof(double)); ++i)
    {
        U8* f2 = ((U8*)e->data) + offset;
        const U64 sze = PlyGetSizeofScalarType(prop->scalarType);
        if (offset + sze > e->dataSize) { /* check for out of bounds read */
            if (success)
                *success = 0;
            return;
        }
        dstBuffer[i] = PlyScaleBytesToD64(f2, prop->scalarType);

        offset += sze;
    }
    if (success)
        *success = 1;
}

void printSceneData(struct PlyScene* scene, char printSceneHeader, char printElementData) {

    if (printSceneHeader) {
        U64 eId = 0;
        for (; eId < scene->elementCount; ++eId)
        {
            U64 oId = 0;
            for (; oId < scene->objectInfoCount; ++oId)
            {
                struct PlyObjectInfo* objInfo = scene->objectInfos + oId;
                printf("-- Object Info %llu --\n", oId);
                printf("\tName: %s\n", objInfo->name);
                printf("\tValue: %f\n\n", objInfo->value);
            }


            struct PlyElement* element = scene->elements + eId;

            printf("-- Element #%llu \"%s\" --\n", eId + 1, element->name);
            printf("\t\tData Line Count %I32u\n", element->dataLineCount);
            printf("\t\tData Size: %llu\n", element->dataSize);
            printf("\tProperty Count:%I32u\n\n", element->propertyCount);


            U64 pId = 0;
            for (; pId < element->propertyCount; ++pId)
            {

                printf("\t-- Property #%llu \"%s\" --\n", pId + 1, element->properties[pId].name);

                struct PlyProperty* property = element->properties + pId;
                printf("\t\tScalar Type: %s\n", PlyScalarTypeToString(property->scalarType));
                printf("\t\tData Type: %s\n", PlyDataTypeToString(property->dataType));
                printf("\t\tList Count Type: %s\n", PlyScalarTypeToString(property->listCountType));
            }


            if (printElementData) {
                U8 success = 0;

                printf("\tElement data (upscaled to double 64):\n");
                U64 lno = 0;
                for (; lno < element->dataLineCount; ++lno)
                {
                    printf("\t\t");
                    pId = 0;
                    for (; pId < element->propertyCount; ++pId)
                    {
                        struct PlyProperty* property = element->properties + pId;
                        if (property->dataType == PLY_DATA_TYPE_LIST) {
                            double d[512];
                            U64 dcount;
                            getDataFromPropertyOfElementAsList(d, sizeof(d), &dcount, element, property, lno, &success);
                            if (!success) {
                                assert(00 && "Bad data read.");
                            }
                            printf("<%llu>{", dcount);
                            U64 a = 0;
                            for (; a < dcount; ++a)
                            {
                                if (a + 1 != dcount) {
                                    printf("%0.4f,", d[a]);
                                }
                                else {
                                    printf("%0.4f", d[a]);
                                }
                            }
                            if (pId + 1 != element->propertyCount) {
                                printf("}, ");
                            }
                            else {
                                printf("}");
                            }
                        }
                        else {
                            double d = getDataFromPropertyOfElement(element, property, lno, &success);
                            if (!success) {
                                assert(00 && "Bad data read.");
                            }
                            printf("%0.4f ", d);
                        }
                    }
                    printf("\n");
                }
            }
        }
    }
}

void printRawDataOfElement(struct PlyElement* ele)
{
    printf("\n Raw element data: ");
    U64 i = 0;
    for (; i < ele->dataSize; ++i)
    {
        printf("%02x ", ((U8*)(ele->data))[i]);
    }
    printf("\n");
}

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
    
    int64_t len = strlen(filename);
    if (len>=(int64_t)sizeof(output)) {
        return "";
    }
    int64_t i = len-1;
    int64_t s = 0;
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
        *dataOut = NULL;
        *fileSizeOut = 0u;
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