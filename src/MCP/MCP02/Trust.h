#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP02_TRUST_H__
#define __MCP02_TRUST_H__ 1

#include <string>
#include "SubChain.h"

namespace MCP02
{
	class Trust : public SubChain
	{
		friend class ::boost::serialization::access;

		protected:
			static const std::string			m_strName;
			virtual void						postInit();

		private:
			// serialization
			template<class Archive>
			void								serialize(Archive &ar, const unsigned int version)
			{
				// call base object serialization
				ar & boost::serialization::base_object<SubChain>(*this);
			}

		public:
			static bool							registerFactory() { return ::MetaChain::getInstance().getStorageManager()->getSubChainManager()->registerSCFactory(m_strName, &createInstance); };
			static bool							registerFactory(MCP02::SubChainManager *ptr) { return ptr->registerSCFactory(m_strName, &createInstance); };
			static SubChain						*createInstance() { return new Trust(); };

			virtual bool						initGenesis(uint8_t initiatorPubKey[64], uint64_t uint64tGenesisCoins);
			virtual void						processBlock(MCP03::Block* Block);
	};
}

BOOST_CLASS_VERSION(MCP02::Trust, 1)
#endif