#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_MC_TRANSACTION_H__
#define __MCP04_MC_TRANSACTION_H__ 1

#include "../../MCP03/Transaction.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
#include "mcTxIn.h"

namespace MCP04
{
	namespace MetaChain
	{
#define CURRENT_MC_TRANSACTION_VERSION 1

		class mcTransaction : public MCP03::Transaction
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
						ar << uint16tVersion << uint32tLockTime << vecIn << vecOut;
				}

			public:
											mcTransaction();

				// overloaded tx vector to reflect our custom data scheme
				std::vector<mcTxIn>			vecIn;
		};
	}
}

BOOST_CLASS_VERSION(MCP04::MetaChain::mcTransaction, CURRENT_MC_TRANSACTION_VERSION)
#endif