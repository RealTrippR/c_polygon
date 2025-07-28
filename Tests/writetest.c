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

    const U32 vertexCount = 10;
    const U32 faceCount = 10;

    struct PlyScene scene = {.format = PLY_FORMAT_ASCII };
    struct PlyElement vertex = { .name = "vertex" };
    
    struct PlyProperty x = {.name = "x", .dataType = PLY_DATA_TYPE_SCALAR, .scalarType = PLY_SCALAR_TYPE_FLOAT};
    struct PlyProperty y = {.name = "y", .dataType = PLY_DATA_TYPE_SCALAR, .scalarType = PLY_SCALAR_TYPE_FLOAT};
    struct PlyProperty z = {.name = "z", .dataType = PLY_DATA_TYPE_SCALAR, .scalarType = PLY_SCALAR_TYPE_FLOAT};
    PlyWriteProperty(&vertex, &x);
    PlyWriteProperty(&vertex, &y);
    PlyWriteProperty(&vertex, &z);


    struct PlyElement faces = {.name = "face"};
    struct PlyProperty indices = { .name = "vertex_indices", .dataType = PLY_DATA_TYPE_LIST, .listCountType = PLY_SCALAR_TYPE_UCHAR, .scalarType = PLY_SCALAR_TYPE_UINT };
    struct PlyProperty garbage_val = { .name = "garbage", .dataType = PLY_DATA_TYPE_SCALAR, .scalarType = PLY_SCALAR_TYPE_FLOAT };
    PlyWriteProperty(&faces, &indices);
    PlyWriteProperty(&faces, &garbage_val);

    PlyCreateDataLines(&vertex, vertexCount);
    PlyCreateDataLines(&faces, faceCount);

    /*
    Data must be written in a linear order - that is; line by line, and on every line, 
    property by property, in the order that they were added to the element.
    Writing data in any other manner may result in undefined behavior.
    */
    U32 i;
    for (i=0; i < vertex.dataLineCount; ++i) {
        PlyWriteData(&vertex, i, /*x*/0, (union PlyScalarUnion){ .f32 = (float)i * 3 });
        PlyWriteData(&vertex, i, /*y*/1, (union PlyScalarUnion){ .f32 = (float)i * 3 + 1});
        PlyWriteData(&vertex, i, /*z*/2, (union PlyScalarUnion){ .f32 = (float)i * 3 + 2});
    }

    for (i=0; i < faces.dataLineCount; ++i) {
        int values[4] = { 0+i,1+i,2+i,3+i };
        PlyWriteDataList(&faces, i, /*vertex_indices*/0, i%4, values);
        PlyWriteData(&faces, i, /*garbage*/1, (union PlyScalarUnion) { .f32 = 5 });
    }

    printRawDataOfElement(&vertex);



    PlyWriteComment(&scene, "C-Polygon is a lightweight.ply(Stanford polygon) file parser written in C89 and x64 assembly. Copyright(C) 2025 Tripp R., under an MIT License.");
    PlyWriteObjectInfo(&scene, "is_test", 1.0);

    PlyWriteElement(&scene, &vertex);
    PlyWriteElement(&scene, &faces);

    struct PlySaveInfo saveInfo = {
        .D64DecimalCount = 50,
        .F32DecimalCount = 15
    };
#define PLY_FILE "res/writeTest.ply"
    enum PlyResult res = PlySaveToDisk(PLY_FILE, &scene, &saveInfo);

    if (res != PLY_SUCCESS) {
        printf("Failed to parse file '%s'. PlyResult: %s\n", PLY_FILE, PlyResultToString(res));
        printf("Hint: ensure that the working directory is /Tests\n");
        PlyDestroyScene(&scene);

        if (promptRestartProgram())
            goto restart_test;

        return EXIT_FAILURE;
    }
#define PRINT_HEADER_DATA 1
#define PRINT_ELEMENT_DATA 0
    printSceneData(&scene, PRINT_HEADER_DATA, PRINT_ELEMENT_DATA);

	PlyDestroyScene(&scene);

    if (promptRestartProgram())
        goto restart_test;
    return EXIT_SUCCESS;
}
