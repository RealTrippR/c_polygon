/*
Copyright (C) 2025 Tripp Robins

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "../c_polygon.h"

#include "test_common.h"

#include <stdio.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>


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
    if (elementCountOut!=NULL) {
        *elementCountOut = count;
    }
    if (!dstBuffer) {
        return;
    }

    offset += PlyGetSizeofScalarType(prop->listCountType);

    U64 i = 0;
    for (; i < min(count,dstBufferSize/sizeof(double)); ++i)
    {
        U8* f2 = ((U8*)e->data) + offset;
        const U64 sze = PlyGetSizeofScalarType(prop->scalarType);
        if (offset + sze> e->dataSize) { /* check for out of bounds read */
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


int promptRestartProgram() {
    printf("Press any key to exit, or 0 to restart the program.\n");
    char ch = 0;
    while ((ch = (char)getchar()) != '\n' && ch != EOF) {if(ch=='0')return 1;}; /*clear stdin*/
    ch = (char)getchar();
    if (ch == '0')
        return 1;
    return 0;
}
int main(void)
{
restart_test:    
    printf("%s", "C-Polygon is a lightweight .ply (Stanford polygon) file parser written in C89 and x64 assembly. Copyright (C) 2025 Tripp R., under an MIT License."
                 "\n----------------------------------------------------------------------------------------------------------------\n");
#ifndef NDEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif /* !NDEBUG */

    clock_t t;
    t = clock();

	struct PlyScene scene;

    /*
    * I would recommend these links for obtaining test .ply files:
    * - Large Geometric Models Archive at Georgia Tech: https://sites.cc.gatech.edu/projects/large_models/
    * - The Stanford 3D Scanning Repository: https://graphics.stanford.edu/data/3Dscanrep/
    * If you want to integrate c_polygon into your software,
    * please take a refer to this paper for more about the .ply standard:
    * https://gamma.cs.unc.edu/POWERPLANT/papers/ply.pdf
    * The original .ply specification by Greg Turk
    * https://web.archive.org/web/20161221115231/http://www.cs.virginia.edu/~gfx/Courses/2001/Advanced.spring.01/plylib/Ply.txt
    */
    struct PlyLoadInfo loadInfo =
    {
        .elements = PLY_LOAD_ALL_ELEMENTS,
        .elementsCount = PLY_LOAD_ALL_ELEMENTS,
        .saveComments = true,
        .allowAnyVersion = false
    };

#define PLY_FILE "res/lucy.ply"
    enum PlyResult lres = PlyLoadFromDisk(PLY_FILE, &scene, &loadInfo);

    t = clock() - t;
    double parseDurationS = ((double)t) / CLOCKS_PER_SEC;
	if (lres != PLY_SUCCESS)
	{
		printf("Failed to parse file '%s'. PlyResult: %s\n", PLY_FILE, DbgPlyResultToString(lres));
        printf("Hint: ensure that the working directory is /Tests");
		PlyDestroyScene(&scene);

        if (promptRestartProgram())
            goto restart_test;

		return EXIT_FAILURE;
	}

    
	printf(".ply file parsing successful. File '%s' of size %s of was loaded and parsed in %f seconds.\n", getFilename(PLY_FILE), getReadableSize(getFileSize(PLY_FILE)), parseDurationS);

    #define PRINT_SCENE_HEADER 1
    #define PRINT_ELEMENT_DATA 0

    if (PRINT_SCENE_HEADER) {
        U64 eId = 0;
        for (; eId < scene.elementCount; ++eId)
        {
            U64 oId = 0;
            for (; oId < scene.objectInfoCount; ++oId)
            {
                struct PlyObjectInfo* objInfo = scene.objectInfos + oId;
                printf("-- Object Info %llu --\n", oId);
                printf("\tName: %s\n", objInfo->name);
                printf("\tValue: %f\n\n", objInfo->value);
            }


            struct PlyElement* element = scene.elements + eId;

            printf("-- Element #%llu \"%s\" --\n", eId+1, element->name);
            printf("\t\tData Line Count %I32u\n", element->dataLineCount);
            printf("\t\tData Size: %llu\n", element->dataSize);
            printf("\tProperty Count:%I32u\n\n", element->propertyCount);


            U64 pId = 0;
            for (; pId < element->propertyCount; ++pId)
            {

                printf("\t-- Property #%llu \"%s\" --\n", pId+1, element->properties[pId].name);

                struct PlyProperty* property = element->properties + pId;
                printf("\t\tScalar Type: %s\n", DbgPlyScalarTypeToString(property->scalarType));
                printf("\t\tData Type: %s\n", DbgPlyDataTypeToString(property->dataType));
                printf("\t\tList Count Type: %s\n", DbgPlyScalarTypeToString(property->listCountType));
            }


            if (PRINT_ELEMENT_DATA) {
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
                                assert(00&&"Bad data read.");
                            }
                            printf("<%llu>{", dcount);
                            U64 a = 0;
                            for (; a < dcount; ++a)
                            {
                                if (a + 1 != dcount) {
                                    printf("%0.2f,", d[a]);
                                }
                                else {
                                    printf("%0.2f", d[a]);
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
                                assert(00&&"Bad data read.");
                            }
                            printf("%0.2f ", d);
                        }
                    }
                    printf("\n");
                }
            }
        }
    }



	PlyDestroyScene(&scene);

    if (promptRestartProgram())
        goto restart_test;
    return EXIT_FAILURE;
}
