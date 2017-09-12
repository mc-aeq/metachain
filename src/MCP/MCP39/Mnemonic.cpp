#include "Mnemonic.h"
#include <cstdint>
#include <mutex>
#include <iterator>
#include "../../crypto/sha3.h"
#include "boost/algorithm/string.hpp"
#include "boost/system/system_error.hpp"

namespace MCP39
{
	uint8_t Mnemonic::shift(size_t bit)
	{
		return (1 << (byte_bits - (bit % byte_bits) - 1));
	}

	data_chunk Mnemonic::build_chunk(loaf slices, size_t extra_reserve)
	{
		size_t size = 0;
		for (const auto slice : slices)
			size += slice.size();

		data_chunk out;
		out.reserve(size + extra_reserve);
		for (const auto slice : slices)
			out.insert(out.end(), slice.begin(), slice.end());

		return out;
	}

	string_list Mnemonic::create(data_slice entropy, const dictionary &lexicon)
	{
		if ((entropy.size() % mnemonic_seed_multiple) != 0)
			return string_list();

		const size_t entropy_bits = (entropy.size() * byte_bits);
		const size_t check_bits = (entropy_bits / entropy_bit_divisor);
		const size_t total_bits = (entropy_bits + check_bits);
		const size_t word_count = (total_bits / bits_per_word);

		hash_digest hash;
		SHA3 crypto;
		memcpy(hash.data(), crypto.hash(SHA3::HashType::DEFAULT, SHA3::HashSize::SHA3_256, entropy.data(), entropy.size()), 32);

		const auto data = build_chunk({ entropy, hash });

		size_t bit = 0;
		string_list words;

		for (size_t word = 0; word < word_count; word++)
		{
			size_t position = 0;
			for (size_t loop = 0; loop < bits_per_word; loop++)
			{
				bit = (word * bits_per_word + loop);
				position <<= 1;

				const auto byte = bit / byte_bits;

				if ((data[byte] & shift(bit)) > 0)
					position++;
			}

			words.push_back(lexicon[position]);
		}

		return words;
	}

	bool Mnemonic::isValid(const string_list& words, const dictionary& lexicon)
	{
		const auto word_count = words.size();
		if ((word_count % mnemonic_word_multiple) != 0)
			return false;

		const auto total_bits = bits_per_word * word_count;
		const auto check_bits = total_bits / (entropy_bit_divisor + 1);
		const auto entropy_bits = total_bits - check_bits;

		size_t bit = 0;
		data_chunk data((total_bits + byte_bits - 1) / byte_bits, 0);

		for (const auto& word : words)
		{
			auto itPos = std::find(lexicon.begin(), lexicon.end(), word);

			if (itPos == lexicon.end())
				return false;

			int position = std::distance(lexicon.begin(), itPos);

			for (size_t loop = 0; loop < bits_per_word; loop++, bit++)
			{
				if (position & (1 << (bits_per_word - loop - 1)))
				{
					const auto byte = bit / byte_bits;
					data[byte] |= shift(bit);
				}
			}
		}

		data.resize(entropy_bits / byte_bits);

		const auto mnemonic = create(data, lexicon);
		return std::equal(mnemonic.begin(), mnemonic.end(), words.begin());
	}

	bool Mnemonic::isValid(const string_list& mnemonic,	const dictionary_list& lexicons)
	{
		for (const auto& lexicon : lexicons)
			if (isValid(mnemonic, *lexicon))
				return true;

		return false;
	}

	long_hash Mnemonic::decode(const string_list& mnemonic, const std::string& passphrase)
	{
		std::string strSentence = boost::join(mnemonic, " " );
		std::string salt = toNormalForm(passphrase);

		SHA3 crypto;
		uint8_t* tmp = crypto.kmac(SHA3::HashSize::SHA3_256, (uint8_t*)strSentence.c_str(), strSentence.length(), SHA3::HashSize::SHA3_256 * 2, salt, MNEMONIC_DECODE_PREFIX);
		
		long_hash hashRet;
		memset(&hashRet, 0x00, sizeof(long_hash));
		memcpy(&hashRet, tmp, sizeof(long_hash));

		return hashRet;
	}

	// The backend selection is ignored if invalid (in this case on Windows).
	std::string Mnemonic::normal_form(const std::string& value, boost::locale::norm_type form)
	{
		auto backend = boost::locale::localization_backend_manager::global();
		backend.select("icu");
		const boost::locale::generator locale(backend);
		return normalize(value, form, locale("en_US.UTF8"));
	}

	// One time verifier of the localization backend manager. This is
	// necessary because boost::normalize will fail silently to perform
	// normalization if the ICU dependency is missing.
	void Mnemonic::validate_localization()
	{
		const auto ascii_space = "> <";
		const auto ideographic_space = ">　<";
		const auto normal = normal_form(ideographic_space, boost::locale::norm_type::norm_nfkd);

		if (normal != ascii_space)
			throw std::runtime_error(
				"Unicode normalization test failed, a dependency may be missing.");
	}

	std::string Mnemonic::toNormalForm(std::string value)
	{
		std::call_once(m_onceMutex, &Mnemonic::validate_localization, this);
		return normal_form(value, boost::locale::norm_type::norm_nfkd);
	}
}