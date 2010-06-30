// Copyright (C) 2002-2009 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine" and the "irrXML" project.
// For conditions of distribution and use, see copyright notice in irrlicht.h and irrXML.h
// Code contributed by Nalin

#ifndef __IRR_USTRING_H_INCLUDED__
#define __IRR_USTRING_H_INCLUDED__

#if (__cplusplus > 199711L) || (_MSC_VER >= 1600) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#	define USTRING_CPP0X
#	if defined(__GXX_EXPERIMENTAL_CXX0X__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)))
#		define USTRING_CPP0X_NEWLITERALS
#	endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef USTRING_CPP0X
#	include <utility>
#endif
//#include <iostream>

#include "irrTypes.h"
#include "irrAllocator.h"
#include "irrArray.h"
#include "irrMath.h"
#include "irrString.h"

//! UTF-16 surrogate start values.
static const irr::u16 UTF16_HI_SURROGATE = 0xD800;
static const irr::u16 UTF16_LO_SURROGATE = 0xDC00;

//! Is a UTF-16 code point a surrogate?
#define UTF16_IS_SURROGATE(c)		(((c) & 0xF800) == 0xD800)
#define UTF16_IS_SURROGATE_HI(c)	(((c) & 0xFC00) == 0xD800)
#define UTF16_IS_SURROGATE_LO(c)	(((c) & 0xFC00) == 0xDC00)


namespace irr
{

	// Define our character types.
#ifdef USTRING_CPP0X_NEWLITERALS	// C++0x
	typedef char32_t uchar32_t;
	typedef char16_t uchar16_t;
	typedef char uchar8_t;
#else
	typedef u32 uchar32_t;
	typedef u16 uchar16_t;
	typedef u8 uchar8_t;
#endif

namespace core
{

namespace unicode
{

//! The unicode replacement character.  Used to replace invalid characters.
const irr::u16 UTF_REPLACEMENT_CHARACTER = 0xFFFD;

//! Convert a UTF-16 surrogate pair into a UTF-32 character.
//! \param high The high value of the pair.
//! \param low The low value of the pair.
//! \return The UTF-32 character expressed by the surrogate pair.
inline uchar32_t toUTF32(uchar16_t high, uchar16_t low)
{
	// Convert the surrogate pair into a single UTF-32 character.
	uchar32_t x = ((high & ((1 << 6) -1)) << 10) | (low & ((1 << 10) -1));
	uchar32_t wu = ((high >> 6) & ((1 << 5) - 1)) + 1;
	return (wu << 16) | x;
}

//! Swaps the endianness of a 16-bit value.
//! \return The new value.
inline uchar16_t swapEndian16(const uchar16_t& c)
{
	return ((c >> 8) & 0x00FF) | ((c << 8) & 0xFF00);
}

//! Swaps the endianness of a 32-bit value.
//! \return The new value.
inline uchar32_t swapEndian32(const uchar32_t& c)
{
	return  ((c >> 24) & 0x000000FF) |
			((c >> 8)  & 0x0000FF00) |
			((c << 8)  & 0x00FF0000) |
			((c << 24) & 0xFF000000);
}

//! The Unicode byte order mark.
const u16 BOM = 0xFEFF;

//! The size of the Unicode byte order mark in terms of the Unicode character size.
const u8 BOM_UTF8_LEN = 3;
const u8 BOM_UTF16_LEN = 1;
const u8 BOM_UTF32_LEN = 1;

//! Unicode byte order marks for file operations.
const u8 BOM_ENCODE_UTF8[3] = { 0xEF, 0xBB, 0xBF };
const u8 BOM_ENCODE_UTF16_BE[2] = { 0xFE, 0xFF };
const u8 BOM_ENCODE_UTF16_LE[2] = { 0xFF, 0xFE };
const u8 BOM_ENCODE_UTF32_BE[4] = { 0x00, 0x00, 0xFE, 0xFF };
const u8 BOM_ENCODE_UTF32_LE[4] = { 0xFF, 0xFE, 0x00, 0x00 };

//! The size in bytes of the Unicode byte marks for file operations.
const u8 BOM_ENCODE_UTF8_LEN = 3;
const u8 BOM_ENCODE_UTF16_LEN = 2;
const u8 BOM_ENCODE_UTF32_LEN = 4;

//! Unicode encoding type.
enum EUTF_ENCODE
{
	EUTFE_NONE		= 0,
	EUTFE_UTF8,
	EUTFE_UTF16,
	EUTFE_UTF16_LE,
	EUTFE_UTF16_BE,
	EUTFE_UTF32,
	EUTFE_UTF32_LE,
	EUTFE_UTF32_BE
};

//! Unicode endianness.
enum EUTF_ENDIAN
{
	EUTFEE_NATIVE	= 0,
	EUTFEE_LITTLE,
	EUTFEE_BIG
};

//! Returns the specified unicode byte order mark in a byte array.
//! The byte order mark is the first few bytes in a text file that signifies its encoding.
/** \param mode The Unicode encoding method that we want to get the byte order mark for.
		If EUTFE_UTF16 or EUTFE_UTF32 is passed, it uses the native system endianness. **/
//! \return An array that contains a byte order mark.
inline core::array<u8> getUnicodeBOM(EUTF_ENCODE mode)
{
#define COPY_ARRAY(source, size) \
	memcpy(ret.pointer(), source, size); \
	ret.set_used(size)

	core::array<u8> ret(4);
	switch (mode)
	{
		case EUTFE_UTF8:
			COPY_ARRAY(BOM_ENCODE_UTF8, BOM_ENCODE_UTF8_LEN);
			break;
		case EUTFE_UTF16:
			#ifdef __BIG_ENDIAN__
				COPY_ARRAY(BOM_ENCODE_UTF16_BE, BOM_ENCODE_UTF16_LEN);
			#else
				COPY_ARRAY(BOM_ENCODE_UTF16_LE, BOM_ENCODE_UTF16_LEN);
			#endif
			break;
		case EUTFE_UTF16_BE:
			COPY_ARRAY(BOM_ENCODE_UTF16_BE, BOM_ENCODE_UTF16_LEN);
			break;
		case EUTFE_UTF16_LE:
			COPY_ARRAY(BOM_ENCODE_UTF16_LE, BOM_ENCODE_UTF16_LEN);
			break;
		case EUTFE_UTF32:
			#ifdef __BIG_ENDIAN__
				COPY_ARRAY(BOM_ENCODE_UTF32_BE, BOM_ENCODE_UTF32_LEN);
			#else
				COPY_ARRAY(BOM_ENCODE_UTF32_LE, BOM_ENCODE_UTF32_LEN);
			#endif
			break;
		case EUTFE_UTF32_BE:
			COPY_ARRAY(BOM_ENCODE_UTF32_BE, BOM_ENCODE_UTF32_LEN);
			break;
		case EUTFE_UTF32_LE:
			COPY_ARRAY(BOM_ENCODE_UTF32_LE, BOM_ENCODE_UTF32_LEN);
			break;
	}
	return ret;

#undef COPY_ARRAY
}

} // end namespace unicode


//! UTF-16 string class.
template <typename TAlloc = irrAllocator<uchar16_t> >
class ustring16
{
public:

	///------------------///
	/// iterator classes ///
	///------------------///

	//! Access an element in a unicode string, allowing one to change it.
	class _ustring16_iterator_access
	{
		public:
			_ustring16_iterator_access(ustring16<TAlloc>& s, u32 p) : ref(s), pos(p) {}

			//! Allow the class to be interpreted as a single UTF-32 character.
			operator uchar32_t()
			{
				const uchar16_t* a = ref.c_str();
				if (!UTF16_IS_SURROGATE(a[pos]))
					return static_cast<uchar32_t>(a[pos]);
				else
				{
					if (pos + 1 >= ref.size_raw())
						return 0;

					return unicode::toUTF32(a[pos], a[pos + 1]);
				}
			}

			//! Allow the class to be interpreted as a single UTF-32 character.
			operator uchar32_t() const
			{
				const uchar16_t* a = ref.c_str();
				if (UTF16_IS_SURROGATE(a[pos]))
					return static_cast<uchar32_t>(a[pos]);
				else
				{
					if (pos + 1 >= ref.size_raw())
						return 0;

					return unicode::toUTF32(a[pos], a[pos + 1]);
				}
			}

