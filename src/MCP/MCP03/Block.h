#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_BLOCK_H__
#define __MCP03_BLOCK_H__ 1

#include <cstdint>
#include <boost/serialization/version.hpp>
#include "../../uint256.h"

namespace MCP03
{

#define CURRENT_BLOCK_VERSION 1

	class Block
	{
		friend class ::boost::serialization::access;

		private:
			// serialization
			template<class Archive>
			void												serialize(Archive &ar, const unsigned int version)
			{
				// note: version is always stored last
				if (version == 1)
					ar & uint16tVersion & hashPrevBlock & hashMerkleRoot & hash & nTime & uint32tByte;
			}

		public:
																Block(uint16_t Version)
																{
																	uint16tVersion = Version;
																	hashPrevBlock.SetNull();
																	hashMerkleRoot.SetNull();
																	hash.SetNull();
																	nTime = 0;
																	uint32tByte = 0;
																};

			// general settings for all blocks
			uint16_t											uint16tVersion;
			uint256												hashPrevBlock;
			uint256												hashMerkleRoot;
			uint256												hash;
			uint32_t											nTime;
			uint32_t											uint32tByte;

			// calculation functions
			virtual void										calcMerkleRoot() = 0;
			virtual uint32_t									calcSize() = 0;
			virtual uint256										calcHash() = 0;
			void												calcAll() { uint32tByte = calcSize(); calcMerkleRoot(); hash = calcHash(); };

			// checking functions
			bool												checkSize() { return (calcSize() == uint32tByte); };
			bool												checkHash() { return (calcHash() == hash); };

			// simple getter and setter
			virtual bool										isEmpty() { return (uint32tByte == 0); };
			virtual std::string									toString() = 0;
	};
}

BOOST_CLASS_VERSION(MCP03::Block, CURRENT_BLOCK_VERSION)
#endif