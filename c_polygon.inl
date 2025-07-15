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


#ifndef C_POLYGON_INL
#define C_POLYGON_INL

#include <stdbool.h>


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
PLY_INLINE static bool streql(const char* str1, const char* str2)
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
PLY_INLINE static bool strneql(const char* str1, const char* str2, size_t n)
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



/*
/// Swaps bytes in place to invert endianness
/// @param U8* mem - data to swap
/// @param PlyScalarType t - type of data to swap (PlyGetSizeofScalarType(t) bytes will be swapped) */
PLY_INLINE void PlySwapBytes(U8* mem, const enum PlyScalarType t)
{
	switch (t)
	{
	case PLY_SCALAR_TYPE_USHORT:
		*(U16*)mem = PLY_BYTESWAP16(*mem);
		break;
	case PLY_SCALAR_TYPE_SHORT:
		*(U16*)mem = PLY_BYTESWAP16(*mem);
		break;
	case PLY_SCALAR_TYPE_UINT:
		*(U32*)mem = PLY_BYTESWAP32(*mem);
		break;
	case PLY_SCALAR_TYPE_INT:
		*(U32*)mem = PLY_BYTESWAP32(*mem);
		break;
	case PLY_SCALAR_TYPE_FLOAT:
		*(U32*)mem = PLY_BYTESWAP32(*mem);
		break;
	case PLY_SCALAR_TYPE_DOUBLE:
		*(U64*)mem = PLY_BYTESWAP64(*mem);
		break;
	}
}



PLY_INLINE const char* dbgPlyDataTypeToString(enum PlyDataType t)
{
	if (t == PLY_DATA_TYPE_LIST)
	{
		return "list";
	}
	if (t == PLY_DATA_TYPE_SCALAR)
	{
		return "scalar";
	}

	return "undefined";
}

PLY_INLINE const char* dbgPlyScalarTypeToString(enum PlyScalarType t)
{

	if (t == PLY_SCALAR_TYPE_CHAR)
	{
		return "char";
	}
	if (t == PLY_SCALAR_TYPE_UCHAR)
	{
		return "uchar";
	}
	if (t == PLY_SCALAR_TYPE_SHORT)
	{
		return "short";
	}
	if (t == PLY_SCALAR_TYPE_USHORT)
	{
		return "short";
	}
	if (t == PLY_SCALAR_TYPE_INT)
	{
		return "int";
	}
	if (t == PLY_SCALAR_TYPE_UINT)
	{
		return "uint";
	}
	if (t == PLY_SCALAR_TYPE_FLOAT)
	{
		return "float";
	}
	if (t == PLY_SCALAR_TYPE_DOUBLE)
	{
		return "double";
	}
	return "undefined";

}
PLY_INLINE const char* dbgPlyResultToString(enum PlyResult res)
{
	if (res == PLY_SUCCESS) {
		return "PLY_SUCCESS";
	}
	if (res == PLY_MALFORMED_DATA_ERROR)
	{
		return "PLY_MALFORMED_DATA_ERROR";
	}
	if (res == PLY_DATA_TYPE_MISMATCH_ERROR) {
		return "PLY_DATA_TYPE_MISMATCH_ERROR";
	}
	if (res == PLY_LIST_COUNT_MISMATCH_ERROR) {
		return "PLY_LIST_COUNT_MISMATCH_ERROR";
	}
	if (res == PLY_MALFORMED_HEADER_ERROR)
	{
		return "PLY_MALFORMED_HEADER_ERROR";
	}
	if (res == PLY_EXCEEDS_BOUND_LIMITS_ERROR) {
		return "PLY_EXCEEDS_BOUND_LIMITS_ERROR";
	}
	if (res == PLY_UNSUPPORTED_VERSION_ERROR) {
		return "PLY_UNSUPPORTED_VERSION_ERROR";
	}
	if (res == PLY_GENERIC_ERROR) {
		return "PLY_GENERIC_ERROR";
	}
	if (res == PLY_MALFORMED_FILE_ERROR) {
		return "PLY_MALFORMED_FILE_ERROR";
	}
	if (res == PLY_FAILED_ALLOC_ERROR) {
		return "PLY_FAILED_ALLOC_ERROR";
	}
	if (res == PLY_FILE_WRITE_ERROR) {
		return "PLY_FILE_WRITE_ERROR";
	}
	if (res == PLY_FILE_READ_ERROR) {
		return "PLY_FILE_READ_ERROR";
	}
	return "Invalid PlyResult";
}



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

PLY_INLINE enum PlyFormat PlyGetSystemEndianness(void)
{
    U16 x = 1;
    char* ptr = (char*)&x;
    if (ptr[0] == 1) {
        return PLY_FORMAT_BINARY_LITTLE_ENDIAN;
    }
    else {
        return PLY_FORMAT_BINARY_BIG_ENDIAN;
    }
}


PLY_INLINE uint32_t PlyScaleBytesToU32(const void* data, const enum PlyScalarType t)
{
    const U8* f = data;
    uint32_t d = 0;

    switch (t) {
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
        d = (uint32_t)0.0;
        break;
    }


    return d;
}

PLY_INLINE int32_t PlyScaleBytesToI32(const void* data, const enum PlyScalarType t)
{
    const U8* f = data;
    int32_t d = 0;

    switch (t) {
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
        d = (int32_t)0.0;
        break;
    }


    return d;
}

PLY_INLINE float PlyScaleBytesToF32(const void* data, const enum PlyScalarType t)
{
    const U8* f = data;
    float d = 0;

    switch (t) {
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
        d = 0.0;
        break;
    }


    return d;
}

PLY_INLINE double PlyScaleBytesToD64(const void* data, const enum PlyScalarType t)
{
    const U8* f = data;
    double d = 0;

    switch (t) {
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
        d = 0.0;
        break;
    }


    return d;

}

PLY_INLINE U8 PlyGetSizeofScalarType(const enum PlyScalarType type)
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

PLY_INLINE static enum PlyScalarType PlyStrToScalarType(const char* str, const U64 strLen)
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


#endif /* !C_POLYGON_INLINE */