#include "../../stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

netPeers::netPeers():
	m_bConnected( false ),
	m_hSocket( INVALID_SOCKET ),
	m_usConnectionTries(0),
	m_timeLastTry(0)
{
}

netPeers::~netPeers()
{
	semGrantOutbound.Release();

	CloseSocket(m_hSocket);
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
	m_CService = CService(vecSplit[0].c_str(), boost::lexical_cast<unsigned short>(vecSplit[1]));

	return true;
}

string netPeers::toString() const
{
	return m_CService.toString();
}

bool netPeers::tryConnect()
{
	// this function will only be called when the connection is not yet established. we will also return true if it's already established
	if (m_bConnected)
		return true;

	// increment the connection try and update the timestamp
	m_usConnectionTries++;
	m_timeLastTry = GetTime();

	// when we're over a certain limit of connection tries (look at define.h for standard value), we don't even try it anymore to not stress the node
	if (tooManyTries())
		return false;

	// alright we have the authorization to try to connect. 
	struct sockaddr_storage sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (!m_CService.GetSockAddr((struct sockaddr*)&sockaddr, &len))
	{
		LOG_ERROR("Cannot connect: unsupported networkd - " + m_CService.toString(), "NET-PEERS");
		return false;
	}

	m_hSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if(m_hSocket == INVALID_SOCKET)
		return false;

#ifdef SO_NOSIGPIPE
	int set = 1;
	// Different way of disabling SIGPIPE on BSD
	setsockopt(hSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif

	// Disable Nagle's algorithm
	SetSocketNoDelay();

	// Set to non-blocking
	if (!SetSocketNonBlocking(m_hSocket, true))
	{
		CloseSocket(m_hSocket);
		LOG_ERROR("Setting listening socket to non-blocking failed - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS");
		return false;
	}

	if (connect(m_hSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
	{
		int nErr = WSAGetLastError();
		// WSAEINVAL is here because some legacy version of winsock uses it
		if (nErr == WSAEINPROGRESS || nErr == WSAEWOULDBLOCK || nErr == WSAEINVAL)
		{
			struct timeval timeout = MillisToTimeval(MetaChain::getInstance().getNetworkManager()->getTimeout());
			fd_set fdset;
			FD_ZERO(&fdset);
			FD_SET(m_hSocket, &fdset);
			int nRet = select(m_hSocket + 1, NULL, &fdset, NULL, &timeout);
			if (nRet == 0)
			{
				LOG_ERROR( "connection timed out - " + m_CService.toString(), "NET-PEERS" );
				CloseSocket(m_hSocket);
				return false;
			}
			if (nRet == SOCKET_ERROR)
			{
				LOG_ERROR("select() failed - " + m_CService.toString() + " - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS" );
				CloseSocket(m_hSocket);
				return false;
			}
			socklen_t nRetSize = sizeof(nRet);
#ifdef _WINDOWS
			if (getsockopt(m_hSocket, SOL_SOCKET, SO_ERROR, (char*)(&nRet), &nRetSize) == SOCKET_ERROR)
#else
			if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, &nRet, &nRetSize) == SOCKET_ERROR)
#endif
			{
				LOG_ERROR("getsockopt() failed - " + m_CService.toString() + " - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS" );
				CloseSocket(m_hSocket);
				return false;
			}
			if (nRet != 0)
			{
				LOG_ERROR("connect() failed after select() - " + m_CService.toString() + " - " + NetworkErrorString(nRet), "NET-PEERS" );
				CloseSocket(m_hSocket);
				return false;
			}
		}
#ifdef _WINDOWS
		else if (WSAGetLastError() != WSAEISCONN)
#else
		else
#endif
		{
			LOG_ERROR("connect() failed to - " + m_CService.toString() + " - " + NetworkErrorString(WSAGetLastError()), "NET-PEERS");
			CloseSocket(m_hSocket);
			return false;
		}
	}

	// find out if it's an selectable socket (only non windows
	if (!IsSelectableSocket(m_hSocket))
	{
		LOG_ERROR("Cannot create connection: non-selectable socket created (fd >= FD_SETSIZE ?)", "NET-PEERS");
		CloseSocket(m_hSocket);
		return false;
	}

#ifdef _DEBUG
	LOG_DEBUG("Socket successfully connected - " + m_CService.toString(), "NET-PEERS");
#endif

	// everything is fine, yay!
	m_bConnected = true;
	return true;
}

bool netPeers::SetSocketNoDelay()
{
	int set = 1;
	int rc = setsockopt(m_hSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&set, sizeof(int));
	return rc == 0;
}