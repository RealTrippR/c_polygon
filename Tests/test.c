#include "../c_polygon.h"
#include <stdio.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))



double getDataFromPropertyOfElement(const struct PlyElement* e, const struct PlyProperty* prop, const U64 dataLineIdx, U8* success)
{
    const U64 offset = e->dataLineBegins[dataLineIdx] + prop->dataLineOffset;
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
    U64 offset = e->dataLineBegins[dataLineIdx] + prop->dataLineOffset;
    if (offset >= e->dataSize) { // check for out of bounds read
        if (success)
            *success = 0;
        return;
    }
    U8* f = ((U8*)e->data) + offset;
    U64 count = (U64)PlyScaleBytesToD64(f, prop->listCountType);
    if (elementCountOut!=NULL) {
        *elementCountOut = count;
    }
    offset += PlyGetSizeofScalarType(prop->listCountType);

    for (U64 i = 0; i < min(count,dstBufferSize/sizeof(double)); ++i)
    {
        U8* f2 = ((U8*)e->data) + offset;
        const U64 sze = PlyGetSizeofScalarType(prop->scalarType);
        if (offset + sze> e->dataSize) { // check for out of bounds read
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
    for (U64 i = 0; i < ele->dataSize; ++i)
    { 
        printf("%02x ", ((U8*)(ele->data))[i]);
    }
    printf("\n");
}

int main(void)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    clock_t t;
    t = clock();

	struct PlyScene scene;

    // I would recommend checking out these links for test .PLY files:
    // - Large Geometric Models Archive at Georgia Tech: https://sites.cc.gatech.edu/projects/large_models/
    // - The Stanford 3D Scanning Repository: https://graphics.stanford.edu/data/3Dscanrep/

	enum PlyResult lres = PlyLoadFromDisk("tests/res/cube.ply", &scene);

    t = clock() - t;
    double parseDurationS = ((double)t) / CLOCKS_PER_SEC;
	if (lres != PLY_SUCCESS)
	{
		printf("res: %s\n", dbgPlyResultToString(lres));
		PlyDestroyScene(&scene);
		return EXIT_FAILURE;
	}

    
	printf(".ply file parsing successful. Duration, sec: %f\n", parseDurationS);

    #define PRINT_SCENE_HEADER 1
    #define PRINT_ELEMENT_DATA 0

    if (PRINT_SCENE_HEADER) {
        for (U64 eId = 0; eId < scene.elementCount; ++eId)
        {
            for (U64 oId = 0; oId < scene.objectInfoCount; ++oId)
            {
                struct PlyObjectInfo* objInfo = scene.objectInfos + oId;
                printf("-- Object Info %llu --\n", oId);
                printf("\tName: %s\n", objInfo->name);
                printf("\tValue: %f\n\n", objInfo->value);
            }


            struct PlyElement* element = scene.elements + eId;
            printf("-- Element #%llu \"%s\" --\n", eId, element->name);
            printf("\t\tData Line Count %I32u\n", element->dataLineCount);
            printf("\t\tData Size: %I64u\n", element->dataSize);;
            printf("\tProperty Count:%I32u\n\n", element->propertyCount);

            //printRawDataOfElement(element);

            for (U64 pId = 0; pId < element->propertyCount; ++pId)
            {
                printf("\t-- Property #%llu \"%s\" --\n", pId, element->properties[pId].name);

                struct PlyProperty* property = element->properties + pId;
                printf("\t\tScalar Type: %s\n", dbgPlyScalarTypeToString(property->scalarType));
                printf("\t\tData Type: %s\n", dbgPlyDataTypeToString(property->dataType));
                printf("\t\tList Count Type: %s\n", dbgPlyScalarTypeToString(property->listCountType));
                printf("\t\tData Line Offset: %u\n\n", property->dataLineOffset);
            }


            if (PRINT_ELEMENT_DATA) {
                U8 success = 0;

                printf("\tElement data (upscaled to double 64):\n");
                for (U64 lno = 0; lno < element->dataLineCount; ++lno)
                {
                    printf("\t\t");
                    for (U64 pId = 0; pId < element->propertyCount; ++pId)
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
                            for (U64 a = 0; a < dcount; ++a)
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

	return EXIT_SUCCESS;
}