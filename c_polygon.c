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


#include "c_polygon.inl"




    /*returns true if collision*/
bool PLY_INLINE checkForElementNameCollision(const struct PlyScene* scene, const char* name)
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

/*returns true if collision*/
bool PLY_INLINE checkForPropertyNameCollision(const struct PlyElement* element, const char* name)
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


static void dtoa_s(double x, uint16_t decimalPlaces, char* buff, const U16 buffSize) {
#ifndef NDEBUG
    if (buffSize == 0)
        assert(00 && "BUFFER SIZE MUST BE GREATER THAN 0.");
#endif 

    snprintf(buff, buffSize, "%.*f", decimalPlaces, x);

    /* Remove trailing zeros */
    char* dot = strchr(buff, '.');
    if (dot) {
        char* end = buff + strlen(buff) - 1;
        while (end > dot && *end == '0') {
            *end-- = '\0';
        }
        /* Remove dot if nothing remains after it */
        if (end == dot) {
            *end = '\0';
        }
    }
    return;
}


static void utoa_s(U32 value, char* dst, const U16 dstSize) {
    if (dstSize == 0) return;

    char buffer[32];
    int i = 0;

    do {
        if (i >= (int)(sizeof(buffer) - 1)) break;
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    } while (value);

    if (i + 1 > dstSize) {
        dst[0] = '\0';
        return;
    }
    int j = 0;
    while (i--) {
        dst[j++] = buffer[i];
    }
    dst[j] = '\0';
}

static void itoa_s(I32 value, char* dst, const U16 dstSize) {
    if (dstSize == 0) return;

    if (value < 0) {
        if (dstSize < 2) {
            dst[0] = '\0';
            return;
        }
        dst[0] = '-';
        utoa_s((U32)(-value), dst + 1, dstSize - 1);
    }
    else {
        utoa_s((U32)value, dst, dstSize);
    }
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

PLY_H_FUNCTION_PREFIX enum PlyResult PlyElementSetName(struct PlyElement* element, const char* name)
{
    const U64 len = strlen(name);
    if (len >= sizeof(element->name)-1) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
    }
    memcpy(element->name, name, len+1);
    element->name[len]='\0';
    return PLY_SUCCESS;
}

PLY_INLINE U32 lineLen_s(const char* srcline, const char* mem, U64 memSize)
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


PLY_INLINE void parseLine(const char* lineIn, U64 lineInSize, char* dst, const U32 dstSize, U32* strlenOut)
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

PLY_INLINE const char* getNextLine(U64* lenOut, const U8* mem, U64 memSize, const char* lastLine, const U64 lastLineLen)
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

PLY_INLINE const char* getNextSpace(const char* srchBegin, const char* srchEnd)
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


PLY_INLINE const char* getNextNonSpace(const char* srchBegin, const char* srchEnd)
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

