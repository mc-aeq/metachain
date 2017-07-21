#include "../../stdafx.h"

netMessage::netMessage():
	m_bComplete(false),
	m_bHeaderComplete(false),
	m_uiPosition(0),
	i64tTimeStart(0),
	i64tTimeDelta(0),
	m_pBuffer(NULL)
{
}

// this constructor will most of the time be called by emplace_back. When called by emplace_back bDirectSend must be true to assemble the data correctly
// when it's not constructed through emplace_back, bDirectSend shall be false and the data needs to be assembled manually
netMessage::netMessage(netMessage::SUBJECT subj, void *ptrData, uint32_t uiDataSize, bool bDirectSend) :
	m_bComplete(false),
	m_bHeaderComplete(false),
	m_uiPosition(0),
	i64tTimeStart(0),
	i64tTimeDelta(0)
{
	// assemble the header
	m_sHeader.ui16tSubject = subj;
	m_sHeader.ui32tPayloadSize = uiDataSize;
	memset( &m_sHeader.ui8tChecksum, 0, sizeof(uint8_t)*CHECKSUM_SIZE);

	// allocate the buffer
	m_pBuffer = new char[sizeof(sHeader) + m_sHeader.ui32tPayloadSize];
	memset(m_pBuffer, '\0', sizeof(sHeader) + m_sHeader.ui32tPayloadSize);

	// copy the data
	if( uiDataSize > 0 )
		memcpy((m_pBuffer + sizeof(sHeader)), ptrData, uiDataSize);

	// set the complete flag for further processing
	m_bComplete = true;

	// get the hash and store it in the header
	if (uiDataSize > 0)
	{
		m_Hasher256.Write((const unsigned char*)ptrData, uiDataSize);
		GetMessageHash();
		memcpy(m_sHeader.ui8tChecksum, m_ui256DataHash.begin(), CHECKSUM_SIZE);
	}

	// copy the header information
	memcpy(m_pBuffer, &m_sHeader, sizeof(sHeader));

#ifdef _DEBUG
	uint256 tmpCmp;
	memcpy(tmpCmp.begin(), m_sHeader.ui8tChecksum, CHECKSUM_SIZE);
	LOG_DEBUG("Writing Header - Header Subj: " + to_string(m_sHeader.ui16tSubject) + " - Header PayloadSize: " + to_string(m_sHeader.ui32tPayloadSize) + " - Header Hash: " + tmpCmp.ToString(), "NET-MSG");
#endif
}

netMessage::netMessage(const netMessage& obj)
{
	makeDeepCopy(obj);
}

netMessage::~netMessage()
{	
	if (m_pBuffer)
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
}

netMessage& netMessage::operator=(const netMessage& obj)
{
	makeDeepCopy(obj);
	return *this;
}

void netMessage::makeDeepCopy(const netMessage & obj)
{
	m_bComplete = obj.m_bComplete;
	m_bHeaderComplete = obj.m_bHeaderComplete;
	m_sHeader = obj.m_sHeader;
	m_vRecv = obj.m_vRecv;
	m_Hasher256 = obj.m_Hasher256;
	m_ui256DataHash = obj.m_ui256DataHash;
	m_uiPosition = obj.m_uiPosition;
	i64tTimeStart = obj.i64tTimeStart;
	i64tTimeDelta = obj.i64tTimeDelta;

	// copy the buffer content
	if (obj.m_pBuffer)
	{
		m_pBuffer = new char[sizeof(sHeader) + m_sHeader.ui32tPayloadSize];
		memcpy(m_pBuffer, obj.m_pBuffer, sizeof(sHeader) + m_sHeader.ui32tPayloadSize);
	}
	else
		m_pBuffer = NULL;
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
		if (m_sHeader.ui32tPayloadSize > MAX_PAYLOAD_SIZE)
		{
			LOG_ERROR("DoS data package filtered, disconnecting node", "NET-MSG");
			return -1;
		}

		// allocating the buffer for the data
		m_pBuffer = new char[sizeof(sHeader) + m_sHeader.ui32tPayloadSize];

		// copying the header into the buffer
		memcpy(m_pBuffer, &m_sHeader, sizeof(sHeader));

#ifdef _DEBUG
		uint256 tmpCmp;
		memcpy(tmpCmp.begin(), m_sHeader.ui8tChecksum, CHECKSUM_SIZE );
		LOG_DEBUG("Header Completed - Subject: " + to_string(m_sHeader.ui16tSubject) + ", Payload Size: " + to_string(m_sHeader.ui32tPayloadSize) + ", Header Hash: " + tmpCmp.ToString(), "NET-MSG");
#endif

		// when we don't have any payload after the header, the message is complete
		if (m_sHeader.ui32tPayloadSize == 0)
			m_bComplete = true;

		// everything smooth, return the bytes read
		return uiBufSize;
	}
	else
	{
		// calculate the remaining bytes of the message as well as the number of bytes to read
		unsigned int uiRemaining = m_sHeader.ui32tPayloadSize - m_uiPosition;
		unsigned int uiBufSize = (std::min)(uiRemaining, nBytes);
		
		memcpy((m_pBuffer + sizeof(sHeader) + m_uiPosition), pch, uiBufSize);
		m_uiPosition += uiBufSize;
		m_Hasher256.Write((const unsigned char*)pch, uiBufSize);

#ifdef _DEBUG
		LOG_DEBUG("Reading Data - Data Position: " + to_string(m_uiPosition) + ", Bytes read: " + to_string(uiBufSize), "NET-MSG");
#endif

		// check wether we completed the data part
		if (m_sHeader.ui32tPayloadSize == m_uiPosition)
		{
			m_bComplete = true;
			GetMessageHash();
#ifdef _DEBUG
			uint256 tmpHead, tmpCalc;
			memcpy(tmpHead.begin(), m_sHeader.ui8tChecksum, CHECKSUM_SIZE );
			memcpy(tmpCalc.begin(), m_ui256DataHash.begin(), CHECKSUM_SIZE);
			LOG_DEBUG("Checking Hashsums - Header hash: " + tmpHead.ToString() + ", calculated data hash: " + tmpCalc.ToString(), "NET-MSG");
#endif

			if (memcmp(m_ui256DataHash.begin(), m_sHeader.ui8tChecksum, CHECKSUM_SIZE) != 0)
			{
				LOG_ERROR("Received package with invalid hash, disconnecting node", "NET-MSG");
				return -1;
			}
#ifdef _DEBUG
			else
				LOG_DEBUG("Header checksum matches", "NET-MSG");
#endif
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