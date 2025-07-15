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



/*
* returns the index of an element within a property.
* If the element is not in that property, -1 will be returned.*/
PLY_H_FUNCTION_PREFIX I64 PlyGetPropertyIndex(const struct PlyElement* element, const struct PlyProperty* property)
{
    const I64 calculatedIndex = property - element->properties;
    if (calculatedIndex < 0 || calculatedIndex > element->propertyCount) {
        return -1;
    }

    return calculatedIndex;
}

/*
* returns the index of an element within a property from it's name.
* If the element is not in that property, -1 will be returned. */
PLY_H_FUNCTION_PREFIX I64 PlyGetPropertyIndexByName(const struct PlyElement* element, const char* propertyName)
{
    U32 i = 0;
    for (; i < element->propertyCount; ++i)
    {
        if (streql(element->properties[i].name, propertyName)) {
            return i;
        }
    }
    return -1;
}

/* adds a PlyProperty to an element.The property will be copied, thus transferring ownership */
PLY_INLINE enum PlyResult  PlyElementAddProperty(struct PlyElement* element, struct PlyProperty* property)
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
PLY_INLINE enum PlyResult PlySceneAddObjectInfo(struct PlyScene* scene, struct PlyObjectInfo* objInfo)
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
PLY_INLINE enum PlyResult PlySceneAddElement(struct PlyScene* scene, struct PlyElement* element)
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


PLY_INLINE static U32 lineLen_s(const char* srcline, const char* mem, U64 memSize)
{
    if (!srcline || srcline < mem || srcline > mem + memSize - 1)
        return 0x0;

    U64 maxDist = (U64)((mem + (U64)memSize) - srcline);
    if (maxDist > UINT32_MAX)
        return 0x0;

    const char* end = mem + memSize;
    const char* cur = srcline;
    while (cur < end)
    {
        char c = *cur;
        if (c == '\0' || c == '\n' || c == '\r')
            return (U32)(cur - srcline);
        ++cur;
    }

    /* invalid line, no null terminator */
    return 0u;
}


PLY_INLINE static void parseLine(const char* lineIn, U64 lineInSize, char* dst, const U32 dstSize, U32* strlenOut)
{
    *strlenOut = 0;

    if (!lineIn || !dst || dstSize == 0 || lineInSize == 0)
        return;

    /* Skip leading whitespace*/
    U64 start = 0;
    while (start < lineInSize && isblank((unsigned char)lineIn[start]))
        ++start;

    if (start == lineInSize)
        return;

    /* Remove trailing whitespace */
    U64 end = lineInSize;
    while (end > start && isblank((unsigned char)lineIn[end - 1]))
        --end;

    U64 cleanLen = end - start;
    if (cleanLen >= dstSize)
        cleanLen = dstSize - 1;

    memcpy(dst, lineIn + start, cleanLen);
    dst[cleanLen] = '\0';
    *strlenOut = (U32)cleanLen;
}

