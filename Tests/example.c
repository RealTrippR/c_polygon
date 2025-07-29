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
    while ((ch = (char)getchar()) != '\n' && ch != EOF) {if(ch=='0')return 1;if((ch=='\n'||ch=='\0')==0) return 0;}; /*clear stdin*/
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

    struct PlyScene scene = { 0 };

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
        .elementCount = PLY_LOAD_ALL_ELEMENTS,
        .saveComments = true,
        .allowAnyVersion = false
    };

#define PLY_FILE "res/cube.ply"
    enum PlyResult lres = PlyLoadFromDisk(PLY_FILE, &scene, &loadInfo);

    t = clock() - t;
    double parseDurationS = ((double)t) / CLOCKS_PER_SEC;
	if (lres != PLY_SUCCESS)
	{
		printf("Failed to parse file '%s'. PlyResult: %s\n", PLY_FILE, PlyResultToString(lres));
        printf("Hint: ensure that the working directory is /Tests\n");
		PlyDestroyScene(&scene);

        if (promptRestartProgram())
            goto restart_test;

		return EXIT_FAILURE;
	}
	printf(".ply file parsing successful. File '%s' of size %s of was loaded and parsed in %f seconds.\n", getFilename(PLY_FILE), getReadableSize(getFileSize(PLY_FILE)), parseDurationS);

    struct PlySaveInfo saveInfo = {
      .D64DecimalCount = 17,
      .F32DecimalCount = 8
    };
    PlySaveToDisk("res/writeTest.ply", &scene, &saveInfo);

#define PRINT_HEADER_DATA 1
#define PRINT_ELEMENT_DATA 0
    printSceneData(&scene, PRINT_HEADER_DATA, PRINT_ELEMENT_DATA);


	PlyDestroyScene(&scene);

    if (promptRestartProgram())
        goto restart_test;
    return EXIT_SUCCESS;
}
