#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TXOUT_H__
#define __MCP03_TXOUT_H__ 1

#include <string>
#include <cstdint>
#include <limits>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

namespace MCP03
{
	class txOut
	{
		private:
			// required to have serialization overrides
			friend class ::boost::serialization::access;

			// serialization
			template<class Archive>
			void								serialize(Archive &ar, const unsigned int version) const
			{
				// note: version is always stored last
				if (version == 1)
					ar << uint64tValue << uint8tPubKey;
			}

		public:
												txOut();
												~txOut();		

			uint64_t							uint64tValue;
			uint8_t								uint8tPubKey[64];

			// simple getter & setter
			virtual bool						isEmpty() { return (uint64tValue == (std::numeric_limits<uint64_t>::max)()); };
			virtual std::string					toString();

			// operators
			virtual bool						operator==(const txOut& ref);
			virtual bool						operator!=(const txOut& ref);
	};
}

BOOST_CLASS_VERSION(MCP03::txOut, 1)
#endif