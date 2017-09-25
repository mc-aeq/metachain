/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Block.h"
#include "../../tinyformat.h"

namespace MCP03
{
	Block::Block()
		: m_uint16tVersion(CURRENT_BLOCK_VERSION),
		m_nTime(0),
		m_uint32tByte(0)
	{
		m_hashPrevBlock.SetNull();
		m_hashMerkleRoot.SetNull();
	}

	Block::~Block()
	{
		m_vecTx.clear();
	}

	uint256 Block::getHash()
	{
		// todo: calculate sha3 hash

		return uint256();
	}

	std::string Block::toString()
	{
		std::stringstream s;
		s << strprintf("Block (Hash=%s, Version=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, Time=%u, Byte=%08x, TXs=%u)\n",
			getHash().ToString(),
			m_uint16tVersion,
			m_hashPrevBlock.ToString(),
			m_hashMerkleRoot.ToString(),
			m_nTime,
			m_uint32tByte,
			m_vecTx.size());

		for( auto &it : m_vecTx)
			s << "  " << it.toString() << "\n";
		
		return s.str();
	}
}