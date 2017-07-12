#include "../../stdafx.h"

static std::atomic<bool> interruptSocks5Recv(false);

NetworkManager::NetworkManager(MetaChain *mc) :
	m_pMC(mc),
	m_iNetConnectTimeout(NET_DEFAULT_CONNECT_TIMEOUT),
	m_bActiveNetwork(false),
	m_iTimeBetweenConnects(0),
	m_pSemOutbound(NULL),
	m_iMaxOutboundConnections(0),
	m_iMaxInboundConnections(0)
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

	// read the default connect timeout from the settings, if it's not set, we'll use our default value
	m_iNetConnectTimeout = iniFile->GetLongValue("network", "connect_timeout", NET_DEFAULT_CONNECT_TIMEOUT);

	// read the time in seconds between two connects that were not succesfull
	m_iTimeBetweenConnects = iniFile->GetLongValue("network", "time_between_unsuccessfull_connects", NET_DEFAULT_TIME_BETWEEN_UNSUCCESSFUL);

	// read the max outbound connections
	m_iMaxOutboundConnections = iniFile->GetLongValue("network", "max_outgoing_connections", NET_DEFAULT_MAX_OUTGOING_CONNECTIONS);

	// read the max total connections
	m_iMaxInboundConnections = iniFile->GetLongValue("network", "max_incoming_connections", NET_DEFAULT_MAX_INCOMING_CONNECTIONS);

	// initialize the semaphores
	m_pSemOutbound = new CSemaphore(m_iMaxOutboundConnections);
	m_pSemInbound = new CSemaphore(m_iMaxInboundConnections);

#ifdef _WINDOWS
	// Initialize Windows Sockets
	WSADATA wsadata;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (ret != NO_ERROR || LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2)
		return false;
#endif

	// initialize the ban list
	LOG("initializing the Ban List", "NET");
	m_pBanList = new ipContainer< CNetAddr >(iniFile->GetValue("network", "ban_file", "bans.dat"));
	m_pBanList->readContents();

	// initialize the outbound peer list
	LOG("initializing the Peer List", "NET");
	m_pPeerListOut = new ipContainer< netPeers >(iniFile->GetValue("network", "peer_file", "peers.dat"));
	m_pPeerListOut->readContents();

	// initialize the inbound peer list (which is ofc empty at start)
	m_pPeerListIn = new ipContainer< netPeers >();

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

	// start the threads for network communication
	LOG("Starting networking threads", "NET");
	startThreads();

	// everything went smoothly
	m_bActiveNetwork = true;
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

	m_hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if(m_hListenSocket == INVALID_SOCKET)
	{
		LOG_ERROR("Couldn't open socket for incoming connections - Socket error: " + NetworkErrorString(WSAGetLastError()), "NET");
		return false;
	}

