#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __NETMESSAGE_H__
#define __NETMESSAGE_H__ 1

#include <stdint.h>

#include "../uint256.h"
#include "../defines.h"
#include "../cDataStream.h"
#include "../crypto/cHash256.h"

/**
the struct of the header
only needed in this class, thus no single file
*/
struct sHeader {
	uint32_t		ui32tPayloadSize;
	uint32_t		ui32tChainIdentifier;
	uint16_t		ui16tSubject;
	uint8_t			ui8tChecksum[CHECKSUM_SIZE];
};

/// our communication message which is delivered via network
class netMessage
{
	private:
		bool							m_bComplete;
		bool							m_bHeaderComplete;
		sHeader							m_sHeader;
		cDataStream						m_vRecv;
		cHash256						m_Hasher256;
		uint256							m_ui256DataHash;
		unsigned int					m_uiPosition;

		char							*m_pBuffer;

		// CC function
		void							makeDeepCopy(const netMessage& obj);

	public:
		// these are the communication definers we use as ui16tSubject
/**
we use a hex coding scheme to categorize our actions for easier splitting into groups.\n
the following binary scheme will be used globally and updated every single time we add new categories and subjects\n
the first bit in the category is reserved for special communications within the SC modules. To use special communications set the first bit to 1\n

<table>
<caption id="ui16tSubject">Network communication Bits</caption>
<tr>
<th colspan="2">8bit CATEGORY</th>
<th colspan="2">8bit SUBJECT</th>
<th>Hex Value</th>
<th>Name</th>
</tr>
<tr>
<th>0 0 0 0</th>
<th>0 0 0 1</th>
<th>&nbsp;</th>
<th>&nbsp;</th>
<th colspan="2">GENERAL NET COMMUNICATION</th>
</tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 0 0 1</td><td>0x0101</td><td>VERSION NUMBER</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 0 1 0</td><td>0x0102</td><td>NEWER VERSION AVAILABLE</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 0 1 1</td><td>0x0103</td><td>NODE MODE</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 1 0 0</td><td>0x0104</td><td>LISTENING PORT GET (for FN)</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 1 0 1</td><td>0x0105</td><td>LISTENING PORT SEND (for FN)</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>1 0 0 0</td><td>0x0108</td><td>PING</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>1 0 0 0</td><td>0x0109</td><td>PONG</td></tr>
<tr>
<th>0 0 0 0</th>
<th>0 0 1 0</th>
<th>&nbsp;</th>
<th>&nbsp;</th>
<th colspan="2">PEER AND BAN LIST COMMUNICATION</th>
</tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 0 0 1</td><td>0x0201</td><td>GET PEER LIST</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 0 1 0</td><td>0x0202</td><td>SEND PEER LIST</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 0 1 1</td><td>0x0203</td><td>GET BAN LIST</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 1 0 0</td><td>0x0204</td><td>SEND BAN LIST</td></tr>
<tr>
<th>0 0 0 0</th>
<th>0 0 1 1</th>
<th>&nbsp;</th>
<th>&nbsp;</th>
<th colspan="2">TESTNET COMMUNICATION</th>
</tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>0 0 0 0</td><td>0 0 0 1</td><td>0x0301</td><td>TESTNET FLAG IDENTIFIER</td></tr>
</table>
*/
		enum SUBJECT {
			NET_VERSION						= 0x0101,
			NET_VERSION_NEWER				= 0x0102,
			NET_NODEMODE					= 0x0103,
			NET_NODE_LISTENING_PORT_GET		= 0x0104,
			NET_NODE_LISTENING_PORT_SEND	= 0x0105,
			NET_NODE_PING					= 0x0108,
			NET_NODE_PONG					= 0x0109,

			NET_PEER_LIST_GET				= 0x0201,
			NET_PEER_LIST_SEND				= 0x0202,
			NET_BAN_LIST_GET				= 0x0203,
			NET_BAN_LIST_SEND				= 0x0204,

			NET_TESTNET						= 0x0301,
		};

							netMessage();
							netMessage(netMessage::SUBJECT subj, void *ptrData, uint32_t uiDataSize, bool bDirectSend);
							netMessage(const netMessage& obj);
							~netMessage();
		netMessage&			operator=(const netMessage& obj);

		// time information about this package
		int64_t				i64tTimeStart;
		int64_t				i64tTimeDelta;

		// data functions
		bool				complete() { return m_bComplete; };
		int					readData(const char *pch, unsigned int nBytes);
		uint256&			GetMessageHash();
		unsigned int		getPackageSize() { return sizeof(sHeader) + m_sHeader.ui32tPayloadSize; };
		void*				getPackage() { return m_pBuffer; };
		char*				getData() { return (m_pBuffer+sizeof(sHeader)); };

		// header functions
		sHeader				getHeader() { return m_sHeader; };
};

#endif