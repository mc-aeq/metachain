/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "netPeers.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "../logger.h"
#include "../functions.h"
#include "../MetaChain.h"

netPeers::netPeers() :
	m_bConnected(false),
	m_bValidConnection(false),
	m_bToDestroy(false),
	hSocket(0),
	m_usConnectionTries(0),
	m_timeLastTry(0),
	m_iUsageCounter(0)
{
	pcshSocket = new cCriticalSection();
	m_pcsvRecv = new cCriticalSection();
	pcsvQueue = new cCriticalSection();
	pcsvSend = new cCriticalSection();
}

// this is the emplace_back constructor for incoming connections
netPeers::netPeers(SOCKET *listenSocket, cSemaphore *semaphore)
{
	m_bConnected = false;
	m_bValidConnection = false;
	m_bToDestroy = false;
	m_usConnectionTries = 0;
	m_timeLastTry = 0;
	m_iUsageCounter = 0;
	pcshSocket = new cCriticalSection();
	m_pcsvRecv = new cCriticalSection();
	pcsvQueue = new cCriticalSection();
	pcsvSend = new cCriticalSection();

	struct sockaddr_storage sockaddr;
	socklen_t len = sizeof(sockaddr);

	hSocket = accept(*listenSocket, (struct sockaddr*)&sockaddr, &len);

	// some security checks that the socket is really useable
	if ((hSocket != INVALID_SOCKET) && (!csAddress.SetSockAddr((const struct sockaddr*)&sockaddr)))
		LOG_ERROR("Warning: Unknown socket family", "NET");

	if (hSocket == INVALID_SOCKET)
	{
		int nErr = WSAGetLastError();
		if (nErr != WSAEWOULDBLOCK)
			LOG_ERROR("socket error accept failed - " + NetworkErrorString(nErr), "NET-PEERS");
		markDestroy();
		return;
	}

	if (!IsSelectableSocket(hSocket))
	{
		LOG_ERROR("connection dropped: non-selectable socket - " + csAddress.toString(), "NET-PEERS");
		markDestroy();
		return;
	}

	// According to the internet TCP_NODELAY is not carried into accepted sockets
	// on all platforms.  Set it again here just to be sure.
	SetSocketNoDelay(hSocket);	

	// get the semaphore grant without blocking
	semaphoreGrant = cSemaphoreGrant(*semaphore, true);
	if (!semaphoreGrant)
	{
		LOG("too many open connections - connection dropped", "NET-PEERS");
		markDestroy();
		return;
	}

	LOG("connection accepted - " + csAddress.toString(), "NET-PEERS");

	// everything went smoothly, flag as connected
	m_bConnected = true;
}


netPeers::netPeers(const netPeers& obj)
{
	makeDeepCopy(obj);
}

netPeers& netPeers::operator=(const netPeers& obj)
{
	makeDeepCopy(obj);
	return *this;
}

void netPeers::makeDeepCopy(const netPeers & obj)
{
	m_bConnected = obj.m_bConnected;
	m_bValidConnection = obj.m_bValidConnection;
	m_bToDestroy = obj.m_bToDestroy;
	m_usConnectionTries = obj.m_usConnectionTries;
	m_timeLastTry = obj.m_timeLastTry;
	m_iUsageCounter = obj.m_iUsageCounter;
	pcshSocket = obj.pcshSocket;
	m_pcsvRecv = obj.m_pcsvRecv;
	pcsvQueue = obj.pcsvQueue;
	pcsvSend = obj.pcsvSend;
	hSocket = obj.hSocket;
	csAddress = obj.csAddress;
	m_netMsg = obj.m_netMsg;
	m_iUsageCounter = obj.m_iUsageCounter;
	m_queueMessages = obj.m_queueMessages;
	listSend = obj.listSend;

	((cSemaphoreGrant)(obj.semaphoreGrant)).MoveTo(semaphoreGrant);
}

netPeers::~netPeers()
{
	LOCK(pcshSocket);
	LOCK(m_pcsvRecv);
	LOCK(pcsvQueue);
	LOCK(pcsvSend);

	// we ignore the socket when it's uninitialized
	if (hSocket != 0)
	{
		if (hSocket != INVALID_SOCKET)
		{
			LOG("disconnecting peer - " + toString(), "NET-PEERS");
			CloseSocket(hSocket);
		}
		else
			LOG("removing peer - " + toString(), "NET-PEERS");
	}

	// TODO: delete them safely. when uncommenting other threads may wait on those locks and the mutex will throw an exception of abandoned lock
	// idea: introduce a general lock for a peer before using those locks. use a try_lock to not wait on this lock within the NetworkManager and a lock in the destructor. so the destructor waits until the locks are released and the networkmanager will not wait for the try_lock
	/*delete m_pcsvRecv;
	delete pcsvQueue;
	delete pcshSocket;
	delete pcsvSend;*/
}

bool netPeers::init(std::string strEntry)
{
	// the format is <ip>:<port>\n
	std::vector< std::string > vecSplit;

	// split IP and Port
	boost::split(vecSplit, strEntry, boost::is_any_of(":"));

	// since we split IP and Port, the size of the vector must be 2, otherwise it's a mistake and we cant continue with this entry
	if (vecSplit.size() != 2)
	{
		LOG_ERROR("Can't parse peers entry. Must be wrong format: " + strEntry, "NETP");
		return false;
	}

	// entry 1 is the IP, entry 2 the port, so lets instantiate this
	csAddress = CService(vecSplit[0].c_str(), boost::lexical_cast<unsigned short>(vecSplit[1]));

	return true;
}

