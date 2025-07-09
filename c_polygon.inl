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

/*
/// Swaps bytes in place to invert endianness
/// @param U8* mem - data to swap
/// @param PlyScalarType t - type of data to swap (PlyGetSizeofScalarType(t) bytes will be swapped) */
static void PLY_INLINE PlySwapBytes(U8* mem, const enum PlyScalarType t)
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



static PLY_INLINE const char* dbgPlyDataTypeToString(enum PlyDataType t)

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

static PLY_INLINE const char* dbgPlyScalarTypeToString(enum PlyScalarType t)
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
static PLY_INLINE const char* dbgPlyResultToString(enum PlyResult res)
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

#endif // !C_POLYGON_INL