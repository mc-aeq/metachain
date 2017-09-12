
/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "base58.h"
#include <cstdint>
#include <vector>
#include <boost/algorithm/string.hpp>

// special include if it's a microsoft visual studio dev IDE
#ifdef _MSC_VER
	#include <basetsd.h>
	typedef SSIZE_T ssize_t;
#endif

namespace MCP01
{
	std::string base58::encode(const uint8_t *buffer, unsigned int uiLength)
	{
		// Skip & count leading zeroes.
		int zeroes = 0;
		int length = 0;

		while( zeroes < uiLength && !buffer[zeroes])
			zeroes++;

		// Allocate enough space in big-endian base58 representation.
		int size = (uiLength - zeroes) * 138 / 100 + 1; // log(256) / log(58), rounded up.
		std::vector<unsigned char> b58(size);

		// Process the bytes.
		for( int j = zeroes; j < uiLength; j++ )
		{
			int carry = buffer[j];
			int i = 0;
			// Apply "b58 = b58 * 256 + ch".
			for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++)
			{
				carry += 256 * (*it);
				*it = carry % 58;
				carry /= 58;
			}

			length = i;
		}

		// Skip leading zeroes in base58 result.
		std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
		while (it != b58.end() && *it == 0)
			it++;

		// Translate the result into a string.
		std::string str;
		str.reserve(zeroes + (b58.end() - it));
		str.assign(zeroes, '1');
		while (it != b58.end())
			str += pszBase58[*(it++)];

		return str;
	}

	//bool base58::decode(const char* psz, std::vector<unsigned char>& vch)
	bool base58::decode(const std::string input, uint8_t *output)
	{
		// remove unwanted spaces
		std::string inputS = input;
		boost::trim(inputS);

		// Skip and count leading '1's.
		int zeroes = 0;
		int length = 0;
		
		boost::trim_left_if(inputS, boost::is_any_of("1"));
		zeroes = input.length() - inputS.length();

		// Allocate enough space in big-endian base256 representation.
		int size = inputS.length() * 733 / 1000 + 1; // log(58) / log(256), rounded up.
		std::vector<unsigned char> b256(size);

		// Process the characters.
		for (auto itS = inputS.begin(); itS != inputS.end(); itS++)
		{
			// Decode base58 character
			const char* ch = strchr(pszBase58, *itS);

			if (ch == nullptr)
				return false;

			// Apply "b256 = b256 * 58 + ch".
			int carry = ch - pszBase58;
			int i = 0;
			for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
				carry += 58 * (*it);
				*it = carry % 256;
				carry /= 256;
			}

			length = i;
		}

		// Skip leading zeroes in b256.
		std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
		while (it != b256.end() && *it == 0)
			it++;

		// Copy result into output uint8_t array
		memset(output, 0x00, zeroes + (b256.end() - it));
		for (int i = zeroes; it != b256.end(); it++, i++)
			output[i] = *it;

		return true;
	}
}