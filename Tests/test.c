#include "../c_polygon.h"
#include <stdio.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>


double getDataFromPropertyOfElement(const struct PlyElement* e, const struct PlyProperty* prop, const U64 dataLineIdx)
{
    const U64 offset = e->dataLineBegins[dataLineIdx] + prop->dataLineOffset;
	if (offset >= e->dataSize || dataLineIdx >= e->dataLineCount) {
		assert(0&&"Out of bounds access");
	}

    U8* f = ((U8*)e->data) + offset;
    return PlyScaleBytesToD64(f, prop->scalarType);
}


void getDataFromPropertyOfElementAsList(double* dstBuffer, const size_t dstBufferSize, size_t* elementCountOut,
    const struct PlyElement* e, const struct PlyProperty* prop, const U64 dataLineIdx)
{
    U64 offset = e->dataLineBegins[dataLineIdx] + prop->dataLineOffset;
    if (offset >= e->dataSize) {
        assert("Out of bounds access");
    }
    U8* f = ((U8*)e->data) + offset;
    U64 count = (U64)PlyScaleBytesToD64(f, prop->listCountType);
    if (elementCountOut!=NULL) {
        *elementCountOut = count;
    }
    offset += PlyGetSizeofScalarType(prop->listCountType);

    for (U64 i = 0; i < min(count,dstBufferSize/sizeof(double)); ++i)
    {
        U8* f = ((U8*)e->data) + offset;
        offset += PlyGetSizeofScalarType(prop->scalarType);
        if (offset >= e->dataSize) {
            assert("Out of bounds access");
        }
        dstBuffer[i] = PlyScaleBytesToD64(f, prop->scalarType);
    }
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
	enum PlyResult lres = PlyLoadFromDisk("Tests/res/cube.ply", &scene);

    t = clock() - t;
    double parseDurationS = ((double)t) / CLOCKS_PER_SEC;

    const char* s = "-126";
    U8 len;
    I16 i = strtoi8(s, &len);
	if (lres != PLY_SUCCESS)
	{
		printf("res: %s\n", dbgPlyResultToString(lres));
		PlyDestroyScene(&scene);
		return EXIT_FAILURE;
	}

    
	printf(".ply file parsing successful. Duration, sec: %f\n", parseDurationS);

    #define PRINT_SCENE_HEADER 1
    #define PRINT_ELEMENT_DATA 1

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
            printf("-- Element %llu --\n", eId);
            printf("\tName: %s\n", element->name);
            printf("\t\tData Line Count %I32u\n", element->dataLineCount);
            printf("\t\tData Size: %I64u\n", element->dataSize);;
            printf("\tProperty Count:%I32u\n", element->propertyCount);

            for (U64 pId = 0; pId < element->propertyCount; ++pId)
            {
                printf("\t-- Property %llu --\n", pId);

                struct PlyProperty* property = element->properties + pId;
                printf("\t\tScalar Type: %s\n", dbgPlyScalarTypeToString(property->scalarType));
                printf("\t\tData Type: %s\n", dbgPlyDataTypeToString(property->dataType));
                printf("\t\tList Count Type: %s\n", dbgPlyScalarTypeToString(property->listCountType));
                printf("\t\tData Line Offset: %lu\n", property->dataLineOffset);
            }


            if (PRINT_ELEMENT_DATA) {
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
                            getDataFromPropertyOfElementAsList(d, sizeof(d), &dcount, element, property, lno);
                            printf("{");
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
                            double d = getDataFromPropertyOfElement(element, property, lno);
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