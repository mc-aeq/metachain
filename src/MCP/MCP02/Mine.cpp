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
#include "../MCP06/script.h"

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

		const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
		std::shared_ptr<MCP03::crTransaction> txNew((MCP03::crTransaction*)m_pPoP->createTXElement());
		txNew->uint16tVersion = 1;
		txNew->vecIn.resize(1);
		txNew->vecOut.resize(1);
		txNew->vecIn[0].scriptSignature = MCP06::CScript() << 486604799 << MCP06::CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew->vecOut[0].uint64tValue = uint64tGenesisCoins;
		memcpy(txNew->vecOut[0].uint8tPubKey, initiatorPubKey, sizeof(uint8_t)*64 );

		MCP03::crBlock *genesis = (MCP03::crBlock*)m_pPoP->createBlockElement();
		genesis->nTime = m_uint32GenesisTimestamp;
		genesis->vecTx.push_back(std::move(txNew));
		genesis->hashPrevBlock.SetNull();
		genesis->calcAll();
		
		LOG("Genesis block hash is: " + genesis->hash.ToString(), "MINE");
#ifdef _DEBUG
		LOG_DEBUG("Genesis block contents: " + genesis->toString(), "MINE");
#endif

		// checking consistency of our MINE genesis block
		uint8_t genesisRawBuffer[32] = { 0xdc, 0x03, 0x4e, 0x58, 0xf0, 0xaa, 0xcf, 0x47, 0x0d, 0x96, 0x73, 0x0d, 0x90, 0xb2, 0x69, 0x1f, 0x41, 0x87, 0x86, 0x5b, 0x0f, 0x75, 0x93, 0x6a, 0x49, 0xc6, 0xbf, 0x53, 0xb5, 0x94, 0xfe, 0x70 };
		const uint256 validMCGenesisHash(&genesisRawBuffer[0], 32);
		if (genesis->hash != validMCGenesisHash)
		{
#ifdef _DEBUG
			LOG_DEBUG("WARNING!!! Genesis Block Hash doesn't match. Not Exiting", "MINE");
#else
			LOG_ERROR("The Genesis block hash doesn't match the precomputed safe hash!", "MINE");
			return false;
#endif
		}

		m_bGenesis = true;
		return true;
	}
}