			//! Allow one to change the character in the unicode string.
			//! \param c The new character to use.
			//! \return Myself.
			_ustring16_iterator_access& operator=(const uchar32_t c)
			{
				const uchar16_t* a = ref.c_str();
				if (c > 0xFFFF)
				{
					// c will be multibyte, so split it up into the high and low surrogate pairs.
					uchar16_t x = static_cast<uchar16_t>(c);
					uchar16_t vh = UTF16_HI_SURROGATE | ((((c >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
					uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));

					// If the previous position was a surrogate pair, just replace them.  Else, insert the low pair.
					if (UTF16_IS_SURROGATE_HI(a[pos]) && pos + 1 != ref.size_raw())
						ref.replace_raw(vl, static_cast<u32>(pos) + 1);
					else ref.insert_raw(vl, static_cast<u32>(pos) + 1);

					ref.replace_raw(vh, static_cast<u32>(pos));
				}
				else
				{
					// c will be a single byte.
					uchar16_t vh = static_cast<uchar16_t>(c);

					// If the previous position was a surrogate pair, remove the extra byte.
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						ref.erase_raw(static_cast<u32>(pos) + 1);

					ref.replace_raw(vh, static_cast<u32>(pos));
				}
				return *this;
			}

		private:
			ustring16<TAlloc>& ref;
			u32 pos;
	};


	//! Access an element in a unicode string.
	class _ustring16_const_iterator_access
	{
		public:
			_ustring16_const_iterator_access(const ustring16<TAlloc>& s, u32 p) : ref(s), pos(p) {}

			//! Allow the class to be interpreted as a single UTF-32 character.
			operator uchar32_t()
			{
				const uchar16_t* a = ref.c_str();
				if (!UTF16_IS_SURROGATE(a[pos]))
					return static_cast<uchar32_t>(a[pos]);
				else
				{
					if (pos + 1 >= ref.size_raw())
						return 0;

					return unicode::toUTF32(a[pos], a[pos + 1]);
				}
			}

			//! Allow the class to be interpreted as a single UTF-32 character.
			operator uchar32_t() const
			{
				const uchar16_t* a = ref.c_str();
				if (UTF16_IS_SURROGATE(a[pos]))
					return static_cast<uchar32_t>(a[pos]);
				else
				{
					if (pos + 1 >= ref.size_raw())
						return 0;

					return unicode::toUTF32(a[pos], a[pos + 1]);
				}
			}

		private:
			const ustring16<TAlloc>& ref;
			u32 pos;
	};


	//! Iterator to iterate through a UTF-16 string.
	class _ustring16_const_iterator;
	class _ustring16_iterator
	{
		public:
			friend class _ustring16_const_iterator;
			typedef typename ustring16<TAlloc>::_ustring16_iterator_access access;

			//! Constructors.
			_ustring16_iterator(ustring16<TAlloc>& s) : ref(s), pos(0) {}
			_ustring16_iterator(const _ustring16_iterator& i) : ref(i.ref), pos(i.pos) {}
			_ustring16_iterator(ustring16<TAlloc>& s, const u32 p) : ref(s), pos(0)
			{
				if (ref.size_raw() == 0 || p == 0)
					return;

				// Go to the appropriate position.
				u32 i = p;
				u32 sr = ref.size_raw();
				const uchar16_t* a = ref.c_str();
				while (i != 0 && pos < sr)
				{
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						pos += 2;
					else ++pos;
					--i;
				}
			}

			//! Test for equalness.
			bool operator==(const _ustring16_iterator& iter) const
			{
				if (&ref == &iter.ref && pos == iter.pos)
					return true;
				return false;
			}

			//! Test for equalness.
			bool operator==(const _ustring16_const_iterator& iter) const
			{
				if (&ref == &iter.ref && pos == iter.pos)
					return true;
				return false;
			}

			//! Test for unequalness.
			bool operator!=(const _ustring16_iterator& iter) const
			{
				if (&ref != &iter.ref || pos != iter.pos)
					return true;
				return false;
			}

			//! Test for unequalness.
			bool operator!=(const _ustring16_const_iterator& iter) const
			{
				if (&ref != &iter.ref || pos != iter.pos)
					return true;
				return false;
			}

			//! Switch to the next full character in the string.
			_ustring16_iterator& operator++()
			{	// ++iterator
				if (pos == ref.size_raw()) return *this;
				const uchar16_t* a = ref.c_str();
				if (UTF16_IS_SURROGATE_HI(a[pos]))
					pos += 2;			// TODO: check for valid low surrogate?
				else ++pos;
				if (pos > ref.size_raw()) pos = ref.size_raw();
				return *this;
			}

			//! Switch to the next full character in the string, returning the previous position.
			_ustring16_iterator operator++(int)
			{	// iterator++
				_ustring16_iterator _tmp(*this);
				++*this;
				return _tmp;
			}

			//! Switch to the previous full character in the string.
			_ustring16_iterator& operator--()
			{	// --iterator
				if (pos == 0) return *this;
				const uchar16_t* a = ref.c_str();
				--pos;
				if (UTF16_IS_SURROGATE_LO(a[pos]) && pos != 0)	// low surrogate, go back one more.
					--pos;
				return *this;
			}

			//! Switch to the previous full character in the string, returning the previous position.
			_ustring16_iterator operator--(int)
			{	// iterator--
				_ustring16_iterator _tmp(*this);
				--*this;
				return _tmp;
			}

			//! Advance a specified number of full characters in the string.
			//! \return Myself.
			_ustring16_iterator& operator+=(const int v)
			{
				if (v == 0) return *this;
				if (v < 0) return operator-=(v * -1);

				if (pos >= ref.size_raw())
					return *this;

				// Go to the appropriate position.
				u32 i = v;
				u32 sr = ref.size_raw();
				const uchar16_t* a = ref.c_str();
				while (i != 0 && pos < sr)
				{
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						pos += 2;
					else ++pos;
					--i;
				}
				if (pos > sr)
					pos = sr;

				return *this;
			}

			//! Go back a specified number of full characters in the string.
			//! \return Myself.
			_ustring16_iterator& operator-=(const int v)
			{
				if (v == 0) return *this;
				if (v > 0) return operator+=(v * -1);

				if (pos == 0)
					return *this;

				// Go to the appropriate position.
				u32 i = v;
				const uchar16_t* a = ref.c_str();
				while (i != 0 && pos != 0)
				{
					--pos;
					if (UTF16_IS_SURROGATE_LO(a[pos]) != 0 && pos != 0)
						--pos;
					--i;
				}

				return *this;
			}

			//! Return a new iterator that is a variable number of full characters forward from the current position.
			_ustring16_iterator operator+(int v) const
			{
				_ustring16_iterator ret(*this);
				ret += v;
				return ret;
			}

			//! Return a new iterator that is a variable number of full characters backward from the current position.
			_ustring16_iterator operator-(int v) const
			{
				_ustring16_iterator ret(*this);
				ret -= v;
				return ret;
			}

			//! Accesses the full character at the iterator's position.
			access operator*() const
			{
				if (pos >= ref.size_raw())
				{
					const uchar16_t* a = ref.c_str();
					u32 p = ref.size_raw();
					if (UTF16_IS_SURROGATE_LO(a[p]))
						--p;
					access ret(ref, p);
					return ret;
				}
				access ret(ref, pos);
				return ret;
			}

			//! Is the iterator at the start of the string?
			bool atStart() const
			{
				return pos == 0;
			}

			//! Is the iterator at the end of the string?
			bool atEnd() const
			{
				const uchar16_t* a = ref.c_str();
				if (UTF16_IS_SURROGATE(a[pos]))
					return (pos + 1) >= ref.size_raw();
				else return pos >= ref.size_raw();
			}

			//! Moves the iterator to the start of the string.
			void toStart()
			{
				pos = 0;
			}

			//! Moves the iterator to the end of the string.
			void toEnd()
			{
				const uchar16_t* a = ref.c_str();
				pos = ref.size_raw();
			}

			//! Returns the iterator's position.
			//! \return The iterator's position.
			u32 getPos() const
			{
				return pos;
			}

			//! Returns the string the iterator is referencing.
			//! \return The string the iterator is referencing.
			ustring16<TAlloc>& getRef()
			{
				return ref;
			}

			//! Returns the string the iterator is referencing.
			//! \return The string the iterator is referencing.
			const ustring16<TAlloc>& getRef() const
			{
				return ref;
			}

		private:
			ustring16<TAlloc>& ref;
			u32 pos;
	};


	//! Iterator to iterate through a UTF-16 string.
	class _ustring16_const_iterator
	{
		public:
			friend class _ustring16_iterator;
			typedef typename ustring16<TAlloc>::_ustring16_const_iterator_access access;

			//! Constructors.
			_ustring16_const_iterator(const ustring16<TAlloc>& s) : ref(s), pos(0) {}
			_ustring16_const_iterator(const _ustring16_iterator& i) : ref(i.getRef()), pos(i.getPos()) {}
			_ustring16_const_iterator(const _ustring16_const_iterator& i) : ref(i.ref), pos(i.pos) {}
			_ustring16_const_iterator(const ustring16<TAlloc>& s, const u32 p) : ref(s), pos(0)
			{
				if (ref.size_raw() == 0 || p == 0)
					return;

				// Go to the appropriate position.
				u32 i = p;
				u32 sr = ref.size_raw();
				const uchar16_t* a = ref.c_str();
				while (i != 0 && pos < sr)
				{
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						pos += 2;
					else ++pos;
					--i;
				}
			}

			//! Test for equalness.
			bool operator==(const _ustring16_iterator& iter) const
			{
				if (&ref == &iter.ref && pos == iter.pos)
					return true;
				return false;
			}

			//! Test for equalness.
			bool operator==(const _ustring16_const_iterator& iter) const
			{
				if (&ref == &iter.ref && pos == iter.pos)
					return true;
				return false;
			}

			//! Test for unequalness.
			bool operator!=(const _ustring16_iterator& iter) const
			{
				if (&ref != &iter.ref || pos != iter.pos)
					return true;
				return false;
			}

			//! Test for unequalness.
			bool operator!=(const _ustring16_const_iterator& iter) const
			{
				if (&ref != &iter.ref || pos != iter.pos)
					return true;
				return false;
			}

			//! Switch to the next full character in the string.
			_ustring16_const_iterator& operator++()
			{	// ++iterator
				if (pos == ref.size_raw()) return *this;
				const uchar16_t* a = ref.c_str();
				if (UTF16_IS_SURROGATE_HI(a[pos]))
					pos += 2;			// TODO: check for valid low surrogate?
				else ++pos;
				if (pos > ref.size_raw()) pos = ref.size_raw();
				return *this;
			}

			//! Switch to the next full character in the string, returning the previous position.
			_ustring16_const_iterator operator++(int)
			{	// iterator++
				_ustring16_const_iterator _tmp(*this);
				++*this;
				return _tmp;
			}

			//! Switch to the previous full character in the string.
			_ustring16_const_iterator& operator--()
			{	// --iterator
				if (pos == 0) return *this;
				const uchar16_t* a = ref.c_str();
				--pos;
				if (UTF16_IS_SURROGATE_LO(a[pos]) && pos != 0)	// low surrogate, go back one more.
					--pos;
				return *this;
			}

			//! Switch to the previous full character in the string, returning the previous position.
			_ustring16_const_iterator operator--(int)
			{	// iterator--
				_ustring16_const_iterator _tmp(*this);
				--*this;
				return _tmp;
			}

			//! Advance a specified number of full characters in the string.
			//! \return Myself.
			_ustring16_const_iterator& operator+=(const int v)
			{
				if (v == 0) return *this;
				if (v < 0) return operator-=(v * -1);

				if (pos >= ref.size_raw())
					return *this;

				// Go to the appropriate position.
				u32 i = v;
				u32 sr = ref.size_raw();
				const uchar16_t* a = ref.c_str();
				while (i != 0 && pos < sr)
				{
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						pos += 2;
					else ++pos;
					--i;
				}
				if (pos > sr)
					pos = sr;

				return *this;
			}

			//! Go back a specified number of full characters in the string.
			//! \return Myself.
			_ustring16_const_iterator& operator-=(const int v)
			{
				if (v == 0) return *this;
				if (v > 0) return operator+=(v * -1);

				if (pos == 0)
					return *this;

				// Go to the appropriate position.
				u32 i = v;
				const uchar16_t* a = ref.c_str();
				while (i != 0 && pos != 0)
				{
					--pos;
					if (UTF16_IS_SURROGATE_LO(a[pos]) != 0 && pos != 0)
						--pos;
					--i;
				}

				return *this;
			}

			//! Return a new iterator that is a variable number of full characters forward from the current position.
			_ustring16_const_iterator operator+(const int v) const
			{
				_ustring16_const_iterator ret(*this);
				ret += v;
				return ret;
			}

			//! Return a new iterator that is a variable number of full characters backward from the current position.
			_ustring16_const_iterator operator-(const int v) const
			{
				_ustring16_const_iterator ret(*this);
				ret -= v;
				return ret;
			}

			//! Accesses the full character at the iterator's position.
			access operator*() const
			{
				if (pos >= ref.size_raw())
				{
					const uchar16_t* a = ref.c_str();
					u32 p = ref.size_raw();
					if (UTF16_IS_SURROGATE_LO(a[p]))
						--p;
					access ret(ref, p);
					return ret;
				}
				access ret(ref, pos);
				return ret;
			}

			//! Is the iterator at the start of the string?
			bool atStart() const
			{
				return pos == 0;
			}

			//! Is the iterator at the end of the string?
			bool atEnd() const
			{
				const uchar16_t* a = ref.c_str();
				if (UTF16_IS_SURROGATE(a[pos]))
					return (pos + 1) >= ref.size_raw();
				else return pos >= ref.size_raw();
			}

			//! Moves the iterator to the start of the string.
			void toStart()
			{
				pos = 0;
			}

			//! Moves the iterator to the end of the string.
			void toEnd()
			{
				const uchar16_t* a = ref.c_str();
				pos = ref.size_raw();
			}

			//! Returns the iterator's position.
			//! \return The iterator's position.
			u32 getPos() const
			{
				return pos;
			}

			//! Returns the string the iterator is referencing.
			//! \return The string the iterator is referencing.
			const ustring16<TAlloc>& getRef() const
			{
				return ref;
			}

		private:
			const ustring16<TAlloc>& ref;
			u32 pos;
	};

	typedef typename ustring16<TAlloc>::_ustring16_iterator iterator;
	typedef typename ustring16<TAlloc>::_ustring16_const_iterator const_iterator;

	///----------------------///
	/// end iterator classes ///
	///----------------------///

	//! Default constructor
	ustring16()
	: array(0), allocated(1), used(1)
	{
#if __BIG_ENDIAN__
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		array = allocator.allocate(1); // new u16[1];
		array[0] = 0x0;
	}


	//! Constructor
	ustring16(const ustring16<TAlloc>& other)
	: array(0), allocated(0), used(0)
	{
#if __BIG_ENDIAN__
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		*this = other;
	}


	//! Constructor from other string types
	template <class B, class A>
	ustring16(const string<B, A>& other)
	: array(0), allocated(0), used(0)
	{
#if __BIG_ENDIAN__
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		*this = other;
	}


	//! Constructor for copying a ustring16 from a pointer with a given length
	ustring16(const uchar16_t* const c, u32 length)
	: array(0), allocated(0), used(0)
	{
#if __BIG_ENDIAN__
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		if (!c)
		{
			// correctly init the ustring16 to an empty one
			*this="";
			return;
		}

		// Check for the BOM to determine the string's endianness.
		unicode::EUTF_ENDIAN c_end = unicode::EUTFEE_NATIVE;
		if (memcmp(c, unicode::BOM_ENCODE_UTF16_LE, unicode::BOM_ENCODE_UTF16_LEN) == 0)
			c_end = unicode::EUTFEE_LITTLE;
		else if (memcmp(c, unicode::BOM_ENCODE_UTF16_BE, unicode::BOM_ENCODE_UTF16_LEN) == 0)
			c_end = unicode::EUTFEE_BIG;

		// If a BOM was found, don't include it in the string.
		const uchar16_t* c2 = c;
		if (c_end != unicode::EUTFEE_NATIVE)
		{
			c2 = c + unicode::BOM_UTF16_LEN;
			length -= unicode::BOM_UTF16_LEN;
		}

		allocated = used = length+1;
		array = allocator.allocate(used); // new u16[used];

		// Copy the string now.
		unicode::EUTF_ENDIAN m_end = getEndianness();
		for (u32 l = 0; l<length; ++l)
		{
			array[l] = (uchar16_t)c2[l];
			if (c_end != unicode::EUTFEE_NATIVE && c_end != m_end)
				array[l] = unicode::swapEndian16(array[l]);
		}

		array[length] = 0;

		// Validate our new UTF-16 string.
		validate();
	}


	//! Constructor for unicode and ascii strings
	template <class B>
	ustring16(const B* const c)
	: array(0), allocated(0), used(0)
	{
#if __BIG_ENDIAN__
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		*this = c;
	}


#ifdef USTRING_CPP0X
	//! Constructor for moving a ustring16
	ustring16(ustring16<TAlloc>&& other)
	: array(other.array), encoding(other.encoding), allocated(other.allocated), used(other.used)
	{
		//std::cout << "MOVE constructor" << std::endl;
		other.array = 0;
		other.allocated = 0;
		other.used = 0;
	}
#endif


	//! Destructor
	~ustring16()
	{
		allocator.deallocate(array); // delete [] array;
	}


	//! Assignment operator
	ustring16& operator=(const ustring16<TAlloc>& other)
	{
		if (this == &other)
			return *this;

		used = other.size_raw()+1;
		if (used>allocated)
		{
			allocator.deallocate(array); // delete [] array;
			allocated = used;
			array = allocator.allocate(used); //new u16[used];
		}

		const uchar16_t* p = other.c_str();
		for (u32 i=0; i<used; ++i, ++p)
			array[i] = *p;

		// Validate our new UTF-16 string.
		validate();

		return *this;
	}


#ifdef USTRING_CPP0X
	//! Move assignment operator
	ustring16& operator=(ustring16<TAlloc>&& other)
	{
		if (this != &other)
		{
			//std::cout << "MOVE operator=" << std::endl;
			allocator.deallocate(array);

			array = other.array;
			encoding = other.encoding;
			used = other.used;
			other.array = 0;
			other.used = 0;
		}
		return *this;
	}
#endif


	//! Assignment operator for other string types
	template <class B, class A>
	ustring16<TAlloc>& operator=(const string<B, A>& other)
	{
		*this = other.c_str();
		return *this;
	}


	//! Assignment operator for UTF-8 strings
	ustring16<TAlloc>& operator=(const uchar8_t* const c)
	{
		if (!c)
		{
			if (!array)
			{
				array = allocator.allocate(1); //new u16[1];
				allocated = 1;
			}
			used = 1;
			array[0] = 0x0;
			return *this;
		}

		// Determine if the string is long enough for a BOM.
		u32 len = 0;
		const uchar8_t* p = c;
		do
		{
			++len;
		} while (*p++ && len < unicode::BOM_ENCODE_UTF8_LEN);

		// Check for BOM.
		unicode::EUTF_ENCODE c_bom = unicode::EUTFE_NONE;
		if (len == unicode::BOM_ENCODE_UTF8_LEN)
		{
			if (memcmp(c, unicode::BOM_ENCODE_UTF8, unicode::BOM_ENCODE_UTF8_LEN) == 0)
				c_bom = unicode::EUTFE_UTF8;
		}

		// If a BOM was found, don't include it in the string.
		const uchar8_t* c2 = c;
		if (c_bom != unicode::EUTFE_NONE)
			c2 = c + unicode::BOM_UTF8_LEN;

		// Check if the string is already set.
		if ((void*)c2 == (void*)array)
			return *this;

		len = 0;
		p = c2;
		do
		{
			++len;
		} while(*p++);

		// we'll keep the old ustring16 for a while, because the new
		// ustring16 could be a part of the current ustring16.
		uchar16_t* oldArray = array;

		used = len;
		if (used>allocated)
		{
			allocated = used;
			array = allocator.allocate(used); //new u16[used];
		}

		// Convert UTF-8 to UTF-16.
		u32 pos = 0;
		for (u32 l = 0; l<len;)
		{
			if (((c2[l] >> 6) & 0x03) == 0x02)
			{	// Invalid continuation byte.
				array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
				++l;
			}
			else if (c2[l] == 0xC0 || c2[l] == 0xC1)
			{	// Invalid byte - overlong encoding.
				array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
				++l;
			}
			else if ((c2[l] & 0xF8) == 0xF0)
			{	// 4 bytes UTF-8, 2 bytes UTF-16.
				// Check for a full string.
				if ((l + 3) >= len)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += 3;
					break;
				}

				// Validate.
				bool valid = true;
				u8 l2 = 0;
				if (valid && (((c2[l+1] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (valid && (((c2[l+2] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (valid && (((c2[l+3] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (!valid)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += l2;
					continue;
				}

				// Decode.
				uchar8_t b1 = ((c2[l] & 0x7) << 2) | ((c2[l+1] >> 4) & 0x3);
				uchar8_t b2 = ((c2[l+1] & 0xF) << 4) | ((c2[l+2] >> 2) & 0xF);
				uchar8_t b3 = ((c2[l+2] & 0x3) << 6) | (c2[l+3] & 0x3F);
				uchar32_t v = b3 | ((uchar32_t)b2 << 8) | ((uchar32_t)b1 << 16);

				// Split v up into a surrogate pair.
				uchar16_t x = static_cast<uchar16_t>(v);
				uchar16_t vh = UTF16_HI_SURROGATE | ((((v >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
				uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));

				array[pos++] = vh;
				array[pos++] = vl;
				l += 4;
			}
			else if ((c2[l] & 0xF0) == 0xE0)
			{	// 3 bytes UTF-8, 1 byte UTF-16.
				// Check for a full string.
				if ((l + 2) >= len)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += 2;
					break;
				}

				// Validate.
				bool valid = true;
				u8 l2 = 0;
				if (valid && (((c2[l+1] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (valid && (((c2[l+2] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (!valid)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += l2;
					continue;
				}

				// Decode.
				uchar8_t b1 = ((c2[l] & 0xF) << 4) | ((c2[l+1] >> 2) & 0xF);
				uchar8_t b2 = ((c2[l+1] & 0x3) << 6) | (c2[l+2] & 0x3F);
				uchar16_t ch = b2 | ((uchar16_t)b1 << 8);
				array[pos++] = ch;
				l += 3;
			}
			else if ((c2[l] & 0xE0) == 0xC0)
			{	// 2 bytes UTF-8, 1 byte UTF-16.
				// Check for a full string.
				if ((l + 1) >= len)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += 1;
					break;
				}

				// Validate.
				if (((c2[l+1] >> 6) & 0x03) != 0x02)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					++l;
					continue;
				}

				// Decode.
				uchar8_t b1 = (c2[l] >> 2) & 0x7;
				uchar8_t b2 = ((c2[l] & 0x3) << 6) | (c2[l+1] & 0x3F);
				uchar16_t ch = b2 | ((uchar16_t)b1 << 8);
				array[pos++] = ch;
				l += 2;
			}
			else
			{	// 1 byte UTF-8, 1 byte UTF-16.
				// Validate.
				if (c2[l] > 0x7F)
				{	// Values above 0xF4 are restricted and aren't used.  By now, anything above 0x7F is invalid.
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
				}
				else array[pos++] = static_cast<uchar16_t>(c2[l]);
				++l;
			}
		}
		used = pos;

		if (oldArray != array)
			allocator.deallocate(oldArray); // delete [] oldArray;

		// Validate our new UTF-16 string.
		validate();

		return *this;
	}


	//! Assignment operator for UTF-16 strings
	ustring16<TAlloc>& operator=(const uchar16_t* const c)
	{
		if (!c)
		{
			if (!array)
			{
				array = allocator.allocate(1); //new u16[1];
				allocated = 1;
			}
			used = 1;
			array[0] = 0x0;
			return *this;
		}

		// Check for the BOM to determine the string's endianness.
		unicode::EUTF_ENDIAN c_end = unicode::EUTFEE_NATIVE;
		if (memcmp(c, unicode::BOM_ENCODE_UTF16_LE, unicode::BOM_ENCODE_UTF16_LEN) == 0)
			c_end = unicode::EUTFEE_LITTLE;
		else if (memcmp(c, unicode::BOM_ENCODE_UTF16_BE, unicode::BOM_ENCODE_UTF16_LEN) == 0)
			c_end = unicode::EUTFEE_BIG;

		// If a BOM was found, don't include it in the string.
		const uchar16_t* c2 = c;
		if (c_end != unicode::EUTFEE_NATIVE)
			c2 = c + unicode::BOM_UTF16_LEN;

		if ((void*)c2 == (void*)array)
			return *this;

		u32 len = 0;
		const uchar16_t* p = c2;
		do
		{
			++len;
		} while(*p++);

		// we'll keep the old ustring16 for a while, because the new
		// ustring16 could be a part of the current ustring16.
		uchar16_t* oldArray = array;

		used = len;
		if (used>allocated)
		{
			allocated = used;
			array = allocator.allocate(used); //new u16[used];
		}

		unicode::EUTF_ENDIAN m_end = getEndianness();
		for (u32 l = 0; l<len; ++l)
		{
			array[l] = (uchar16_t)c2[l];
			if (c_end != unicode::EUTFEE_NATIVE && c_end != m_end)
				array[l] = unicode::swapEndian16(array[l]);
		}

		if (oldArray != array)
			allocator.deallocate(oldArray); // delete [] oldArray;

		// Validate our new UTF-16 string.
		validate();

		return *this;
	}


	//! Assignment operator for UTF-32 strings
	ustring16<TAlloc>& operator=(const uchar32_t* const c)
	{
		if (!c)
		{
			if (!array)
			{
				array = allocator.allocate(1); //new u16[1];
				allocated = 1;
			}
			used = 1;
			array[0] = 0x0;
			return *this;
		}

		// Check for the BOM to determine the string's endianness.
		unicode::EUTF_ENDIAN c_end = unicode::EUTFEE_NATIVE;
		if (memcmp(c, unicode::BOM_ENCODE_UTF32_LE, unicode::BOM_ENCODE_UTF32_LEN) == 0)
			c_end = unicode::EUTFEE_LITTLE;
		else if (memcmp(c, unicode::BOM_ENCODE_UTF32_BE, unicode::BOM_ENCODE_UTF32_LEN) == 0)
			c_end = unicode::EUTFEE_BIG;

		// If a BOM was found, don't include it in the string.
		const uchar32_t* c2 = c;
		if (c_end != unicode::EUTFEE_NATIVE)
			c2 = c + unicode::BOM_UTF32_LEN;

		if ((void*)c2 == (void*)array)
			return *this;

		u32 len = 0;
		const uchar32_t* p = c2;
		do
		{
			++len;
		} while(*p++);

		// we'll keep the old ustring16 for a while, because the new
		// ustring16 could be a part of the current ustring16.
		uchar16_t* oldArray = array;

		used = len * 2;		// In case all of the UTF-32 string is split into surrogate pairs.
		if (used>allocated)
		{
			allocated = used;
			array = allocator.allocate(used); //new u16[used];
		}

		// Convert UTF-32 to UTF-16.
		unicode::EUTF_ENDIAN m_end = getEndianness();
		u32 pos = 0;
		for (u32 l = 0; l<len; ++l)
		{
			uchar32_t ch = c2[l];
			if (c_end != unicode::EUTFEE_NATIVE && c_end != m_end)
				ch = unicode::swapEndian32(ch);

			if (ch > 0xFFFF)
			{
				// Split ch up into a surrogate pair as it is over 16 bits long.
				uchar16_t x = static_cast<uchar16_t>(ch);
				uchar16_t vh = UTF16_HI_SURROGATE | ((((ch >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
				uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));
				array[pos++] = vh;
				array[pos++] = vl;
			}
			else if (ch >= 0xD800 && ch <= 0xDFFF)
			{
				// Between possible UTF-16 surrogates (invalid!)
				array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
			}
			else array[pos++] = static_cast<uchar16_t>(ch);
		}
		used = pos;

		if (oldArray != array)
			allocator.deallocate(oldArray); // delete [] oldArray;

		// Validate our new UTF-16 string.
		validate();

		return *this;
	}


	//! Assignment operator for wchar_t strings.
	/** Note that this assumes that a correct unicode string is stored in the wchar_t string.
		Since wchar_t changes depending on its platform, it could either be a UTF-8, -16, or -32 string.
		This function assumes you are storing the correct unicode encoding inside the wchar_t string. **/
	ustring16<TAlloc>& operator=(const wchar_t* const c)
	{
		if (sizeof(wchar_t) == 4)
			*this = reinterpret_cast<const uchar32_t* const>(c);
		else if (sizeof(wchar_t) == 2)
			*this = reinterpret_cast<const uchar16_t* const>(c);
		else if (sizeof(wchar_t) == 1)
			*this = reinterpret_cast<const uchar8_t* const>(c);

		return *this;
	}


	//! Assignment operator for other strings.
	/** Note that this assumes that a correct unicode string is stored in the string. **/
	template <class B>
	ustring16<TAlloc>& operator=(const B* const c)
	{
		if (sizeof(B) == 4)
			*this = reinterpret_cast<const uchar32_t* const>(c);
		else if (sizeof(B) == 2)
			*this = reinterpret_cast<const uchar16_t* const>(c);
		else if (sizeof(B) == 1)
			*this = reinterpret_cast<const uchar8_t* const>(c);

		return *this;
	}


	//! Direct access operator
	_ustring16_iterator_access operator [](const u32 index)
	{
		_IRR_DEBUG_BREAK_IF(index>=size()) // bad index
		iterator iter(*this, index);
		return iter.operator*();
	}


	//! Direct access operator
	const _ustring16_const_iterator_access operator [](const u32 index) const
	{
		_IRR_DEBUG_BREAK_IF(index>=size()) // bad index
		const_iterator iter(*this, index);
		return iter.operator*();
	}


	//! Equality operator
	bool operator ==(const uchar16_t* const str) const
	{
		if (!str)
			return false;

		u32 i;
		for(i=0; array[i] && str[i]; ++i)
			if (array[i] != str[i])
				return false;

		return !array[i] && !str[i];
	}


	//! Equality operator
	bool operator ==(const ustring16<TAlloc>& other) const
	{
		for(u32 i=0; array[i] && other.array[i]; ++i)
			if (array[i] != other.array[i])
				return false;

		return used == other.used;
	}


	//! Is smaller comparator
	bool operator <(const ustring16<TAlloc>& other) const
	{
		for(u32 i=0; array[i] && other.array[i]; ++i)
		{
			s32 diff = array[i] - other.array[i];
			if ( diff )
				return diff < 0;
		}

		return used < other.used;
	}


	//! Inequality operator
	bool operator !=(const uchar16_t* const str) const
	{
		return !(*this == str);
	}


	//! Inequality operator
	bool operator !=(const ustring16<TAlloc>& other) const
	{
		return !(*this == other);
	}


	//! Returns the length of a ustring16 in full characters.
	//! \return Length of a ustring16 in full characters.
	u32 size() const
	{
		const_iterator i(*this, 0);
		u32 pos = 0;
		while (!i.atEnd())
		{
			++i;
			++pos;
		}
		return pos;
	}


	//! Returns a pointer to the raw UTF-16 string data.
	/** \return pointer to C-style NUL terminated array of UTF-16 code points. */
	const uchar16_t* c_str() const
	{
		return array;
	}


	//! compares the first n characters of the strings
	/** \param other Other ustring16 to compare.
	\param n Number of characters to compare
	\return True if the n first characters of both strings are equal. */
	bool equalsn(const ustring16<TAlloc>& other, u32 n) const
	{
		u32 i;
		const uchar16_t* oa = other.c_str();
		for(i=0; array[i] && oa[i] && i < n; ++i)
			if (array[i] != oa[i])
				return false;

		// if one (or both) of the strings was smaller then they
		// are only equal if they have the same length
		return (i == n) || (used == other.used);
	}


	//! compares the first n characters of the strings
	/** \param str Other ustring16 to compare.
	\param n Number of characters to compare
	\return True if the n first characters of both strings are equal. */
	bool equalsn(const uchar16_t* const str, u32 n) const
	{
		if (!str)
			return false;
		u32 i;
		for(i=0; array[i] && str[i] && i < n; ++i)
			if (array[i] != str[i])
				return false;

		// if one (or both) of the strings was smaller then they
		// are only equal if they have the same length
		return (i == n) || (array[i] == 0 && str[i] == 0);
	}


	//! Appends a character to this ustring16
	/** \param character: Character to append. */
	ustring16<TAlloc>& append(uchar32_t character)
	{
		if (used + 2 > allocated)
			reallocate(used + 2);

		if (character > 0xFFFF)
		{
			used += 2;

			// character will be multibyte, so split it up into a surrogate pair.
			uchar16_t x = static_cast<uchar16_t>(character);
			uchar16_t vh = UTF16_HI_SURROGATE | ((((character >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
			uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));
			array[used-3] = vh;
			array[used-2] = vl;
		}
		else
		{
			++used;
			array[used-2] = character;
		}
		array[used-1] = 0;

		return *this;
	}


	//! Appends a UTF-16 string to this ustring16
	/** \param other: Char ustring16 to append. */
	/** \param length: The length of the string to append. */
	ustring16<TAlloc>& append(const uchar16_t* const other, u32 length=0xffffffff)
	{
		if (!other)
			return;

		u32 len = 0;
		const uchar16_t* p = other;
		while(*p)
		{
			++len;
			++p;
		}
		if (len > length)
			len = length;

		if (used + len > allocated)
			reallocate(used + len);

		--used;
		++len;

		for (u32 l=0; l<len; ++l)
			array[l+used] = *(other+l);

		used += len;

		return *this;
	}


	//! Appends a ustring16 to this ustring16
	/** \param other: ustring16 to append. */
	ustring16<TAlloc>& append(const ustring16<TAlloc>& other)
	{
		const uchar16_t* oa = other.c_str();

		--used;
		u32 len = other.size_raw()+1;

		if (used + len + 1 > allocated)
			reallocate(used + len + 1);

		for (u32 l=0; l<len; ++l)
			array[used+l] = oa[l];

		used += len;

		return *this;
	}


	//! Appends a certain amount of characters of a ustring16 to this ustring16.
	/** \param other: other ustring16 to append to this ustring16.
	\param length: How many characters of the other ustring16 to add to this one. */
	ustring16<TAlloc>& append(const ustring16<TAlloc>& other, u32 length)
	{
		if (other.size() == 0)
			return *this;

		if (other.size() < length)
		{
			append(other);
			return;
		}

		if (used + length * 2 > allocated)
			reallocate(used + length * 2);

		--used;

		const_iterator iter(other, 0);
		u32 l = length;
		while (!iter.atEnd() && l)
		{
			uchar32_t c = *iter;
			append(c);
			++iter;
			--l;
		}

		return *this;
	}


	//! Reserves some memory.
	/** \param count: Amount of characters to reserve. */
	void reserve(u32 count)
	{
		if (count < allocated)
			return;

		reallocate(count);
	}


	//! Finds first occurrence of character.
	/** \param c: Character to search for.
	\return Position where the character has been found,
	or -1 if not found. */
	s32 findFirst(uchar32_t c) const
	{
		const_iterator i(*this, 0);

		s32 pos = 0;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			if (c == t)
				return pos;
			++pos;
			++i;
		}

		return -1;
	}

	//! Finds first occurrence of a character of a list.
	/** \param c: List of characters to find. For example if the method
	should find the first occurrence of 'a' or 'b', this parameter should be "ab".
	\param count: Amount of characters in the list. Usually,
	this should be strlen(c)
	\return Position where one of the characters has been found,
	or -1 if not found. */
	s32 findFirstChar(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(*this, 0);

		s32 pos = 0;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			for (u32 j=0; j<count; ++j)
				if (t == c[j])
					return pos;
			++pos;
			++i;
		}

		return -1;
	}


	//! Finds first position of a character not in a given list.
	/** \param c: List of characters not to find. For example if the method
	should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
	\param count: Amount of characters in the list. Usually,
	this should be strlen(c)
	\return Position where the character has been found,
	or -1 if not found. */
	s32 findFirstCharNotInList(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(*this, 0);

		s32 pos = 0;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			u32 j;
			for (j=0; j<count; ++j)
				if (t == c[j])
					break;

			if (j==count)
				return pos;
			++pos;
			++i;
		}

		return -1;
	}

	//! Finds last position of a character not in a given list.
	/** \param c: List of characters not to find. For example if the method
	should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
	\param count: Amount of characters in the list. Usually,
	this should be strlen(c)
	\return Position where the character has been found,
	or -1 if not found. */
	s32 findLastCharNotInList(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(end());

		s32 pos = size() - 1;
		while (!i.atStart())
		{
			uchar32_t t = *i;
			u32 j;
			for (j=0; j<count; ++j)
				if (t == c[j])
					break;

			if (j==count)
				return pos;
			--pos;
			--i;
		}

		return -1;
	}

	//! Finds next occurrence of character.
	/** \param c: Character to search for.
	\param startPos: Position in ustring16 to start searching.
	\return Position where the character has been found,
	or -1 if not found. */
	s32 findNext(uchar32_t c, u32 startPos) const
	{
		const_iterator i(*this, startPos);

		s32 pos = startPos;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			if (t == c)
				return pos;
			++pos;
			++i;
		}

		return -1;
	}


	//! Finds last occurrence of character.
	/** \param c: Character to search for.
	\param start: start to search reverse ( default = -1, on end )
	\return Position where the character has been found,
	or -1 if not found. */
	s32 findLast(uchar32_t c, s32 start = -1) const
	{
		u32 s = size();
		start = core::clamp ( start < 0 ? (s32)s : start, 0, (s32)s ) - 1;

		const_iterator i(*this, start);
		u32 pos = start;
		while (!i.atStart())
		{
			uchar32_t t = *i;
			if (t == c)
				return pos;
			--pos;
			--i;
		}

		return -1;
	}

	//! Finds last occurrence of a character in a list.
	/** \param c: List of strings to find. For example if the method
	should find the last occurrence of 'a' or 'b', this parameter should be "ab".
	\param count: Amount of characters in the list. Usually,
	this should be strlen(c)
	\return Position where one of the characters has been found,
	or -1 if not found. */
	s32 findLastChar(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(end());
		s32 pos = size();
		while (!i.atStart())
		{
			uchar32_t t = *i;
			for (u32 j=0; j<count; ++j)
				if (t == c[j])
					return pos;
			--pos;
			--i;
		}

		return -1;
	}


	//! Finds another ustring16 in this ustring16.
	/** \param str: Another ustring16
	\param start: Start position of the search
	\return Positions where the ustring16 has been found,
	or -1 if not found. */
	s32 find(const ustring16<TAlloc>& str, const u32 start = 0) const
	{
		u32 my_size = size();
		u32 their_size = str.size();

		if (their_size == 0 || my_size - start < their_size)
			return -1;

		const_iterator i(*this, start);

		s32 pos = start;
		while (!i.atEnd())
		{
			const_iterator i2(i);
			const_iterator j(str, 0);
			uchar32_t t1 = (uchar32_t)*i2;
			uchar32_t t2 = (uchar32_t)*j;
			while (t1 == t2)
			{
				++i2;
				++j;
				if (j.atEnd())
					return pos;
				t1 = (uchar32_t)*i2;
				t2 = (uchar32_t)*j;
			}
			++i;
			++pos;
		}

		return -1;
	}


	//! Returns a substring.
	/** \param begin: Start of substring.
	\param length: Length of substring. */
	ustring16<TAlloc> subString(u32 begin, s32 length) const
	{
		u32 len = size();
		// if start after ustring16
		// or no proper substring length
		if ((length <= 0) || (begin>=len))
			return ustring16<TAlloc>("");
		// clamp length to maximal value
		if ((length+begin) > len)
			length = len-begin;

		ustring16<TAlloc> o;
		o.reserve((length+1) * 2);

		const_iterator i(*this, begin);
		while (!i.atEnd() && length)
		{
			o.append(*i);
			++i;
			--length;
		}

		return o;
	}


	//! Appends a character to this ustring16.
	/** \param c Character to append. */
	ustring16<TAlloc>& operator += (uchar32_t c)
	{
		append(c);
		return *this;
	}


	//! Appends a char ustring16 to this ustring16.
	/** \param c Char ustring16 to append. */
	ustring16<TAlloc>& operator += (const uchar16_t* const c)
	{
		append(c);
		return *this;
	}


	//! Appends a ustring16 to this ustring16.
	/** \param other ustring16 to append. */
	ustring16<TAlloc>& operator += (const ustring16<TAlloc>& other)
	{
		append(other);
		return *this;
	}


	//! Replaces all characters of a given type with another one.
	/** \param toReplace Character to replace.
	\param replaceWith Character replacing the old one. */
	ustring16<TAlloc>& replace(uchar32_t toReplace, uchar32_t replaceWith)
	{
		iterator i(*this, 0);
		while (!i.atEnd())
		{
			typename ustring16<TAlloc>::iterator::access a = *i;
			if ((uchar32_t)a == toReplace)
				a = replaceWith;
			++i;
		}
		return *this;
	}


	//! Removes characters from a ustring16..
	/** \param c: Character to remove. */
	ustring16<TAlloc>& remove(uchar32_t c)
	{
		u32 pos = 0;
		u32 found = 0;
		u32 len = (c > 0xFFFF ? 2 : 1);		// Remove characters equal to the size of c as a UTF-16 character.
		for (u32 i=0; i<used; ++i)
		{
			uchar32_t uc32 = 0;
			if (!UTF16_IS_SURROGATE_HI(array[i]))
				uc32 |= array[i];
			else if (i + 1 < used)
			{
				// Convert the surrogate pair into a single UTF-32 character.
				uc32 = unicode::toUTF32(array[i], array[i + 1]);
			}
			u32 len2 = (uc32 > 0xFFFF ? 2 : 1);

			if (uc32 == c)
			{
				found += len;
				continue;
			}

			array[pos++] = array[i];
			if (len2 == 2)
				array[pos++] = array[++i];
		}
		used -= found;
		array[used] = 0;
		return *this;
	}


	//! Removes a ustring16 from the ustring16.
	/** \param toRemove: ustring16 to remove. */
	ustring16<TAlloc>& remove(const ustring16<TAlloc>& toRemove)
	{
		u32 size = toRemove.size_raw();
		if (size == 0) return *this;

		const uchar16_t* tra = toRemove.c_str();
		u32 pos = 0;
		u32 found = 0;
		for (u32 i=0; i<used; ++i)
		{
			u32 j = 0;
			while (j < size)
			{
				if (array[i + j] != tra[j])
					break;
				++j;
			}
			if (j == size)
			{
				found += size;
				i += size - 1;
				continue;
			}

			array[pos++] = array[i];
		}
		used -= found;
		array[used] = 0;
		return *this;
	}


	//! Removes characters from the ustring16.
	/** \param characters: Characters to remove. */
	ustring16<TAlloc>& removeChars(const ustring16<TAlloc>& characters)
	{
		if (characters.size_raw() == 0)
			return *this;

		u32 pos = 0;
		u32 found = 0;
		const_iterator iter(characters);
		for (u32 i=0; i<used; ++i)
		{
			uchar32_t uc32 = 0;
			if (!UTF16_IS_SURROGATE_HI(array[i]))
				uc32 |= array[i];
			else if (i + 1 < used)
			{
				// Convert the surrogate pair into a single UTF-32 character.
				uc32 = unicode::toUTF32(array[i], array[i+1]);
			}
			u32 len2 = (uc32 > 0xFFFF ? 2 : 1);

			bool cont = false;
			iter.toStart();
			while (!iter.atEnd())
			{
				uchar32_t c = *iter;
				if (uc32 == c)
				{
					found += (c > 0xFFFF ? 2 : 1);		// Remove characters equal to the size of c as a UTF-16 character.
					++i;
					cont = true;
					break;
				}
				++iter;
			}
			if (cont) continue;

			array[pos++] = array[i];
			if (len2 == 2)
				array[pos++] = array[++i];
		}
		used -= found;
		array[used] = 0;
		return *this;
	}


	//! Trims the ustring16.
	/** Removes the specified characters (by default, Latin-1 whitespace)
	from the begining and the end of the ustring16. */
	ustring16<TAlloc>& trim(const ustring16<TAlloc>& whitespace = " \t\n\r")
	{
		core::array<uchar32_t> utf32white = whitespace.toUTF32();

		// find start and end of the substring without the specified characters
		const s32 begin = findFirstCharNotInList(utf32white.const_pointer(), whitespace.used);
		if (begin == -1)
			return (*this="");

		const s32 end = findLastCharNotInList(utf32white.const_pointer(), whitespace.used);

		return (*this = subString(begin, (end +1) - begin));
	}


	//! Erases a character from the ustring16.
	/** May be slow, because all elements
	following after the erased element have to be copied.
	\param index: Index of element to be erased. */
	ustring16<TAlloc>& erase(u32 index)
	{
		_IRR_DEBUG_BREAK_IF(index>=used) // access violation

		iterator i(*this, index);

		uchar32_t t = *i;
		u32 len = (t > 0xFFFF ? 2 : 1);

		for (u32 j = static_cast<u32>(i.getPos()) + len; j < used; ++j)
			array[j - len] = array[j];

		used -= len;
		return *this;
	}


	//! Validate the existing ustring16, checking for valid surrogate pairs and checking for proper termination.
	ustring16<TAlloc>& validate()
	{
		// Validate all unicode characters.
		for (u32 i=0; i<allocated; ++i)
		{
			// Terminate on existing null.
			if (array[i] == 0)
			{
				used = i + 1;
				return *this;
			}
			if (UTF16_IS_SURROGATE(array[i]))
			{
				if (((i+1) >= allocated) || UTF16_IS_SURROGATE_LO(array[i]))
					array[i] = unicode::UTF_REPLACEMENT_CHARACTER;
				else if (UTF16_IS_SURROGATE_HI(array[i]) && !UTF16_IS_SURROGATE_LO(array[i+1]))
					array[i] = unicode::UTF_REPLACEMENT_CHARACTER;
				++i;
			}
			if (array[i] >= 0xFDD0 && array[i] <= 0xFDEF)
				array[i] = unicode::UTF_REPLACEMENT_CHARACTER;
		}

		// terminate
		used = 0;
		if ( allocated > 0 )
		{
			used = allocated - 1;
			array[used] = 0;
		}
		return *this;
	}


	//! Gets the last char of the ustring16, or 0.
	uchar32_t lastChar() const
	{
		if (used < 2)
			return 0;

		if (UTF16_IS_SURROGATE_LO(array[used-2]))
		{
			// Make sure we have a paired surrogate.
			if (used < 3)
				return 0;

			// Check for an invalid surrogate.
			if (!UTF16_IS_SURROGATE_HI(array[used-3]))
				return 0;

			// Convert the surrogate pair into a single UTF-32 character.
			return unicode::toUTF32(array[used-3], array[used-2]);
		}
		else
		{
			return array[used-2];
		}
	}


	//! Split the ustring16 into parts.
	/** This method will split a ustring16 at certain delimiter characters
	into the container passed in as reference. The type of the container
	has to be given as template parameter. It must provide a push_back and
	a size method.
	\param ret The result container
	\param c C-style ustring16 of delimiter characters
	\param count Number of delimiter characters
	\param ignoreEmptyTokens Flag to avoid empty substrings in the result
	container. If two delimiters occur without a character in between, an
	empty substring would be placed in the result. If this flag is set,
	only non-empty strings are stored.
	\param keepSeparators Flag which allows to add the separator to the
	result ustring16. If this flag is true, the concatenation of the
	substrings results in the original ustring16. Otherwise, only the
	characters between the delimiters are returned.
	\return The number of resulting substrings
	*/
	template<class container>
	u32 split(container& ret, const uchar32_t* const c, u32 count=1, bool ignoreEmptyTokens=true, bool keepSeparators=false) const
	{
		if (!c)
			return 0;

		const_iterator i(*this);
		const u32 oldSize=ret.size();
		u32 pos = 0;
		u32 lastpos = 0;
		u32 lastpospos = 0;
		bool lastWasSeparator = false;
		while (!i.atEnd())
		{
			uchar32_t ch = *i;
			bool foundSeparator = false;
			for (u32 j=0; j<count; ++j)
			{
				if (ch == c[j])
				{
					if ((!ignoreEmptyTokens || pos - lastpos != 0) &&
							!lastWasSeparator)
						ret.push_back(ustring16<TAlloc>(&array[lastpospos], pos - lastpos));
					foundSeparator = true;
					lastpos = (keepSeparators ? pos : pos + 1);
					lastpospos = (keepSeparators ? i.getPos() : i.getPos() + 1);
					break;
				}
			}
			lastWasSeparator = foundSeparator;
			++pos;
			++i;
		}
		u32 s = size() + 1;
		if (s > lastpos)
			ret.push_back(ustring16<TAlloc>(&array[lastpospos], s - lastpos));
		return ret.size()-oldSize;
	}


	//! Gets the size of the allocated memory buffer for the string.
	//! \return The size of the allocated memory buffer.
	u32 capacity() const
	{
		return allocated;
	}


	//! Returns the raw number of UTF-16 code points in the string, including the individual surrogates.
	//! \return The raw number of UTF-16 code points, excluding the trialing NUL.
	u32 size_raw() const
	{
		return used-1;
	}


	//! Inserts a character into the string.
	//! \param c The character to insert.
	//! \param pos The position to insert the character.
	ustring16<TAlloc>& insert(uchar32_t c, u32 pos)
	{
		u8 len = (c > 0xFFFF ? 2 : 1);

		if (used + len > allocated)
			reallocate(used + len);

		used += len;

		iterator iter(*this, pos);
		for (u32 i = used - 2; i > iter.getPos(); --i)
			array[i] = array[i - len];

		if (c > 0xFFFF)
		{
			// c will be multibyte, so split it up into a surrogate pair.
			uchar16_t x = static_cast<uchar16_t>(c);
			uchar16_t vh = UTF16_HI_SURROGATE | ((((c >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
			uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));
			array[iter.getPos()] = vh;
			array[iter.getPos()+1] = vl;
		}
		else
		{
			array[iter.getPos()] = static_cast<uchar16_t>(c);
		}
		array[used-1] = 0;
		return *this;
	}


	//! Inserts a string into the string.
	//! \param c The string to insert.
	//! \param pos The position to insert the string.
	ustring16<TAlloc>& insert(const ustring16<TAlloc>& c, u32 pos)
	{
		u32 len = c.size_raw();
		if (len == 0) return *this;

		if (used + len > allocated)
			reallocate(used + len);

		used += len;

		iterator iter(*this, pos);
		for (u32 i = used - 2; i > iter.getPos() + len; --i)
			array[i] = array[i - len];

		const uchar16_t* s = c.c_str();
		for (u32 i = 0; i < len; ++i)
		{
			array[pos++] = *s;
			++s;
		}

		array[used-1] = 0;
		return *this;
	}


	//! Inserts a character into the string.
	//! \param c The character to insert.
	//! \param pos The position to insert the character.
	ustring16<TAlloc>& insert_raw(uchar16_t c, u32 pos)
	{
		if (used + 1 > allocated)
			reallocate(used + 1);

		++used;

		for (u32 i = used - 2; i > pos; --i)
			array[i] = array[i - 1];

		array[pos] = c;
		array[used-1] = 0;
		return *this;
	}


	//! Removes a character from string.
	//! \param pos Position of the character to remove.
	ustring16<TAlloc>& erase_raw(u32 pos)
	{
		for (u32 i=pos; i<used; ++i)
		{
			array[i] = array[i + 1];
		}
		--used;
		array[used] = 0;
		return *this;
	}


	//! Replaces a character in the string.
	//! \param c The new character.
	//! \param pos The position of the character to replace.
	ustring16<TAlloc>& replace_raw(uchar16_t c, u32 pos)
	{
		array[pos] = c;
		return *this;
	}


	//! Returns an iterator to the beginning of the string.
	//! \return An iterator to the beginning of the string.
	iterator begin()
	{
		iterator i(*this, 0);
		return i;
	}


	//! Returns an iterator to the beginning of the string.
	//! \return An iterator to the beginning of the string.
	const_iterator begin() const
	{
		const_iterator i(*this, 0);
		return i;
	}


	//! Returns an iterator to the end of the string.
	//! \return An iterator to the end of the string.
	iterator end()
	{
		iterator i(*this, 0);
		i.toEnd();
		return i;
	}


	//! Returns an iterator to the end of the string.
	//! \return An iterator to the end of the string.
	const_iterator end() const
	{
		const_iterator i(*this, 0);
		i.toEnd();
		return i;
	}


	//! Converts the string to a UTF-8 encoded string.
	//! \return A pointer to a string containing the UTF-8 encoded string.
	core::string<uchar8_t> toUTF8_s(const bool addBOM = false) const
	{
		core::string<uchar8_t> ret;
		ret.reserve(used * 4 + (addBOM ? unicode::BOM_UTF8_LEN : 0));
		const_iterator iter(*this, 0);

		// Add the byte order mark if the user wants it.
		if (addBOM)
		{
			ret.append(unicode::BOM_ENCODE_UTF8[0]);
			ret.append(unicode::BOM_ENCODE_UTF8[1]);
			ret.append(unicode::BOM_ENCODE_UTF8[2]);
		}

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (c > 0xFFFF)
			{	// 4 bytes
				uchar8_t b1 = (0x1E << 3) | ((c >> 18) & 0x7);
				uchar8_t b2 = (0x2 << 6) | ((c >> 12) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b4 = (0x2 << 6) | (c & 0x3F);
				ret.append(b1);
				ret.append(b2);
				ret.append(b3);
				ret.append(b4);
			}
			else if (c > 0x7FF)
			{	// 3 bytes
				uchar8_t b1 = (0xE << 4) | ((c >> 12) & 0xF);
				uchar8_t b2 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | (c & 0x3F);
				ret.append(b1);
				ret.append(b2);
				ret.append(b3);
			}
			else if (c > 0x7F)
			{	// 2 bytes
				uchar8_t b1 = (0x6 << 5) | ((c >> 6) & 0x1F);
				uchar8_t b2 = (0x2 << 6) | (c & 0x3F);
				ret.append(b1);
				ret.append(b2);
			}
			else
			{	// 1 byte
				ret.append(static_cast<uchar8_t>(c));
			}
			++iter;
		}
		return ret;
	}


	//! Converts the string to a UTF-8 encoded string array.
	//! \return A pointer to an array containing the UTF-8 encoded string.
	core::array<uchar8_t> toUTF8(const bool addBOM = false) const
	{
		core::array<uchar8_t> ret((used - 1) * 4 + (addBOM ? unicode::BOM_UTF8_LEN : 0));
		const_iterator iter(*this, 0);

		// Add the byte order mark if the user wants it.
		if (addBOM)
		{
			ret.push_back(unicode::BOM_ENCODE_UTF8[0]);
			ret.push_back(unicode::BOM_ENCODE_UTF8[1]);
			ret.push_back(unicode::BOM_ENCODE_UTF8[2]);
		}

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (c > 0xFFFF)
			{	// 4 bytes
				uchar8_t b1 = (0x1E << 3) | ((c >> 18) & 0x7);
				uchar8_t b2 = (0x2 << 6) | ((c >> 12) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b4 = (0x2 << 6) | (c & 0x3F);
				ret.push_back(b1);
				ret.push_back(b2);
				ret.push_back(b3);
				ret.push_back(b4);
			}
			else if (c > 0x7FF)
			{	// 3 bytes
				uchar8_t b1 = (0xE << 4) | ((c >> 12) & 0xF);
				uchar8_t b2 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | (c & 0x3F);
				ret.push_back(b1);
				ret.push_back(b2);
				ret.push_back(b3);
			}
			else if (c > 0x7F)
			{	// 2 bytes
				uchar8_t b1 = (0x6 << 5) | ((c >> 6) & 0x1F);
				uchar8_t b2 = (0x2 << 6) | (c & 0x3F);
				ret.push_back(b1);
				ret.push_back(b2);
			}
			else
			{	// 1 byte
				ret.push_back(static_cast<uchar8_t>(c));
			}
			++iter;
		}
		return ret;
	}


#ifdef USTRING_CPP0X_NEWLITERALS	// C++0x
	//! Converts the string to a UTF-16 encoded string.
	//! \param endian The desired endianness of the string.
	//! \return A pointer to a string containing the UTF-16 encoded string.
	core::string<char16_t> toUTF16_s(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::string<char16_t> ret;
		ret.reserve(used + (addBOM ? unicode::BOM_UTF16_LEN : 0));

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				ret[0] = unicode::BOM;
			else if (endian == unicode::EUTFEE_LITTLE)
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ret.c_str());
				*ptr8++ = unicode::BOM_ENCODE_UTF16_LE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_LE[1];
			}
			else
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ret.c_str());
				*ptr8++ = unicode::BOM_ENCODE_UTF16_BE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_BE[1];
			}
		}

		ret.append(array);
		if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
		{
			char16_t* ptr = ret.c_str();
			for (u32 i = 0; i < ret.size(); ++i)
				*ptr++ = unicode::swapEndian16(*ptr);
		}
		return ret;
	}
#endif


	//! Converts the string to a UTF-16 encoded string array.
	//! Unfortunately, no toUTF16_s() version exists due to limitations with Irrlicht's string class.
	//! \param endian The desired endianness of the string.
	//! \return A pointer to an array containing the UTF-16 encoded string.
	core::array<uchar16_t> toUTF16(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::array<uchar16_t> ret((used - 1) + (addBOM ? unicode::BOM_UTF16_LEN : 0));
		uchar16_t* ptr = ret.pointer();

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				*ptr = unicode::BOM;
			else if (endian == unicode::EUTFEE_LITTLE)
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ptr);
				*ptr8++ = unicode::BOM_ENCODE_UTF16_LE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_LE[1];
			}
			else
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ptr);
				*ptr8++ = unicode::BOM_ENCODE_UTF16_BE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_BE[1];
			}
			++ptr;
		}

		memcpy((void*)ptr, (void*)array, (used - 1) * sizeof(uchar16_t));
		if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
		{
			for (u32 i = 0; i < (used - 2); ++i)
				*ptr++ = unicode::swapEndian16(*ptr);
		}
		ret.set_used((used - 1) + (addBOM ? unicode::BOM_UTF16_LEN : 0));
		return ret;
	}


#ifdef USTRING_CPP0X_NEWLITERALS	// C++0x
	//! Converts the string to a UTF-32 encoded string.
	//! \param endian The desired endianness of the string.
	//! \return A pointer to a string containing the UTF-32 encoded string.
	core::string<char32_t> toUTF32_s(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::string<char32_t> ret;
		ret.reserve(size() + 1 + (addBOM ? unicode::BOM_UTF32_LEN : 0));
		const_iterator iter(*this, 0);

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				ret.append(unicode::BOM);
			else
			{
				union
				{
					uchar32_t full;
					u8[4] chunk;
				} t;

				if (endian == unicode::EUTFEE_LITTLE)
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_LE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_LE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_LE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_LE[3];
				}
				else
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_BE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_BE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_BE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_BE[3];
				}
				ret.append(t.full);
			}
		}

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
				c = unicode::swapEndian32(c);
			ret.append(c);
			++iter;
		}
		return ret;
	}
#endif


	//! Converts the string to a UTF-32 encoded string array.
	//! Unfortunately, no toUTF32_s() version exists due to limitations with Irrlicht's string class.
	//! \param endian The desired endianness of the string.
	//! \return A pointer to an array containing the UTF-32 encoded string.
	core::array<uchar32_t> toUTF32(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::array<uchar32_t> ret(size() + (addBOM ? unicode::BOM_UTF32_LEN : 0));
		const_iterator iter(*this, 0);

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				ret.push_back(unicode::BOM);
			else
			{
				union
				{
					uchar32_t full;
					u8 chunk[4];
				} t;

				if (endian == unicode::EUTFEE_LITTLE)
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_LE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_LE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_LE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_LE[3];
				}
				else
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_BE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_BE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_BE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_BE[3];
				}
				ret.push_back(t.full);
			}
		}

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
				c = unicode::swapEndian32(c);
			ret.push_back(c);
			++iter;
		}
		return ret;
	}


	//! Converts the string to a wchar_t encoded string.
	/** The size of a wchar_t changes depending on the platform.  This function will store a
	correct UTF-8, -16, or -32 encoded string depending on the size of a wchar_t. **/
	//! \param endian The desired endianness of the string.
	//! \return A pointer to a string containing the wchar_t encoded string.
	core::string<wchar_t> toWCHAR_s(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		if (sizeof(wchar_t) == 4)
		{
			core::array<uchar32_t> a(toUTF32(endian, addBOM));
			core::stringw ret(a.pointer());
			return ret;
		}
		else if (sizeof(wchar_t) == 2)
		{
			if (endian == unicode::EUTFEE_NATIVE && addBOM == false)
			{
				core::stringw ret(array);
				return ret;
			}
			else
			{
				core::array<uchar16_t> a(toUTF16(endian, addBOM));
				core::stringw ret(a.pointer());
				return ret;
			}
		}
		else if (sizeof(wchar_t) == 1)
		{
			core::array<uchar8_t> a(toUTF8(addBOM));
			core::stringw ret(a.pointer());
			return ret;
		}

		// Shouldn't happen.
		return core::stringw();
	}


	//! Converts the string to a wchar_t encoded string array.
	/** The size of a wchar_t changes depending on the platform.  This function will store a
	correct UTF-8, -16, or -32 encoded string depending on the size of a wchar_t. **/
	//! \param endian The desired endianness of the string.
	//! \return A pointer to an array containing the wchar_t encoded string.
	core::array<wchar_t> toWCHAR(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		if (sizeof(wchar_t) == 4)
		{
			core::array<uchar32_t> a(toUTF32(endian, addBOM));
			core::array<wchar_t> ret(a.size());
			ret.set_used(a.size());
			memcpy((void*)ret.pointer(), (void*)a.pointer(), a.size() * sizeof(uchar32_t));
			return ret;
		}
		if (sizeof(wchar_t) == 2)
		{
			if (endian == unicode::EUTFEE_NATIVE && addBOM == false)
			{
				core::array<wchar_t> ret(used);
				ret.set_used(used);
				memcpy((void*)ret.pointer(), (void*)array, used * sizeof(uchar16_t));
				return ret;
			}
			else
			{
				core::array<uchar16_t> a(toUTF16(endian, addBOM));
				core::array<wchar_t> ret(a.size());
				ret.set_used(a.size());
				memcpy((void*)ret.pointer(), (void*)a.pointer(), a.size() * sizeof(uchar16_t));
				return ret;
			}
		}
		if (sizeof(wchar_t) == 1)
		{
			core::array<uchar8_t> a(toUTF8(addBOM));
			core::array<wchar_t> ret(a.size());
			ret.set_used(a.size());
			memcpy((void*)ret.pointer(), (void*)a.pointer(), a.size() * sizeof(uchar8_t));
			return ret;
		}

		// Shouldn't happen.
		return core::array<wchar_t>();
	}

	//! Gets the encoding of the Unicode string this class contains.
	//! \return An enum describing the current encoding of this string.
	unicode::EUTF_ENCODE getEncoding() const
	{
		return encoding;
	}

	//! Gets the endianness of the Unicode string this class contains.
	//! \return An enum describing the endianness of this string.
	unicode::EUTF_ENDIAN getEndianness() const
	{
		if (encoding == unicode::EUTFE_UTF16_LE ||
			encoding == unicode::EUTFE_UTF32_LE)
			return unicode::EUTFEE_LITTLE;
		else return unicode::EUTFEE_BIG;
	}

private:

	//! Reallocate the array, make it bigger or smaller
	void reallocate(u32 new_size)
	{
		uchar16_t* old_array = array;

		array = allocator.allocate(new_size); //new u16[new_size];
		allocated = new_size;

		u32 amount = used < new_size ? used : new_size;
		for (u32 i=0; i<amount; ++i)
			array[i] = old_array[i];

		if (allocated < used)
			used = allocated;

		allocator.deallocate(old_array); // delete [] old_array;
	}

	//--- member variables

	uchar16_t* array;
	unicode::EUTF_ENCODE encoding;
	u32 allocated;
	u32 used;
	TAlloc allocator;
	//irrAllocator<uchar16_t> allocator;
};

typedef ustring16<irrAllocator<uchar16_t> > ustring;


//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a null-terminated unicode string.
template <typename TAlloc, class B>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const B* const right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a null-terminated unicode string.
template <class B, typename TAlloc>
inline ustring16<TAlloc> operator+(const B* const left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


#ifdef USTRING_CPP0X
//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc>&& operator+(const ustring16<TAlloc>& left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(&, &&)" << std::endl;
	right.insert(left, 0);
	return std::move(right);
}


//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, const ustring16<TAlloc>& right)
{
	//std::cout << "MOVE operator+(&&, &)" << std::endl;
	left.append(right);
	return std::move(left);
}


//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(&&, &&)" << std::endl;
	if ((right.size_raw() <= left.capacity() - left.size_raw()) ||
		(right.capacity() - right.size_raw() < left.size_raw()))
	{
		left.append(right);
		return std::move(left);
	}
	else
	{
		right.insert(left, 0);
		return std::move(right);
	}
}


//! Appends a ustring16 and a null-terminated unicode string.
template <typename TAlloc, class B>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, const B* const right)
{
	//std::cout << "MOVE operator+(&&, B*)" << std::endl;
	left.append(right);
	return std::move(left);
}


//! Appends a ustring16 and a null-terminated unicode string.
template <class B, typename TAlloc>
inline ustring16<TAlloc>&& operator+(const B* const left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(B*, &&)" << std::endl;
	right.insert(left, 0);
	return std::move(right);
}
#endif

} // end namespace core
} // end namespace irr

#endif