PLY_INLINE enum PlyResult parseProperty(struct PlyElement* owningElement, const char* propRangeFirst, const char* propRangeLast)
{
    /* data type */
    enum PlyDataType dtype = PLY_DATA_TYPE_SCALAR;
    /* scalar type */
    enum PlyScalarType stype = PLY_SCALAR_TYPE_UNDEFINED;
    /* list count type */
    enum PlyScalarType ltype = PLY_SCALAR_TYPE_UNDEFINED;

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

    if (stype == PLY_SCALAR_TYPE_UNDEFINED) {
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


PLY_INLINE enum PlyResult parseObjectInfo(struct PlyScene* scene, const char* objBegin, const char* objEnd)
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

PLY_INLINE enum PlyResult readHeaderLine(const char* line, const U32 lineLen, bool* readingHeader, struct PlyElement** curElement, struct PlyScene* scene, struct PlyLoadInfo* loadInfo)
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
                /*get version number(currently only v 1.0 is considered valid, unless allowAnyVersion is true)*/
                scene->versionNumber = strtof(vbegin, NULL);
                if (loadInfo->allowAnyVersion == false && scene->versionNumber != 1.0)
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
                    if (strneql(elementNameBegin,loadInfo->elements[i],elementNameLast-elementNameBegin+1)) {
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
        
        if (*curElement != NULL) {
            c = "property ";
            /* check for proerty declaration */
            if (strneql(line, c, min(strlen(c), lineLen)) == true)
            {
                
                enum PlyResult r = parseProperty(*curElement, line+strlen(c), lineLast);
                return r;
            }
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


PLY_INLINE enum PlyResult allocateDataLinesForElement(struct PlyElement* element)
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
    if (dataBegin > dataLast) {
        return PLY_GENERIC_ERROR;
    }
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
                        listCount = (U64)PlyScaleBytesToU64(dataPrev - listcountTypeSize, listcountTypeSize);
                        
                        if (systemEndianness != scene->format)
                            PlySwapBytes((U8*)&listCount, property->listCountType);

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
                        listCount = (U64)PlyScaleBytesToU64(dataPrev, listcountTypeSize);
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
#ifndef NDEBUG
    if (scene->format == PLY_FORMAT_BINARY_MATCH_SYSTEM) {
        assert(00 && "INVALID SCENE FORMAT: PLY_FORMAT_BINARY_MATCH_SYSTEM CAN ONLY BE USED WHEN SAVING FILES");
    }
#endif

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
            if (srcline > (const char*)mem+memSize) {
                return PLY_MALFORMED_FILE_ERROR;
            }
            enum PlyResult exRes = readDataBinary(scene, (const U8*)srcline, (const U8*)(mem + memSize));
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

/* memcpy clamped */
static void memcpy_c(U8* dst, const U8* dstEnd, const U64 cpySize, const U8* src, const U8* srcEnd, U64* totalDataLen)
{
    if (cpySize == 0)
        return;

    const U8* cpyEnd = src + cpySize - 1;
    if (dst) {
        while (true)
        {
            *dst = *src;
            if (src == srcEnd || src == cpyEnd || dst == dstEnd) {
                return;
            }
            if (totalDataLen) {
                (*totalDataLen)++;
            }
            src++;
            dst++;
        }
    }
    else {
        while (true)
        {
            if (src == srcEnd || src == cpyEnd) {
                return;
            }
            if (totalDataLen) {
                (*totalDataLen)++;
            }
            src++;
        }
    }
    
}
/* memcpy clamped advance dest*/
static void memcpy_ca(U8** dst, const U8* dstEnd, const U64 cpySize, const U8* src, const U8* srcEnd, U64* totalDataLen)
{
    if (cpySize == 0)
        return;

    const U8* cpyEnd = src + cpySize - 1;
    if (*dst) {
        while (true)
        {
            **dst = *src;
            if (src == srcEnd || src == cpyEnd || *dst == dstEnd) {
                return;
            }
            if (totalDataLen) {
                (*totalDataLen)++;
            }
            src++;
            if (*dst) {
                (*dst)++;
            }
        }
    }
    else {
        while (true)
        {
            if (src == srcEnd || src == cpyEnd) {
                return;
            }
            if (totalDataLen) {
                (*totalDataLen)++;
            }
            src++;
        }
    }

}

/* non-null-terminated strcpy clamped */
static U32 nntstrcpy_c(char* dst, const char* dstEnd, const char* src)
{
    const U32 srclen = strlen(src);
    
    U32 i = 0;
    for (; i < srclen; ++i) {
        if (dst!=NULL&&dst == dstEnd) {
            break;
        }
        if (dst) {
            *dst = src[i];
        }
        dst++;
    }
    if (srclen > 0 && dst) {
        /*dst = '\0'; */
    }

    return srclen;
}

/* non-null-terminated strcpy clamped advance dest*/
static U32 nntstrcpy_ca(char** dst, const char* dstEnd, const char* src, U64* totalDataLen) {
    const U32 srclen = strlen(src);

    U32 i = 0;
    for (; i < srclen; ++i) {
        if (*dst != NULL && *dst == dstEnd) {
            break;
        }
        if (*dst) {
            **dst = src[i];
        }
        if (*dst) {
            (*dst)++;
        }
    }
    if (srclen > 0 && *dst) {
        /* **dst = '\0'; */
    }

    if (totalDataLen)
        *totalDataLen += srclen;
    return srclen;
}

static enum PlyResult writeHeaderProperty(const struct PlyProperty* property, char** dst, const char* dstEnd, U64* totalDataLen)
{
    nntstrcpy_ca(dst, dstEnd, "property ", totalDataLen);
    if (property->dataType == PLY_DATA_TYPE_LIST) 
    {
        nntstrcpy_ca(dst, dstEnd, "list ", totalDataLen);

        const char* propListTypeName = PlyScalarTypeToString(property->listCountType);
        if (propListTypeName == NULL) {
            return PLY_MALFORMED_HEADER_ERROR;
        }
        nntstrcpy_ca(dst, dstEnd, propListTypeName, totalDataLen);
        nntstrcpy_ca(dst, dstEnd, " ", totalDataLen);
    }

    const char* propScalarTypeName = PlyScalarTypeToString(property->scalarType);
    if (propScalarTypeName == NULL) {
        return PLY_MALFORMED_HEADER_ERROR;
    }
    nntstrcpy_ca(dst, dstEnd, propScalarTypeName, totalDataLen);
    nntstrcpy_ca(dst, dstEnd, " ", totalDataLen);
    nntstrcpy_ca(dst, dstEnd, property->name, totalDataLen);
    nntstrcpy_ca(dst, dstEnd, "\n", totalDataLen);

    return PLY_SUCCESS;
}

static enum PlyResult writeHeaderElement(const struct PlyElement* element, char** dst, const char* dstEnd, U64* totalDataLen)
{
    nntstrcpy_ca(dst, dstEnd, "element ", totalDataLen);
    nntstrcpy_ca(dst, dstEnd, element->name, totalDataLen);
    nntstrcpy_ca(dst, dstEnd, " ", totalDataLen);
    char datalineCountAsStr[12];
    itoa(element->dataLineCount, datalineCountAsStr, 10);
    nntstrcpy_ca(dst, dstEnd, datalineCountAsStr, totalDataLen);
    nntstrcpy_ca(dst, dstEnd, "\n", totalDataLen);

    U32 i = 0;
    for (; i < element->propertyCount; ++i)
    {
        const struct PlyProperty* property = element->properties + i;
        writeHeaderProperty(property, dst, dstEnd, totalDataLen);
    }
    return PLY_SUCCESS;
}

enum PlyResult PlySaveToMemory(struct PlyScene* scene, U8* data, U64 dataSize, U64* writeSizeOut, const struct PlySaveInfo* writeInfo)
{
    enum PlyFormat format = scene->format;
    if (scene->format == PLY_FORMAT_BINARY_MATCH_SYSTEM) {
        scene->format = PlyGetSystemEndianness();
        format = scene->format;
    }
    if (scene->format == PLY_FORMAT_BINARY_BIG_ENDIAN || scene->format == PLY_FORMAT_BINARY_LITTLE_ENDIAN) {
        format = PlyGetSystemEndianness();
    }

    *writeSizeOut = 0;
    const U8* dataLast= 0u;
    if (data) {
        dataLast = data + dataSize - 1;
    } 

    U8* cur = data;
    /* BEGIN HEADER */
    nntstrcpy_ca((char**)&cur, (const char*)dataLast, "ply\n", writeSizeOut);
    /* WRITE FORMAT */
    nntstrcpy_ca((char**)&cur, (const char*)dataLast, "format ", writeSizeOut);
    nntstrcpy_ca((char**)&cur, (const char*)dataLast, PlyFormatToString(format), writeSizeOut);
    /* WRITE VERSION */
    nntstrcpy_ca((char**)&cur, (const char*)dataLast, " 1.0\n", writeSizeOut);

    /* WRITE COMMENTS */
    U32 ci = 0;
    for (; ci < scene->commentCount; ++ci) {
        const unsigned char* comment = scene->comments[ci];
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, "comment ", writeSizeOut);
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, (const char*)comment, writeSizeOut);
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, "\n", writeSizeOut);
    }


    /* WRITE OBJ_INFOS */
    U32 oii = 0;
    for (; oii < scene->objectInfoCount; ++oii) {
        struct PlyObjectInfo* objinfo = scene->objectInfos + oii;
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, "obj_info ", writeSizeOut);
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, objinfo->name, writeSizeOut);
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, " ", writeSizeOut);
        char buff[512];
        dtoa_s(objinfo->value, 15, buff, sizeof(buff));
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, buff, writeSizeOut);
        nntstrcpy_ca((char**)&cur, (const char*)dataLast, "\n", writeSizeOut);
    }



    /* WRITE ELEMENTS */
    U32 ei = 0;
    for (; ei < scene->elementCount; ++ei) {
        const struct PlyElement* element = scene->elements + ei;
        enum PlyResult res = writeHeaderElement(element, (char**)&cur, (const char*)dataLast, writeSizeOut);
        if (res != PLY_SUCCESS) {
            return res;
        }
    }
    /* END HEADER  */
    nntstrcpy_ca((char**)&cur, (const char*)dataLast, "end_header\n", writeSizeOut);

    /* BEGIN WRITING DATA */
    if (format == PLY_FORMAT_ASCII) {
        /* WRITE ASCII DATA */

        U32 ei = 0;
        for (; ei < scene->elementCount; ++ei) {
            struct PlyElement* element = scene->elements + ei;
            U64 dli = 0;
            for (; dli < element->dataLineCount; ++dli) {
                if (element->dataLineBegins == NULL) {
                #ifndef NDEBUG
                    assert(00 && "DATA LINES WERE EXPECTED FOR AN ELEMENT, BUT THEY WERE NEVER ALLOCATED. IF DATA LINE COUNT OF AN ELEMENT IS GREATER THAN 0, IT MUST HAVE AN ALLOCATED DATA LINES ARRAY.");
                #endif
                    return PLY_MALFORMED_DATA_ERROR;
                }
                const U64 lineBegin = element->dataLineBegins[dli];
                U32 pi=0;
                for (; pi < element->propertyCount; ++pi)
                {
                    if (element->data) {
                        struct PlyProperty* property = element->properties + pi;
                        const U32 lineOffset = property->dataLineOffsets[dli];
                        if (property->dataType == PLY_DATA_TYPE_LIST) {
                            char str[512];
                            const U8* copyFrom = (U8*)element->data + lineBegin + lineOffset;
                            const U32 listCount = PlyScaleBytesToU32(copyFrom, property->listCountType);
                            /*WRITE LIST COUNT*/
                            PlyDataToString(copyFrom, str, sizeof(str), property->listCountType, writeInfo->F32DecimalCount, writeInfo->D64DecimalCount);
                            nntstrcpy_ca((char**)&cur, (const char*)dataLast, str, writeSizeOut);
                            if (listCount > 0) { /*prevent double space*/
                                nntstrcpy_ca((char**)&cur, (const char*)dataLast, " ", writeSizeOut);
                            }

                            U8 scalarSize = PlyGetSizeofScalarType(property->listCountType);
                            copyFrom += scalarSize;
                            scalarSize = PlyGetSizeofScalarType(property->scalarType);

                            U32 lsti;
                            for (lsti = 0; lsti < listCount; ++lsti)
                            {
                                PlyDataToString(copyFrom, str, sizeof(str), property->scalarType, writeInfo->F32DecimalCount, writeInfo->D64DecimalCount);
                                nntstrcpy_ca((char**)&cur, (const char*)dataLast, str, writeSizeOut);
                                if (lsti != listCount - 1) {
                                   nntstrcpy_ca((char**)&cur, (const char*)dataLast, " ", writeSizeOut);
                                }
                                copyFrom += scalarSize;
                            }
                        }
                        else {
                            char str[512];
                            const U8* copyFrom = (U8*)element->data + lineBegin + lineOffset;
                            PlyDataToString(copyFrom, str, sizeof(str), property->scalarType, writeInfo->F32DecimalCount, writeInfo->D64DecimalCount);

                            nntstrcpy_ca((char**)&cur, (const char*)dataLast, str, writeSizeOut);
                        }
                        if (pi == element->propertyCount - 1) {
                            nntstrcpy_ca((char**)&cur, (const char*)dataLast, "\n", writeSizeOut);
                        }
                        else {
                            nntstrcpy_ca((char**)&cur, (const char*)dataLast, " ", writeSizeOut);
                        }
                    }
                    else {
                        return PLY_MALFORMED_DATA_ERROR;
                    }
                }
            }
        }
    }
    else {
        /*WRITE BINARY DATA*/
        U32 ei = 0;
        for (; ei < scene->elementCount; ++ei) {
            struct PlyElement* element = scene->elements + ei;
            memcpy_ca(&cur, dataLast, element->dataSize, element->data, (U8*)element->data + element->dataSize, writeSizeOut);
        }
    }
    /* END WRITING DATA */

    /* NULL TERMINATE */
    if (data)
        data[min(dataSize-1, (*writeSizeOut)-1)] = 0;

    return PLY_SUCCESS;
}

