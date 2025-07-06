#include "../c_polygon.h"
#include <stdio.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <assert.h>


double bytesToDoubleByScalarType(U8* mem, enum ScalarType type)
{
    U8* f = mem;
    double d = 0;

    switch (type) {
    case PLY_SCALAR_TYPE_FLOAT: {
        float temp = 0x0;;
        memcpy(&temp, f, sizeof(float));
        d = (double)temp;
        break;
    }
    case PLY_SCALAR_TYPE_DOUBLE: {
        double temp = 0x0;
        memcpy(&temp, f, sizeof(double));
        d = temp;
        break;
    }
    case PLY_SCALAR_TYPE_CHAR: {
        int8_t temp = 0x0;
        memcpy(&temp, f, sizeof(int8_t));
        d = (double)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UCHAR: {
        uint8_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint8_t));
        d = (double)temp;
        break;
    }
    case PLY_SCALAR_TYPE_SHORT: {
        int16_t temp = 0x0;
        memcpy(&temp, f, sizeof(int16_t));
        d = (double)temp;
        break;
    }
    case PLY_SCALAR_TYPE_USHORT: {
        uint16_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint16_t));
        d = (double)temp;
        break;
    }
    case PLY_SCALAR_TYPE_INT: {
        int32_t temp = 0x0;
        memcpy(&temp, f, sizeof(int32_t));
        d = (double)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UINT: {
        uint32_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint32_t));
        d = (double)temp;
        break;
    }
    default:
        assert("bad scalar type");
        d = 0.0;
        break;
    }


    return d;
}

double getDataFromPropertyOfElement(const struct PlyElement* e, const struct PlyProperty* prop, const U64 dataLineIdx)
{
    const U64 offset = e->dataLineBegins[dataLineIdx] + prop->dataLineOffset;
	if (offset >= e->dataSize || dataLineIdx >= e->dataLineCount) {
		assert(0&&"Out of bounds access");
	}

    U8* f = ((U8*)e->data) + offset;
    return bytesToDoubleByScalarType(f, prop->scalarType);
}


void getDataFromPropertyOfElementAsList(double* dstBuffer, const size_t dstBufferSize, size_t* elementCountOut, const struct PlyElement* e, const struct PlyProperty* prop, const U64 dataLineIdx)
{
    U64 offset = e->dataLineBegins[dataLineIdx] + prop->dataLineOffset;
    if (offset >= e->dataSize) {
        assert("Out of bounds access");
    }
    U8* f = ((U8*)e->data) + offset;
    U64 count = bytesToDoubleByScalarType(f, prop->listCountType);
    if (*elementCountOut) {
        *elementCountOut = count;
    }
    offset += PlyGetSizeofScalarType(prop->listCountType);

    for (U64 i = 0; i < min(count,dstBufferSize/sizeof(double)); ++i)
    {
        offset += PlyGetSizeofScalarType(prop->scalarType);
        U8* f = ((U8*)e->data) + offset;
        dstBuffer[i] = bytesToDoubleByScalarType(f, prop->scalarType);
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

	struct PlyScene scene;
	enum PlyResult lres = PlyLoadFromDisk("Tests/res/cube.ply", &scene);
	if (lres != PLY_SUCCESS)
	{
		printf("res: %s\n", dbgPlyResultToString(lres));
		PlyDestroyScene(&scene);
		return EXIT_FAILURE;
	}
	printf(".ply file parsing successful.\n");

	for (U64 eId = 0; eId < scene.elementCount; ++eId)
	{
		struct PlyElement* element = scene.elements + eId;
		printf("-- Element %llu --\n", eId);
		printf("\tName: %s\n", element->name);
		printf("\t\tData Line Count %I32u\n", element->dataLineCount);
		printf("\tProperty Count:%I32u\n", element->propertyCount);

        //printRawDataOfElement(element);

		for (U64 pId = 0; pId < element->propertyCount; ++pId)
		{
			printf("\t-- Property %llu --\n", pId);

			struct PlyProperty* property = element->properties + pId;
			printf("\t\tScalar Type: %s \n", dbgPlyScalarTypeToString(property->scalarType));
			printf("\t\tData Type: %s \n", dbgPlyDataTypeToString(property->dataType));
			printf("\t\tList Count Type: %s\n", dbgPlyScalarTypeToString(property->listCountType));
			printf("\t\tData Line Offset: %lu\n", property->dataLineOffset);
		}

        printf("\tElement data (upscaled to double 64): ");
        for (U64 lno = 0; lno < element->dataLineCount; ++lno)
        {
            printf("\n\t\t");
            for (U64 pId = 0; pId < element->propertyCount; ++pId)
            {
                struct PlyProperty* property = element->properties + pId;
               if (property->dataType == PLY_DATA_TYPE_LIST) {
                    double d[128];
                    U64 dcount;
                    getDataFromPropertyOfElementAsList(d, sizeof(d), &dcount, element, property, lno);
                    for (U64 a = 0; a < dcount; ++a)
                    {
                        printf("%0.2f ", d[a]);
                    }
                }
                else {
                    double d = getDataFromPropertyOfElement(element, property, lno);
                    printf("%0.2f ", d);
                }
            }
        }
		printf("\n");
	}



	PlyDestroyScene(&scene);

	return EXIT_SUCCESS;
}