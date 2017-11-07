#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_MC_BLOCK_H__
#define __MCP04_MC_BLOCK_H__ 1

#include <memory>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "mcTransaction.h"
#include "../Block.h"

namespace MCP03
{
	namespace MetaChain
	{
#define CURRENT_MC_BLOCK_VERSION 1

		class mcBlock : public Block
		{
			private:
				// required to have serialization overrides
				friend class ::boost::serialization::access;

				// serialization
				template<class Archive>
				void													serialize(Archive &ar, const unsigned int version) const
				{
					// note: version is always stored last
					if (version == 1)
						ar << uint16tVersion << initiatorPubKey << hashPrevBlock << hashMerkleRoot << hash << nTime << uint32tByte << pTransaction;
				}

			public:
																		mcBlock(uint16_t Version = CURRENT_MC_BLOCK_VERSION);
																		~mcBlock();

				// the tx inside this block. in the metachain only one transaction per block is allowed
				std::shared_ptr<mcTransaction>							pTransaction;

				// every block in the MC is initiated by one single entity and stored for reference
				uint8_t													initiatorPubKey[64];

				// calculation functions
				void													calcMerkleRoot();
				void													calcSize();
				void													calcHash();

				// simple getter and setter
				std::string												toString();
		};
	}
}

BOOST_CLASS_VERSION(MCP03::MetaChain::mcBlock, CURRENT_MC_BLOCK_VERSION)
#endif