enum PlyResult PlySaveToDisk(const char* fileName, struct PlyScene* scene, const struct PlySaveInfo* writeInfo)
{
    enum PlyResult resCode;
    FILE* fptr;
    fopen_s(&fptr, fileName, "wb");
    if (!fptr)
        return PLY_FILE_WRITE_ERROR;

    U8* data=NULL;
    U64 dataSize;
    enum PlyResult r1 = PlySaveToMemory(scene, NULL, 0, &dataSize, writeInfo);

    if (r1 != PLY_SUCCESS) {
        resCode = r1;
        goto bail;
    }
    if (dataSize == 0)
        goto bail;

    data = plyRealloc(data, dataSize);

    if (!data) {
        resCode = PLY_FAILED_ALLOC_ERROR;
        goto bail;
    }

    resCode = PlySaveToMemory(scene, data, dataSize, &dataSize, writeInfo);

    if (resCode != PLY_SUCCESS)
        goto bail;

     fwrite(data, dataSize, 1, fptr);
bail:
    if (fptr) {
        fclose(fptr);
    }

    return resCode;
}

enum PlyResult PlySaveToDiskW(const wchar_t* fileName, struct PlyScene* scene, const struct PlySaveInfo* writeInfo)
{
    enum PlyResult resCode;
    FILE* fptr;
    _wfopen_s(&fptr, fileName, L"wb");
    if (!fptr)
        return PLY_FILE_WRITE_ERROR;

