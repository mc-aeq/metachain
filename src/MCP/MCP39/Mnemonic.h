#pragma once

/**
* Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
*
* This file is part of libbitcoin.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

// originally BIP39 but due to heavy changes in the underlying structure of SHA3, this is MCP39 now

#ifndef __MCP39_MNEMONIC_H__
#define __MCP39_MNEMONIC_H__

#include <cstdint>
#include <mutex>
#include "../array_slice.h"
#include "dictionairy.h"
#include "boost/locale.hpp"

// standard prefix for decoding mnemonic
#define MNEMONIC_DECODE_PREFIX "mnemonicMCP39"

typedef array_slice<uint8_t> data_slice;
typedef std::vector<uint8_t> data_chunk;
typedef std::initializer_list<data_slice> loaf;

template <size_t Size>
using byte_array = std::array<uint8_t, Size>;

namespace MCP39
{
	typedef std::vector<std::string> string_list;

	const size_t hash_size = 32;
	const size_t half_hash_size = hash_size / 2;
	const size_t quarter_hash_size = half_hash_size / 2;
	const size_t long_hash_size = 2 * hash_size;
	const size_t short_hash_size = 20;
	const size_t mini_hash_size = 6;


	typedef byte_array<hash_size> hash_digest;
	typedef byte_array<half_hash_size> half_hash;
	typedef byte_array<quarter_hash_size> quarter_hash;
	typedef byte_array<long_hash_size> long_hash;
	typedef byte_array<short_hash_size> short_hash;
	typedef byte_array<mini_hash_size> mini_hash;

	static constexpr size_t bits_per_word = 11;
	static constexpr size_t entropy_bit_divisor = 32;
	static size_t mnemonic_word_multiple = 3;
	static size_t mnemonic_seed_multiple = 4;
	const size_t byte_bits = 8;

	class Mnemonic
	{
		private:
			inline uint8_t					shift(size_t bit);
			inline data_chunk				build_chunk(loaf slices, size_t extra_reserve = 0);

			// normalization functions
			std::once_flag					m_onceMutex;
			std::string						normal_form(const std::string& value, boost::locale::norm_type form);
			void							validate_localization();
			std::string						toNormalForm(std::string value);

		public:
			string_list						create(data_slice entropy, const dictionary &lexicon = language::en);
			bool							isValid(const string_list& mnemonic, const dictionary &lexicon);
			bool							isValid(const string_list& mnemonic, const dictionary_list& lexicons = language::all);
			long_hash						decode(const string_list& mnemonic, const std::string& passphrase = "");
	};
}
#endif
