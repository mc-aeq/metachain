#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_MC_TXIN_H__
#define __MCP04_MC_TXIN_H__ 1

#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include "../txIn.h"
#include "../txOutRef.h"
#include "mcActions.h"

namespace MCP03
{
	namespace MetaChain
	{

#define CURRENT_MC_TXIN_VERSION 1

		class mcTxIn : public txIn
		{
			private:
				// required to have serialization overrides
				friend class ::boost::serialization::access;

				// serialization
				template<class Archive>
				void											serialize(Archive& ar, const unsigned int version)
				{
					// note: version is always stored last
					if (version == 1)
					{
						ar & boost::serialization::base_object<txIn>(*this) & eAction;
						switch (eAction)
						{
							case ACTION::CREATE_SUBCHAIN:		ar & *(createSubchain*)pPayload;		break;
						}
					}
				}

			public:
				// enum that defines the action of this tx
				enum ACTION
				{
					CREATE_SUBCHAIN = 1
				};

																mcTxIn();
																mcTxIn(ACTION eAction );
																~mcTxIn();
				void											init(ACTION eAction);


				// variables
				ACTION											eAction;
				void *											pPayload;

				// we don't have a previons tx in the MC scheme since we're not having in and outgoing tx. we have single commands that get validated
				const std::shared_ptr<MCP03::txOutRef>			txPrev = nullptr;

				// getter
				std::string										toString();
				uint32_t										getSize();
		};
	}
}

BOOST_CLASS_VERSION(MCP03::MetaChain::mcTxIn, CURRENT_MC_TXIN_VERSION)
#endif