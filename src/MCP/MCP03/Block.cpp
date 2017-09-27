/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Block.h"
#include <boost/archive/binary_oarchive.hpp>
#include "../../tinyformat.h"
#include "../../crypto/sha3.h"

namespace MCP03
{
	Block::Block(uint16_t Version)
		: uint16tVersion(Version),
		nTime(0),
		uint32tByte(0)
	{
		hashPrevBlock.SetNull();
		hashMerkleRoot.SetNull();
	}

	Block::~Block()
	{
		vecTx.clear();
	}

	void Block::calcHash()
	{
		// the hash of this block is the combined headers, plus the hash of our merkle root
		SHA3 crypto(SHA3::HashType::DEFAULT, SHA3::HashSize::SHA3_256);
		crypto.absorb(&uint16tVersion, sizeof(uint16_t));
		crypto.absorb(hashPrevBlock.begin(), hashPrevBlock.size());
		crypto.absorb(&nTime, sizeof(uint32_t));
		crypto.absorb(&uint32tByte, sizeof(uint32_t));
		crypto.absorb(hashMerkleRoot.begin(), hashMerkleRoot.size());

		hash = crypto.digest256();
	}

	std::string Block::toString()
	{
		std::stringstream s;
		s << strprintf("Block (Hash=%s, Version=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, Time=%u, Byte=%08x, TXs=%u)\n",
			hash.ToString(),
			uint16tVersion,
			hashPrevBlock.ToString(),
			hashMerkleRoot.ToString(),
			nTime,
			uint32tByte,
			vecTx.size());

		for( auto &it : vecTx)
			s << "  " << it->toString() << "\n";
		
		return s.str();
	}

	void Block::calcSize()
	{
		// serialize this block
		std::stringstream stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		{
			boost::archive::binary_oarchive oa(stream, boost::archive::no_header | boost::archive::no_tracking);
			oa << *this;
		}

		uint32tByte = stream.str().size();
	}

	void Block::calcMerkleRoot()
	{
		SHA3 crypto;
		std::vector< uint256 > leaves, tmp;

		// get all our leaves in place
		for (auto it : vecTx)
			leaves.push_back(it->getHash());

		// if we don't have leaves we reset the merkle root
		if (leaves.size() == 0)
		{
			hashMerkleRoot.SetNull();
			return;
		}

		// if we have only one tx, we add a second one so that we have atleast one iteration in the merkle root calculation
		// if we have more than one but an odd number, also add the last one
		if ( (leaves.size() == 1) || (leaves.size() > 1 && !(leaves.size() % 2)) )
			leaves.push_back(leaves.back());

		// we continue the hash calculation until we only have one last result - the merkle tree
		while (leaves.size() != 1)
		{
			// clean up our tmp vector
			tmp.clear();

			while (!leaves.empty())
			{
				// combine both leaves into one
				uint8_t cmb[64];
				memcpy(&cmb[0], leaves.back().begin(), 32);
				leaves.pop_back();
				memcpy(&cmb[31], leaves.back().begin(), 32);
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
}