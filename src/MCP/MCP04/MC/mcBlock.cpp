/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "mcBlock.h"
#include "../../../crypto/sha3.h"

namespace MCP04
{
	namespace MetaChain
	{
		mcBlock::mcBlock()
			: Block(CURRENT_MC_BLOCK_VERSION)
		{
		}

		void mcBlock::calcMerkleRoot()
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
			if ((leaves.size() == 1) || (leaves.size() > 1 && !(leaves.size() % 2)))
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
	}
}