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

#include "c_polygon.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#ifdef __cplusplus
namespace cply
#endif /*__cplusplus*/

#ifndef TR_STR_EQL_H
#define TR_STR_EQL_H

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#if defined(_MSC_VER)
#if defined(_M_X64) || (_M_IX86_FP >= 2)
#define TRIPP_STREQL_USE_SIMD
#endif
#else
#if defined(__SSE4_2__)
#define TRIPP_STREQL_USE_SIMD
#else
#define TRIPP_STREQL_SIMD_NOT_SUPPORTED
#endif
#endif

#ifdef TRIPP_STREQL_USE_SIMD
#include <immintrin.h>
#endif /* TRIPP_STREQL_USE_SIMD */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*
/// streql() - Tests equality between two strings
/// @name streql
/// @brief Tests equality between two C strings. It differs from strcmp in 
/// the fact that it stops once immediately after a mismatch is detected, 
/// thus it is more efficient for comparison of direct equality. */
static PLY_INLINE bool streql(const char* str1, const char* str2) 
{
#ifdef TRIPP_STREQL_SIMD_NOT_SUPPORTED
    uint64_t i = 0u;
    while (true) {
        if (str1[i] != str2[i]) {
            return false;
        }
        if (str1[i] == '\0' || str2[i] == '\0') {
            return true;
        }
        i++;
    }
    return true;
#elif defined(TRIPP_STREQL_USE_SIMD)

    while (true)
    {
        register __m128i va = _mm_loadu_si128((const __m128i*)str1);
        register __m128i vb = _mm_loadu_si128((const __m128i*)str2);
        if (_mm_cmpistrc(va, vb, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY)) {
            return false; /* not equal */
        }
        if (_mm_cmpistrz(va, vb, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH) == true) {
            /* a null terminator was hit */
            return true;
        }

        str1 += 16;
        str2 += 16;
    }


#endif
}
/*
/// strneql() - Tests equality between two strings within range n
/// @name strneql
/// @brief Tests equality between two C strings within range n. It differs from strncmp in 
/// the fact that it stops once immediately after a mismatch is detected, 
/// thus it is more efficient for comparison of direct equality. */
static PLY_INLINE bool strneql(const char* str1, const char* str2, size_t n)
{
#ifdef TRIPP_STREQL_SIMD_NOT_SUPPORTED
    size_t i = 0;
    while (i < n) {
        if (str1[i] != str2[i]) {
            return false;
        }
        if (str1[i] == '\0' || str2[i] == '\0') {
            return true;
        }
        i++;
    }
    return true;
#elif defined(TRIPP_STREQL_USE_SIMD)

    while (n > 0) {
        const uint8_t buffLen = (n >= 16) ? 16 : (uint8_t)n;

        /* Load next 16 bytes */
        __m128i va = _mm_loadu_si128((const __m128i*)str1);
        __m128i vb = _mm_loadu_si128((const __m128i*)str2);

        /* Create / mask to 0 out all characters above bufflen */
        __m128i indices = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8,
            7, 6, 5, 4, 3, 2, 1, 0);
        __m128i limit = _mm_set1_epi8((char)buffLen);
        __m128i mask = _mm_cmplt_epi8(indices, limit);

        /* Mask out all characters above buffLen */
        va = _mm_and_si128(va, mask);
        vb = _mm_and_si128(vb, mask);

        if (_mm_cmpistrc(va, vb,
            _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY)) {
            return false;
        }

        if (_mm_cmpistrz(va, vb, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH)) {
            return true;
        }

        str1 += buffLen;
        str2 += buffLen;
        n -= buffLen;
    }
    return true;

#endif
}

#endif




/*for ASNI C compatability*/
#if (__STDC_VERSION__ == 0)
#include "c_polygon.inl"
#endif




static bool PLY_INLINE checkForElementNameCollision(const struct PlyScene* scene, const char* name)
{
    U32 ei = 0;
    for (; ei < scene->elementCount; ++ei)
    {
        if (streql(scene->elements[ei].name, name) == true)
        {
            return true;
        }
    }
    return false;
}

static bool PLY_INLINE checkForPropertyNameCollision(const struct PlyElement* element, const char* name)
{
    U32 ei = 0;
    for (; ei < element->propertyCount; ++ei)
    {
        if (streql(element->properties[ei].name, name) == true)
        {
            return true;
        }
    }
    return false;
}

static void* plyReallocDefault(void* oldBlock, U64 s) 
{
    if (s == 0) {
        return NULL;
    }
    return realloc(oldBlock, s);
}


static void* plyReCallocDefault(void* oldBlock, U32 cc, U32 ec, U32 es) 
{
    if (ec == 0 || es == 0) {
        return NULL;
    }
    /* overflow check */
    if (ec >= UINT32_MAX || ec > UINT32_MAX / es) 
    {
        return NULL;
    }

    const U64 newSizeBYTE = (ec * es);
    void* tmp = realloc(oldBlock, newSizeBYTE);

    if (!tmp) {
        if (oldBlock) {
            free(oldBlock);
        }
        return NULL;
    }

    if (ec > cc) {
        memset((U8*)tmp+(cc*es), 0, (ec - cc) * es);
    }

    return tmp;
}

static void plyDeallocDefault(void* m) {
    free(m);
}



/* 
(https://paulbourke.net/dataformats/ply/)
name        type        number of bytes
---------------------------------------
char       character                 1
uchar      unsigned character        1
short      short integer             2
ushort     unsigned short integer    2
int        integer                   4
uint       unsigned integer          4
float      single-precision float    4
double     double-precision float    8
*/



static union PlyScalarUnion PlyStrToScalar(const char* str, const enum PlyScalarType type, U8* strlen)
{
    union PlyScalarUnion d;
    d.d64 = 0.0;

