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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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



PLY_INLINE const char* DbgPlyDataTypeToString(enum PlyDataType t)
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

PLY_INLINE const char* DbgPlyScalarTypeToString(enum PlyScalarType t)
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
PLY_INLINE const char* DbgPlyResultToString(enum PlyResult res)
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



PLY_INLINE union PlyScalarUnion PlyStrToScalar(const char* str, const enum PlyScalarType type, U8* strlen)
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

PLY_INLINE void PlyScalarUnionCpyIntoLocation(void* dst, const union PlyScalarUnion* u, const enum PlyScalarType t)
{
	const U64 copylen = PlyGetSizeofScalarType(t);
	memcpy(dst, (void*)u, copylen);
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
        return 0;
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
        return 0;
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
        return 0;
    }


    return d;
}


PLY_INLINE U64 PlyScaleBytesToU64(const void* data, const enum PlyScalarType t) {
    const U8* f = data;
    U64 d = 0;

    switch (t) {
    case PLY_SCALAR_TYPE_FLOAT: {
        float temp = 0x0;
        memcpy(&temp, f, sizeof(float));
        d = (U64)temp;
        break;
    }
    case PLY_SCALAR_TYPE_DOUBLE: {
        double temp = 0x0;
        memcpy(&temp, f, sizeof(double));
        d = (U64)temp;
        break;
    }
    case PLY_SCALAR_TYPE_CHAR: {
        int8_t temp = 0x0;
        memcpy(&temp, f, sizeof(int8_t));
        d = (U64)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UCHAR: {
        uint8_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint8_t));
        d = (U64)temp;
        break;
    }
    case PLY_SCALAR_TYPE_SHORT: {
        int16_t temp = 0x0;
        memcpy(&temp, f, sizeof(int16_t));
        d = (U64)temp;
        break;
    }
    case PLY_SCALAR_TYPE_USHORT: {
        uint16_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint16_t));
        d = (U64)temp;
        break;
    }
    case PLY_SCALAR_TYPE_INT: {
        int32_t temp = 0x0;
        memcpy(&temp, f, sizeof(int32_t));
        d = (U64)temp;
        break;
    }
    case PLY_SCALAR_TYPE_UINT: {
        uint32_t temp = 0x0;
        memcpy(&temp, f, sizeof(uint32_t));
        d = (U64)temp;
        break;
    }
    default:
        return 0;
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
        return 0;
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

PLY_INLINE enum PlyScalarType PlyStrToScalarType(const char* str, const U64 strLen)
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