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
#include "../../MCP03/Block.h"

namespace MCP04
{
	namespace MetaChain
	{
#define CURRENT_MC_BLOCK_VERSION 1

		class mcBlock : public MCP03::Block
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
						ar << uint16tVersion << hashPrevBlock << hashMerkleRoot << hash << nTime << uint32tByte << vecTx;
				}

			public:
																		mcBlock();
				
				 void													calcMerkleRoot();

				// the tx inside this block
				std::vector< std::shared_ptr<mcTransaction> >			vecTx;
		};
	}
}

BOOST_CLASS_VERSION(MCP04::MetaChain::mcBlock, CURRENT_MC_BLOCK_VERSION)
#endif