    *strlen = 0;
    switch (type)
    {
    case PLY_SCALAR_TYPE_UCHAR:
        d.u8 = strtou8(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_CHAR:
        d.i8 = strtoi8(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_USHORT:
        d.u16 = strtou16(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_SHORT:
        d.i16 = strtoi16(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_UINT:
        d.u32 = strtou32(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_INT:
        d.i32 = strtoi32(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_FLOAT:
        d.f32 = strtof32(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_DOUBLE:
        d.d64 = strtod64(str, strlen);
        return d;
    case PLY_SCALAR_TYPE_UNDEFINED:
        return d;
    default:
        return d;
    }
}

enum PlyFormat PlyGetSystemEndianness(void)
{
    unsigned int x = 1;
    char* ptr = (char*)&x;
    if (ptr[0] == 1) {
        return PLY_FORMAT_BINARY_LITTLE_ENDIAN;
    }
    else {
        return PLY_FORMAT_BINARY_BIG_ENDIAN;
    }
}


uint32_t PlyScaleBytesToU32(void* data, const enum PlyScalarType t)
{
    U8* f = data;
    uint32_t d = 0;

    switch (t) {
    case PLY_SCALAR_TYPE_UNDEFINED:
        assert("C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = 0;
        break;
    case PLY_SCALAR_TYPE_FLOAT: {
        float temp = 0x0;;
        memcpy(&temp, f, sizeof(float));
        d = (uint32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_DOUBLE: {
        double temp = 0x0;
        memcpy(&temp, f, sizeof(double));
        d = (uint32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_CHAR: {
        int8_t temp = 0x0;
        memcpy(&temp, f, sizeof(int8_t));
        d = (uint32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UCHAR: {
        uint8_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint8_t));
        d = (uint32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_SHORT: {
        int16_t temp = 0x0;
        memcpy(&temp, f, sizeof(int16_t));
        d = (uint32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_USHORT: {
        uint16_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint16_t));
        d = (uint32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_INT: {
        int32_t temp = 0x0;
        memcpy(&temp, f, sizeof(int32_t));
        d = (uint32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UINT: {
        uint32_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint32_t));
        d = (uint32_t)temp;
        break;
    }
    default:
        assert(0x0 && "C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = (uint32_t)0.0;
        break;
    }


    return d;
}

int32_t PlyScaleBytesToI32(void* data, const enum PlyScalarType t)
{
    U8* f = data;
    int32_t d = 0;

    switch (t) {
    case PLY_SCALAR_TYPE_UNDEFINED:
        assert("C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = 0;
        break;
    case PLY_SCALAR_TYPE_FLOAT: {
        float temp = 0x0;;
        memcpy(&temp, f, sizeof(float));
        d = (int32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_DOUBLE: {
        double temp = 0x0;
        memcpy(&temp, f, sizeof(double));
        d = (int32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_CHAR: {
        int8_t temp = 0x0;
        memcpy(&temp, f, sizeof(int8_t));
        d = (int32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UCHAR: {
        uint8_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint8_t));
        d = (int32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_SHORT: {
        int16_t temp = 0x0;
        memcpy(&temp, f, sizeof(int16_t));
        d = (int32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_USHORT: {
        uint16_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint16_t));
        d = (int32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_INT: {
        int32_t temp = 0x0;
        memcpy(&temp, f, sizeof(int32_t));
        d = (int32_t)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UINT: {
        uint32_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint32_t));
        d = (int32_t)temp;
        break;
    }
    default:
        assert(0x0 && "C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = (int32_t)0.0;
        break;
    }


    return d;
}

float PlyScaleBytesToF32(void* data, const enum PlyScalarType t)
{
    U8* f = data;
    float d = 0;

    switch (t) {
    case PLY_SCALAR_TYPE_UNDEFINED:
        assert("C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = 0;
        break;
    case PLY_SCALAR_TYPE_FLOAT: {
        float temp = 0x0;;
        memcpy(&temp, f, sizeof(float));
        d = (float)temp;
        break;
    }
    case PLY_SCALAR_TYPE_DOUBLE: {
        double temp = 0x0;
        memcpy(&temp, f, sizeof(double));
        d = (float)temp;
        break;
    }
    case PLY_SCALAR_TYPE_CHAR: {
        int8_t temp = 0x0;
        memcpy(&temp, f, sizeof(int8_t));
        d = (float)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UCHAR: {
        uint8_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint8_t));
        d = (float)temp;
        break;
    }
    case PLY_SCALAR_TYPE_SHORT: {
        int16_t temp = 0x0;
        memcpy(&temp, f, sizeof(int16_t));
        d = (float)temp;
        break;
    }
    case PLY_SCALAR_TYPE_USHORT: {
        uint16_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint16_t));
        d = (float)temp;
        break;
    }
    case PLY_SCALAR_TYPE_INT: {
        int32_t temp = 0x0;
        memcpy(&temp, f, sizeof(int32_t));
        d = (float)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UINT: {
        uint32_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint32_t));
        d = (float)temp;
        break;
    }
    default:
        assert(0x0 && "C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = 0.0;
        break;
    }


    return d;
}

double PlyScaleBytesToD64(void* data, const enum PlyScalarType t)
{
    U8* f = data;
    double d = 0;

    switch (t) {
    case PLY_SCALAR_TYPE_UNDEFINED:
        assert("C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = 0;
        break;
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
        assert(0x0 && "C-Polygon: Bad scalar type - this likely indicates memory corruption within the program.");
        d = 0.0;
        break;
    }


    return d;

}

U8 PlyGetSizeofScalarType(const enum PlyScalarType type)
{
    /*enums can be negative apparently*/
    if (type < 0 || type > 8) {
        return 0;
    }
    const U8 tbl[] =
    {
        0,
        1,
        1,
        2,
        2,
        4,
        4,
        4,
        8
    };

    return tbl[type];
}

enum PlyScalarType PlyStrToScalarType(const char* str, const U64 strLen)
{
    if (strneql(str, "char", min(strLen, strlen("char"))) == true)
    {
        return PLY_SCALAR_TYPE_CHAR;
    }
    if (strneql(str, "uchar", min(strLen, strlen("uchar"))) == true)
    {
        return PLY_SCALAR_TYPE_UCHAR;
    }
    if (strneql(str, "short", min(strLen, strlen("short"))) == true)
    {
        return PLY_SCALAR_TYPE_SHORT;
    }
    if (strneql(str, "ushort", min(strLen, strlen("ushort"))) == true)
    {
        return PLY_SCALAR_TYPE_USHORT;
    }
    if (strneql(str, "int", min(strLen, strlen("int"))) == true)
    {
        return PLY_SCALAR_TYPE_INT;
    }
    if (strneql(str, "uint", min(strLen, strlen("uint"))) == true)
    {
        return PLY_SCALAR_TYPE_UINT;
    }
    if (strneql(str, "float", min(strLen, strlen("float"))) == true)
    {
        return PLY_SCALAR_TYPE_FLOAT;
    }
    if (strneql(str, "double", min(strLen, strlen("double"))) == true)
    {
        return PLY_SCALAR_TYPE_DOUBLE;
    }

    return PLY_SCALAR_TYPE_UNDEFINED;
}



static PlyReallocT plyRealloc = plyReallocDefault;

static PlyReCallocT plyReCalloc = plyReCallocDefault;

static PlyDeallocT plyDealloc = plyDeallocDefault;

void PlySetCustomAllocator(PlyReallocT A)
{
    plyRealloc = A;
}


void PlySetCustomRecallocator(PlyReCallocT A)
{
    plyReCalloc = A;
}

void PlySetCustomDeallocator(PlyDeallocT deA)
{
    plyDealloc = deA;
}



/* adds a PlyProperty to an element.The property will be copied, thus transferring ownership */
enum PlyResult  PlyElementAddProperty(struct PlyElement* element, struct PlyProperty* property)
{
    if (element->propertyCount == UINT32_MAX - 1) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
    }
    const U32 newPropertyCount = element->propertyCount + 1;
    struct PlyProperty* tmp = (struct PlyProperty*)plyReCalloc(element->properties, element->propertyCount, newPropertyCount, sizeof(struct PlyProperty));
    if (!tmp) {
        return PLY_FAILED_ALLOC_ERROR;
    }
    else {
        element->properties = tmp;
        element->properties[element->propertyCount] = *property;
        element->propertyCount = newPropertyCount;
    }
    return PLY_SUCCESS;
}

/* adds a PlyObjectInfo to an element.The property will be copied, thus transferring ownership */
enum PlyResult PlySceneAddObjectInfo(struct PlyScene* scene, struct PlyObjectInfo* objInfo)
{
#define OBJ_INFOS scene->objectInfos
#define OBJ_INFO_COUNT scene->objectInfoCount
    if (OBJ_INFO_COUNT == UINT32_MAX - 1) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
    }
    const U32 newObjInfoCount = OBJ_INFO_COUNT + 1;
    struct PlyObjectInfo* tmp = (struct PlyObjectInfo*)plyReCalloc(
        OBJ_INFOS,
        OBJ_INFO_COUNT, 
        newObjInfoCount, 
        sizeof(struct PlyObjectInfo));

    if (!tmp) {
        return PLY_FAILED_ALLOC_ERROR;
    }
    else {
        OBJ_INFOS = tmp;
        OBJ_INFOS[OBJ_INFO_COUNT] = *objInfo;
        OBJ_INFO_COUNT = newObjInfoCount;
    }
    return PLY_SUCCESS;
#undef OBJ_INFO_COUNT
#undef OBJ_INFOS
}


/* adds a PlyElement to a scene.The element will be copied, thus transferring ownership */
enum PlyResult PlySceneAddElement(struct PlyScene* scene, struct PlyElement* element)
{
    if (scene->elementCount == UINT32_MAX - 1)  {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
    }
    const U32 newElementCount = scene->elementCount + 1;
    struct PlyElement* tmp = (struct PlyElement*)plyReCalloc(scene->elements, scene->elementCount, newElementCount, sizeof(struct PlyElement));
    if (!tmp) {
        return PLY_FAILED_ALLOC_ERROR;
    }
    else {
        scene->elements = tmp;
        scene->elements[scene->elementCount] = *element;
        scene->elementCount = newElementCount;
    }
    return PLY_SUCCESS;
}


static U32 lineLen_s(const char* srcline, const char* mem, U64 memSize)
{
    if (!srcline || srcline < mem || srcline > mem + memSize - 1)
        return 0x0;

    U32 maxDist = (U32)(mem + (U32)memSize - srcline);

    U64 i = 0;
    for (; i < maxDist; ++i) {
        const char* cur = srcline + i;
        if (cur == (const char* )UINT64_MAX) { return 0x0; }

        if (*cur == '\0')
        {
            return (U32)i;
        }
        if (*cur == '\r' || *cur == '\n')
        {
            return (U32)i;
        }
    }

    /* invalid line, no null terminator */
    return 0u;
}


static void parseLine(const char* lineIn, U64 lineInSize, char* dst, const U32 dstSize, U32* strlenOut)
{
    *strlenOut = 0x0;

    if (dstSize == 0 || lineInSize == 0) {
        return;
    }

    if (lineInSize >= dstSize) {
        return;
    }

    U64 i = 0;
    for (; i < lineInSize-1; ++i)
    {
        if (isblank((unsigned char)lineIn[i])) {
            continue;
        }
        else {
           
            memcpy_s(dst + i, dstSize, lineIn + i, lineInSize);
            *strlenOut = lineLen_s(lineIn, lineIn, lineInSize+1);
            /* remove any trailing spaces */

            const U64 len = *strlenOut;
            U64 j=0;
            for (; j < len; ++j)
            {
                char c = lineIn[len-j-1];
                if (isblank(c)) {
                    if (*strlenOut > 0) {
                        (*strlenOut)--;
                    }
                }
                else {
                    break;
                }
            }


            if (dst) {
                dst[*strlenOut] = '\0';
            }
            return;
        }
    }
}

static const char* getNextLine(U64* lenOut, const U8* mem, U64 memSize, const char* lastLine, const U64 lastLineLen)
{
    if (lastLineLen > UINT32_MAX) {
        return NULL;
    }
    if (memSize > UINT32_MAX)
    {
        return NULL;
    }

   
    const char* lastLineLast = lastLine + lastLineLen;
    if (!lastLine || !mem || !memSize || (const U8*)lastLine < mem || (const U8*)lastLineLast >= (mem + (memSize - 1))) {
        return NULL;
    }

    /* the first char in the line */
    const char* lineBegin = NULL;
    /* 1-past the last char in the line. */
    const char* lineLast = NULL;

    /* find lineBegin */
    U64 dist = (mem + memSize - (const U8*)lastLine);
    const U64 loopLimit1 = dist - 1;;
    U64 i = 0;
    for (; i < loopLimit1; ++i)
    {
        const char* cur = lastLine + i;
        if (cur == (const char*)UINT64_MAX) { return NULL; }

        if (*cur == '\n')
        {
            lineBegin = cur + 1;
            break;
        }
        if (*cur == '\0')
        {
            return NULL;
        }
    }

    if (!lineBegin)
    {
        return NULL;
    }



    /* find line end */
    dist = (mem + memSize - (const U8*)lineBegin);
    
    i = 0;
    for (; i < dist; ++i)
    {
        const char* cur = lineBegin + i;
        if (cur == (const char*)UINT64_MAX) { return NULL; }
        if (*cur == '\n' || *cur == '\r' || cur == (const char*)(mem + memSize - 1))
        {
            if (i == dist) {
                lineLast = NULL;
                break;
            }

            if (cur == (const char*)(mem + memSize - 1)) {
                lineLast = cur;
            } else {
                lineLast = cur - 1;
            }
            break;
        }
        if (*cur == '\0')
        {
            lineLast = cur - 1;
            break;
        }
    }

    if (!lineLast) {
        return NULL;
    }

    if (lineLast < lineBegin) {
        return lineBegin; /*for lines that only have newlines preceding them (i.e. begin ... /n/n/n/n ... more lines)*/
    }
    if (lineLast == (const char*)UINT64_MAX - 1) {
        return NULL;
    }



    *lenOut = lineLast - lineBegin + 1;

    return lineBegin;
}

const char* getNextSpace(const char* srchBegin, const char* srchEnd)
{
    const char* ch = srchBegin;
    for (; ch <= srchEnd; ++ch)
    {
        if (isspace(*ch) != 0) {
            return ch;
        }
    }
    return NULL;
}


const char* getNextNonSpace(const char* srchBegin, const char* srchEnd)
{
    const char* ch = srchBegin;
    for (; ch <= srchEnd; ++ch)
    {
        if (isspace(*ch) == 0) {
            return ch;
        }
    }
    return NULL;
}

static enum PlyResult parseProperty(struct PlyElement* owningElement, const char* propRangeFirst, const char* propRangeLast)
{
    /* data type */
    enum PlyDataType dtype = PLY_DATA_TYPE_SCALAR;
    /* scalar type */
    enum PlyDataType stype = PLY_SCALAR_TYPE_UNDEFINED;
    /* list count type */
    enum PlyDataType ltype = PLY_SCALAR_TYPE_UNDEFINED;

    const char* next = NULL;
    next = getNextNonSpace(propRangeFirst, propRangeLast);
    if (!next)
    {
        return PLY_MALFORMED_FILE_ERROR;
    }



    /* get data type(list / scalar) */
    U64 dist = 0u;
    dist = (propRangeLast - next) + 1;
    if (strneql(next, "list", min(dist, strlen("list")))==true) {
        dtype = PLY_DATA_TYPE_LIST;
    }

    /* get scalar type */
    next = getNextNonSpace(next, propRangeLast);
    if (!next) {
        return PLY_MALFORMED_FILE_ERROR;
    }

    dist = (propRangeLast - next) + 1;
    if (dtype == PLY_DATA_TYPE_SCALAR) {
        /* get scalar type */
        stype = PlyStrToScalarType(next, dist);
    }
    else if (dtype == PLY_DATA_TYPE_LIST) {

        /* get list type */
        next = getNextSpace(next, propRangeLast);
        if (!next)
            return PLY_MALFORMED_FILE_ERROR;
        next = getNextNonSpace(next, propRangeLast);
        if (!next) {
            return PLY_MALFORMED_FILE_ERROR;
        }

        ltype = PlyStrToScalarType(next, dist);


        /* get scalar type */
        next = getNextSpace(next, propRangeLast);
        if (!next)
            return PLY_MALFORMED_FILE_ERROR;
        next = getNextNonSpace(next, propRangeLast);
        if (!next) {
            return PLY_MALFORMED_FILE_ERROR;
        }

        dist = (propRangeLast - next) + 1;
        stype = PlyStrToScalarType(next, dist);
    }

    if (stype == PLY_DATA_TYPE_UNDEFINED) {
        return PLY_MALFORMED_FILE_ERROR;
    }


    /*get name*/
    next = getNextSpace(next, propRangeLast);
    if (!next)
        return PLY_MALFORMED_FILE_ERROR;
    next = getNextNonSpace(next, propRangeLast);
    if (!next)
        return PLY_MALFORMED_FILE_ERROR;

    const char* nameFirst = next;
    const U64 nameLen = (propRangeLast - next)+1;
    if (nameLen > PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
    }

 

    struct PlyProperty property = {0};
    property.dataType = dtype;
    property.listCountType = ltype;
    property.scalarType = stype;

    memcpy_s(property.name, sizeof(property.name), nameFirst, nameLen);
    property.name[nameLen] = '\0';
    
    if (checkForPropertyNameCollision(owningElement, property.name) == true)
        return PLY_MALFORMED_HEADER_ERROR;

    const enum PlyResult r = PlyElementAddProperty(owningElement, &property);
    
    return r;
}


static enum PlyResult parseObjectInfo(struct PlyScene* scene, const char* objBegin, const char* objEnd)
{
    /*get name begin*/
    const char* NameBegin = NULL;

    const char* ch = objBegin;
    for (; ch <= objEnd; ++ch) {
        if (isspace(*ch) == 0) 
        {
            NameBegin = ch;
            break;
        }
    }
    if (!NameBegin)
        return PLY_MALFORMED_HEADER_ERROR;
    

    /*get name end*/
    const char* NameEnd = NULL;
    ch = NameBegin;
    for (; ch <= objEnd; ++ch) {
        if (isspace(*ch) != 0) {
            NameEnd = ch-1;
            break; 
        }
    }
    if (!NameEnd)
        return PLY_MALFORMED_HEADER_ERROR;
    
    /*get val begin*/
    const char* ValBegin=NULL;
    ch = NameEnd+1;
    for (; ch <= objEnd; ++ch) {
        if (isspace(*ch) == 0)
        {
            ValBegin = ch;
            break;
        }
    }
    if (!ValBegin)
        return PLY_MALFORMED_HEADER_ERROR;

    /*get val end*/
    const char* ValEnd = NULL;
    ch = ValBegin;
    for (; ch <= objEnd; ++ch) {
        if (ch == objEnd) /*if ValBegin is at the end of the obj data src*/
        {
            ValEnd = ValBegin;
            break;
        }
        if (isspace(*ch) != 0) {

            ValEnd = ch-1;
            break;
        }
    }
    if (!ValEnd)
        return PLY_MALFORMED_HEADER_ERROR;


    struct PlyObjectInfo objInfo = { 0 };
    memset(objInfo.name, 0, sizeof(objInfo.name));
    const U64 nameCpySize = NameEnd - NameBegin + 1;
    memcpy_s(objInfo.name, sizeof(objInfo.name) - 1, NameBegin, nameCpySize);



    U8 strlen=0u;
    double val = strtod64(ValBegin, &strlen);
    if (strlen == 0) {
        return PLY_MALFORMED_HEADER_ERROR;
    }

    objInfo.value = val;

    enum PlyResult r = PlySceneAddObjectInfo(scene, &objInfo);
    return r;
}

static enum PlyResult readHeaderLine(const char* line, const U32 lineLen, bool* readingHeader, struct PlyElement** curElement, struct PlyScene* scene, struct PlyLoadInfo* loadInfo)
{
    if (lineLen == 0) {
        return PLY_SUCCESS;
    }


    /* the last character in the line.*/
    const char* lineLast = line + lineLen - 1;

    if (lineLast == NULL || !readingHeader) {
        return PLY_MALFORMED_HEADER_ERROR;
    }

    const char* c = "ply";
    if (strneql(line, c, min(strlen(c), lineLen)) == true)
    {
        *curElement = NULL;
        *readingHeader = true;
        return PLY_SUCCESS;
    }
    
    c = "end_header";
    if (strneql(line, c, min(strlen(c), lineLen)) == true)
    {
        *curElement = NULL;
        *readingHeader = false;
        return PLY_SUCCESS;
    }




    if (*readingHeader) { 
        if (loadInfo && loadInfo->saveComments) 
        {
            c = "comment";
            if (strneql(line, c, min(strlen(c), lineLen)) == true)
            {
                const char* commentBegin = line + strlen(c);

                for (;
                    commentBegin < line + lineLen && isspace(*commentBegin);
                    ++commentBegin) {
                }




                const U64 commentLen = (line + lineLen) - commentBegin;


                U32 newCommentCount = scene->commentCount + 1;
                if (newCommentCount < scene->commentCount) /*prevent overflow*/
                    return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                scene->comments = plyReCalloc(scene->comments, scene->commentCount, newCommentCount, sizeof(char*));

                unsigned char* tmp = (char*)plyRealloc(NULL, commentLen + 1);
                if (!tmp)
                    return PLY_FAILED_ALLOC_ERROR;



                scene->comments[scene->commentCount] = tmp;

                memcpy(scene->comments[scene->commentCount], commentBegin, commentLen);

                scene->comments[scene->commentCount][commentLen] = '\0';

                scene->commentCount = newCommentCount;

                return PLY_SUCCESS;
            }
        }

        /*look for format keyword*/
        c = "format ";
        if (strneql(line, c, min(strlen(c), lineLen)) == true)
        {
            *curElement = NULL;

            /*just beyond the end of format(last char + 1)*/
            const char* formatEnd = NULL;
            /*look for ascii keyword*/
            U64 i = strlen("format");
            for (; i < lineLen; ++i) {
                if (strneql(line + i, "ascii", min(strlen("ascii"), lineLen - i)) == true)
                {
                    scene->format = PLY_FORMAT_ASCII;
                    formatEnd = line + i + strlen("ascii");
                    goto getVersion;
                }
            }


            /*look for binary_big_endian  keyword*/
            i = strlen("format");
            for (; i < lineLen; ++i) {
                if (strneql(line + i, "binary_big_endian", min(strlen("binary_big_endian"), lineLen - i)) == true)
                {
                    scene->format = PLY_FORMAT_BINARY_BIG_ENDIAN;
                    formatEnd = line + i + strlen("binary_big_endian");
                    goto getVersion;
                }
            }


            /*look for binary_little_endian keyword*/
            i = strlen("format");
            for (; i < lineLen; ++i) {
                if (strneql(line + i, "binary_little_endian", min(strlen("binary_little_endian"), lineLen - i)) == true)
                {
                    scene->format = PLY_FORMAT_BINARY_LITTLE_ENDIAN;
                    formatEnd = line + i + strlen("binary_little_endian");
                    goto getVersion;
                }
            }

        getVersion:
            /*no format was found*/
            if (scene->format == PLY_FORMAT_UNDEFINED) {
                return PLY_MALFORMED_HEADER_ERROR;
            }

            if (formatEnd != NULL) {
                const char* vbegin = NULL;
                const char* ch = formatEnd;
                for (; ch <= lineLast; ch++) {
                    if (isspace(*ch) == false) {
                        vbegin = ch;
                        break;
                    }
                }
                if (vbegin == NULL) {
                    return PLY_MALFORMED_HEADER_ERROR;
                }
                /*get version number(currently only v 1.0 is supported)*/
                scene->versionNumber = strtof(vbegin, NULL);
                if (scene->versionNumber != 1.0)
                {
                    scene->versionNumber = 0.0;
                    return PLY_UNSUPPORTED_VERSION_ERROR;
                }
            }
            return PLY_SUCCESS;
        } /*end format keyword check*/





        c = "element ";
        /*check for element declaration*/
        if (strneql(line, c, min(strlen(c), lineLen))==true) 
        {

            *curElement = NULL;

            /*find element name begin*/

            /*first char in the element's name*/
            const char* elementNameBegin = NULL;
            /*last char in the element's name*/
            const char* elementNameLast = lineLast;

            const char* ch = line+strlen(c);
            for (; ch <= lineLast; ++c) {
                if (isspace(*ch) == 0) {
                    elementNameBegin = ch;
                    break;
                }
            }
            if (elementNameBegin == NULL) {
                return PLY_MALFORMED_HEADER_ERROR;
            }

         


            /*find element name last*/
            ch = elementNameBegin+1;
            for (; ch <= lineLast; ++ch)
            {
                if (isspace(*ch) != 0) {
                    elementNameLast = ch-1;
                    break;
                }
            }

            const U64 elementNameLen = (elementNameLast - elementNameBegin) + 1u;

            if (elementNameLen > PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH) {
                return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
            }




            /*find propertyCountBegin*/
            const char* dataCountBegin = NULL;

            /*find element name last*/
            ch = elementNameLast + 1;
            for (; ch <= lineLast; ++ch)
            {
                if (isspace(*ch)) {
                    continue;
                }
                if (*ch < '0' || *ch > '9') {
                    return PLY_MALFORMED_HEADER_ERROR;
                }
                else {
                    dataCountBegin = ch;
                    break;
                }
            }

            if (dataCountBegin == NULL) {
                return PLY_MALFORMED_HEADER_ERROR;
            }

            const U32 dataCount = strtoul(dataCountBegin, NULL, 0u);



            struct PlyElement element = { 0 };
            
            if (loadInfo->elementsCount != PLY_LOAD_ALL_ELEMENTS) {
                bool told = false;
                U64 i = 0;
                for (; i < loadInfo->elementsCount; ++i) {
                    if (streql(elementNameBegin, loadInfo->elements[i])) {
                        told = true;
                        break;
                    }
                }
                if (!told)
                    return PLY_SUCCESS;
            }


            memcpy_s(element.name, sizeof(element.name)-1, elementNameBegin, elementNameLen);
            element.name[elementNameLen] = '\0';
            element.dataLineCount = dataCount;

            if (checkForElementNameCollision(scene, element.name) == true)
                return PLY_MALFORMED_HEADER_ERROR;

            if (PlySceneAddElement(scene, &element)!=PLY_SUCCESS)
            {
                return PLY_FAILED_ALLOC_ERROR;
            }

            *curElement = &scene->elements[scene->elementCount-1];

            return PLY_SUCCESS;
        } /* end get element */
        
        c = "property ";
        /* check for proerty declaration */
        if (strneql(line, c, min(strlen(c), lineLen)) == true)
        {
            if (*curElement == NULL)
            {
                return PLY_MALFORMED_HEADER_ERROR;
            }
            enum PlyResult r = parseProperty(*curElement, line+strlen(c), lineLast);
            return r;
        }

        c = "obj_info";
        if (strneql(line, c, min(strlen(c), lineLen)) == true)
        {
            enum PlyResult r = parseObjectInfo(scene, line + strlen(c), lineLast);
            return r;
        }
    }


    /* there is nothing to read, skip this line. */
    return PLY_SUCCESS;
}


static PLY_INLINE enum PlyResult allocateDataLinesForElement(struct PlyElement* element)
{

    element->dataLineBegins = plyReCalloc(element->dataLineBegins, 0, element->dataLineCount, sizeof(U64));
    if (!element->dataLineBegins) {
        return PLY_FAILED_ALLOC_ERROR;
    }

    /* create data line offsets for the properties of this element*/
    U64 pi = 0u;
    for (; pi < element->propertyCount; ++pi) {
        struct PlyProperty* property = element->properties + pi;
        property->dataLineOffsets = plyReCalloc(NULL, 0, element->dataLineCount, sizeof(U32));
        if (!property->dataLineOffsets) {
            return PLY_FAILED_ALLOC_ERROR;
        }
    }
    return PLY_SUCCESS;
}



enum PlyResult readDataBinary(struct PlyScene* scene, const U8* dataBegin, const U8* dataLast)
{
    const U64 dataSize = (dataLast - dataBegin) + 1;

    enum PlyFormat systemEndianness = PlyGetSystemEndianness();

    U64 ei = 0;
    const U8* dataPrev = dataBegin;

       
    U64 totalAllocSize = 0u;

    /* precompute the total amount of data that will be needed for each element */
    for (; ei < scene->elementCount; ++ei)
    {
        struct PlyElement* element = scene->elements + ei;
        if (element->dataLineCount == 0) {
            continue; /* empty element (idk if this is permitted by the standard or not) */
        }

        element->dataSize = 0u;
        dataBegin = dataPrev; /* reset on every new elemene that is being read to prevent incorrect offset of dataLineBegins */
   

        /* create data lines for element and all its properties*/
        if (allocateDataLinesForElement(element) != PLY_SUCCESS)
            return PLY_FAILED_ALLOC_ERROR;

        U64 dli = 0;
        for (; dli < element->dataLineCount; ++dli)
        {


            if (dataPrev >= dataLast) {
                return PLY_MALFORMED_DATA_ERROR;
            }


            U32 countedProperties = 0u;

            const U8* dataLineBegin = dataPrev;
            element->dataLineBegins[dli] = dataPrev - dataBegin;

            U64 pi;
            for (pi = 0; pi < element->propertyCount; ++pi)
            {
                ++countedProperties;
                if (countedProperties > element->propertyCount) {
                    return PLY_MALFORMED_DATA_ERROR;
                }

                struct PlyProperty* property = element->properties + pi;

                if (property->dataType == PLY_DATA_TYPE_SCALAR)
                {
                    const U8 scalarSize = PlyGetSizeofScalarType(property->scalarType);
                    const U64 newSize = element->dataSize + scalarSize;

                    /* prevent overflow */
                    if (newSize < element->dataSize) {
                        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                    }

                    /*advance data pointer by sizeof(property.scalarType)*/
                    dataPrev += scalarSize;

                    /* prevent buffer overrun */
                    if (dataPrev > dataLast) {
                        return PLY_MALFORMED_DATA_ERROR;
                    }

                    element->dataSize = newSize;
                }
                else
                {
                    U64 listCount = 0u;
                    {
                        /* get list data count */
                        const U8 listcountTypeSize = PlyGetSizeofScalarType(property->listCountType);

                        const U64 newLen = element->dataSize + listcountTypeSize;

                        /* prevent overflow */
                        if (newLen < element->dataSize) {
                            return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                        }

                        U8 sze = PlyGetSizeofScalarType(property->scalarType);

                        /*advance data pointer by sizeof(property.scalarType)*/
                        dataPrev += listcountTypeSize;

                        /* prevent buffer overrun */
                        if (dataPrev > dataLast)
                            return PLY_MALFORMED_DATA_ERROR;
                        

                        /*copy list count from data into list count var */
                        listCount = PlyScaleBytesToD64(dataPrev - listcountTypeSize, listcountTypeSize);

                        element->dataSize = newLen;
                    }

                    /* copy list elements into data*/
                    U64 readPropCount;
                    for (readPropCount = 0u; readPropCount < listCount; ++readPropCount)
                    {
                        /* get list data count */
                        const U8 scalarSize = PlyGetSizeofScalarType(property->scalarType);

                        const U64 newLen = element->dataSize + scalarSize;

                        /* prevent overflow */
                        if (newLen < element->dataSize) {
                            return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                        }

                        /*advance data pointer by sizeof(property.scalarType)*/
                        dataPrev += scalarSize;

                        /* prevent buffer overrun */
                        if (dataPrev > dataLast)
                            return PLY_MALFORMED_DATA_ERROR;

                        element->dataSize = newLen;
                    }

                    if (readPropCount != listCount) {
                        return PLY_MALFORMED_DATA_ERROR;
                    }
                }
            }
        }
        element->data = (void*)totalAllocSize;
        totalAllocSize += element->dataSize;
    }


   
    scene->sharedElementData = plyRealloc(NULL, totalAllocSize);
    /* now that data has been allocated, the actual buffer offsets still need to be updated */
    for (ei = 0; ei < scene->elementCount; ++ei)
    {
        scene->elements[ei].data = (U8*)scene->sharedElementData + (U64)(scene->elements[ei].data); /*apply offsets*/
    }


    /*reset data prev*/
    dataPrev = dataLast-dataSize+1;

    for (ei = 0; ei < scene->elementCount; ++ei)
    {
        struct PlyElement* element = scene->elements + ei;
        if (element->dataLineCount == 0) {
            continue; /* empty element (idk if this is permitted by the standard or not) */
        }

      
        dataBegin = dataPrev; /* reset on every new elemene that is being read to prevent incorrect offset of dataLineBegins */

        U64 curDataOffset = 0;
      
        U64 dli = 0;
        for (; dli < element->dataLineCount; ++dli)
        {


            const U8* dataLineBegin = dataPrev;
            element->dataLineBegins[dli] = dataPrev - dataBegin;

            U64 pi;
            for (pi = 0; pi < element->propertyCount; ++pi)
            {
                struct PlyProperty* property = element->properties + pi;

                if (property->dataType == PLY_DATA_TYPE_SCALAR)
                {
                    const U8 scalarSize = PlyGetSizeofScalarType(property->scalarType);


                    /* set property data line offset */
                    const U64 datalineOffset = dataPrev - dataLineBegin;
                    property->dataLineOffsets[dli] = datalineOffset;
                    
                
                    const U64 totalOffset = datalineOffset + element->dataLineBegins[dli];
                    U8* copyTo = (U8*)(element->data) + totalOffset;
                    memcpy(
                        copyTo, /*copy into data*/
                        dataPrev, /*from mem*/
                        scalarSize
                    ); 

                    if (systemEndianness != scene->format)
                        PlySwapBytes(copyTo, property->scalarType);

                    /*advance data pointer by sizeof(property.scalarType)*/
                    dataPrev += scalarSize;

                }
                else
                {
                    U64 listCount = 0u;
                    {
                        /* get list data count */
                        const U8 listcountTypeSize = PlyGetSizeofScalarType(property->listCountType);

                        /* set property data line offset */
                        const U64 datalineOffset = dataPrev - dataLineBegin;
                        property->dataLineOffsets[dli] = datalineOffset;

                        const U64 totalOffset = datalineOffset + element->dataLineBegins[dli];
                        U8* copyTo = (U8*)(element->data) + totalOffset;
                        memcpy(
                            copyTo, /*copy into data*/
                            dataPrev, /*from mem*/
                            listcountTypeSize
                        );

                        /*copy list count from data into list count var */
                        listCount = PlyScaleBytesToD64(dataPrev, listcountTypeSize);

                   
                        if (systemEndianness != scene->format)
                            PlySwapBytes(copyTo, property->listCountType);

                        /*advance data pointer by sizeof(property.scalarType)*/
                        dataPrev += listcountTypeSize;
                    }

                    /* get list data count */
                    const U8 scalarSize = PlyGetSizeofScalarType(property->scalarType);

                    /* copy list elements into data*/
                    U64 readPropCount;
                    for (readPropCount=0u; readPropCount < listCount; ++readPropCount)
                    {
                        const U64 offsetFromDatalineOffset = dataPrev - dataLineBegin;

                        const U64 totalOffset = offsetFromDatalineOffset + element->dataLineBegins[dli];
                        U8* copyTo = (U8*)(element->data) + totalOffset;
                        memcpy(
                            copyTo, /*copy into data*/
                            dataPrev, /*from mem*/
                            scalarSize
                        );

                        if (systemEndianness != scene->format)
                            PlySwapBytes(copyTo, property->scalarType);

                        /*advance data pointer by sizeof(property.scalarType)*/
                        dataPrev += scalarSize;
                    }

                    if (readPropCount != listCount) {
                        return PLY_MALFORMED_DATA_ERROR;
                    }
                }
            }
        }
    }

    return PLY_SUCCESS;
}








enum PlyResult readDataASCII(struct PlyScene* scene, const U8* dataBegin, const U8* dataLast)
{
    const U64 dataSize = (dataLast - dataBegin) + 1;

    const char* line = (const char*)dataBegin;
    U64 lineLen = lineLen_s(line, (const char*)dataBegin, dataSize);




    U64 totalAllocSize = 0u;

    U64 ei;
    /*precompute the total amount of data that will be needed for each element*/
    for (ei = 0; ei < scene->elementCount; ++ei)
    {
        struct PlyElement* element = scene->elements + ei;
        if (element->dataLineCount == 0) {
            continue; /* empty element (idk if this is permitted by the standard or not) */
        }

        /* create data lines for element and all its properties*/
        if (allocateDataLinesForElement(element) != PLY_SUCCESS)
            return PLY_FAILED_ALLOC_ERROR;

        element->dataSize = 0u;

        U64 dli = 0;
        for (; dli < element->dataLineCount; ++dli)
        {
            U32 ploffset = 0u;
            const char* ch = line;

            const U64 lineBegin = element->dataSize;
            element->dataLineBegins[dli] = lineBegin;

            U32 countedProperties = 0u;

            U32 pi = 0u;
            for (; pi < element->propertyCount; ++pi)
            {
                struct PlyProperty* property = element->properties + pi;
                if (property->dataType == PLY_DATA_TYPE_SCALAR)
                {
                    for (; ch < line + lineLen; ++ch)
                    {
                        if (isspace(*ch) == 0)
                        {
                            countedProperties++;

                            U8 scalarStrLen;
                            union PlyScalarUnion dataU = PlyStrToScalar(ch, property->scalarType, &scalarStrLen);
                            if (scalarStrLen == 0u) {
                                return PLY_DATA_TYPE_MISMATCH_ERROR;
                            }
                            else {
                                ch += scalarStrLen;
                                /* prevent buffer overrun */
                                if (ch > line + lineLen) {
                                    return PLY_MALFORMED_DATA_ERROR;
                                }
                            }

                            const U8 sze = PlyGetSizeofScalarType(element->properties[pi].scalarType);

                            const U64 newLen = element->dataSize + sze;
                            if (newLen < element->dataSize)
                            { /* prevent overflow */
                                return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                            }

                            property->dataLineOffsets[dli] = ploffset;

                            /* prevent overflow */
                            if (ploffset > (U32)ploffset + sze) {
                                return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                            }

                            ploffset += sze;
                            element->dataSize = newLen;


                            break; /*found the property value, now stop and go to the next.*/
                        }
                    }
                }
                else
                {
                    /* get list data count */
                    U64 listCount = 0u;

                    for (; ch < line + lineLen; ++ch)
                    {
                        if (isspace(*ch) == 0)
                        {
                            countedProperties++;

                            U8 listCountStrLen;
                            union PlyScalarUnion listCountU = PlyStrToScalar(ch, property->listCountType, &listCountStrLen);
                            if (listCountStrLen == 0u) {
                                return PLY_DATA_TYPE_MISMATCH_ERROR;
                            }
                            else {
                                ch += listCountStrLen;
                                /* prevent buffer overrun */
                                if (ch > line + lineLen) {
                                    return PLY_MALFORMED_DATA_ERROR;
                                }
                            }

                            U8 sze = PlyGetSizeofScalarType(property->listCountType);

                            const U64 newLen = element->dataSize + sze;
  

                            element->dataSize = newLen;
                            element->properties[pi].dataLineOffsets[dli] = ploffset;


                            listCount = (U64)PlyScaleBytesToD64(&listCountU, property->listCountType);


                            /* prevent overflow */
                            if (ploffset > (U32)(ploffset + sze + PlyGetSizeofScalarType(property->scalarType) * listCount)) {
                                return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                            }
                            ploffset += (U32)(sze + PlyGetSizeofScalarType(property->scalarType) * listCount);

                            break; /* found the list size */
                        }
                    }


                    U64 readPropCount = 0u;
                    /* get list items */
                    for (; ch < line + lineLen; ++ch)
                    {
                        if (isspace(*ch) == 0)
                        {
                            U8 scalarStrLen;
                            union PlyScalarUnion dataU = PlyStrToScalar(ch, property->scalarType, &scalarStrLen);
                            if (scalarStrLen == 0u) {
                                return PLY_DATA_TYPE_MISMATCH_ERROR;
                            }
                            else {
                                ch += scalarStrLen;
                                /* prevent buffer overrun */
                                if (ch > line + lineLen) {
                                    return PLY_MALFORMED_DATA_ERROR;;
                                }
                            }

                            U8 sze = PlyGetSizeofScalarType(property->scalarType);

                            const U64 newLen = element->dataSize + sze;

                            element->dataSize = newLen;

                            if (readPropCount + 1 == listCount) {
                                readPropCount++;
                                break; /* all elements have been read */
                            }
                            readPropCount++;

                            /* DO NOT BREAK LOOP, KEEP READING UNTIL LINE END */
                        }
                    }

                    if (readPropCount != listCount)

                    { /* mismatch between actual list count and expected list count */
                        return PLY_LIST_COUNT_MISMATCH_ERROR;
                    }
                }
            }
            ploffset = 0u;

            if (countedProperties != element->propertyCount) {
                return PLY_MALFORMED_DATA_ERROR;
            }

            line = getNextLine(&lineLen, dataBegin, dataSize, line, lineLen);
            if (dli != element->dataLineCount - 1 && !line) {
                return PLY_MALFORMED_DATA_ERROR;
            }
        }

        element->data = (void*)totalAllocSize;
        totalAllocSize += element->dataSize;
    }



    scene->sharedElementData = plyRealloc(NULL, totalAllocSize);
    /* now that data has been allocated, the actual buffer offsets still need to be updated */
    for (ei = 0; ei < scene->elementCount; ++ei) {
        scene->elements[ei].data = (U8*)scene->sharedElementData + (U64)(scene->elements[ei].data); /*apply offsets*/
    }




    line = (const char*)dataBegin;
    lineLen = lineLen_s(line, (const char*)dataBegin, dataSize);


    for (ei=0; ei < scene->elementCount; ++ei)
    {
        struct PlyElement* element = scene->elements + ei;
        if (element->dataLineCount == 0) {
            continue; /* empty element (idk if this is permitted by the standard or not) */
        }

        U64 curDataOffset = 0u;

        U64 dli = 0;
        for (; dli < element->dataLineCount; ++dli)
        {
            U32 ploffset = 0u;
            const char* ch = line;

            U32 countedProperties = 0u;

            U32 pi = 0u;
            for (; pi < element->propertyCount; ++pi)
            {
                struct PlyProperty* property = element->properties + pi;
                if (property->dataType == PLY_DATA_TYPE_SCALAR)
                {
                    for (; ch < line + lineLen; ++ch)
                    {
                        if (isspace(*ch) == 0)
                        {
                            countedProperties++;

                            U8 scalarStrLen;
                            union PlyScalarUnion dataU = PlyStrToScalar(ch, property->scalarType, &scalarStrLen);
                            if (scalarStrLen == 0u) {
                                return PLY_DATA_TYPE_MISMATCH_ERROR;
                            }
                            else {
                                ch += scalarStrLen;
                            }
                            
                            const U8 sze = PlyGetSizeofScalarType(element->properties[pi].scalarType);


                            property->dataLineOffsets[dli] = ploffset;

 

                            const U8* copyTo = (U8*)element->data + curDataOffset;
                            PlyScalarUnionCpyIntoLocation(copyTo, &dataU, property->scalarType);

                            ploffset += sze;
                            curDataOffset += sze;

                            break; /*found the property value, now stop and go to the next.*/
                        }
                    }
                }
                else
                {  
                    /* get list data count */
                    U64 listCount = 0u;
 
                    for (; ch < line + lineLen; ++ch)
                    {
                        if (isspace(*ch) == 0)
                        {
                            countedProperties++;

                            U8 listCountStrLen;
                            union PlyScalarUnion listCountU = PlyStrToScalar(ch, property->listCountType, &listCountStrLen);
                            if (listCountStrLen == 0u) {
                                return PLY_DATA_TYPE_MISMATCH_ERROR;
                            }
                            else {
                                ch += listCountStrLen;
                            }

                            U8 sze = PlyGetSizeofScalarType(property->listCountType);


                            element->properties[pi].dataLineOffsets[dli] = ploffset;



                            /*copy list count into data*/
                            const U8* copyTo = (U8*)element->data + curDataOffset;
                            PlyScalarUnionCpyIntoLocation(copyTo, &listCountU, property->listCountType);



                            listCount = (U64)PlyScaleBytesToD64(&listCountU, property->listCountType);
                            ploffset += (U32)(sze + PlyGetSizeofScalarType(property->scalarType)*listCount);

                            curDataOffset += sze;

                            break; /* found the list size */
                        }
                    }


                    U64 readPropCount=0u;
                    /* get list items */
                    for (; ch < line + lineLen; ++ch)
                    {
                        if (isspace(*ch) == 0)
                        {
                            U8 scalarStrLen;
                            union PlyScalarUnion dataU = PlyStrToScalar(ch, property->scalarType, &scalarStrLen);
                            if (scalarStrLen == 0u) {
                                return PLY_DATA_TYPE_MISMATCH_ERROR;
                            }
                            else {
                                ch += scalarStrLen;
                            }
                            
                            U8 sze = PlyGetSizeofScalarType(property->scalarType);

                           

                            /*copy list element into data*/
                            const U8* copyTo = (U8*)element->data + curDataOffset;
                            PlyScalarUnionCpyIntoLocation(
                                copyTo, &dataU, property->scalarType);

                            curDataOffset += sze;

                            if (readPropCount+1 == listCount)
                            {
                                readPropCount++;
                                break; /* all elements have been read */
                            }
                            readPropCount++;

                            /* DO NOT BREAK LOOP, KEEP READING UNTIL LINE END */
                        }
                    }

                    if (readPropCount<listCount)

                    { /* mismatch between actual list count and expected list count */
                        return PLY_LIST_COUNT_MISMATCH_ERROR;
                    }
                }
            }
            ploffset = 0u;

            if (countedProperties != element->propertyCount)
            {
                return PLY_MALFORMED_DATA_ERROR;
            }

            line = getNextLine(&lineLen, dataBegin, dataSize, line, lineLen);
            if (dli != element->dataLineCount-1 && !line) {
                return PLY_MALFORMED_DATA_ERROR;
            }
        }
    }

    return PLY_SUCCESS;
}















enum PlyResult PlyLoadFromMemory(const U8* mem, U64 memSize, struct PlyScene* scene, struct PlyLoadInfo* loadInfo)
{
    if (memSize == 0)
    {
        return PLY_SUCCESS; /* there is nothing to read */
    }

    memset(scene, 0, sizeof(*scene));

    const char* srcline = (const char*)mem;
    U64 srclineSize = lineLen_s(srcline, (const char*)mem, memSize);

    struct PlyElement* curElement = NULL;

    bool readingHeader = false;
    bool headerFinished = false;

    U64 i = 0;
    for (; i < UINT32_MAX; ++i)
    {
        if (srcline > (const char*)(mem + memSize)) { break; }

        U32 lineLen;
        char line[C_PLY_MAX_LINE_LENGTH+1];


        parseLine(srcline, srclineSize, line, sizeof(line), &lineLen);
        

        if (lineLen > 0)
        {
            if (!headerFinished) {
                static bool lastReadingHeader = false;
                lastReadingHeader = readingHeader;
                /* parse header */
                const enum PlyResult exRes = readHeaderLine(line, lineLen, &readingHeader, &curElement, scene, loadInfo);

                if (exRes == PLY_SUCCESS)
                {
                    if (!readingHeader && lastReadingHeader==true) {
                        headerFinished = true;
                    }
                }
                else {
                    return exRes;
                }
            }
            else {
                /* the header has ended, read data(the line no.will stop incrementing, */
                /* control over data traversal is handed to readData) */

                enum PlyResult exRes = PLY_GENERIC_ERROR;
                if (scene->format == PLY_FORMAT_ASCII) {
                    exRes = readDataASCII(scene, (const U8*)srcline, (const U8*)(mem + memSize) - 1);
                }
                if (exRes == PLY_SUCCESS) {
                    return PLY_SUCCESS;
                } else {
                    return exRes;
                }
            }
        }

        
        if (headerFinished && (scene->format == PLY_FORMAT_BINARY_BIG_ENDIAN || scene->format == PLY_FORMAT_BINARY_LITTLE_ENDIAN)) {
            srcline = srcline + srclineSize + strlen("\n");

            enum PlyResult exRes = PLY_GENERIC_ERROR;
            exRes = readDataBinary(scene, (const U8*)srcline, (const U8*)(mem + memSize));
            if (exRes == PLY_SUCCESS) {
                return PLY_SUCCESS;
            } else {
                return exRes;
            }
        }
        else {
            srcline = getNextLine(&srclineSize, mem, memSize, srcline, srclineSize);
            if (srcline == NULL) {
                break;
            }
        }
    }

    /* the header never ended */
    if (headerFinished==false) {
        return PLY_MALFORMED_HEADER_ERROR;
    }

    return PLY_SUCCESS;
}

enum PlyResult PlyLoadFromDisk(const char* fileName, struct PlyScene* scene, struct PlyLoadInfo* loadInfo)
{
	enum PlyResult resCode = PLY_SUCCESS;
	FILE* fptr = NULL;
	fopen_s(&fptr, fileName, "rb");
	if (fptr == NULL) {
        memset(scene, 0, sizeof(*scene));
		resCode = PLY_FILE_READ_ERROR;
		return resCode;
	}

    /* get file size */
    _fseeki64(fptr, 0, SEEK_END);
    const long long fsze = _ftelli64(fptr);
    rewind(fptr);

    U8* fileData = NULL;

    if (fsze < 0)
        goto bail;


    fileData = plyRealloc(NULL, fsze + 1);

    if (fileData == NULL) {
        resCode = PLY_FAILED_ALLOC_ERROR;
        goto bail;
    }


	fread_s(fileData, fsze, fsze, 1, fptr);
    if (fptr)
        fclose(fptr);

    fileData[fsze] = '\0';
    resCode=PlyLoadFromMemory(fileData, fsze, scene, loadInfo);

bail:
	if (fileData) {
		plyDealloc(fileData);
	}
	
	return resCode;
}

enum PlyResult PlyLoadFromDiskW(const wchar_t* fileName, struct PlyScene* scene, struct PlyLoadInfo* loadInfo)
{
    enum PlyResult resCode = PLY_SUCCESS;
    FILE* fptr = NULL;
    _wfopen_s(&fptr, fileName, L"rb");
    if (fptr == NULL) {
        memset(scene, 0, sizeof(*scene));
        resCode = PLY_FILE_READ_ERROR;
        return resCode;
    }

    /* get file size */
    _fseeki64(fptr, 0, SEEK_END);
    const long long fsze = _ftelli64(fptr);
    rewind(fptr);

    U8* fileData = NULL;

    if (fsze < 0)
        goto bail;
    

    fileData = plyRealloc(NULL, fsze + 1);

    if (fileData == NULL) {
        resCode = PLY_FAILED_ALLOC_ERROR;
        goto bail;
    }

    fread_s(fileData, fsze, fsze, 1, fptr);
    if (fptr)
        fclose(fptr);

    fileData[fsze] = '\0';
    resCode = PlyLoadFromMemory(fileData, fsze, scene, loadInfo);

bail:
    if (fileData) {
        plyDealloc(fileData);
    }

    return resCode;
}





void PlyDestroyScene(struct PlyScene* scene)
{
    if (scene->elements) {
        U64 i = 0;
        for (; i < scene->elementCount; ++i)
        {
            struct PlyElement* ele = scene->elements + i;
            U64 pi;
            for (pi = 0; pi < ele->propertyCount; ++pi)
            {
                if (ele->properties[pi].dataLineOffsets)
                    free(ele->properties[pi].dataLineOffsets);
            }


            if (ele->properties)
            {
                plyDealloc(ele->properties);
            }
            if (ele->dataLineBegins)
            {
                plyDealloc(ele->dataLineBegins);
            }
        }
        plyDealloc(scene->elements);
        scene->elementCount = 0u;
        scene->elements = NULL;
    }

    if (scene->sharedElementData) {
        free(scene->sharedElementData);
        scene->sharedElementData = NULL;
    }
    if (scene->objectInfos) {
        free(scene->objectInfos);
        scene->objectInfoCount = 0u;
        scene->objectInfos = NULL;
    }

    if (scene->comments) {
        U64 ci;
        for (ci = 0; ci < scene->commentCount; ++ci)
        {
            free(scene->comments[ci]);
        }
        free(scene->comments);
        scene->comments = NULL;
        scene->commentCount = 0u;
    }
}










U8 strtou8(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0-init */
        *strLenOut = 0;
    U8 num = 0;
    const U8 max_digits = 3;
    
    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {

            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                if (num > (UINT8_MAX - (str[i] - '0')) / 10) {
                    if (strLenOut)
                        *strLenOut = 0;
                    return 0;
                }
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    return num;
}

U16 strtou16(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0 - init */
        *strLenOut = 0;
    U16 num = 0;
    const U8 max_digits = 5;
    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {
            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                if (num > (UINT16_MAX - (str[i] - '0')) / 10) {
                    if (strLenOut)
                        *strLenOut = 0;
                    return 0;
                }
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    return num;
}

U32 strtou32(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0-init */
        *strLenOut = 0;
    U32 num = 0;

    const U8 max_digits = 10;
    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {
            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                if (num > (UINT32_MAX - (str[i] - '0')) / 10) {
                    if (strLenOut)
                        *strLenOut = 0;
                    return 0;
                }
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    return num;
}

U64 strtou64(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0-init */
        *strLenOut = 0;
    U32 num = 0;
    const U8 max_digits = 20;
    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {
            if (num > (UINT64_MAX - (str[i]-'0')) / 10) {
                if (strLenOut)
                    *strLenOut = 0;
                return 0;
            }
            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    return num;
}


I8 strtoi8(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0-init */
        *strLenOut = 0;
    I8 num = 0;
    char negative = 0u;
    if (*str == '-') {
        negative = 1;
        str++;
    }
    const U8 max_digits = 3;
    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {
            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                if (negative == 0) {
                    if (num > (INT8_MAX - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                else {
                    if (num > (I8)(abs(INT8_MIN) - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    if (negative)
        num *= -1;
    return num;
}

I16 strtoi16(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0-init */
        *strLenOut = 0;
    I16 num = 0;
    char negative = 0u;
    if (*str == '-') {
        negative = 1;
        str++;
    }
    const U8 max_digits = 5;
    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {
            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                if (negative == 0) {
                    if (num > (INT16_MAX - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                else {
                    if (num > (I16)(abs(INT16_MIN) - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    if (negative)
        num *= -1;
    return num;
}

I32 strtoi32(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0-init */
        *strLenOut = 0;
    I32 num = 0;
    char negative = 0u;
    if (*str == '-') {
        negative = 1;
        str++;
    }

    const U8 max_digits = 5;

    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {
            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                if (negative == 0) {
                    if (num > (INT32_MAX - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                else {
                    if (num > (I32)(abs(INT32_MIN) - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    if (negative)
        num *= -1;
    return num;
}

I64 strtoi64(const char* str, U8* strLenOut)
{
    if (*strLenOut) /* 0-init */
        *strLenOut = 0;
    I64 num = 0;
    char negative = 0u;
    if (*str == '-') {
        negative = 1;
        str++;
    }
    const U8 max_digits = 5;

    I8 i = 0;
    for (; i <= max_digits; ++i)
    {
        if (i == max_digits) {
            if (!(str[i] == '\0' || str[i] == '\r' || str[i] == '\n') && !isspace(str[i])) {
                *strLenOut = 0x0;
                return 0x0;
            }
        }
        else {
            if (str[i] < '0' || str[i] > '9') {
                break;
            }
            else {
                if (negative == 0) {
                    if (num > (INT64_MAX - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                else {
                    if (num > (I64)(llabs(INT64_MIN) - (str[i] - '0')) / 10) {
                        if (strLenOut)
                            *strLenOut = 0;
                        return 0;
                    }
                }
                num = num * 10 + (str[i] - 48);
                if (strLenOut)
                    (*strLenOut)++;
            }
        }
    }
    if (negative)
        num *= -1;
    return num;
}


float strtof32(const char* str, U8* strLenOut)
{
    const char* start = str;
    char* end;
    errno = 0;
    float num = strtof(str, &end);
    if (errno == ERANGE) {
        if (strLenOut)
            *strLenOut = 0x0;
        return 0;
    }
    if (strLenOut)
        *strLenOut = (U8)(end - start);

    return num;
}

double strtod64(const char* str, U8* strLenOut)
{
    const char* start = str;
    char* end;
    errno = 0;
    double num = strtod(str, &end);
    if (errno == ERANGE) {
        if (strLenOut)
            *strLenOut = 0x0;
        return 0;
    }
    if (strLenOut)
        *strLenOut = (U8)(end - start);
   


    return num;
}



#ifdef __cplusplus
}
#endif /*__cplusplus*/