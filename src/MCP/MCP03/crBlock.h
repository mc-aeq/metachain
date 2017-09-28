#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_CRBLOCK_H__
#define __MCP03_CRBLOCK_H__ 1

#include <cstdint>
#include <vector>
#include <memory>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "../../uint256.h"
#include "crTransaction.h"
#include "Block.h"

namespace MCP03
{

#define CURRENT_CRBLOCK_VERSION 1

	class crBlock : public Block
	{
		private:
			// required to have serialization overrides
			friend class ::boost::serialization::access;

			// serialization
			template<class Archive>
			void												serialize(Archive &ar, const unsigned int version) const
			{
				// note: version is always stored last
				if (version == 1)
					ar << uint16tVersion << hashPrevBlock << hashMerkleRoot << hash << nTime << uint32tByte << vecTx;
			}

		public:
																crBlock(uint16_t Version = CURRENT_CRBLOCK_VERSION);
																~crBlock();
																
			// the tx inside this block
			std::vector< std::shared_ptr<crTransaction> >		vecTx;

			// calculation functions
			void												calcMerkleRoot();
			void												calcSize();
			void												calcHash();

			// simple getter and setter
			std::string											toString();
	};
}

BOOST_CLASS_VERSION(MCP03::crBlock, CURRENT_CRBLOCK_VERSION)
#endif