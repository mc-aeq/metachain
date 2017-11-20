/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Trust.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

// declare the name
const std::string MCP02::Trust::m_strName = "TRUST";

// register this class for polymorphic exporting
BOOST_CLASS_EXPORT_GUID(MCP02::Trust, "MCP02::Trust")

namespace MCP02
{
	void Trust::postInit()
	{
	}

	bool Trust::initGenesis(uint8_t initiatorPubKey[64], uint64_t uint64tGenesisCoins)
	{
		if (m_bGenesis)
		{
			LOG_ERROR("Trying to create a genesis block even though one was already created. Aborting for security reasons", "TRUST");
			return false;
		}

		m_bGenesis = true;
		return true;
	}
}