bool netPeers::tryConnectOutbound()
{
	// this function will only be called when the connection is not yet established. we will also return true if it's already established
	if (m_bConnected)
		return true;

	// increment the connection try and update the timestamp
	m_usConnectionTries++;
	m_timeLastTry = GetTime();

	// when we're over a certain limit of connection tries (look at define.h for standard value), we don't even try it anymore to not stress the node
	if (tooManyTriesOutbound())
		return false;

	// alright we have the authorization to try to connect. 
	struct sockaddr_storage sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (!csAddress.GetSockAddr((struct sockaddr*)&sockaddr, &len))
	{
		LOG_ERROR("Cannot connect: unsupported networkd - " + csAddress.toString(), "NET-PEERS");
		return false;
	}

	hSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if(hSocket == INVALID_SOCKET)
		return false;

#ifdef SO_NOSIGPIPE
	int set = 1;
	// Different way of disabling SIGPIPE on BSD
	setsockopt(hSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif

	// Disable Nagle's algorithm
	SetSocketNoDelay(hSocket);

	// Set to non-blocking
	if (!SetSocketNonBlocking(hSocket, true))
	{
		CloseSocket(hSocket);
		LOG_ERROR("Setting listening socket to non-blocking failed - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS");
		return false;
	}

	if (connect(hSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
	{
		int nErr = WSAGetLastError();
		// WSAEINVAL is here because some legacy version of winsock uses it
		if (nErr == WSAEINPROGRESS || nErr == WSAEWOULDBLOCK || nErr == WSAEINVAL)
		{
			struct timeval timeout = MillisToTimeval(MetaChain::getInstance().getNetworkManager()->getTimeout());
			fd_set fdset;
			FD_ZERO(&fdset);
			FD_SET(hSocket, &fdset);
			int nRet = select(hSocket + 1, NULL, &fdset, NULL, &timeout);
			if (nRet == 0)
			{
				LOG_ERROR( "connection timed out - " + csAddress.toString(), "NET-PEERS" );
				CloseSocket(hSocket);
				return false;
			}
			if (nRet == SOCKET_ERROR)
			{
				LOG_ERROR("select() failed - " + csAddress.toString() + " - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS" );
				CloseSocket(hSocket);
				return false;
			}
			socklen_t nRetSize = sizeof(nRet);
#ifdef _WINDOWS
			if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, (char*)(&nRet), &nRetSize) == SOCKET_ERROR)
#else
			if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, &nRet, &nRetSize) == SOCKET_ERROR)
#endif
			{
				LOG_ERROR("getsockopt() failed - " + csAddress.toString() + " - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS" );
				CloseSocket(hSocket);
				return false;
			}
			if (nRet != 0)
			{
				LOG_ERROR("connect() failed after select() - " + csAddress.toString() + " - " + NetworkErrorString(nRet), "NET-PEERS" );
				CloseSocket(hSocket);
				return false;
			}
		}
#ifdef _WINDOWS
		else if (WSAGetLastError() != WSAEISCONN)
#else
		else
#endif
		{
			LOG_ERROR("connect() failed to - " + csAddress.toString() + " - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS");
			CloseSocket(hSocket);
			return false;
		}
	}

	// find out if it's an selectable socket (only non windows
	if (!IsSelectableSocket(hSocket))
	{
		LOG_ERROR("Cannot create connection: non-selectable socket created (fd >= FD_SETSIZE ?)", "NET-PEERS");
		CloseSocket(hSocket);
		return false;
	}


	LOG("Outgoing connection success - " + csAddress.toString(), "NET-PEERS");

	// everything is fine, yay!
	m_bConnected = true;
	return true;
}

bool netPeers::ReceiveMsgBytes(const char *pch, unsigned int nBytes, bool& complete)
{
	complete = false;
	int64_t nTimeMicros = GetTimeMicros();
	LOCK(m_pcsvRecv);

	while (nBytes > 0)
	{
		// if we have more than MAX_MSG_QUEUE amount of messages to process, we skip reading. this prevents flodding from malicious nodes
		if (m_queueMessages.size() >= MAX_MSG_QUEUE)
		{
			complete = true; // make sure the msg handler will be called to process those queued up msgs
			return true;
		}

		// read network data
		int handled = m_netMsg.readData(pch, nBytes);

		if (handled < 0)
			return false;

		pch += handled;
		nBytes -= handled;

		if (m_netMsg.complete())
		{
			// update some time stats and set the message to be complete
			m_netMsg.i64tTimeStart = nTimeMicros;
			m_netMsg.i64tTimeDelta = GetTimeMicros() - nTimeMicros;
			complete = true;

			// push the message into our queue
			{
				LOCK(pcsvQueue);
				m_queueMessages.push(m_netMsg);
			}

			// reset the pointer to a new Message
			m_netMsg = netMessage();
		}
	}

	return true;
}

void netPeers::validConnection(bool bValid)
{
	m_bValidConnection = bValid;
	LOG("Connection Valid - " + toString() , "NET-PEERS");
}