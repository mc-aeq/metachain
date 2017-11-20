/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Mine.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "../MCP03/crBlock.h"
#include "../MCP03/crTransaction.h"

// declare the name
const std::string MCP02::Mine::m_strName = "MINE";

// register this class for polymorphic exporting
BOOST_CLASS_EXPORT_GUID(MCP02::Mine, "MCP02::Mine")

namespace MCP02
{
	void Mine::postInit()
	{
		m_uint64tMaxCoins = boost::lexical_cast<uint64_t>(m_mapParams["maxCoins"].c_str());
		m_uiMaxDecimalPlaces = boost::lexical_cast<unsigned int>(m_mapParams["maxDecimalPlaces"].c_str());
		m_uint64tMaxAmountCoins = m_uint64tMaxCoins*m_uiMaxDecimalPlaces;
	}

	bool Mine::initGenesis(uint8_t initiatorPubKey[64], uint64_t uint64tGenesisCoins)
	{
		if (m_bGenesis)
		{
			LOG_ERROR("Trying to create a genesis block even though one was already created. Aborting for security reasons", "MINE");
			return false;
		}
		LOG("Generating Genesis Block", "MINE");

		// setting the timestamp for this genesis block
		m_uint32GenesisTimestamp = std::time(nullptr);

		std::shared_ptr<MCP03::crTransaction> txNew((MCP03::crTransaction*)m_pPoP->createTXElement());
		txNew->uint16tVersion = 1;
		txNew->vecIn.resize(1);
		txNew->vecOut.resize(1);
		//txNew->vecIn[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew->vecOut[0].uint64tValue = uint64tGenesisCoins;
		memcpy(txNew->vecOut[0].uint8tPubKey, initiatorPubKey, sizeof(uint8_t)*64 );

		MCP03::crBlock *genesis = (MCP03::crBlock*)m_pPoP->createBlockElement();
		genesis->nTime = m_uint32GenesisTimestamp;
		genesis->vecTx.push_back(std::move(txNew));
		genesis->hashPrevBlock.SetNull();
		genesis->calcAll();

		m_bGenesis = true;
		return true;
	}
}