#include "base16.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "../logger.h"

base16::base16()
{
}

base16::base16(const std::string& hexcode)
{
	std::stringstream(hexcode) >> *this;
}

base16::base16(const data_chunk& value)
	: value_(value)
{
}

base16::base16(const base16& other)
	: base16(other.value_)
{
}

base16::operator const data_chunk&() const
{
	return value_;
}

base16::operator data_slice() const
{
	return value_;
}

std::istream& operator>>(std::istream& input, base16& argument)
{
	std::string hexcode;
	input >> hexcode;

	if (!decode_base16(argument.value_, hexcode))
	{
		LOG_ERROR("invalid something " + hexcode, "base16");
	}

	return input;
}

std::ostream& operator<<(std::ostream& output, const base16& argument)
{
	output << encode_base16(argument.value_);
	return output;
}

std::string encode_base16(data_slice data)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (int val : data)
		ss << std::setw(2) << val;
	return ss.str();
}

std::string encode_base16(unsigned char* data, unsigned int uiLength)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (int i = 0; i < uiLength; i++)
		ss << std::setw(2) << (int)data[i];
	return ss.str();
}

bool decode_base16(data_chunk& out, const std::string& in)
{
	// This prevents a last odd character from being ignored:
	if (in.size() % 2 != 0)
		return false;

	data_chunk result(in.size() / 2);
	if (!decode_base16_private(result.data(), result.size(), in.data()))
		return false;

	out = result;
	return true;
}


bool is_base16(const char c)
{
	return
		('0' <= c && c <= '9') ||
		('A' <= c && c <= 'F') ||
		('a' <= c && c <= 'f');
}

static unsigned from_hex(const char c)
{
	if ('A' <= c && c <= 'F')
		return 10 + c - 'A';
	if ('a' <= c && c <= 'f')
		return 10 + c - 'a';
	return c - '0';
}

// For support of template implementation only, do not call directly.
bool decode_base16_private(uint8_t* out, size_t out_size, const char* in)
{
	if (!std::all_of(in, in + 2 * out_size, is_base16))
		return false;

	for (size_t i = 0; i < out_size; ++i)
	{
		out[i] = (from_hex(in[0]) << 4) + from_hex(in[1]);
		in += 2;
	}

	return true;
}