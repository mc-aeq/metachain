#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_STUB_H__
#define __MCP04_STUB_H__ 1

#include "PoPInterface.h"
#include "../MCP03/crBlock.h"
#include "../MCP03/crTransaction.h"
#include "../MCP02/SubChainManager.h"

namespace MCP04
{
	// this is just a stub for reference
	// if you want to build your own proof of process, use this stub as a template
	// never use this class directly
#error Never include stub.h directly or use it!

	class STUB : public PoPInterface
	{
		protected:
			static const std::string			m_strName;

		public:
			static bool							registerFactory() { return ::MetaChain::getInstance().getStorageManager()->getSubChainManager()->registerFactory(m_strName, &createInstance); };
			static bool							registerFactory(MCP02::SubChainManager *ptr) { return ptr->registerFactory(m_strName, &createInstance); };
			static PoPInterface					*createInstance() { return new STUB(); };

			MCP03::Transaction*					createTXElement() { return new MCP03::crTransaction(); };
			MCP03::Block*						createBlockElement() { return new MCP03::crBlock(); };
	};
}

// declare the name
const std::string MCP04::STUB::m_strName = "STUB";
#endif