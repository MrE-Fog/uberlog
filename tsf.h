#pragma once

#if defined(_MSC_VER)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string>

namespace tsf {

/*

tsf: A small, typesafe, cross-platform printf replacement (tested on Windows & linux).

We use snprintf as a backend, so all of the regular formatting commands
that you expect from the printf family of functions work.
This also makes the code much smaller than other implementations.

Usage:
tsf::fmt("%v %v", "abc", 123)        -->  "abc 123"     <== Use %v as a generic value type
tsf::fmt("%s %d", "abc", 123)        -->  "abc 123"     <== Specific value types are fine too, unless they conflict with the provided type, in which case they are overridden
tsf::fmt("%v", std::string("abc"))   -->  "abc"         <== std::string
tsf::fmt("%v", std::wstring("abc"))  -->  "abc"         <== std::wstring
tsf::fmt("%.3f", 25.5)               -->  "25.500"      <== Use format strings as usual

Known unsupported features:
* Positional arguments
* %*s (integer width parameter)	-- wouldn't be hard to add. Presently ignored.

fmt() returns std::string.

If you want to provide a stack-based buffer to avoid memory allocations, then you can use fmt_static_buf().

By providing a cast operator to fmtarg, you can get an arbitrary type supported as an argument, provided it fits into one of the molds of the printf family of arguments.

*/

class fmtarg
{
public:
	enum Types
	{
		TNull,	// Used as a sentinel to indicate that no parameter was passed
		TCStr,
		TWStr,
		TI32,
		TU32,
		TI64,
		TU64,
		TDbl,
	};
	union
	{
		const char*		CStr;
		const wchar_t*	WStr;
		int32_t			I32;
		uint32_t		UI32;
		int64_t			I64;
		uint64_t		UI64;
		double			Dbl;
	};
	Types Type;

	fmtarg()								: Type(TNull), CStr(NULL) {}
	fmtarg(const char* v)					: Type(TCStr), CStr(v) {}
	fmtarg(const wchar_t* v)				: Type(TWStr), WStr(v) {}
	fmtarg(const std::string& v)			: Type(TCStr), CStr(v.c_str()) {}
	fmtarg(const std::wstring& v)			: Type(TWStr), WStr(v.c_str()) {}
	fmtarg(int32_t v)						: Type(TI32), I32(v) {}
	fmtarg(uint32_t v)						: Type(TU32), UI32(v) {}
#ifdef _MSC_VER
	fmtarg(long v)							: Type(TI32), I32(v) {}
	fmtarg(unsigned long v)					: Type(TU32), UI32(v) {}
#endif
	fmtarg(int64_t v)						: Type(TI64), I64(v) {}
	fmtarg(uint64_t v)						: Type(TU64), UI64(v) {}
	fmtarg(double v)						: Type(TDbl), Dbl(v) {}
};

struct CharLenPair
{
	char*  Str;
	size_t Len;
};

std::string fmt_core(const char* fmt, ssize_t nargs, const fmtarg* args);
CharLenPair fmt_core(const char* fmt, ssize_t nargs, const fmtarg* args, char* staticbuf, size_t staticbuf_size);

inline void fmt_pack(fmtarg* pack, const fmtarg& arg)
{
	*pack = arg;
}

template<typename... Args>
void fmt_pack(fmtarg* pack, const fmtarg& arg, const Args&... args)
{
	*pack = arg;
	fmt_pack(pack + 1, args...);
}

template<typename... Args>
std::string fmt(const char* fs, const Args&... args)
{
	const auto num_args = sizeof...(Args);
	fmtarg pack_array[num_args + 1]; // +1 for zero args case
	fmt_pack(pack_array, args...);
	return fmt_core(fs, (ssize_t) num_args, pack_array);
}

// If the formatted string, with null terminator, fits inside staticbuf_len, then the returned pointer is staticbuf,
// and no memory allocation takes place.
// However, if the formatted string is too large to fit inside staticbuf_len, then the returned pointer must
// be deleted with "delete[] ptr".
template<typename... Args>
CharLenPair fmt_static_buf(char* staticbuf, size_t staticbuf_len, const char* fs, const Args&... args)
{
	const auto num_args = sizeof...(Args);
	fmtarg pack_array[num_args + 1]; // +1 for zero args case
	fmt_pack(pack_array, args...);
	return fmt_core(fs, (ssize_t) num_args, pack_array, staticbuf, staticbuf_len);
}

template<typename... Args>
size_t fmt_write(FILE* file, const char* fs, const Args&... args)
{
	auto res = fmt(fs, args...);
	return fwrite(res.c_str(), 1, res.length(), file);
}

template<typename... Args>
size_t fmt_write(const char* fs, const Args&... args)
{
	return fmt_write(stdout, fs, args...);
}

/*  cross-platform "snprintf"

	destination			Destination buffer
	count				Number of characters available in 'destination'. This must include space for the null terminating character.
	format_str			The format string
	return
		-1				Not enough space
		0..count-1		Number of characters written, excluding the null terminator. The null terminator was written though.
*/
int fmt_snprintf(char* destination, size_t count, const char* format_str, ...);

// Identical in all respects to fmt_snprintf, except that we deal with wide character strings
int fmt_swprintf(wchar_t* destination, size_t count, const wchar_t* format_str, ...);

} // namespace tsf
