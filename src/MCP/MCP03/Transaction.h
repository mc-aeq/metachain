#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TRANSACTION_H__
#define __MCP03_TRANSACTION_H__ 1

#include <cstdint>
#include <sstream>
#include <boost/serialization/version.hpp>
#include "../../uint256.h"
#include "../../crypto/sha3.h"

namespace MCP03
{

#define CURRENT_TRANSACTION_VERSION 1

	class Transaction
	{
		friend class ::boost::serialization::access;

		private:
			// serialization
			template<class Archive>
			void								serialize(Archive &ar, const unsigned int version)
			{
				// note: version is always stored last
				if (version == 1)
					ar & uint16tVersion & hash;
			}

		public:
												Transaction(uint16_t Version) { uint16tVersion = Version; };

			// general settings for this transaction
			uint16_t							uint16tVersion;
			uint256								hash;

			// simple getter and setter
			virtual uint256						calcHash() = 0;
			bool								checkHash() { return hash == calcHash(); };
			virtual bool						isEmpty() = 0;
			virtual uint64_t					getValueOut() = 0;
			virtual uint32_t					getTotalSize() = 0;
			virtual std::string					toString() = 0;
	};
}

BOOST_CLASS_VERSION(MCP03::Transaction, CURRENT_TRANSACTION_VERSION)
#endif