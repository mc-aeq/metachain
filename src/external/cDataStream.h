#pragma once

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __CDATASTREAM_H__
#define __CDATASTREAM_H__ 1

#include <assert.h>
#include <ios>

#include "zeroAfterFree.h"

/** Double ended buffer combining vector and stream-like interfaces.
*
* >> and << read and write unformatted data using the above serialization templates.
* Fills with data in linear time; some stringstream implementations take N^2 time.
*/
class cDataStream
{
protected:
	typedef CSerializeData vector_type;
	vector_type vch;
	unsigned int nReadPos;
public:

	typedef vector_type::allocator_type   allocator_type;
	typedef vector_type::size_type        size_type;
	typedef vector_type::difference_type  difference_type;
	typedef vector_type::reference        reference;
	typedef vector_type::const_reference  const_reference;
	typedef vector_type::value_type       value_type;
	typedef vector_type::iterator         iterator;
	typedef vector_type::const_iterator   const_iterator;
	typedef vector_type::reverse_iterator reverse_iterator;

	explicit cDataStream() : nReadPos(0)
	{
	}

	cDataStream(const_iterator pbegin, const_iterator pend) : vch(pbegin, pend), nReadPos(0)
	{
	}

	cDataStream(const char* pbegin, const char* pend) : vch(pbegin, pend), nReadPos(0)
	{
	}

	cDataStream(const vector_type& vchIn) : vch(vchIn.begin(), vchIn.end()), nReadPos(0)
	{
	}

	cDataStream(const std::vector<char>& vchIn) : vch(vchIn.begin(), vchIn.end()), nReadPos(0)
	{
	}

	cDataStream(const std::vector<unsigned char>& vchIn) : vch(vchIn.begin(), vchIn.end()), nReadPos(0)
	{
	}

	template <typename... Args>
	cDataStream(int nTypeIn, int nVersionIn, Args&&... args)
	{
		Init(nTypeIn, nVersionIn);
		::SerializeMany(*this, std::forward<Args>(args)...);
	}

	cDataStream& operator+=(const cDataStream& b)
	{
		vch.insert(vch.end(), b.begin(), b.end());
		return *this;
	}

	friend cDataStream operator+(const cDataStream& a, const cDataStream& b)
	{
		cDataStream ret = a;
		ret += b;
		return (ret);
	}

	std::string str() const
	{
		return (std::string(begin(), end()));
	}


	//
	// Vector subset
	//
	const_iterator begin() const { return vch.begin() + nReadPos; }
	iterator begin() { return vch.begin() + nReadPos; }
	const_iterator end() const { return vch.end(); }
	iterator end() { return vch.end(); }
	size_type size() const { return vch.size() - nReadPos; }
	bool empty() const { return vch.size() == nReadPos; }
	void resize(size_type n, value_type c = 0) { vch.resize(n + nReadPos, c); }
	void reserve(size_type n) { vch.reserve(n + nReadPos); }
	const_reference operator[](size_type pos) const { return vch[pos + nReadPos]; }
	reference operator[](size_type pos) { return vch[pos + nReadPos]; }
	void clear() { vch.clear(); nReadPos = 0; }
	iterator insert(iterator it, const char& x = char()) { return vch.insert(it, x); }
	void insert(iterator it, size_type n, const char& x) { vch.insert(it, n, x); }
	value_type* data() { return vch.data() + nReadPos; }
	const value_type* data() const { return vch.data() + nReadPos; }

	void insert(iterator it, std::vector<char>::const_iterator first, std::vector<char>::const_iterator last)
	{
		if (last == first) return;
		assert(last - first > 0);
		if (it == vch.begin() + nReadPos && (unsigned int)(last - first) <= nReadPos)
		{
			// special case for inserting at the front when there's room
			nReadPos -= (last - first);
			memcpy(&vch[nReadPos], &first[0], last - first);
		}
		else
			vch.insert(it, first, last);
	}

	void insert(iterator it, const char* first, const char* last)
	{
		if (last == first) return;
		assert(last - first > 0);
		if (it == vch.begin() + nReadPos && (unsigned int)(last - first) <= nReadPos)
		{
			// special case for inserting at the front when there's room
			nReadPos -= (last - first);
			memcpy(&vch[nReadPos], &first[0], last - first);
		}
		else
			vch.insert(it, first, last);
	}

