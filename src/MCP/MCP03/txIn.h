#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TXIN_H__
#define __MCP03_TXIN_H__ 1

#include <string>
#include <memory>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "txOutRef.h"

namespace MCP03
{
	class txIn
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
					ar << txPrev << strSignature;
			}

		public:
														txIn();
														txIn(std::shared_ptr<txOutRef> ref, std::string sig);
														~txIn();
			
			std::shared_ptr<txOutRef>					txPrev;
			std::string									strSignature;

			virtual std::string							toString();

			// operators
			virtual bool								operator==(const txIn& ref);
			virtual bool								operator!=(const txIn& ref);
	};
}

BOOST_CLASS_VERSION(MCP03::txIn, 1)
#endif