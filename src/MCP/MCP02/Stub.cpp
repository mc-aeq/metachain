/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Stub.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

// declare the name
const std::string MCP02::Stub::m_strName = "Stub";

// register this class for polymorphic exporting
BOOST_CLASS_EXPORT_GUID(MCP02::Stub, "MCP02::Stub")

namespace MCP02
{
	// this function gets called after the first initialization and after every serialization load.
	// use this for generating precalculations or loading variables into buffers or any other one-time initializations
	void Stub::postInit()
	{
	}

	// implement genesis functionality here
	bool Stub::initGenesis(uint8_t initiatorPubKey[64], uint64_t uint64tGenesisCoins)
	{
		if (m_bGenesis)
		{
			LOG_ERROR("Trying to create a genesis block even though one was already created. Aborting for security reasons", "Stub");
			return false;
		}

		m_bGenesis = true;
		return true;
	}
}