	iterator erase(iterator it)
	{
		if (it == vch.begin() + nReadPos)
		{
			// special case for erasing from the front
			if (++nReadPos >= vch.size())
			{
				// whenever we reach the end, we take the opportunity to clear the buffer
				nReadPos = 0;
				return vch.erase(vch.begin(), vch.end());
			}
			return vch.begin() + nReadPos;
		}
		else
			return vch.erase(it);
	}

	iterator erase(iterator first, iterator last)
	{
		if (first == vch.begin() + nReadPos)
		{
			// special case for erasing from the front
			if (last == vch.end())
			{
				nReadPos = 0;
				return vch.erase(vch.begin(), vch.end());
			}
			else
			{
				nReadPos = (last - vch.begin());
				return last;
			}
		}
		else
			return vch.erase(first, last);
	}

	inline void Compact()
	{
		vch.erase(vch.begin(), vch.begin() + nReadPos);
		nReadPos = 0;
	}

	bool Rewind(size_type n)
	{
		// Rewind by n characters if the buffer hasn't been compacted yet
		if (n > nReadPos)
			return false;
		nReadPos -= n;
		return true;
	}


	//
	// Stream subset
	//
	bool eof() const { return size() == 0; }
	cDataStream* rdbuf() { return this; }
	int in_avail() { return size(); }

	void read(char* pch, size_t nSize)
	{
		if (nSize == 0) return;

		// Read from the beginning of the buffer
		unsigned int nReadPosNext = nReadPos + nSize;
		if (nReadPosNext >= vch.size())
		{
			if (nReadPosNext > vch.size())
			{
				throw std::ios_base::failure("CDataStream::read(): end of data");
			}
			memcpy(pch, &vch[nReadPos], nSize);
			nReadPos = 0;
			vch.clear();
			return;
		}
		memcpy(pch, &vch[nReadPos], nSize);
		nReadPos = nReadPosNext;
	}

	void ignore(int nSize)
	{
		// Ignore from the beginning of the buffer
		if (nSize < 0) {
			throw std::ios_base::failure("CDataStream::ignore(): nSize negative");
		}
		unsigned int nReadPosNext = nReadPos + nSize;
		if (nReadPosNext >= vch.size())
		{
			if (nReadPosNext > vch.size())
				throw std::ios_base::failure("CDataStream::ignore(): end of data");
			nReadPos = 0;
			vch.clear();
			return;
		}
		nReadPos = nReadPosNext;
	}

	void write(const char* pch, size_t nSize)
	{
		// Write to the end of the buffer
		vch.insert(vch.end(), pch, pch + nSize);
	}

	template<typename Stream>
	void Serialize(Stream& s) const
	{
		// Special case: stream << stream concatenates like stream += stream
		if (!vch.empty())
			s.write((char*)&vch[0], vch.size() * sizeof(vch[0]));
	}

	template<typename T>
	cDataStream& operator<<(const T& obj)
	{
		// Serialize to this stream
		::Serialize(*this, obj);
		return (*this);
	}

	template<typename T>
	cDataStream& operator>>(T& obj)
	{
		// Unserialize from this stream
		::Unserialize(*this, obj);
		return (*this);
	}

	void GetAndClear(CSerializeData &d) {
		d.insert(d.end(), begin(), end());
		clear();
	}

	/**
	* XOR the contents of this stream with a certain key.
	*
	* @param[in] key    The key used to XOR the data in this stream.
	*/
	void Xor(const std::vector<unsigned char>& key)
	{
		if (key.size() == 0) {
			return;
		}

		for (size_type i = 0, j = 0; i != size(); i++) {
			vch[i] ^= key[j++];

			// This potentially acts on very many bytes of data, so it's
			// important that we calculate `j`, i.e. the `key` index in this
			// way instead of doing a %, which would effectively be a division
			// for each byte Xor'd -- much slower than need be.
			if (j == key.size())
				j = 0;
		}
	}
};

#endif