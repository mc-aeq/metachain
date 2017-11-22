#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_MC_TRANSACTION_H__
#define __MCP04_MC_TRANSACTION_H__ 1

#include "../Transaction.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include "mcTxIn.h"

namespace MCP03
{
	namespace MetaChain
	{

#define CURRENT_MC_TRANSACTION_VERSION 1

		class mcTransaction : public Transaction
		{
			private:
				// required to have serialization overrides
				friend class ::boost::serialization::access;

				// serialization
				template<class Archive>
				void												serialize(Archive &ar, const unsigned int version)
				{
					// serialize transaction, then add the incoming tx
					if (version == 1)
						ar & boost::serialization::base_object<Transaction>(*this) & txIn;
				}

			public:
																	mcTransaction(uint16_t Version = CURRENT_MC_TRANSACTION_VERSION);
																	~mcTransaction();

				// the tx that are in this transaction
				// note: in the MC we're not working with coins or something like that.
				// this is just a control chamber for our sidechains. so we don't need outgoing tx and we also have only one TX per transaction
				mcTxIn												txIn;

				// simple getter and setter
				uint256												calcHash();
				bool												isEmpty() { return (txIn.pPayload == nullptr); };
				uint64_t											getValueOut() { return 0; };
				uint32_t											getTotalSize();
				std::string											toString();
		};
	}
}

BOOST_CLASS_VERSION(MCP03::MetaChain::mcTransaction, CURRENT_MC_TRANSACTION_VERSION)
#endif