    U8* data=NULL;
    U64 dataSize;
    enum PlyResult r1 = PlySaveToMemory(scene, NULL, 0, &dataSize, writeInfo);

    if (r1 != PLY_SUCCESS) {
        resCode = r1;
        goto bail;
    }
    if (dataSize == 0)
        goto bail;

    data = plyRealloc(data, dataSize);

    if (!data) {
        resCode = PLY_FAILED_ALLOC_ERROR;
        goto bail;
    }

    resCode = PlySaveToMemory(scene, data, dataSize, &dataSize, writeInfo);

    if (resCode != PLY_SUCCESS)
        goto bail;

     fwrite(data, dataSize, 1, fptr);
bail:
    if (fptr) {
        fclose(fptr);
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


enum PlyResult PlyCreateDataLines(struct PlyElement* element, const U32 linecount)
{
    element->dataLineCount = linecount;
    return allocateDataLinesForElement(element);
}


enum PlyResult PlyWriteElement(struct PlyScene* scene, struct PlyElement* element)
{
    if (checkForElementNameCollision(scene, element->name))
        return PLY_GENERIC_ERROR;

    if (scene->elementCount == UINT32_MAX - 1) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
    }
    const U32 newElementCount = scene->elementCount + 1;
    struct PlyElement* tmp = (struct PlyElement*)plyReCalloc(scene->elements, scene->elementCount, newElementCount, sizeof(*element));
    if (!tmp) {
        return PLY_FILE_READ_ERROR;
    }
    scene->elements = tmp;
    scene->elements[scene->elementCount] = *element;
    scene->elementCount = newElementCount;
    return PLY_SUCCESS;
}

