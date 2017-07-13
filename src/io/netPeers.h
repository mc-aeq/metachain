#pragma once

/*
this class is used as template type for ipContainer. Especially to store and retrieve all relevant information about peers
*/
class netPeers
{
private:
	bool							m_bConnected;		// this value shows whether the socket is connected or not.
	bool							m_bToDestroy;		// this marks whether this peer has to be destroyed. this will result in a delete of this instance
	int64_t							m_timeLastTry;

	// variable that counts connection tries. over a certain limit we throw this peer away as unusable
	unsigned short					m_usConnectionTries;

	cCriticalSection				*m_pcsvRecv;
	cSemaphoreGrant					m_semGrant;

	/*atomic<int64_t>					nLastSend;
	atomic<int64_t>					nLastRecv;*/
	uint64_t						nRecvBytes;
	uint64_t						nSendBytes;

public:
									netPeers();
									netPeers(SOCKET *listenSocket, cSemaphore *semaphore);		// constructor used for emplace_back
									~netPeers();

	bool							init(string strEntry);
	string							toString() const;

	bool							tryConnectOutbound();
	bool							isConnected() { return m_bConnected; };
	bool							tooManyTriesOutbound() { return (m_usConnectionTries >= NET_DEFAULT_CONNECT_TRIES ? true : false); };
	int64_t							getTimeLastTry() { return m_timeLastTry; };

	bool							operator==(const netPeers& b) const { return this->toString() == b.toString(); };

	SOCKET							hSocket;
	cCriticalSection				*pcshSocket;
	CService						csAddress;
	cSemaphoreGrant					semaphoreGrant;

	bool							ReceiveMsgBytes(const char *pch, unsigned int nBytes, bool& complete);
	void							markDestroy() { m_bToDestroy = true; };
	bool							toDestroy() { return m_bToDestroy; };
};