#ifndef _WINDOWS
#ifdef SO_NOSIGPIPE
	// Different way of disabling SIGPIPE on BSD
	setsockopt(m_hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
#endif
	// Allow binding if the port is still in TIME_WAIT state after
	// the program was closed and restarted.
	setsockopt(m_hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
	// Disable Nagle's algorithm
	setsockopt(m_hListenSocket, IPPROTO_TCP, TCP_NODELAY, (void*)&nOne, sizeof(int));
#else
	setsockopt(m_hListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOne, sizeof(int));
	setsockopt(m_hListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&nOne, sizeof(int));
#endif

	// Set to non-blocking, incoming connections will also inherit this
	if(!SetSocketNonBlocking(m_hListenSocket, true))
	{
		CloseSocket(m_hListenSocket);
		LOG_ERROR("BindListenPort: Setting listening socket to non-blocking failed - " + NetworkErrorString(WSAGetLastError()), "NET");
		return false;
	}

	// some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
	// and enable it by default or not. Try to enable it, if possible.
	if(m_pServiceLocal->IsIPv6())
	{
#ifdef IPV6_V6ONLY
#ifdef _WINDOWS
		setsockopt(m_hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int));
#else
		setsockopt(m_hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int));
#endif
#endif
#ifdef _WINDOWS
		int nProtLevel = PROTECTION_LEVEL_UNRESTRICTED;
		setsockopt(m_hListenSocket, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (const char*)&nProtLevel, sizeof(int));
#endif
	}

	if (::bind(m_hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
	{
		int nErr = WSAGetLastError();
		if (nErr == WSAEADDRINUSE)
			LOG_ERROR( strprintf("Unable to bind to %s on this computer. A process is already running.", m_pServiceLocal->toString() ), "NET");
		else
			LOG_ERROR( strprintf("Unable to bind to %s on this computer (bind returned error %s)", m_pServiceLocal->toString(), NetworkErrorString(nErr)), "NET");
			
		CloseSocket(m_hListenSocket);
		return false;
	}
	LOG( "Bound to " + m_pServiceLocal->toString(), "NET" );

	// Listen for incoming connections
	if (listen(m_hListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		LOG_ERROR( strprintf("Listening for incoming connections failed (listen returned error %s)", NetworkErrorString(WSAGetLastError())), "NET");
		CloseSocket(m_hListenSocket);
		return false;
	}

	return true;
}

void NetworkManager::startThreads()
{
	LOG("Starting Threads", "NET");

	interruptSocks5(false);
	m_interruptNet.reset();
	m_abflagInterruptMsgProc = false;

	{
		std::unique_lock<std::mutex> lock(m_mutexMsgProc);
		m_bfMsgProcWake = false;
	}

	// Send and receive from sockets, accept connections
	threadSocketHandler = thread(&TraceThread<function<void()> >, "net", function<void()>(bind(&NetworkManager::ThreadSocketHandler, this)));

	// new connections
	threadOpenAddedConnections = std::thread(&TraceThread<std::function<void()> >, "addcon", std::function<void()>(std::bind(&NetworkManager::ThreadOpenAddedConnections, this)));

	// Initiate outbound connections
	threadOpenConnections = std::thread(&TraceThread<std::function<void()> >, "opencon", std::function<void()>(std::bind(&NetworkManager::ThreadOpenConnections, this)));

	// Process messages
	threadMessageHandler = std::thread(&TraceThread<std::function<void()> >, "msghand", std::function<void()>(std::bind(&NetworkManager::ThreadMessageHandler, this)));

	// Dump network addresses with a lightweight scheduler
	m_pMC->getScheduler()->scheduleEvery(std::bind(&NetworkManager::DumpData, this), DUMP_INTERVAL * 1000);
}

void NetworkManager::interruptSocks5(bool bInterrupt)
{
	interruptSocks5Recv = bInterrupt;
}

void NetworkManager::ThreadSocketHandler()
{
	unsigned int nPrevNodeCount = 0;
	while (!m_interruptNet)
	{
		fd_set fdsetRecv;
		fd_set fdsetSend;
		fd_set fdsetError;
		bool have_fds = false;
		FD_ZERO(&fdsetRecv);
		FD_ZERO(&fdsetSend);
		FD_ZERO(&fdsetError);

		
		// Find which sockets have data to receive
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 50000; // frequency to poll

		SOCKET hSocketMax = 0;
		
		FD_SET(m_hListenSocket, &fdsetRecv);
		hSocketMax = m_hListenSocket;
		have_fds = true;

		/*{
			LOCK(cs_vNodes);
			for (CNode* pnode : vNodes)
			{
				// Implement the following logic:
				// * If there is data to send, select() for sending data. As this only
				//   happens when optimistic write failed, we choose to first drain the
				//   write buffer in this case before receiving more. This avoids
				//   needlessly queueing received data, if the remote peer is not themselves
				//   receiving data. This means properly utilizing TCP flow control signalling.
				// * Otherwise, if there is space left in the receive buffer, select() for
				//   receiving data.
				// * Hand off all complete messages to the processor, to be handled without
				//   blocking here.

				bool select_recv = !pnode->fPauseRecv;
				bool select_send;
				{
					LOCK(pnode->cs_vSend);
					select_send = !pnode->vSendMsg.empty();
				}

				LOCK(pnode->cs_hSocket);
				if (pnode->hSocket == INVALID_SOCKET)
					continue;

				FD_SET(pnode->hSocket, &fdsetError);
				hSocketMax = std::max(hSocketMax, pnode->hSocket);
				have_fds = true;

				if (select_send) {
					FD_SET(pnode->hSocket, &fdsetSend);
					continue;
				}
				if (select_recv) {
					FD_SET(pnode->hSocket, &fdsetRecv);
				}
			}
		}*/

		int nSelect = select(have_fds ? hSocketMax + 1 : 0,
			&fdsetRecv, &fdsetSend, &fdsetError, &timeout);
		if (m_interruptNet)
			return;

		if (nSelect == SOCKET_ERROR)
		{
			if (have_fds)
			{
				int nErr = WSAGetLastError();
				LOG_ERROR("socket select error - " + NetworkErrorString(nErr), "NET");
				for (unsigned int i = 0; i <= hSocketMax; i++)
					FD_SET(i, &fdsetRecv);
			}
			FD_ZERO(&fdsetSend);
			FD_ZERO(&fdsetError);
			if (!m_interruptNet.sleep_for(std::chrono::milliseconds(timeout.tv_usec / 1000)))
				return;
		}

		// Accept new connections
		if(m_hListenSocket != INVALID_SOCKET && FD_ISSET(m_hListenSocket, &fdsetRecv))
			AcceptConnection();

		/*
		//
		// Service each socket
		//
		std::vector<CNode*> vNodesCopy;
		{
			LOCK(cs_vNodes);
			vNodesCopy = vNodes;
			for (CNode* pnode : vNodesCopy)
				pnode->AddRef();
		}
		for (CNode* pnode : vNodesCopy)
		{
			if (interruptNet)
				return;

			//
			// Receive
			//
			bool recvSet = false;
			bool sendSet = false;
			bool errorSet = false;
			{
				LOCK(pnode->cs_hSocket);
				if (pnode->hSocket == INVALID_SOCKET)
					continue;
				recvSet = FD_ISSET(pnode->hSocket, &fdsetRecv);
				sendSet = FD_ISSET(pnode->hSocket, &fdsetSend);
				errorSet = FD_ISSET(pnode->hSocket, &fdsetError);
			}
			if (recvSet || errorSet)
			{
				// typical socket buffer is 8K-64K
				char pchBuf[0x10000];
				int nBytes = 0;
				{
					LOCK(pnode->cs_hSocket);
					if (pnode->hSocket == INVALID_SOCKET)
						continue;
					nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
				}
				if (nBytes > 0)
				{
					bool notify = false;
					if (!pnode->ReceiveMsgBytes(pchBuf, nBytes, notify))
						pnode->CloseSocketDisconnect();
					RecordBytesRecv(nBytes);
					if (notify) {
						size_t nSizeAdded = 0;
						auto it(pnode->vRecvMsg.begin());
						for (; it != pnode->vRecvMsg.end(); ++it) {
							if (!it->complete())
								break;
							nSizeAdded += it->vRecv.size() + CMessageHeader::HEADER_SIZE;
						}
						{
							LOCK(pnode->cs_vProcessMsg);
							pnode->vProcessMsg.splice(pnode->vProcessMsg.end(), pnode->vRecvMsg, pnode->vRecvMsg.begin(), it);
							pnode->nProcessQueueSize += nSizeAdded;
							pnode->fPauseRecv = pnode->nProcessQueueSize > nReceiveFloodSize;
						}
						WakeMessageHandler();
					}
				}
				else if (nBytes == 0)
				{
					// socket closed gracefully
					if (!pnode->fDisconnect) {
						LogPrint(BCLog::NET, "socket closed\n");
					}
					pnode->CloseSocketDisconnect();
				}
				else if (nBytes < 0)
				{
					// error
					int nErr = WSAGetLastError();
					if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
					{
						if (!pnode->fDisconnect)
							LogPrintf("socket recv error %s\n", NetworkErrorString(nErr));
						pnode->CloseSocketDisconnect();
					}
				}
			}

			//
			// Send
			//
			if (sendSet)
			{
				LOCK(pnode->cs_vSend);
				size_t nBytes = SocketSendData(pnode);
				if (nBytes) {
					RecordBytesSent(nBytes);
				}
			}

			//
			// Inactivity checking
			//
			int64_t nTime = GetSystemTimeInSeconds();
			if (nTime - pnode->nTimeConnected > 60)
			{
				if (pnode->nLastRecv == 0 || pnode->nLastSend == 0)
				{
					LogPrint(BCLog::NET, "socket no message in first 60 seconds, %d %d from %d\n", pnode->nLastRecv != 0, pnode->nLastSend != 0, pnode->GetId());
					pnode->fDisconnect = true;
				}
				else if (nTime - pnode->nLastSend > TIMEOUT_INTERVAL)
				{
					LogPrintf("socket sending timeout: %is\n", nTime - pnode->nLastSend);
					pnode->fDisconnect = true;
				}
				else if (nTime - pnode->nLastRecv > (pnode->nVersion > BIP0031_VERSION ? TIMEOUT_INTERVAL : 90 * 60))
				{
					LogPrintf("socket receive timeout: %is\n", nTime - pnode->nLastRecv);
					pnode->fDisconnect = true;
				}
				else if (pnode->nPingNonceSent && pnode->nPingUsecStart + TIMEOUT_INTERVAL * 1000000 < GetTimeMicros())
				{
					LogPrintf("ping timeout: %fs\n", 0.000001 * (GetTimeMicros() - pnode->nPingUsecStart));
					pnode->fDisconnect = true;
				}
				else if (!pnode->fSuccessfullyConnected)
				{
					LogPrintf("version handshake timeout from %d\n", pnode->GetId());
					pnode->fDisconnect = true;
				}
			}
		}
		{
			LOCK(cs_vNodes);
			for (CNode* pnode : vNodesCopy)
				pnode->Release();
		}*/
	}
}

void NetworkManager::ThreadOpenAddedConnections()
{
	/*{
		LOCK(cs_vAddedNodes);
		vAddedNodes = gArgs.GetArgs("-addnode");
	}*/

	while (true)
	{/*
		CSemaphoreGrant grant(*semAddnode);
		std::vector<AddedNodeInfo> vInfo = GetAddedNodeInfo();
		bool tried = false;
		for (const AddedNodeInfo& info : vInfo) {
			if (!info.fConnected) {
				if (!grant.TryAcquire()) {
					// If we've used up our semaphore and need a new one, lets not wait here since while we are waiting
					// the addednodeinfo state might change.
					break;
				}
				// If strAddedNode is an IP/port, decode it immediately, so
				// OpenNetworkConnection can detect existing connections to that IP/port.
				tried = true;
				CService service(LookupNumeric(info.strAddedNode.c_str(), Params().GetDefaultPort()));
				OpenNetworkConnection(CAddress(service, NODE_NONE), false, &grant, info.strAddedNode.c_str(), false, false, true);
				if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
					return;
			}
		}
		// Retry every 60 seconds if a connection was attempted, otherwise two seconds
		if (!interruptNet.sleep_for(std::chrono::seconds(tried ? 60 : 2)))
			return;*/
	}
}

void NetworkManager::ThreadOpenConnections()
{
	// run this loop until we interrupt it
	while (!m_interruptNet)
	{
		// is the network up and running
		if (!m_bActiveNetwork)
			return;

		// sleep for 500 ms
		if (!m_interruptNet.sleep_for(std::chrono::milliseconds(500)))
			return;

		// get the semaphore grant
		CSemaphoreGrant grant(*m_pSemOutbound);
		if (m_interruptNet)
			return;
		
		// get the first one in the vector which is not connected
		vector<netPeers>::iterator itPeer = getNextOutNode(false, true);
		if (itPeer == m_pPeerListOut->vecIP.end())
			continue;

		// try to connect
		if (itPeer->tryConnectOutbound())
		{
			// connection successfull move the semaphore
			if (grant)
				grant.MoveTo(itPeer->semaphoreGrant);
		}
		else
		{
			// the connection was not successfull, check if we're already above our possible tries - if so we remove him
			if (itPeer->tooManyTriesOutbound())
			{
				LOG("removing node due to too many connection errors - " + itPeer->toString(), "NET");
				{
					LOCK(m_csPeerListOut);
					m_pPeerListOut->vecIP.erase(itPeer);
				}
			}
		}
	}
}

void NetworkManager::ThreadMessageHandler()
{
	while (!m_abflagInterruptMsgProc)
	{/*
		std::vector<CNode*> vNodesCopy;
		{
			LOCK(cs_vNodes);
			vNodesCopy = vNodes;
			for (CNode* pnode : vNodesCopy) {
				pnode->AddRef();
			}
		}

		bool fMoreWork = false;

		for (CNode* pnode : vNodesCopy)
		{
			if (pnode->fDisconnect)
				continue;

			// Receive messages
			bool fMoreNodeWork = GetNodeSignals().ProcessMessages(pnode, *this, flagInterruptMsgProc);
			fMoreWork |= (fMoreNodeWork && !pnode->fPauseSend);
			if (flagInterruptMsgProc)
				return;

			// Send messages
			{
				LOCK(pnode->cs_sendProcessing);
				GetNodeSignals().SendMessages(pnode, *this, flagInterruptMsgProc);
			}
			if (flagInterruptMsgProc)
				return;
		}

		{
			LOCK(cs_vNodes);
			for (CNode* pnode : vNodesCopy)
				pnode->Release();
		}

		std::unique_lock<std::mutex> lock(mutexMsgProc);
		if (!fMoreWork) {
			condMsgProc.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::milliseconds(100), [this] { return fMsgProcWake; });
		}
		fMsgProcWake = false;*/
	}
}

vector< netPeers >::iterator NetworkManager::getNextOutNode(bool bConnected, bool bCheckTimeDelta)
{
	LOCK(m_csPeerListOut);
	for (vector< netPeers >::iterator it = m_pPeerListOut->vecIP.begin(); it != m_pPeerListOut->vecIP.end(); it++ )
	{
		if (it->isConnected() != bConnected)
			continue;

		if (bCheckTimeDelta && (it->getTimeLastTry() != 0) && (GetTime() - it->getTimeLastTry() <= m_iTimeBetweenConnects))
			continue;

		return it;
	}
	return m_pPeerListOut->vecIP.end();
}


void NetworkManager::DumpData()
{
	// write the ban list
	m_pBanList->writeContents();

	// write the Peer List Out
	LOCK(m_csPeerListOut);
	m_pPeerListOut->writeContents();
}

NetworkManager::~NetworkManager()
{
	// stop and delete the threads
	Interrupt();
	Stop();

	// dump the data one last time
	DumpData();

	// delete the ipContainers
	delete m_pBanList;
	delete m_pPeerListOut;
	delete m_pPeerListIn;

	// delete the semaphores
	delete m_pSemInbound;
	delete m_pSemInbound;
}

void NetworkManager::Interrupt()
{
	{
		std::lock_guard<std::mutex> lock(m_mutexMsgProc);
		m_abflagInterruptMsgProc = true;
	}
	m_condMsgProc.notify_all();

	m_interruptNet();
	interruptSocks5(true);
}

void NetworkManager::Stop()
{
	if (threadMessageHandler.joinable())
		threadMessageHandler.join();
	if (threadOpenConnections.joinable())
		threadOpenConnections.join();
	if (threadOpenAddedConnections.joinable())
		threadOpenAddedConnections.join();
	if (threadSocketHandler.joinable())
		threadSocketHandler.join();
}

void NetworkManager::AcceptConnection()
{
	netPeers tmpPeer;
	struct sockaddr_storage sockaddr;
	socklen_t len = sizeof(sockaddr);

	tmpPeer.hSocket = accept(m_hListenSocket, (struct sockaddr*)&sockaddr, &len);

	// some security checks that the socket is really useable
	if ((tmpPeer.hSocket != INVALID_SOCKET) && (!tmpPeer.csAddress.SetSockAddr((const struct sockaddr*)&sockaddr)))
			LOG_ERROR( "Warning: Unknown socket family", "NET" );

	if (tmpPeer.hSocket == INVALID_SOCKET)
	{
		int nErr = WSAGetLastError();
		if (nErr != WSAEWOULDBLOCK)
			LOG_ERROR("socket error accept failed - " + NetworkErrorString(nErr), "NET");
		return;
	}

	if (!m_bActiveNetwork)
	{
		LOG_ERROR("connection dropped: network not initialized - " + tmpPeer.csAddress.toString(), "NET");
		CloseSocket(tmpPeer.hSocket);
		return;
	}

	if (!IsSelectableSocket(tmpPeer.hSocket))
	{
		LOG_ERROR("connection dropped: non-selectable socket - " + tmpPeer.csAddress.toString(), "NET");
		CloseSocket(tmpPeer.hSocket);
		return;
	}

	// According to the internet TCP_NODELAY is not carried into accepted sockets
	// on all platforms.  Set it again here just to be sure.
	SetSocketNoDelay(tmpPeer.hSocket);

	// check if the ip address isnt banned
	if (isBanned(tmpPeer.csAddress.toStringIP()))
	{
		LOG("connection dropped - banned - " + tmpPeer.csAddress.toString(), "NET");
		CloseSocket(tmpPeer.hSocket);
		return;
	}

	// get the semaphore grant without blocking
	CSemaphoreGrant grant(*m_pSemInbound, true);
	if (!grant)
	{
		LOG("too many open connections - connection dropped", "NET");
		CloseSocket(tmpPeer.hSocket);
		return;
	}
	else
		grant.MoveTo(tmpPeer.semaphoreGrant);

	// everything went smooth, we add this inbound connection to our vector
	{
		LOCK(m_csPeerListIn);
		m_pPeerListIn->vecIP.push_back(tmpPeer);
	}
	LOG("connection accepted - " + tmpPeer.csAddress.toString(), "NET");
}

bool NetworkManager::isBanned(string strAddress)
{
	for (vector< CNetAddr >::iterator it = m_pBanList->vecIP.begin(); it != m_pBanList->vecIP.end(); it++)
	{
		if (it->toString() == strAddress)
			return true;
	}
	return false;
}