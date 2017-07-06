#include "../../stdafx.h"

NetworkManager::NetworkManager(MetaChain *mc) :
	m_pMC(mc)
{
#ifdef _DEBUG
	LOG_DEBUG("instantiate NetworkManager", "NET");
#endif
}

bool NetworkManager::initialize(CSimpleIniA* iniFile)
{
#ifdef _DEBUG
	LOG_DEBUG("initializing NetworkManager", "NET");
#endif

#ifdef _WINDOWS
	// Initialize Windows Sockets
	WSADATA wsadata;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (ret != NO_ERROR || LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2)
		return false;
#endif

	// creating a CService class for the listener and storing it for further purposes
	try
	{
		m_pServiceLocal = new CService(iniFile->GetValue("network", "listening_ip", NET_STANDARD_CATCHALL_LISTENING), (unsigned short)iniFile->GetLongValue("network", "listening_port", NET_STANDARD_PORT_LISTENING));
	}
	catch (exception e)
	{
		LOG_ERROR("That wasn't good. An error occured creating the local service listener. Please check your configuration file.", "NET");
		return false;
	}

	// now we start the listening socket
	if (!this->startListeningSocket())
	{
		LOG_ERROR("Not able to bind listening Socket", "NET");
		return false;
	}

	// everything went smoothly
	return true;
}

bool NetworkManager::startListeningSocket()
{
	int nOne = 1;

	// Create socket for listening for incoming connections
	struct sockaddr_storage sockaddr;
	socklen_t len = sizeof(sockaddr);
	if(!m_pServiceLocal->GetSockAddr((struct sockaddr*)&sockaddr, &len))
	{
		LOG_ERROR("Bind address family not supported - " + m_pServiceLocal->toString(), "NET");
		return false;
	}

	SOCKET hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if(hListenSocket == INVALID_SOCKET)
	{
		LOG_ERROR("Couldn't open socket for incoming connections - Socket error: " + NetworkErrorString(WSAGetLastError()), "NET");
		return false;
	}

#ifndef _WINDOWS
#ifdef SO_NOSIGPIPE
	// Different way of disabling SIGPIPE on BSD
	setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
#endif
	// Allow binding if the port is still in TIME_WAIT state after
	// the program was closed and restarted.
	setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
	// Disable Nagle's algorithm
	setsockopt(hListenSocket, IPPROTO_TCP, TCP_NODELAY, (void*)&nOne, sizeof(int));
#else
	setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOne, sizeof(int));
	setsockopt(hListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&nOne, sizeof(int));
#endif

	// Set to non-blocking, incoming connections will also inherit this
	if(!SetSocketNonBlocking(hListenSocket, true))
	{
		LOG_ERROR("BindListenPort: Setting listening socket to non-blocking failed - " + NetworkErrorString(WSAGetLastError()), "NET");
		return false;
	}

	// some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
	// and enable it by default or not. Try to enable it, if possible.
	if(m_pServiceLocal->IsIPv6())
	{
#ifdef IPV6_V6ONLY
#ifdef _WINDOWS
		setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int));
#else
		setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int));
#endif
#endif
#ifdef _WINDOWS
		int nProtLevel = PROTECTION_LEVEL_UNRESTRICTED;
		setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (const char*)&nProtLevel, sizeof(int));
#endif
	}

	if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
	{
		int nErr = WSAGetLastError();
		if (nErr == WSAEADDRINUSE)
			LOG_ERROR( strprintf("Unable to bind to %s on this computer. A process is already running.", m_pServiceLocal->toString() ), "NET");
		else
			LOG_ERROR( strprintf("Unable to bind to %s on this computer (bind returned error %s)", m_pServiceLocal->toString(), NetworkErrorString(nErr)), "NET");
			
		CloseSocket(hListenSocket);
		return false;
	}
	LOG( "Bound to " + m_pServiceLocal->toString(), "NET" );

	// Listen for incoming connections
	if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		LOG_ERROR( strprintf("Listening for incoming connections failed (listen returned error %s)", NetworkErrorString(WSAGetLastError())), "NET");
		CloseSocket(hListenSocket);
		return false;
	}

	/*vhListenSocket.push_back(ListenSocket(hListenSocket, fWhitelisted));

	if (addrBind.IsRoutable() && fDiscover && !fWhitelisted)
		AddLocal(addrBind, LOCAL_BIND);
		*/
	return true;
}

#ifdef WIN32
string NetworkManager::NetworkErrorString(int err)
{
	char buf[256];
	buf[0] = 0;
	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), NULL))
	{
		return strprintf("%s (%d)", buf, err);
	}
	else
	{
		return strprintf("Unknown error (%d)", err);
	}
}
#else
string NetworkManager::NetworkErrorString(int err)
{
	char buf[256];
	buf[0] = 0;
	/* Too bad there are two incompatible implementations of the
	* thread-safe strerror. */
	const char *s;
#ifdef STRERROR_R_CHAR_P /* GNU variant can return a pointer outside the passed buffer */
	s = strerror_r(err, buf, sizeof(buf));
#else /* POSIX variant always returns message in buffer */
	s = buf;
	if (strerror_r(err, buf, sizeof(buf)))
		buf[0] = 0;
#endif
	return strprintf("%s (%d)", s, err);
}
#endif

