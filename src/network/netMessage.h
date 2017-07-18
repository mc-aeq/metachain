#pragma once


// the struct of the header
// only needed in this class, thus no single file
struct sHeader {
	unsigned int	uiPayloadSize;
	uint8_t			ui8tChecksum[CHECKSUM_SIZE];
};

// our communication message which is delivered via network
class netMessage
{
	private:
		bool				m_bComplete;
		bool				m_bHeaderComplete;
		unsigned int		m_uiPosition;
		sHeader				m_sHeader;
		cDataStream			m_vRecv;
		cHash256			m_Hasher256;
		uint256				m_ui256DataHash;

	public:
							netMessage();
		bool				complete() { return m_bComplete; };
		int					readData( const char *pch, unsigned int nBytes );
		uint256&			GetMessageHash();
		int64_t				i64tTimeStart;
		int64_t				i64tTimeDelta;
};