PLY_INLINE static const char* getNextLine(U64* lenOut, const U8* mem, U64 memSize, const char* lastLine, const U64 lastLineLen)
{
    if (lastLineLen > UINT32_MAX || memSize > UINT32_MAX) {
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
    if (dist == 0) {
        return NULL;
    }
    const U64 loopLimit1 = dist - 1;
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
        return NULL;



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

static PLY_INLINE const char* getNextSpace(const char* srchBegin, const char* srchEnd)
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


static PLY_INLINE const char* getNextNonSpace(const char* srchBegin, const char* srchEnd)
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

static PLY_INLINE enum PlyResult parseProperty(struct PlyElement* owningElement, const char* propRangeFirst, const char* propRangeLast)
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


static PLY_INLINE enum PlyResult parseObjectInfo(struct PlyScene* scene, const char* objBegin, const char* objEnd)
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

static PLY_INLINE enum PlyResult readHeaderLine(const char* line, const U32 lineLen, bool* readingHeader, struct PlyElement** curElement, struct PlyScene* scene, struct PlyLoadInfo* loadInfo)
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

                unsigned char* tmp = (unsigned char*)plyRealloc(NULL, commentLen + 1);
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
            
            if (loadInfo && loadInfo->elementsCount != PLY_LOAD_ALL_ELEMENTS) {
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



static enum PlyResult readDataBinary(struct PlyScene* scene, const U8* dataBegin, const U8* dataLast)
{
    if (scene->elementCount == 0)
        return PLY_SUCCESS;

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


            element->dataLineBegins[dli] = dataPrev - dataBegin;

            U64 pi;
            for (pi = 0; pi < element->propertyCount; ++pi)
            {

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

                        /*advance data pointer by sizeof(property.scalarType)*/
                        dataPrev += listcountTypeSize;

                        /* prevent buffer overrun */
                        if (dataPrev > dataLast)
                            return PLY_MALFORMED_DATA_ERROR;
                        

                        /*copy list count from data into list count var */
                        listCount = (U64)PlyScaleBytesToD64(dataPrev - listcountTypeSize, listcountTypeSize);
                        
                        if (systemEndianness != scene->format)
                            listCount = PLY_BYTESWAP64(listCount);
                            //PlySwapBytes((U8*)&listCount, property->listCountType);

                        element->dataSize = newLen;
                    }


                    const U8 scalarSize = PlyGetSizeofScalarType(property->scalarType);

                    const U64 listSize = scalarSize * listCount; /*in bytes*/

                    /*prevent overflow*/
                    if (element->dataSize + listSize < element->dataSize) {
                        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
                    }

                    element->dataSize = element->dataSize + listSize;

                    dataPrev += listSize;
                    if (dataPrev > dataLast) /*prevent out of bounds read*/
                        return PLY_MALFORMED_FILE_ERROR;
                }
            }
        }
        element->data = (void*)totalAllocSize;
        totalAllocSize += element->dataSize;
    }


    if (totalAllocSize == 0) {
        return PLY_SUCCESS; /*nothing to allocate*/
    }

    scene->sharedElementData = plyRealloc(NULL, totalAllocSize);
    if (!scene->sharedElementData) {
        return PLY_FAILED_ALLOC_ERROR;
    }

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

      
        dataBegin = dataPrev; /* reset on every new element that is being read to prevent incorrect offset of dataLineBegins */
      
        U64 dli = 0;
        for (; dli < element->dataLineCount; ++dli)
        {
            const U8* dataLineBegin = dataPrev;

            U64 pi;
            for (pi = 0; pi < element->propertyCount; ++pi)
            {
                struct PlyProperty* property = element->properties + pi;
                
                const U8 scalarSize = PlyGetSizeofScalarType(property->scalarType);

                if (property->dataType == PLY_DATA_TYPE_SCALAR)
                {


                    /* set property data line offset */
                    const U64 datalineOffset = dataPrev - dataLineBegin;
                    if (datalineOffset > UINT32_MAX)
                        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;

                    property->dataLineOffsets[dli] = (U32)datalineOffset;
                    
                
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
                        if (datalineOffset > UINT32_MAX)
                            return PLY_EXCEEDS_BOUND_LIMITS_ERROR;

                        property->dataLineOffsets[dli] = (U32)datalineOffset;

                        const U64 totalOffset = datalineOffset + element->dataLineBegins[dli];
                        U8* copyTo = (U8*)(element->data) + totalOffset;
                        memcpy(
                            copyTo, /*copy into data*/
                            dataPrev, /*from mem*/
                            listcountTypeSize
                        );

                        /*copy list count from data into list count var */
                        listCount = (U64)PlyScaleBytesToD64(dataPrev, listcountTypeSize);
                        if (systemEndianness != scene->format)
                            PlySwapBytes(copyTo, property->listCountType);

                        /*advance data pointer by sizeof(property.scalarType)*/
                        dataPrev += listcountTypeSize;
                    }

                    U64 offsetFromDatalineOffset = dataPrev - dataLineBegin;

                    const U64 listSize = scalarSize * listCount; /*in bytes*/
                    const U64 totalOffset = offsetFromDatalineOffset + element->dataLineBegins[dli];
                    U8* copyTo = (U8*)(element->data) + totalOffset;
                    memcpy(
                        copyTo, /*copy into data*/
                        dataPrev, /*from mem */
                        listSize
                    );

                    /* correct endianness */
                    if (systemEndianness != scene->format) {
                        /*offsetFromDatalineOffset is not actually line offset, just data offset. I'm just reusing stack space.*/
                        for (offsetFromDatalineOffset = 0; offsetFromDatalineOffset < listCount*scalarSize; offsetFromDatalineOffset +=scalarSize) {
                            PlySwapBytes(copyTo + offsetFromDatalineOffset, property->scalarType);

                        }
                    }

                    /*increment data prev*/
                    dataPrev += listSize;
                }
            }
        }
    }

    return PLY_SUCCESS;
}








static enum PlyResult readDataASCII(struct PlyScene* scene, const U8* dataBegin, const U8* dataLast)
{
    if (scene->elementCount == 0)
        return PLY_SUCCESS;

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
                            PlyStrToScalar(ch, property->scalarType, &scalarStrLen);

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
                            PlyStrToScalar(ch, property->scalarType, &scalarStrLen);

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


    if (totalAllocSize == 0) {
        return PLY_SUCCESS; /*nothing to allocate*/
    }

    scene->sharedElementData = plyRealloc(NULL, totalAllocSize);
    if (!scene->sharedElementData) {
        return PLY_FAILED_ALLOC_ERROR;
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

 

                            U8* copyTo = (U8*)element->data + curDataOffset;
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
                            U8* copyTo = (U8*)element->data + curDataOffset;
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
                            U8* copyTo = (U8*)element->data + curDataOffset;
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

        if (srclineSize == 0)
            continue; /*skip empty line*/

        parseLine(srcline, srclineSize, line, C_PLY_MAX_LINE_LENGTH, &lineLen);
        

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
                return exRes;
            }
        }

        
        if (headerFinished && (scene->format == PLY_FORMAT_BINARY_BIG_ENDIAN || scene->format == PLY_FORMAT_BINARY_LITTLE_ENDIAN)) {
            srcline = srcline + srclineSize + strlen("\n");

            enum PlyResult exRes = PLY_GENERIC_ERROR;
            exRes = readDataBinary(scene, (const U8*)srcline, (const U8*)(mem + memSize));
            return exRes;
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
    if (scene->elementCount > 0) { /*element were expected, but data was never read*/
        return PLY_MALFORMED_DATA_ERROR;
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

    if (fsze <= 0)
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