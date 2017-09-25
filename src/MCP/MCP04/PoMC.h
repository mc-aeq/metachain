#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_POMC_H__
#define __MCP04_POMC_H__ 1

#include "ChainInterface.h"

namespace MCP04
{
	class PoMC : public ChainInterface
	{
		protected:
			static const std::string			m_strName;

		public:
			static bool							registerFactory() { return MetaChain::getInstance().getStorageManager()->getSubChainManager()->registerFactory(m_strName, &createInstance); };
			static ChainInterface				*createInstance() { return new PoMC(); };
	};
}
#endif