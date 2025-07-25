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

#ifndef C_POLYGON_H
#define C_POLYGON_H



#ifdef __cplusplus
namespace cply {
#endif /* __cplusplus */

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define C_PLY_MAX_LINE_LENGTH ((uint32_t)200000lu)
#define PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH ((uint16_t)127u)
#define PLY_LOAD_ALL_ELEMENTS 0u
#define PLY_FORMAT_BINARY_MATCH_SYSTEM ~((int)0)

/*In C89, the version will not be defined, or it will be equal to 0.*/
#if defined(_MSC_VER) || defined(__STDC_VERSION__) || (__STDC_VERSION__ != 0)
#define PLY_INLINE static inline
#else
#define PLY_INLINE
#endif

#ifdef __cplusplus
#define PLY_H_FUNCTION_PREFIX extern "C"
#else
#define PLY_H_FUNCTION_PREFIX
#endif /* __cplusplus */

#ifdef _MSC_VER
#define PLY_STATIC_ASSERT static_assert
#else
	#define PLY_STATIC_ASSERT _Static_assert
#endif

#include "ASM/streqlasm.h"

PLY_STATIC_ASSERT(C_PLY_MAX_LINE_LENGTH >= 2, "C_PLY_MAX_LINE_LENGTH must be 2 or greater");
PLY_STATIC_ASSERT(PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH >= 2, "PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH must be 2 or greater");

#ifdef _MSC_VER
#define PLY_BYTESWAP16(x) _byteswap_ushort(x)
#define PLY_BYTESWAP32(x) _byteswap_ulong(x)
#define PLY_BYTESWAP64(x) _byteswap_uint64(x)
#elif defined(__GNUC__)
#define PLY_BYTESWAP16(x) __builtin_bswap16(x)
#define PLY_BYTESWAP32(x) __builtin_bswap32(x)
#define PLY_BYTESWAP64(x) __builtin_bswap64(x)
#else /*probably glibc or some other obscure compiler*/
#include <byteswap.h>
#define PLY_BYTESWAP16(x) bswap_16(x)
#define PLY_BYTESWAP32(x) bswap_32(x)
#define PLY_BYTESWAP64(x) bswap_64(x)
#endif /* !_MSC_VER */



typedef uint8_t		U8;
typedef uint16_t	U16;
typedef uint32_t	U32;
typedef uint64_t	U64;


typedef int8_t		I8;
typedef int16_t		I16;
typedef int32_t		I32;
typedef int64_t		I64;

PLY_H_FUNCTION_PREFIX U8 strtou8(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX U16 strtou16(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX U32 strtou32(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX U64 strtou64(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX I8 strtoi8(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX I16 strtoi16(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX I32 strtoi32(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX I64 strtoi64(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX float strtof32(const char* str, U8* strLenOut);

PLY_H_FUNCTION_PREFIX double strtod64(const char* str, U8* strLenOut);

enum PlyResult
{
	PLY_GENERIC_ERROR=0,
	PLY_SUCCESS,
	PLY_FAILED_ALLOC_ERROR,
	PLY_EXCEEDS_BOUND_LIMITS_ERROR,
	PLY_MALFORMED_DATA_ERROR,
	PLY_MALFORMED_FILE_ERROR,
	PLY_DATA_TYPE_MISMATCH_ERROR,
	PLY_LIST_COUNT_MISMATCH_ERROR,
	PLY_MALFORMED_HEADER_ERROR,
	PLY_FILE_WRITE_ERROR,
	PLY_FILE_READ_ERROR,
	PLY_UNSUPPORTED_VERSION_ERROR
};

enum PlyDataType
{
	PLY_DATA_TYPE_UNDEFINED = 0,
	PLY_DATA_TYPE_SCALAR = 1,
	PLY_DATA_TYPE_LIST = 2
};


enum PlyScalarType
{
	PLY_SCALAR_TYPE_UNDEFINED=0,
	PLY_SCALAR_TYPE_CHAR = 1,
	PLY_SCALAR_TYPE_UCHAR = 2,
	PLY_SCALAR_TYPE_SHORT = 3,
	PLY_SCALAR_TYPE_USHORT = 4,
	PLY_SCALAR_TYPE_INT = 5,
	PLY_SCALAR_TYPE_UINT = 6,
	PLY_SCALAR_TYPE_FLOAT = 7,
	PLY_SCALAR_TYPE_DOUBLE = 8
};

union PlyScalarUnion
{
	U8 u8;
	I8 i8;
	U16 u16;
	I16 i16;
	U32 u32;
	I32 i32;
	float f32;
	double d64;
};

struct PlyProperty
{
	char name[PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH+1];

	U32* dataLineOffsets;

	enum PlyScalarType listCountType; /*undefined if it's not a list*/
	enum PlyDataType dataType;
	enum PlyScalarType scalarType;
};

enum PlyFormat 
{
	PLY_FORMAT_UNDEFINED=0x0,
	PLY_FORMAT_ASCII,
	PLY_FORMAT_BINARY_BIG_ENDIAN,
	PLY_FORMAT_BINARY_LITTLE_ENDIAN
};

struct PlyElement
{
	char name[PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH + 1];
	struct PlyProperty* properties;
	void* data;
	U32 propertyCount;
	U32 dataLineCount;
	U64 dataSize;

	U64* dataLineBegins;
};

struct PlyObjectInfo
{
	char name[PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH + 1];
	double value;
};

struct PlyLoadInfo
{
	const char** elements; /*don't forget to set elementsCount*/
	U32 elementsCount;
	char saveComments;
	char allowAnyVersion;
};

struct PlySaveInfo
{
	/*Recommended: 50*/
	const U16 D64DecimalCount;
	/*Recommended: 10*/
	const U8 F32DecimalCount;
};

struct PlyScene
{
	void* sharedElementData;
	struct PlyElement* elements;
	struct PlyObjectInfo* objectInfos;
	unsigned char** comments;
	U32 commentCount;
	U32 elementCount;
	U32 objectInfoCount;
	enum PlyFormat format;
	float versionNumber;
};


/*PlyReallocT:
* - should act as malloc if void* is null
* - should return NULL is size is 0u
* - should act as realloc is a valid pointer is passed as the first argument
*/
typedef void* (*PlyReallocT)(void*, const U64);


/*PlyReCallocT:
* - should act as calloc is void* is null
* - should check for overflow (if U32 * U32 > U64_MAX, return NULL)
* - should return NULL is size is 0u
* - must zero-initialize memory
* - if allocation fails, the old block should be freed
* - if it returns a new block and a valid old block was passed in, data must be copied from the old block to the new block, and if the newsize is greater than it's old size, the memory must be zero-extended.)
*/

typedef void* (*PlyReCallocT)(void*/*old block*/, const U32/*old count*/, const U32 /*new count*/, const U32/*element size*/);

typedef void (*PlyDeallocT)(void*);






/* -+- FUNCTION DECLARATIONS -+- */

/*
/// Returns the system endianness as a PLY_FORMAT
/// @return enum PlyFormat - returns either PLY_FORMAT_BIG_ENDIAN or PLY_FORMAT_LITTLE_ENDIAN */
PLY_INLINE enum PlyFormat PlyGetSystemEndianness(void);

/*
/// Swaps bytes in place to invert endianness
/// @param U8* mem - data to swap
/// @param PlyScalarType t - type of data to swap (PlyGetSizeofScalarType(t) bytes will be swapped) */
PLY_INLINE void PlySwapBytes(U8* mem, const enum PlyScalarType t);

/*
/// PlyScaleBytesToU32() - Converts variable length data to an unsigned int
/// @param void* data - Start of data
/// @param enum PlyScalarType t - Scalar type used to infer size of data to convert
/// @return unsigned int - Data as an unsigned int */
PLY_INLINE uint32_t PlyScaleBytesToU32(const void* data, const enum PlyScalarType t);

/*
/// PlyScaleBytesToI32() - Converts variable length data to a int
/// @param void* data - Start of data
/// @param enum PlyScalarType t - Scalar type used to infer size of data to convert
/// @return int - Data as a int */
PLY_INLINE int32_t PlyScaleBytesToI32(const void* data, const enum PlyScalarType t);

/*
/// PlyScaleBytesToF32() - Converts variable length data to a float
/// @param void* data - Start of data
/// @param enum PlyScalarType t - Scalar type used to infer size of data to convert
/// @return float - Data as a float */
PLY_INLINE float PlyScaleBytesToF32(const void* data, const enum PlyScalarType t);



/*
/// PlyScaleBytesToU64() - Converts variable length data to an unsigned long long
/// @param void* data - Start of data
/// @param enum PlyScalarType t - Scalar type used to infer size of data to convert
/// @return U64 - Data as an unsigned long long */
PLY_INLINE U64 PlyScaleBytesToU64(const void* data, const enum PlyScalarType t);

/*
/// PlyScaleBytesToD64() - Converts variable length data to a double
/// @param void* data - Start of data
/// @param enum PlyScalarType t - Scalar type used to infer size of data to convert
/// @return double - Data as a double */
PLY_INLINE double PlyScaleBytesToD64(const void* data, const enum PlyScalarType t);

/*
/// Returns the sizeof a Scalar type in bytes
/// @param enum PlyScalarType type - Scalar type to get the size of
/// @return U8 - Size of scalar type (1 . . . 8) */
PLY_INLINE U8 PlyGetSizeofScalarType(const enum PlyScalarType type);

/*
/// Converts a str to a scalar type
/// @param const char* str - c string to read from
/// @param const U64 strLen - the max length to read
/// @return PlyScalarType - upon failure PLY_SCALAR_TYPE_UNDEFINED will be returned. */
PLY_INLINE enum PlyScalarType PlyStrToScalarType(const char* str, const U64 strLen);


/*
/// Converts a str to a scalar value
/// @param const char* str - c string to read from
/// @param const PlyScalarType type - the type of the value to read
/// @param strlen - the length of the substring that was read. Upon failure, it will be set to 0.
/// @return PlyScalarUnion - the scalar value that was read*/
PLY_INLINE union PlyScalarUnion PlyStrToScalar(const char* str, const enum PlyScalarType type, U8* strlen);

/*
/// Copies a scalar union into a given location
/// @param void* dst - copy destination
/// @param PlyScalarUnion* u - value to copy
/// @param PlyScalarType t - scalar type of u*/
PLY_INLINE void PlyScalarUnionCpyIntoLocation(void* dst, const union PlyScalarUnion* u, const enum PlyScalarType t);

PLY_H_FUNCTION_PREFIX void PlySetCustomReallocator(PlyReallocT);

PLY_H_FUNCTION_PREFIX void PlySetCustomRecallocator(PlyReCallocT);

PLY_H_FUNCTION_PREFIX void PlySetCustomDeallocator(PlyDeallocT);

/* 
* returns the index of an element within a property. 
* If the element is not in that property, -1 will be returned.*/
PLY_H_FUNCTION_PREFIX I64 PlyGetPropertyIndex(const struct PlyElement* element, const struct PlyProperty* property);

/* 
* returns the index of an element within a property from it's name.
* If the element is not in that property, -1 will be returned. */
PLY_H_FUNCTION_PREFIX I64 PlyGetPropertyIndexByName(const struct PlyElement* element, const char* propertyName);


/* adds a PlyProperty to an element. The property will be copied, thus transferring ownership */
PLY_INLINE enum PlyResult PlyElementAddProperty(struct PlyElement* element, struct PlyProperty* property);

/* adds a PlyObjectInfo to an element. The property will be copied, thus transferring ownership */
PLY_INLINE enum PlyResult PlySceneAddObjectInfo(struct PlyScene* scene, struct PlyObjectInfo* objInfo);

/* adds a PlyElement to a scene. The element will be copied, thus transferring ownership */
PLY_INLINE enum PlyResult  PlySceneAddElement(struct PlyScene* scene, struct PlyElement* element);

PLY_H_FUNCTION_PREFIX enum PlyResult PlyElementSetName(struct PlyElement* element, const char* name); 


/*
/// Loads a PlyScene from memory.
/// @param const U8* mem - the beginning of the memory to read.
/// @param const U8* mem - the length of the memory to read. (memEnd - memBegin + 1)
/// @param struct PlyScene* scene - scene to write to
/// @param struct PlyLoadInfo* loadInfo - optional constraints that can be placed on scene parsing */
PLY_H_FUNCTION_PREFIX enum PlyResult PlyLoadFromMemory(const U8* mem, U64 memSize, struct PlyScene* scene, struct PlyLoadInfo* loadInfo);

/*
/// Loads a PlyScene from a given filename.
/// @param const char* fileName - filename to read
/// @param struct PlyScene* scene - scene to write to
/// @param struct PlyLoadInfo* loadInfo - optional constraints that can be placed on scene parsing */
PLY_H_FUNCTION_PREFIX enum PlyResult PlyLoadFromDisk(const char* fileName, struct PlyScene* scene, struct PlyLoadInfo* loadInfo);

/*
/// Loads a PlyScene from a given wide filename.
/// @param const wchar_t* fileName - filename to read, as a wide (UTF-16) string
/// @param struct PlyScene* scene - scene to write to
/// @param struct PlyLoadInfo* loadInfo - optional constraints that can be placed on scene parsing */
PLY_H_FUNCTION_PREFIX enum PlyResult PlyLoadFromDiskW(const wchar_t* fileName, struct PlyScene* scene, struct PlyLoadInfo* loadInfo);

/*
/// Destroys the scene and all associated memory
/// @param struct PlyScene* scene - scene to destroy */
PLY_H_FUNCTION_PREFIX void PlyDestroyScene(struct PlyScene* scene);



PLY_H_FUNCTION_PREFIX enum PlyResult PlySaveToMemory(struct PlyScene* scene, U8* data, U64 dataSize, U64* writeSizeOut, const struct PlySaveInfo* writeInfo);

PLY_H_FUNCTION_PREFIX enum PlyResult PlySaveToDisk(const char* fileName, struct PlyScene* scene, const struct PlySaveInfo* writeInfo);

PLY_H_FUNCTION_PREFIX enum PlyResult PlySaveToDiskW(const wchar_t* fileName, struct PlyScene* scene, const struct PlySaveInfo* writeInfo);


PLY_H_FUNCTION_PREFIX enum PlyResult PlyCreateDataLines(struct PlyElement* element, const U32 datalineCount);

PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteElement(struct PlyScene* scene, struct PlyElement* element);

PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteProperty(struct PlyElement* element, struct PlyProperty* property);

PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteObjectInfo(struct PlyScene* scene, const char* name, double value);

PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteComment(struct PlyScene* scene, const char* name);


PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteData(struct PlyElement* element, U32 datalineIdx, const char* propertyName, const union PlyScalarUnion value);

PLY_H_FUNCTION_PREFIX enum PlyResult PlyWriteDataList(struct PlyElement* element, U32 datalineIdx, const char* propertyName, const U32 listCount, const void* values);


PLY_H_FUNCTION_PREFIX void PlyDataToString(const U8* data, char* buff, const U16 buffSize, enum PlyScalarType type, const U8 F32DecimalCount, const U16 D64DecimalCount);

PLY_INLINE const char* PlyFormatToString(enum PlyFormat t);

PLY_INLINE const char* PlyDataTypeToString(enum PlyDataType t);

PLY_INLINE const char* PlyScalarTypeToString(enum PlyScalarType t);

PLY_INLINE const char* PlyResultToString(enum PlyResult res);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#if defined(_MSC_VER) || defined(__STDC_VERSION__) || (__STDC_VERSION__ != 0)
#include "c_polygon.inl"
#endif

#endif /* !C_POLYGON_H */