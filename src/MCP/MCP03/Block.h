#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_BLOCK_H__
#define __MCP03_BLOCK_H__ 1

#include <cstdint>
#include <vector>
#include <memory>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "../../uint256.h"
#include "Transaction.h"

namespace MCP03
{

#define CURRENT_BLOCK_VERSION 1

	class Block
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
																Block(uint16_t Version = CURRENT_BLOCK_VERSION);
																~Block();

			// general settings for this block
			uint16_t											uint16tVersion;
			uint256												hashPrevBlock;
			uint256												hashMerkleRoot;
			uint256												hash;
			uint32_t											nTime;
			uint32_t											uint32tByte;

			// the tx inside this block
			std::vector< std::shared_ptr<Transaction> >			vecTx;

			virtual void										calcMerkleRoot();
			virtual void										calcSize();
			virtual void										calcHash();
			virtual void										calcAll() { calcSize(); calcMerkleRoot(); calcHash(); };

			// simple getter and setter
			virtual bool										isEmpty() { return (uint32tByte == 0); };
			virtual std::string									toString();
	};
}

BOOST_CLASS_VERSION(MCP03::Block, CURRENT_BLOCK_VERSION)
#endif