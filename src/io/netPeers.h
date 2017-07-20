#pragma once

/*
this class is used as template type for ipContainer. Especially to store and retrieve all relevant information about peers
*/
class netPeers
{
private:
	bool							m_bConnected;		// this value shows whether the socket is connected or not.
	bool							m_bValidConnection;	// true when the initial communication with the version number worked and checked out
	bool							m_bToDestroy;		// this marks whether this peer has to be destroyed. this will result in a delete of this instance
	int64_t							m_timeLastTry;

	// variable that counts connection tries. over a certain limit we throw this peer away as unusable
	unsigned short					m_usConnectionTries;

	cCriticalSection				*m_pcsvRecv;
	cSemaphoreGrant					m_semGrant;

	netMessage						m_netMsg;

	int								m_iUsageCounter;
	queue< netMessage >				m_queueMessages;

public:
									netPeers();
									netPeers(SOCKET *listenSocket, cSemaphore *semaphore);		// constructor used for emplace_back
									~netPeers();

	bool							init(string strEntry);
	string							toString() const;

	void							validConnection(bool bValid);
	bool							validConnection() { return m_bValidConnection; };

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
	
	cCriticalSection				*pcsvQueue;
	bool							readStop() { return (m_queueMessages.size() >= MAX_MSG_QUEUE); };
	bool							hasMessage() { return !m_queueMessages.empty(); };
	netMessage						getMessage() { return m_queueMessages.front(); };
	void							popMessage() { m_queueMessages.pop(); };

	cCriticalSection				*pcsvSend;
	list< netMessage >				listSend;

	void							mark() { m_iUsageCounter++; };					// mark this peer as currently used by a process.
	void							unmark() { m_iUsageCounter--; };				// remove the marking or atleast decrement the number of processes using this peer
	bool							inUse() { return (m_iUsageCounter != 0); };		// check whether a process is using this peer right now
};