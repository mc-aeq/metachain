#include "../../stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

netPeers::netPeers() :
	m_bConnected(false),
	m_bToDestroy(false),
	hSocket( INVALID_SOCKET ),
	m_usConnectionTries(0),
	m_timeLastTry(0)
{
	pcshSocket = new cCriticalSection();
	m_pcsvRecv = new cCriticalSection();
}

netPeers::netPeers(SOCKET *listenSocket, cSemaphore *semaphore)
{
	m_bConnected = false;
	m_bToDestroy = false;
	m_usConnectionTries = 0;
	m_timeLastTry = 0;
	pcshSocket = new cCriticalSection();
	m_pcsvRecv = new cCriticalSection();

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
}

netPeers::~netPeers()
{
	semaphoreGrant.Release();

	LOCK(pcshSocket);
	if (hSocket != INVALID_SOCKET)
	{
		LOG("disconnecting peer - " + toString(), "NET-PEERS");
		CloseSocket(hSocket);
	}
	else
		LOG("removing peer - " + toString(), "NET-PEERS");

	delete m_pcsvRecv;
	delete pcshSocket;
}

bool netPeers::init(string strEntry)
{
	// the format is <ip>:<port>\n
	vector< string > vecSplit;

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

string netPeers::toString() const
{
	return csAddress.toString();
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

#ifdef _DEBUG
	LOG_DEBUG("Socket successfully connected - " + csAddress.toString(), "NET-PEERS");
#endif

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
		// get current incomplete message, or create a new one
		if( m_vecMsgBuffer.empty() || m_vecMsgBuffer.back().complete() )
			m_vecMsgBuffer.push_back( netMessage() );

		netMessage& msg = m_vecMsgBuffer.back();

		// read network data
		int handled = msg.readData(pch, nBytes);

		if (handled < 0)
			return false;

		pch += handled;
		nBytes -= handled;

		if (msg.complete())
		{
			// update some time stats and set the message to be complete
			msg.i64tTimeStart = nTimeMicros;
			msg.i64tTimeDelta = GetTimeMicros() - nTimeMicros;
			complete = true;
		}
	}

	return true;
}