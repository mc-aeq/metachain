#include "../../stdafx.h"

static std::atomic<bool> interruptSocks5Recv(false);

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

	// initialize the ban list
	LOG("initializing the Ban List", "NET");
	m_pBanList = new ipContainer(iniFile->GetValue("network", "ban_file", "bans.dat"));
	m_pBanList->readContents();

	// initialize the peer list
	LOG("initializing the Peer List", "NET");
	//m_pPeerList = new peerContainer(iniFile->GetValue("network", "peer_file", "peers.dat"));

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

	// Initiate outbound connections
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
	{/*
		//
		// Disconnect nodes
		//
		{
			LOCK(cs_vNodes);
			// Disconnect unused nodes
			std::vector<CNode*> vNodesCopy = vNodes;
			for (CNode* pnode : vNodesCopy)
			{
				if (pnode->fDisconnect)
				{
					// remove from vNodes
					vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());

					// release outbound grant (if any)
					pnode->grantOutbound.Release();

					// close socket and cleanup
					pnode->CloseSocketDisconnect();

					// hold in disconnected pool until all refs are released
					pnode->Release();
					vNodesDisconnected.push_back(pnode);
				}
			}
		}
		{
			// Delete disconnected nodes
			std::list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
			for (CNode* pnode : vNodesDisconnectedCopy)
			{
				// wait until threads are done using it
				if (pnode->GetRefCount() <= 0) {
					bool fDelete = false;
					{
						TRY_LOCK(pnode->cs_inventory, lockInv);
						if (lockInv) {
							TRY_LOCK(pnode->cs_vSend, lockSend);
							if (lockSend) {
								fDelete = true;
							}
						}
					}
					if (fDelete) {
						vNodesDisconnected.remove(pnode);
						DeleteNode(pnode);
					}
				}
			}
		}
		size_t vNodesSize;
		{
			LOCK(cs_vNodes);
			vNodesSize = vNodes.size();
		}
		if (vNodesSize != nPrevNodeCount) {
			nPrevNodeCount = vNodesSize;
			if (clientInterface)
				clientInterface->NotifyNumConnectionsChanged(nPrevNodeCount);
		}

		//
		// Find which sockets have data to receive
		//
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 50000; // frequency to poll pnode->vSend

		fd_set fdsetRecv;
		fd_set fdsetSend;
		fd_set fdsetError;
		FD_ZERO(&fdsetRecv);
		FD_ZERO(&fdsetSend);
		FD_ZERO(&fdsetError);
		SOCKET hSocketMax = 0;
		bool have_fds = false;

		for (const ListenSocket& hListenSocket : vhListenSocket) {
			FD_SET(hListenSocket.socket, &fdsetRecv);
			hSocketMax = std::max(hSocketMax, hListenSocket.socket);
			have_fds = true;
		}

		{
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
		}

		int nSelect = select(have_fds ? hSocketMax + 1 : 0,
			&fdsetRecv, &fdsetSend, &fdsetError, &timeout);
		if (interruptNet)
			return;

		if (nSelect == SOCKET_ERROR)
		{
			if (have_fds)
			{
				int nErr = WSAGetLastError();
				LogPrintf("socket select error %s\n", NetworkErrorString(nErr));
				for (unsigned int i = 0; i <= hSocketMax; i++)
					FD_SET(i, &fdsetRecv);
			}
			FD_ZERO(&fdsetSend);
			FD_ZERO(&fdsetError);
			if (!interruptNet.sleep_for(std::chrono::milliseconds(timeout.tv_usec / 1000)))
				return;
		}

		//
		// Accept new connections
		//
		for (const ListenSocket& hListenSocket : vhListenSocket)
		{
			if (hListenSocket.socket != INVALID_SOCKET && FD_ISSET(hListenSocket.socket, &fdsetRecv))
			{
				AcceptConnection(hListenSocket);
			}
		}

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
	/*// Connect to specific addresses
	if (gArgs.IsArgSet("-connect"))
	{
		for (int64_t nLoop = 0;; nLoop++)
		{
			ProcessOneShot();
			for (const std::string& strAddr : gArgs.GetArgs("-connect"))
			{
				CAddress addr(CService(), NODE_NONE);
				OpenNetworkConnection(addr, false, NULL, strAddr.c_str());
				for (int i = 0; i < 10 && i < nLoop; i++)
				{
					if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
						return;
				}
			}
			if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
				return;
		}
	}

	// Initiate network connections
	int64_t nStart = GetTime();

	// Minimum time before next feeler connection (in microseconds).
	int64_t nNextFeeler = PoissonNextSend(nStart * 1000 * 1000, FEELER_INTERVAL);*/
	while (!m_interruptNet)
	{
		/*ProcessOneShot();

		if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
			return;

		CSemaphoreGrant grant(*semOutbound);
		if (interruptNet)
			return;

		// Add seed nodes if DNS seeds are all down (an infrastructure attack?).
		if (addrman.size() == 0 && (GetTime() - nStart > 60)) {
			static bool done = false;
			if (!done) {
				LogPrintf("Adding fixed seed nodes as DNS doesn't seem to be available.\n");
				CNetAddr local;
				local.SetInternal("fixedseeds");
				addrman.Add(convertSeed6(Params().FixedSeeds()), local);
				done = true;
			}
		}

		//
		// Choose an address to connect to based on most recently seen
		//
		CAddress addrConnect;

		// Only connect out to one peer per network group (/16 for IPv4).
		// Do this here so we don't have to critsect vNodes inside mapAddresses critsect.
		int nOutbound = 0;
		int nOutboundRelevant = 0;
		std::set<std::vector<unsigned char> > setConnected;
		{
			LOCK(cs_vNodes);
			for (CNode* pnode : vNodes) {
				if (!pnode->fInbound && !pnode->fAddnode) {

					// Count the peers that have all relevant services
					if (pnode->fSuccessfullyConnected && !pnode->fFeeler && ((pnode->nServices & nRelevantServices) == nRelevantServices)) {
						nOutboundRelevant++;
					}
					// Netgroups for inbound and addnode peers are not excluded because our goal here
					// is to not use multiple of our limited outbound slots on a single netgroup
					// but inbound and addnode peers do not use our outbound slots.  Inbound peers
					// also have the added issue that they're attacker controlled and could be used
					// to prevent us from connecting to particular hosts if we used them here.
					setConnected.insert(pnode->addr.GetGroup());
					nOutbound++;
				}
			}
		}

		// Feeler Connections
		//
		// Design goals:
		//  * Increase the number of connectable addresses in the tried table.
		//
		// Method:
		//  * Choose a random address from new and attempt to connect to it if we can connect
		//    successfully it is added to tried.
		//  * Start attempting feeler connections only after node finishes making outbound
		//    connections.
		//  * Only make a feeler connection once every few minutes.
		//
		bool fFeeler = false;
		if (nOutbound >= nMaxOutbound) {
			int64_t nTime = GetTimeMicros(); // The current time right now (in microseconds).
			if (nTime > nNextFeeler) {
				nNextFeeler = PoissonNextSend(nTime, FEELER_INTERVAL);
				fFeeler = true;
			}
			else {
				continue;
			}
		}

		int64_t nANow = GetAdjustedTime();
		int nTries = 0;
		while (!interruptNet)
		{
			CAddrInfo addr = addrman.Select(fFeeler);

			// if we selected an invalid address, restart
			if (!addr.IsValid() || setConnected.count(addr.GetGroup()) || IsLocal(addr))
				break;

			// If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
			// stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
			// already-connected network ranges, ...) before trying new addrman addresses.
			nTries++;
			if (nTries > 100)
				break;

			if (IsLimited(addr))
				continue;

			// only connect to full nodes
			if ((addr.nServices & REQUIRED_SERVICES) != REQUIRED_SERVICES)
				continue;

			// only consider very recently tried nodes after 30 failed attempts
			if (nANow - addr.nLastTry < 600 && nTries < 30)
				continue;

			// only consider nodes missing relevant services after 40 failed attempts and only if less than half the outbound are up.
			ServiceFlags nRequiredServices = nRelevantServices;
			if (nTries >= 40 && nOutbound < (nMaxOutbound >> 1)) {
				nRequiredServices = REQUIRED_SERVICES;
			}

			if ((addr.nServices & nRequiredServices) != nRequiredServices) {
				continue;
			}

			// do not allow non-default ports, unless after 50 invalid addresses selected already
			if (addr.GetPort() != Params().GetDefaultPort() && nTries < 50)
				continue;

			addrConnect = addr;

			// regardless of the services assumed to be available, only require the minimum if half or more outbound have relevant services
			if (nOutboundRelevant >= (nMaxOutbound >> 1)) {
				addrConnect.nServices = REQUIRED_SERVICES;
			}
			else {
				addrConnect.nServices = nRequiredServices;
			}
			break;
		}

		if (addrConnect.IsValid()) {

			if (fFeeler) {
				// Add small amount of random noise before connection to avoid synchronization.
				int randsleep = GetRandInt(FEELER_SLEEP_WINDOW * 1000);
				if (!interruptNet.sleep_for(std::chrono::milliseconds(randsleep)))
					return;
				LogPrint(BCLog::NET, "Making feeler connection to %s\n", addrConnect.ToString());
			}

			OpenNetworkConnection(addrConnect, (int)setConnected.size() >= std::min(nMaxConnections - 1, 2), &grant, NULL, false, fFeeler);
		}*/
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


void NetworkManager::DumpData()
{
	// write the ban list
	m_pBanList->writeContents();
}

NetworkManager::~NetworkManager()
{
	// stop and delete the threads
	Interrupt();
	Stop();

	// delete the ipContainers
	delete m_pBanList;
	//delete m_pPeerList;
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

	DumpData();

	// todo: close all sockets including listening socket
}