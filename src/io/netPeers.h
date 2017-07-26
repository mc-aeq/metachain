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
#include "../external/cCriticalSection.h"
#include "../external/cSemaphore.h"
#include "../network/netMessage.h"
#include "../network/CService.h"

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
	netMessage						m_netMsg;

	int								m_iUsageCounter;
	std::queue< netMessage >		m_queueMessages;

	void							makeDeepCopy(const netPeers & obj);

public:
									netPeers();
									netPeers(SOCKET *listenSocket, cSemaphore *semaphore);		// constructor used for emplace_back
									~netPeers();
									netPeers(const netPeers& obj);
	netPeers&						operator=(const netPeers& obj);

	bool							init(std::string strEntry);
	std::string						toString() const { return csAddress.toString(); };
	unsigned short					getPort() { return csAddress.GetPort(); };

	void							validConnection(bool bValid);
	bool							validConnection() { return m_bValidConnection; };

	bool							tryConnectOutbound();
	bool							isConnected() { return m_bConnected; };
	bool							tooManyTriesOutbound() { return (m_usConnectionTries >= NET_DEFAULT_CONNECT_TRIES ? true : false); };
	int64_t							getTimeLastTry() { return m_timeLastTry; };

	bool							operator==(const netPeers& b) const { return this->csAddress == b.csAddress; };

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
	std::list< netMessage >			listSend;

	void							mark() { m_iUsageCounter++; };					// mark this peer as currently used by a process.
	void							unmark() { m_iUsageCounter--; };				// remove the marking or atleast decrement the number of processes using this peer
	bool							inUse() { return (m_iUsageCounter != 0); };		// check whether a process is using this peer right now
};

#endif