 enum PlyResult PlyWriteProperty(struct PlyElement* element, struct PlyProperty* property)
{
    if (checkForPropertyNameCollision(element, property->name))
        return PLY_GENERIC_ERROR;

    if (element->propertyCount == UINT32_MAX - 1) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;
    }
    const U32 newPropertyCount = element->propertyCount+1;
    struct PlyProperty* tmp = (struct PlyProperty*)plyReCalloc(element->properties, element->propertyCount, newPropertyCount, sizeof(*property));
    if (!tmp) {
        return PLY_FILE_READ_ERROR;
    }
    element->properties = tmp;
    element->properties[element->propertyCount] = *property;
    element->propertyCount = newPropertyCount;
    return PLY_SUCCESS;
}


enum PlyResult PlyWriteObjectInfo(struct PlyScene* scene, const char* name, double value)
{
    const U32 newcount = scene->objectInfoCount + 1;
    if (newcount < scene->objectInfoCount)
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;

    void* tmp = plyReCalloc(scene->objectInfos, scene->objectInfoCount, newcount, sizeof(struct PlyObjectInfo));
    if (!tmp)
        return PLY_FAILED_ALLOC_ERROR;
    scene->objectInfos = tmp;
    struct PlyObjectInfo* info = scene->objectInfos + scene->objectInfoCount;
    scene->objectInfoCount = newcount;
    strcpy_s(info->name, sizeof(info->name), name);
    info->value = value;
    return PLY_SUCCESS;
}

