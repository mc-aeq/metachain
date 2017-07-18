#include "../../stdafx.h"

netMessage::netMessage():
	m_bComplete(false),
	m_bHeaderComplete(false),
	m_uiPosition(0),
	i64tTimeStart(0),
	i64tTimeDelta(0)
{

}

int netMessage::readData(const char *pch, unsigned int nBytes)
{
	if (!m_bHeaderComplete)
	{
		// read the header first
		unsigned int uiRemaining = sizeof(sHeader) - m_uiPosition;
		unsigned int uiBufSize = (std::min)(uiRemaining, nBytes);

		// buffer allocation, reading and freeing
		char *pHeaderPos = (char *)(&m_sHeader) + m_uiPosition;
		memcpy(pHeaderPos, pch, uiBufSize);
		m_uiPosition += uiBufSize;

#ifdef _DEBUG
		LOG_DEBUG("Reading Header - Header Position: " + to_string(m_uiPosition) + ", Bytes read: " + to_string(uiBufSize), "NET-MSG");
#endif

		// if the header is not fully read, we return the number of bytes read
		if (m_uiPosition < sizeof(sHeader))
			return uiBufSize;			

		// when we reach this point the header is fully read, we sanitize everything for data reading
		m_bHeaderComplete = true;
		m_uiPosition = 0;

		// security check for payload DoS
		if (m_sHeader.uiPayloadSize > MAX_PAYLOAD_SIZE)
		{
			LOG_ERROR("DoS data package filtered, disconnecting node", "NET-MSG");
			return -1;
		}

#ifdef _DEBUG
		uint256 tmpCmp;
		memcpy(tmpCmp.begin(), m_sHeader.ui8tChecksum, CHECKSUM_SIZE );
		LOG_DEBUG("Header Completed - Payload Size: " + to_string(m_sHeader.uiPayloadSize) + ", Header Hash: " + tmpCmp.ToString(), "NET-MSG");
#endif

		// everything smooth, return the bytes read
		return uiBufSize;
	}
	else
	{
		// calculate the remaining bytes of the message as well as the number of bytes to read
		unsigned int uiRemaining = m_sHeader.uiPayloadSize - m_uiPosition;
		unsigned int uiBufSize = (std::min)(uiRemaining, nBytes);

		// Allocate up to 256 KiB ahead, but never more than the total message size.
		if (m_vRecv.size() < m_uiPosition + uiBufSize)
			m_vRecv.resize((std::min)(m_sHeader.uiPayloadSize, m_uiPosition + uiBufSize + 256 * 1024));

		memcpy(&m_vRecv[m_uiPosition], pch, uiBufSize);
		m_uiPosition += uiBufSize;
		m_Hasher256.Write((const unsigned char*)pch, uiBufSize);

#ifdef _DEBUG
		LOG_DEBUG("Reading Data - Data Position: " + to_string(m_uiPosition) + ", Bytes read: " + to_string(uiBufSize), "NET-MSG");
#endif

		// check wether we completed the data part
		if (m_sHeader.uiPayloadSize == m_uiPosition)
		{
			m_bComplete = true;
			GetMessageHash();
#ifdef _DEBUG
			uint256 tmpCmp;
			memcpy(tmpCmp.begin(), m_sHeader.ui8tChecksum, CHECKSUM_SIZE );
			LOG_DEBUG("Checking Hashsums - Header hash: " + tmpCmp.ToString() + ", calculated data hash: " + m_ui256DataHash.ToString(), "NET-MSG");
#endif

			if (memcmp(m_ui256DataHash.begin(), m_sHeader.ui8tChecksum, CHECKSUM_SIZE) != 0)
			{
				LOG_ERROR("Received package with invalid hash, disconnecting node", "NET-MSG");
				return -1;
			}
		}

		return uiBufSize;
	}
}

uint256& netMessage::GetMessageHash()
{
	assert(complete());
	if (m_ui256DataHash.IsNull())
		m_Hasher256.Finalize(m_ui256DataHash.begin());
	return m_ui256DataHash;
}