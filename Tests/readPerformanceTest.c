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
    while ((ch = (char)getchar()) != '\n' && ch != EOF) { if (ch == '0')return 1; }; /*clear stdin*/
    ch = (char)getchar();
    if (ch == '0')
        return 1;
    return 0;
}

enum PlyResult readAndTime(const char* filepath, double* timeOut)
{
    struct PlyScene scene;
    
    struct PlyLoadInfo loadInfo =
    {
        .elements = PLY_LOAD_ALL_ELEMENTS,
        .elementsCount = PLY_LOAD_ALL_ELEMENTS,
        .saveComments = true,
        .allowAnyVersion = false
    };

    unsigned char* data;
    size_t dataSize;
    loadFile(filepath, &data, &dataSize);

    clock_t t;
    t = clock();
    enum PlyResult lres = PlyLoadFromMemory(data, dataSize, &scene, &loadInfo);

    t = clock() - t;
    double parseDurationS = ((double)t) / CLOCKS_PER_SEC;

    *timeOut = parseDurationS;
    PlyDestroyScene(&scene);
    if (lres != PLY_SUCCESS)
    {
        printf("Failed to parse file '%s'. PlyResult: %s\n", filepath, PlyResultToString(lres));
        printf("Hint: ensure that the working directory is /Tests");
        exit(EXIT_FAILURE);
    }

    return t;
}


double readAndTimeTakeAvg(const char* filepath, uint16_t iterations) 
{
    uint16_t i;
    double avg = 0;
    for (i = 0; i < iterations; ++i)
    {
        double t;
        readAndTime(filepath, &t);
        avg += t;
    }
    avg /= iterations;

    return avg;
}

int main(void)
{
restart_test:
    printf("%s", "C-Polygon is a lightweight .ply (Stanford polygon) file parser written in C89 and x64 assembly. Copyright (C) 2025 Tripp R., under an MIT License."
        "\n----------------------------------------------------------------------------------------------------------------\n");
#ifndef NDEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif /* !NDEBUG */


 
#define PLY_FILE "res/lucy.ply"
#define ITERATIONS 5
    double avgtime = readAndTimeTakeAvg(PLY_FILE, ITERATIONS);

    printf("Completed %d iterations with an average time of %f seconds.\n", ITERATIONS, avgtime);
    


    if (promptRestartProgram())
        goto restart_test;
    return EXIT_SUCCESS;
}