bool NetworkManager::CloseSocket(SOCKET& hSocket)
{
	if (hSocket == INVALID_SOCKET)
		return false;
#ifdef WIN32
	int ret = closesocket(hSocket);
#else
	int ret = close(hSocket);
#endif
	hSocket = INVALID_SOCKET;
	return ret != SOCKET_ERROR;
}

bool NetworkManager::SetSocketNonBlocking( SOCKET& hSocket, bool fNonBlocking )
{
	if(fNonBlocking)
	{
#ifdef _WINDOWS
		u_long nOne = 1;
		if(ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
		{
#else
		int fFlags = fcntl(hSocket, F_GETFL, 0);
		if(fcntl(hSocket, F_SETFL, fFlags | O_NONBLOCK) == SOCKET_ERROR)
		{
#endif
			CloseSocket(hSocket);
			return false;
		}
	}
	else
	{
#ifdef _WINDOWS
		u_long nZero = 0;
		if(ioctlsocket(hSocket, FIONBIO, &nZero) == SOCKET_ERROR)
		{
#else
		int fFlags = fcntl(hSocket, F_GETFL, 0);
		if(fcntl(hSocket, F_SETFL, fFlags & ~O_NONBLOCK) == SOCKET_ERROR)
		{
#endif
			CloseSocket(hSocket);
			return false;
		}
	}

	return true;
}



/*


bool CConnman::Start(CScheduler& scheduler, Options connOptions)
{
	nTotalBytesRecv = 0;
	nTotalBytesSent = 0;
	nMaxOutboundTotalBytesSentInCycle = 0;
	nMaxOutboundCycleStartTime = 0;

	nRelevantServices = connOptions.nRelevantServices;
	nLocalServices = connOptions.nLocalServices;
	nMaxConnections = connOptions.nMaxConnections;
	nMaxOutbound = std::min((connOptions.nMaxOutbound), nMaxConnections);
	nMaxAddnode = connOptions.nMaxAddnode;
	nMaxFeeler = connOptions.nMaxFeeler;

	nSendBufferMaxSize = connOptions.nSendBufferMaxSize;
	nReceiveFloodSize = connOptions.nReceiveFloodSize;

	nMaxOutboundLimit = connOptions.nMaxOutboundLimit;
	nMaxOutboundTimeframe = connOptions.nMaxOutboundTimeframe;

	SetBestHeight(connOptions.nBestHeight);

	clientInterface = connOptions.uiInterface;

	if (fListen && !InitBinds(connOptions.vBinds, connOptions.vWhiteBinds)) {
		if (clientInterface) {
			clientInterface->ThreadSafeMessageBox(
				_("Failed to listen on any port. Use -listen=0 if you want this."),
				"", CClientUIInterface::MSG_ERROR);
		}
		return false;
	}


	if (semOutbound == NULL) {
		// initialize semaphore
		semOutbound = new CSemaphore(std::min((nMaxOutbound + nMaxFeeler), nMaxConnections));
	}
	if (semAddnode == NULL) {
		// initialize semaphore
		semAddnode = new CSemaphore(nMaxAddnode);
	}

	//
	// Start threads
	//
	InterruptSocks5(false);
	interruptNet.reset();
	flagInterruptMsgProc = false;

	{
		std::unique_lock<std::mutex> lock(mutexMsgProc);
		fMsgProcWake = false;
	}

	// Send and receive from sockets, accept connections
	threadSocketHandler = std::thread(&TraceThread<std::function<void()> >, "net", std::function<void()>(std::bind(&CConnman::ThreadSocketHandler, this)));

	if (!GetBoolArg("-dnsseed", true))
		LogPrintf("DNS seeding disabled\n");
	else
		threadDNSAddressSeed = std::thread(&TraceThread<std::function<void()> >, "dnsseed", std::function<void()>(std::bind(&CConnman::ThreadDNSAddressSeed, this)));

	// Initiate outbound connections from -addnode
	threadOpenAddedConnections = std::thread(&TraceThread<std::function<void()> >, "addcon", std::function<void()>(std::bind(&CConnman::ThreadOpenAddedConnections, this)));

	// Initiate outbound connections unless connect=0
	if (!gArgs.IsArgSet("-connect") || gArgs.GetArgs("-connect").size() != 1 || gArgs.GetArgs("-connect")[0] != "0")
		threadOpenConnections = std::thread(&TraceThread<std::function<void()> >, "opencon", std::function<void()>(std::bind(&CConnman::ThreadOpenConnections, this)));

	// Process messages
	threadMessageHandler = std::thread(&TraceThread<std::function<void()> >, "msghand", std::function<void()>(std::bind(&CConnman::ThreadMessageHandler, this)));

	// Dump network addresses
	scheduler.scheduleEvery(std::bind(&CConnman::DumpData, this), DUMP_ADDRESSES_INTERVAL * 1000);

	return true;
}
*/