/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "mcBlock.h"
#include <sstream>
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "../../MCP01/base58.h"
#include "../../../crypto/sha3.h"
#include "../../../logger.h"
#include "../../../tinyformat.h"

// register this class for polymorphic exporting
BOOST_CLASS_EXPORT_GUID(MCP03::MetaChain::mcBlock, "MCP03::MetaChain::mcBlock")

namespace MCP03
{
	namespace MetaChain
	{
		mcBlock::mcBlock(uint16_t Version)
			: Block(Version)
		{
		}

		mcBlock::~mcBlock()
		{
		}

		void mcBlock::calcMerkleRoot()
		{
			SHA3 crypto;
			std::vector< uint256 > leaves, tmp;

			// we only have one transaction in a MC block, so we add it twice that we have atleast one iteration
			// it is also guaranteed that no MC block will be made with no TX, so we skip checks whether the transaction exists
			pTransaction->hash = pTransaction->calcHash();
			leaves.push_back(pTransaction->hash);
			leaves.push_back(leaves.back());

			// we continue the hash calculation until we only have one last result - the merkle tree
			while (leaves.size() != 1)
			{
				// clean up our tmp vector
				tmp.clear();

				while (!leaves.empty())
				{
					// combine both leaves into one
					uint8_t cmb[64] = {};
					memcpy(&cmb[0], leaves.back().begin(), 32);
					leaves.pop_back();
					memcpy(&cmb[32], leaves.back().begin(), 32);
					leaves.pop_back();

					// calc combined hash and add it
					tmp.push_back(crypto.hash256(SHA3::HashType::DEFAULT, cmb, 64));
				}

				// if we have more than 1 result in the vector and an odd number, we add the last entry again for hashing
				if (tmp.size() != 1 && !(tmp.size() % 2))
					tmp.push_back(tmp.back());

				// move our tmp data to our leaves vector
				leaves.swap(tmp);
			}

			// update the merkle root
			hashMerkleRoot = leaves[0];
		}

		uint32_t mcBlock::calcSize()
		{
			// serialize this block
			std::stringstream stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
			{
				boost::archive::binary_oarchive oa(stream, boost::archive::no_header | boost::archive::no_tracking);
				oa << *this;
			}

			return stream.str().size();
		}

		uint256 mcBlock::calcHash()
		{
			// the hash of this block is the combined headers, plus the hash of our merkle root
			SHA3 crypto(SHA3::HashType::DEFAULT, SHA3::HashSize::SHA3_256);
			crypto.absorb(&uint16tVersion, sizeof(uint16_t));
			crypto.absorb(hashPrevBlock.begin(), hashPrevBlock.size());
			crypto.absorb(&nTime, sizeof(uint32_t));
			crypto.absorb(&uint32tByte, sizeof(uint32_t));
			crypto.absorb(hashMerkleRoot.begin(), hashMerkleRoot.size());

			return crypto.digest256();
		}

		std::string mcBlock::toString()
		{
			std::stringstream s;
			s << strprintf("Block (Initiator=%s, Hash=%s, Version=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, Time=%u, Byte=%u)\n",
				MCP01::base58::encode(initiatorPubKey, 64),
				hash.ToString(),
				uint16tVersion,
				hashPrevBlock.ToString(),
				hashMerkleRoot.ToString(),
				nTime,
				uint32tByte);

			s << "  " << pTransaction->toString() << "\n";

			return s.str();
		}
	}
}