/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Transaction.h"
#include <boost/archive/binary_oarchive.hpp>
#include "../../logger.h"
#include "../../tinyformat.h"
#include "../../hash.h"
#include "../../prevector.h"
#include "../../crypto/sha3.h"

namespace MCP03
{
	Transaction::Transaction(uint16_t Version)
		: uint16tVersion(Version),
		uint32tLockTime(0)
	{

	}

	Transaction::~Transaction()
	{
		vecIn.clear();
		vecOut.clear();
	}


	std::string Transaction::toString()
	{
		std::string str;
		str += strprintf("Transaction(Hash=%s, Version=%d, vecIn.size=%u, vecOut.size=%u, LockTime=%u)\n",
			getHash().ToString().substr(0, 10),
			uint16tVersion,
			vecIn.size(),
			vecOut.size(),
			uint32tLockTime);

		for( auto &it : vecIn )
			str += "    " + it.toString() + "\n";
		for( auto &it : vecOut )
			str += "    " + it.toString() + "\n";

		return str;
	}

	uint256 Transaction::getHash()
	{
		// serialize this transaction
		std::stringstream stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		{
			boost::archive::binary_oarchive oa(stream, boost::archive::no_header | boost::archive::no_tracking);
			oa << *this;
		}

		// return the calculated hash
		SHA3 crypto;
		return crypto.hash256(SHA3::HashType::DEFAULT, (uint8_t *)stream.str().data(), stream.str().size());
	}

	uint64_t Transaction::getValueOut()
	{
		uint64_t nValueOut = 0;
		for( auto &it : vecOut)
		{
			nValueOut += it.getValue();
			if (!validCoinRange(it.getValue()) || !validCoinRange(nValueOut))
			{
				LOG_ERROR("Coin Amount out of range: " + it.toString(), "TX");
				return (std::numeric_limits<uint64_t>::max)();
			}
		}
		return nValueOut;
	}

	uint32_t Transaction::getTotalSize()
	{
		// header size
		uint32_t size = sizeof(uint16tVersion) + sizeof(uint32tLockTime);

		// tx sizes
		for (auto &it : vecIn)
			size += it.getSize();
		for (auto &it : vecOut)
			size += it.getSize();

		return size;
	}
}