#pragma once

/*
this class is used as template type for ipContainer. Especially to store and retrieve all relevant information about peers
*/
class netPeers
{
private:
	CService						m_CService;
	SOCKET							m_hSocket;
	bool							m_bConnected;
	int64_t							m_timeLastTry;

	// variable that counts connection tries. over a certain limit we throw this peer away as unusable
	unsigned short					m_usConnectionTries;

	bool							SetSocketNoDelay();

public:
									netPeers();
									~netPeers();
	bool							init(string strEntry);
	string							toString() const;

	bool							tryConnect();
	bool							isConnected() { return m_bConnected; };
	bool							tooManyTries() { return (m_usConnectionTries >= NET_DEFAULT_CONNECT_TRIES ? true : false); };
	int64_t							getTimeLastTry() { return m_timeLastTry; };

	CSemaphoreGrant					semGrantOutbound;
};