enum PlyResult PlyWriteComment(struct PlyScene* scene, const char* comment)
{
#ifndef NDEBUG
    U32 i = 0;
    for (; i < strlen(comment); ++i) {
        if (comment[i] == '\n') {
            assert(00 && "COMMENTS SHOULD NEVER CONTAIN A NEWLINE CHARACTER");
        }
    }
#endif

    const U64 commentLen = strlen(comment);

    const U32 newcount = scene->commentCount + 1;
    if (newcount < scene->commentCount)
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;

    void* tmpComments = plyReCalloc(scene->comments, scene->commentCount, newcount, sizeof(comment));
    if (!tmpComments)
        return PLY_FAILED_ALLOC_ERROR;

    scene->comments = tmpComments;

    unsigned char* commentBuffer = (unsigned char*)plyRealloc(scene->comments[scene->commentCount], commentLen + 1);
    if (!commentBuffer)
        return PLY_FAILED_ALLOC_ERROR;

    scene->comments[scene->commentCount] = commentBuffer;

    memcpy(scene->comments[scene->commentCount], comment, commentLen);
    scene->comments[scene->commentCount][commentLen] = '\0';


    scene->commentCount = newcount;

    return PLY_SUCCESS;
}






enum PlyResult PlyWriteData(struct PlyElement* element, const U32 datalineIdx, const U32 pi, const union PlyScalarUnion value)
{    
    if (element->propertyCount == 0) {
#ifndef NDEBUG
        assert("PlyWriteData: CANNOT WRITE DATA TO ELEMENT WHICH HAS NO PROPERTIES");
#endif
        return PLY_GENERIC_ERROR;
    }

    struct PlyProperty* pr = element->properties + pi;
    if (pr->dataType != PLY_DATA_TYPE_SCALAR) {
#ifndef NDEBUG
        assert("MISMATCH BETWEEN ACTUAL LIST COUNT TYPE AND EXPECTED LIST COUNT TYPE.");
#endif
        return PLY_DATA_TYPE_MISMATCH_ERROR;
    }





    const U8 scalarSize = PlyGetSizeofScalarType(pr->scalarType);
    if (element->dataSize + scalarSize < element->dataSize) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;/*prevent overflow*/
    }
    U8* tmp = realloc(element->data, element->dataSize + scalarSize);
    if (!tmp) {
        return PLY_FAILED_ALLOC_ERROR;
    }
    element->data = tmp;
    element->dataSize = element->dataSize + scalarSize;
    
    if (datalineIdx < element->dataLineCount-1) {
        U64 nxtDataLineBegin = element->dataLineBegins[datalineIdx+1];
        if (nxtDataLineBegin == 0) {
            nxtDataLineBegin = element->dataLineBegins[datalineIdx];
        }
        nxtDataLineBegin += scalarSize;
        element->dataLineBegins[datalineIdx + 1] = nxtDataLineBegin;
    }


    const U32 dlOffset = pr->dataLineOffsets[datalineIdx];
    const U64 dlBegin = element->dataLineBegins[datalineIdx];
    if (pi < element->propertyCount - 1) {
        struct PlyProperty* prNxt = element->properties + pi + 1;
        prNxt->dataLineOffsets[datalineIdx] = dlOffset + scalarSize;
    }
    
    U8* cpyTo = (U8*)element->data + dlBegin + dlOffset;
    memcpy(cpyTo, &value, scalarSize);

    return PLY_SUCCESS;
}


PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteDataList(struct PlyElement* element, const U32 datalineIdx, const U32 pi, const U32 listCount, const void* values)
{
    if (element->propertyCount == 0) {
#ifndef NDEBUG
        assert("PlyWriteDataList: CANNOT WRITE DATA TO ELEMENT WHICH HAS NO PROPERTIES");
#endif
        return PLY_GENERIC_ERROR;
    }

    struct PlyProperty* pr = element->properties + pi;
    if (pr->dataType != PLY_DATA_TYPE_LIST) {
#ifndef NDEBUG
        assert("MISMATCH BETWEEN ACTUAL LIST COUNT TYPE AND EXPECTED LIST COUNT TYPE.");
#endif

        return PLY_DATA_TYPE_MISMATCH_ERROR;
    }
    const U32 listDataSize = PlyGetSizeofScalarType(pr->scalarType) * listCount;
    const U32 totalListSize = PlyGetSizeofScalarType(pr->listCountType) + listDataSize;

    /*EXPAND DATA*/
    if (element->dataSize + totalListSize < element->dataSize) {
        return PLY_EXCEEDS_BOUND_LIMITS_ERROR;/*prevent overflow*/
    }
    U8* tmp = realloc(element->data, element->dataSize + totalListSize);
    if (!tmp) {
        return PLY_FAILED_ALLOC_ERROR;
    }
    element->data = tmp;

    const U32 dlOffset = pr->dataLineOffsets[datalineIdx];
    const U64 dlBegin = element->dataLineBegins[datalineIdx];

    U8* cur = (U8*)element->data + dlBegin + dlOffset;
    union PlyScalarUnion u = { .u32 = listCount };
    /* COPY LIST COUNT */
    PlyScalarUnionCpyIntoLocation(cur, &u, pr->listCountType);
    cur += PlyGetSizeofScalarType(pr->listCountType);
    /* COPY LIST DATA */
    memcpy(cur, values, listDataSize);

    element->dataSize = element->dataSize + totalListSize;

    /*UPDATE DATA LINE OFFSETS*/
    if (datalineIdx < element->dataLineCount - 1) {
        U64 nxtDataLineBegin = element->dataLineBegins[datalineIdx + 1];
        if (nxtDataLineBegin == 0) {
            nxtDataLineBegin = element->dataLineBegins[datalineIdx];
        }
        nxtDataLineBegin += totalListSize;
        element->dataLineBegins[datalineIdx + 1] = nxtDataLineBegin;
    }
    if (pi < element->propertyCount - 1) {
        struct PlyProperty* prNxt = element->properties + pi + 1;
        prNxt->dataLineOffsets[datalineIdx] = dlOffset + totalListSize;
    }

    return PLY_SUCCESS;
}



PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteDataByName(struct PlyElement* element, const U32 datalineIdx, const char* propertyName, const union PlyScalarUnion value)
{
    
    U32 pi = 0;
    for (; pi < element->propertyCount; pi++) {
        struct PlyProperty* pr = element->properties + pi;
        if (streql(pr->name, propertyName)) {
            break;
        }
    }

    return PlyWriteData(element,datalineIdx,pi,value);
}

PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteDataListByName(struct PlyElement* element, const U32 datalineIdx, const char* propertyName, const U32 listCount, const void* values)
{
    
    U32 pi = 0;
    for (; pi < element->propertyCount; pi++) {
        struct PlyProperty* pr = element->properties + pi;
        if (streql(pr->name, propertyName)) {
            break;
        }
    }

    return PlyWriteDataList(element,datalineIdx,pi,listCount,values);
}

void PlyDataToString(const U8* data, char* dst, const U16 dstSize, enum PlyScalarType type, const U8 F32DecimalCount, const U16 D64DecimalCount)
{
    union PlyScalarUnion u = { 0 };
    switch (type)
    {
    case PLY_SCALAR_TYPE_UNDEFINED:
        break;
    case PLY_SCALAR_TYPE_CHAR:
        u.i8 = *(I8*)data;
        itoa_s(u.i8, dst, dstSize);
        break;
    case PLY_SCALAR_TYPE_UCHAR:
        u.u8 = *(U8*)data;
        utoa_s(u.i8, dst, dstSize);
        break;
    case PLY_SCALAR_TYPE_SHORT:
        u.i16 = *(I16*)data;
        itoa_s(u.i16, dst, dstSize);
        break;
    case PLY_SCALAR_TYPE_USHORT:
        u.u16 = *(U16*)data;
        utoa_s(u.u16, dst, dstSize);
        break;
    case PLY_SCALAR_TYPE_INT:
        u.i32 = *(I32*)data;
        itoa_s(u.i32, dst, dstSize);
        break;
    case PLY_SCALAR_TYPE_UINT:
        u.u32 = *(U32*)data;
        utoa_s(u.u32, dst, dstSize);
        break;
    case PLY_SCALAR_TYPE_FLOAT:
        u.f32 = *(float*)data;
        dtoa_s(u.f32, F32DecimalCount, dst, dstSize);
        break;
    case PLY_SCALAR_TYPE_DOUBLE:
        u.d64 = *(double*)data;
        dtoa_s(u.f32, D64DecimalCount, dst, dstSize);
        break;
    default:
        break;
    }
}



#include <stdbool.h>




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