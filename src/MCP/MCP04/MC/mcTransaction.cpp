/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "mcTransaction.h"
#include <sstream>
#include <boost/archive/binary_oarchive.hpp>
#include "../../../crypto/sha3.h"

namespace MCP04
{
	namespace MetaChain
	{
		mcTransaction::mcTransaction()
			: Transaction(CURRENT_MC_TRANSACTION_VERSION)
		{
		}

		uint256 mcTransaction::getHash()
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
	}
}