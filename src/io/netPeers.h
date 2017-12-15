#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __NETPEERS_H__
#define __NETPEERS_H__ 1

#include <queue>
#include <string>
#include <list>

#include "../defines.h"
#include "../cCriticalSection.h"
#include "../cSemaphore.h"
#include "../network/netMessage.h"
#include "../network/CService.h"

/**
this class is used as template type for ipContainer. Especially to store and retrieve all relevant information about peers
*/
class netPeers
{
private:
	bool							m_bConnected;				/// this value shows whether the socket is connected or not.
	bool							m_bValidConnection;			/// true when the initial communication with the version number worked and checked out
	bool							m_bToDestroy;				/// this marks whether this peer has to be destroyed. this will result in a delete of this instance
	int64_t							m_timeLastTry;				/// timestamp of the last connection try	
	unsigned short					m_usConnectionTries;		/// variable that counts connection tries. over a certain limit we throw this peer away as unusable
	int								m_iUsageCounter;			/// counter that shows if this peer is still used. needed for safe destruction
	bool							m_bNodeMode;				/// flag that indicates if the peer is a Full Node (m_bNodeMode) or a Client (!m_bNodeMode)
	unsigned short					m_usListeningPort;			/// the listening port of the peer - only relevant when peer is FN

	// variables used for receiving messages and storing them
	cCriticalSection				*m_pcsvRecv;
	netMessage						m_netMsg;
	std::queue< netMessage >		m_queueMessages;

	// CC function
	void							makeDeepCopy(const netPeers & obj);

public:
									netPeers();
									netPeers(SOCKET *listenSocket, cSemaphore *semaphore);		// constructor used for emplace_back
									~netPeers();
									netPeers(const netPeers& obj);
	netPeers&						operator=(const netPeers& obj);
	bool							operator==(const netPeers& b) const { return this->csAddress == b.csAddress; };

	// initialization functions, getter and setter
	bool							init(std::string strEntry);
	std::string						toString();
	unsigned short					getPort() { return csAddress.GetPort(); };
	void							setNodeMode(bool bNodeMode) { m_bNodeMode = bNodeMode; };
	bool							isFN() { return m_bNodeMode; };
	void							setListeningPort(unsigned short usListeningPort) { m_usListeningPort = usListeningPort; };
	unsigned short					getListeningPort() { return m_usListeningPort; };

	// connection functions and variables
	SOCKET							hSocket;
	cCriticalSection				*pcshSocket;
	CService						csAddress;
	cSemaphoreGrant					semaphoreGrant;
	void							validConnection(bool bValid);
	bool							validConnection() { return m_bValidConnection; };
	bool							tryConnectOutbound();
	bool							isConnected() { return m_bConnected; };
	bool							tooManyTriesOutbound() { return (m_usConnectionTries >= NET_DEFAULT_CONNECT_TRIES ? true : false); };
	int64_t							getTimeLastTry() { return m_timeLastTry; };

	// data functions and variables
	bool							ReceiveMsgBytes(const char *pch, unsigned int nBytes, bool& complete);	
	cCriticalSection				*pcsvQueue;
	bool							readStop() { return (m_queueMessages.size() >= MAX_MSG_QUEUE); };
	bool							hasMessage() { return !m_queueMessages.empty(); };
	netMessage						getMessage() { return m_queueMessages.front(); };
	void							popMessage() { m_queueMessages.pop(); };

	// list of sending cmds by this peer
	cCriticalSection				*pcsvSend;
	std::list< netMessage >			listSend;

	// flagging functions
	void							markDestroy() { m_bToDestroy = true; };
	bool							toDestroy() { return m_bToDestroy; };
	void							mark() { m_iUsageCounter++; };					/// mark this peer as currently used by a process.
	void							unmark() { m_iUsageCounter--; };				/// remove the marking or atleast decrement the number of processes using this peer
	bool							inUse() { return (m_iUsageCounter != 0); };		/// check whether a process is using this peer right now
};

#endif