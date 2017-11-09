#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP02_STUB_H__
#define __MCP02_STUB_H__ 1

#include <string>
#include "SubChain.h"

namespace MCP02
{
	class Stub : public SubChain
	{
	protected:
		static const std::string			m_strName;

	public:
		static bool							registerFactory() { return ::MetaChain::getInstance().getStorageManager()->getSubChainManager()->registerSCFactory(m_strName, &createInstance); };
		static bool							registerFactory(MCP02::SubChainManager *ptr) { return ptr->registerSCFactory(m_strName, &createInstance); };
		static SubChain						*createInstance() { return new Stub(); };
	};
}

BOOST_CLASS_VERSION(MCP02::Stub, 1)
#endif