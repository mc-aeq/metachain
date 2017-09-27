/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Block.h"
#include "../../tinyformat.h"

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
		// todo: calculate sha3 hash
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
		// general header size
		uint32tByte = sizeof(uint16tVersion) + sizeof(hashPrevBlock) + sizeof(hashMerkleRoot) + sizeof(nTime);

		// add tx
		for (auto it : vecTx)
			uint32tByte += it->getTotalSize();
	}

	void Block::calcMerkleRoot()
	{
		std::vector< uint256 > leaves, tmp;

		// get all our leaves in place
		for (auto it : vecTx)
			leaves.push_back(it->getHash());

		// if we don't have leaves we reset the merkle root
		if (leaves.size() == 0)
			hashMerkleRoot.SetNull();

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
				// calculate the combined hashes of the leaves
				uint256 leavesCombined;
				// calc 2xsha3
				leaves.pop_back();
				leaves.pop_back();
				tmp.push_back(leavesCombined);
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