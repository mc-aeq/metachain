/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "NetworkManager.h"

#include <exception>
#include <mutex>
#include <thread>
#include <list>

#include "../MetaChain.h"
#include "../defines.h"
#include "../logger.h"
#include "../cSemaphore.h"
#include "../io/ipContainer.h"
#include "../io/netPeers.h"
#include "../functions.h"
#include "CNetAddr.h"
#include "CService.h"
#include "../tinyformat.h"
#include "netMessage.h"
#include "../cCriticalSection.h"

NetworkManager::NetworkManager(MetaChain *mc) :
	m_pMC(mc),
	m_iNetConnectTimeout(NET_DEFAULT_CONNECT_TIMEOUT),
	m_bActiveNetwork(false),
	m_bSync(false),
	m_iTimeBetweenConnects(0),
	m_pSemOutbound(NULL),
	m_pSemInbound(NULL),
	m_usMaxOutboundConnections(0),
	m_usMinOutboundConnections(0),
	m_usMaxInboundConnections(0)
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
	m_usMaxOutboundConnections = iniFile->GetLongValue("network", "max_outgoing_connections", NET_DEFAULT_MAX_OUTGOING_CONNECTIONS);

	// read the min outbound connections before we are active and can make a sync
	m_usMinOutboundConnections = iniFile->GetLongValue("network", "min_outgoing_connections", NET_DEFAULT_MIN_OUTGOING_CONNECTIONS);

	// we have min and max value of outbound connections. We need a hysteresis that defines when we call our nodes for new nodes if we fall short on connections
	m_usHysteresisOutboundConnections = m_usMinOutboundConnections + ceil((m_usMaxOutboundConnections - m_usMinOutboundConnections)*0.66);

	// read the max total connections
	m_usMaxInboundConnections = iniFile->GetLongValue("network", "max_incoming_connections", NET_DEFAULT_MAX_INCOMING_CONNECTIONS);

	// initialize the semaphores
	m_pSemOutbound = new cSemaphore(m_usMaxOutboundConnections);
	m_pSemInbound = new cSemaphore(m_usMaxInboundConnections);

#ifdef _WIN32
	// Initialize Windows Sockets
	WSADATA wsadata;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (ret != NO_ERROR || LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2)
		return false;
#endif

	// initialize the ban list
	LOG("initializing the Ban List", "NET");
	m_lstBanList = ipContainer< CNetAddr >(iniFile->GetValue("network", "ban_file", "bans.dat"));
	m_lstBanList.readContents();

	// initialize the outbound peer list
	LOG("initializing the Peer List", "NET");
	m_lstPeerListOut = ipContainer< netPeers >(iniFile->GetValue("network", "peer_file", "peers.dat"));
	m_lstPeerListOut.readContents();

	// initialize the inbound peer list (which is ofc empty at start, but we attach it to the same file as the outbound peer list so that we can sync FN nodes into the same file)
	m_lstPeerListIn = ipContainer< netPeers >(iniFile->GetValue("network", "peer_file", "peers.dat"));

	// creating a CService class for the listener and storing it for further purposes
	try
	{
		m_pServiceLocal = new CService(iniFile->GetValue("network", "listening_ip", NET_STANDARD_CATCHALL_LISTENING), (unsigned short)iniFile->GetLongValue("network", "listening_port", NET_STANDARD_PORT_LISTENING));

		// if we have a testnet, we increment the listening port by 100 to ensure no conflicts happen when a real and a testnet node are on the same system.
		if (MetaChain::getInstance().isTestNet())
			m_pServiceLocal->SetPort(m_pServiceLocal->GetPort() + 100);
	}
	catch (std::exception e)
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

#ifndef _WIN32
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
#ifdef _WIN32
		setsockopt(m_hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int));
#else
		setsockopt(m_hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int));
#endif
#endif
#ifdef _WIN32
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

	m_interruptNet.reset();
	m_abflagInterruptMsgProc = false;

	{
		boost::unique_lock<boost::mutex> lock(m_mutexMsgProc);
		m_bfMsgProcWake = false;
	}

	// Send and receive from sockets, accept connections
	threadSocketHandler = std::thread(&TraceThread<std::function<void()> >, "net", std::function<void()>(std::bind(&NetworkManager::ThreadSocketHandler, this)));
	
	// Initiate outbound connections
	threadOpenConnections = std::thread(&TraceThread<std::function<void()> >, "opencon", std::function<void()>(std::bind(&NetworkManager::ThreadOpenConnections, this)));

	// Process messages
	threadMessageHandler = std::thread(&TraceThread<std::function<void()> >, "msghand", std::function<void()>(std::bind(&NetworkManager::ThreadMessageHandler, this)));

	// Dump network addresses with a lightweight scheduler
	m_pMC->getScheduler()->scheduleEvery(std::bind(&NetworkManager::DumpData, this), DUMP_INTERVAL * 1000);

	// do maintaining functions every 60 seconds
	m_pMC->getScheduler()->scheduleEvery(std::bind(&NetworkManager::Maintainer, this), 60000);
}

void NetworkManager::ThreadSocketHandler()
{
	while (!m_interruptNet)
	{
		fd_set fdsetRecv;
		fd_set fdsetSend;
		fd_set fdsetError;
		FD_ZERO(&fdsetRecv);
		FD_ZERO(&fdsetSend);
		FD_ZERO(&fdsetError);
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 50000; // frequency to poll
				
		// flag the listening socket to read from him for new connections	
		FD_SET(m_hListenSocket, &fdsetRecv);

		int nSelect = select(m_hListenSocket + 1, &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
		if (m_interruptNet)
			return;

		if (nSelect == SOCKET_ERROR)
		{
			int nErr = WSAGetLastError();
			LOG_ERROR("socket select error - " + NetworkErrorString(nErr), "NET");
			for (unsigned int i = 0; i <= (m_hListenSocket+1); i++)
				FD_SET(i, &fdsetRecv);

			FD_ZERO(&fdsetSend);
			FD_ZERO(&fdsetError);
			if (!m_interruptNet.sleep_for(std::chrono::milliseconds(timeout.tv_usec / 1000)))
				return;
		}

		// Accept new connections
		if (m_hListenSocket != INVALID_SOCKET && FD_ISSET(m_hListenSocket, &fdsetRecv))
		{
			LOCK(m_csPeerListIn);
			m_lstPeerListIn.lstIP.emplace_back(&m_hListenSocket, m_pSemInbound);

			netPeers *newElem = &m_lstPeerListIn.lstIP.back();
			newElem->mark();
			// todo: when c++17 is adopted into compilers, use the reference returned from emplace_back instead of using the back() function.			 
			// check if the ip address isnt banned
			CNetAddr addr = (CNetAddr)newElem->csAddress;
			if (!newElem->toDestroy() && m_lstBanList.entryExists(&addr) )
			{
				LOG("connection dropped - banned - " + newElem->csAddress.toStringIP(), "NET");
				newElem->markDestroy();
			}

			// after we successfully received a connection, we send them our version info so that they know how to talk to us
			LOCK(newElem->pcsvSend);
			newElem->listSend.emplace_back( netMessage::SUBJECT::NET_VERSION, (void *)&g_cuint32tVersion, sizeof(g_cuint32tVersion), true);
			newElem->unmark();
		}

		// inbound and outbound socket handling
		handlePeers(&m_lstPeerListIn, &m_csPeerListIn);
		handlePeers(&m_lstPeerListOut, &m_csPeerListOut);
	}
}

void NetworkManager::handlePeers(ipContainer< netPeers> *peers, cCriticalSection *cs)
{
	fd_set fdsetRecv;
	fd_set fdsetSend;
	fd_set fdsetError;
	FD_ZERO(&fdsetRecv);
	FD_ZERO(&fdsetSend);
	FD_ZERO(&fdsetError);
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 50000; // frequency to poll
	SOCKET hSocketMax = 0;

	// first delete nodes that disconnected gracefully or on error
	{
		LOCK(cs);
		for (std::list< netPeers >::iterator it = peers->lstIP.begin(); it != peers->lstIP.end(); )
		{
			if ((it->toDestroy() || ( (it->hSocket == INVALID_SOCKET) && it->isConnected())) && !it->inUse() )
				it = peers->lstIP.erase(it);
			else
				it++;
		}
	}	

	// prepare the sockets for receiving and sending data (if necessary)
	{
		LOCK(cs);
		for (std::list< netPeers >::iterator it = peers->lstIP.begin(); it != peers->lstIP.end(); it++)
		{
			it->mark();

			// check whether the socket is connected
			if (!it->isConnected())
			{
				it->unmark();
				continue;
			}

			// check if the peer has something to send
			bool select_send;
			{
				LOCK(it->pcsvSend);
				select_send = !it->listSend.empty();
			}

			LOCK(it->pcshSocket);
			if (it->hSocket == INVALID_SOCKET)
			{
				it->unmark();
				continue;
			}

			FD_SET(it->hSocket, &fdsetError);
			hSocketMax = (std::max)(hSocketMax, it->hSocket);

			// when we have to send something, we dont receive this round
			if (select_send)
			{
				FD_SET(it->hSocket, &fdsetSend);
				it->unmark();
				continue;
			}

			// we always try to receive data unless we send data
			FD_SET(it->hSocket, &fdsetRecv);

			it->unmark();
		}
	}

	// when we have no sockets to work with, or have only non connected sockets, we skip the rest of the function
	if (hSocketMax == 0)
		return;

	int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
	if (m_interruptNet)
		return;

	if (nSelect == SOCKET_ERROR)
	{
		int nErr = WSAGetLastError();
		LOG_ERROR("socket select error - " + NetworkErrorString(nErr), "NET");
		for (unsigned int i = 0; i <= hSocketMax; i++)
			FD_SET(i, &fdsetRecv);

		FD_ZERO(&fdsetSend);
		FD_ZERO(&fdsetError);
		if (!m_interruptNet.sleep_for(std::chrono::milliseconds(timeout.tv_usec / 1000)))
			return;
	}

	// socket handling (read, write, timeout)
	LOCK(cs);
	for (std::list< netPeers >::iterator it = peers->lstIP.begin(); it != peers->lstIP.end(); it++)
	{
		if (m_interruptNet)
			return;

		it->mark();

		// is this peer about to be destroyed? if so, dont work with it
		// also when there are too many messages, skip the reading
		if (it->toDestroy() || it->readStop() || !it->isConnected())
		{
			it->unmark();
			continue;
		}

		// check for send and receive flags
		bool recvSet = false;
		bool sendSet = false;
		bool errorSet = false;
		{
			LOCK(it->pcshSocket);
			if (it->hSocket == INVALID_SOCKET)
			{
				it->markDestroy();
				it->unmark();
				continue;
			}
			recvSet = FD_ISSET(it->hSocket, &fdsetRecv);
			sendSet = FD_ISSET(it->hSocket, &fdsetSend);
			errorSet = FD_ISSET(it->hSocket, &fdsetError);
		}

		// Receive
		if (recvSet || errorSet)
		{
			// typical socket buffer is 8K-64K
			char pchBuf[0x10000];
			int nBytes = 0;
			{
				LOCK(it->pcshSocket);
				if (it->hSocket == INVALID_SOCKET)
				{
					it->markDestroy();
					it->unmark();
					continue;
				}
				nBytes = recv(it->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
			}

			if (nBytes > 0)
			{
				bool bNotifyWake = false;
				if (!it->ReceiveMsgBytes(pchBuf, nBytes, bNotifyWake))
					it->markDestroy();

				if (bNotifyWake)
					WakeMessageHandler();
			}
			else if (nBytes == 0)
			{
				// socket closed gracefully
				it->markDestroy();
				LOG("socket closed", "NET");
			}
			else if (nBytes < 0)
			{
				// error
				int nErr = WSAGetLastError();
				if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
				{
					it->markDestroy();
					LOG_ERROR("socket recv error - " + NetworkErrorString(nErr), "NET");
				}
			}

		}

		// Send
		if (sendSet && (it->hSocket != INVALID_SOCKET))
		{
			LOCK(it->pcsvSend);

			std::list< netMessage >::iterator msg = it->listSend.begin();
			size_t nSentSize = 0;

			while (msg != it->listSend.end())
			{
				int nBytes = 0;
				{
					LOCK(it->pcshSocket);
					nBytes = send(it->hSocket, reinterpret_cast<const char*>(msg->getPackage()) + nSentSize, msg->getPackageSize() - nSentSize, MSG_NOSIGNAL | MSG_DONTWAIT);
				}
				if (nBytes > 0)
				{
					nSentSize += nBytes;
					if (nSentSize == msg->getPackageSize())
					{
						// msg fully sent
						nSentSize = 0;
						msg++;
					}
					else
						// msg not fully sent, next try!
						break;
				}
				else
				{
					if (nBytes < 0)
					{
						int nErr = WSAGetLastError();
						if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
						{
							it->markDestroy();
							LOG_ERROR("socket send error - " + NetworkErrorString(nErr), "NET");
						}
					}
					// couldn't send anything at all
					break;
				}
			}

			// remove the sent messages
			it->listSend.erase(it->listSend.begin(), msg);
		}
		it->unmark();

		/*
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
		}*/
	}
}

void NetworkManager::WakeMessageHandler()
{
	{
		boost::lock_guard<boost::mutex> lock(m_mutexMsgProc);
		m_bfMsgProcWake = true;
	}
	m_condMsgProc.notify_one();
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
		cSemaphoreGrant grant(*m_pSemOutbound);
		if (m_interruptNet)
			return;
		
		// get the first one in the list which is not connected
		std::list<netPeers>::iterator itPeer = getNextOutNode(false, true);
		if (itPeer == m_lstPeerListOut.lstIP.end())
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
					m_lstPeerListOut.lstIP.erase(itPeer);
				}
			}
		}
	}
}

void NetworkManager::ThreadMessageHandler()
{
	while (!m_abflagInterruptMsgProc)
	{
		bool fMoreWork = false;

		// work those messages
		handleMessage(&m_lstPeerListIn, &m_csPeerListIn, true);
		handleMessage(&m_lstPeerListOut, &m_csPeerListOut, false);

		boost::unique_lock<boost::mutex> lock(m_mutexMsgProc);
		if (!fMoreWork)
			m_condMsgProc.wait_until(lock, boost::chrono::steady_clock::now() + boost::chrono::milliseconds(100), [this] { return m_bfMsgProcWake; });

		m_bfMsgProcWake = false;		
	}
}

void NetworkManager::handleMessage(ipContainer< netPeers> *peers, cCriticalSection *cs, bool bInbound)
{
	LOCK(cs);
	for (std::list< netPeers >::iterator it = peers->lstIP.begin(); it != peers->lstIP.end(); it++)
	{
		it->mark();
		if (it->toDestroy() || !it->hasMessage())
		{
			it->unmark();
			continue;
		}

		// process received messages
		while (it->hasMessage())
		{
			netMessage cpy;

			// we make a temporary copy of the message so that we can release the lock faster in order to not block other threads
			{
				LOCK(it->pcsvQueue);
				cpy = it->getMessage();
				it->popMessage();
			}
			if (!ProcessMessage(cpy, it, bInbound))
			{
				LOG_ERROR("Error processing message - disconnecting peer " + it->toString(), "NET");
				it->markDestroy();
				break;
			}
		}
		it->unmark();

		if (m_abflagInterruptMsgProc)
			return;
	}
}

/**
@brief processes network messages and acts accordingly

@detail This function processes a network message for a certain peer and handles accordingly. New network connections are verified in the following sequence:\n
\image html network_initialization.svg "New Network Connection"\n
As soon as the configured node.ini number of connections to fullnodes are established, the following sequence to sync the metachain and subchains is triggered:\n
\image html sync_subchain.svg "SubChain Sync Process"\n

@return true on success, false on error. false implies that the network connection will be terminated.
*/
bool NetworkManager::ProcessMessage(netMessage msg, std::list< netPeers >::iterator peer, bool bInbound)
{
	// security check: when we receive a package that is not NET_VERSION in subject and validConnection() == false, it means we didnt receive the version string and the connection is possibly malicious. 
	// we skip all messages and destroy the connection for security reasons
	if (!peer->validConnection() && (msg.getHeader().ui16tSubject != netMessage::SUBJECT::NET_VERSION))
	{
		LOG_ERROR("Received a different package than NET_VERSION in the first communication. Destroying connection for security reasons!", "NET");
		return false;
	}

	switch (msg.getHeader().ui16tSubject)
	{
		case netMessage::SUBJECT::NET_VERSION:
		{
			// if the connection is already valid, we don't care for the version number anymore
			if (peer->validConnection())
				break;

			uint32_t uint32tReceivedVersion;
			memcpy(&uint32tReceivedVersion, msg.getData(), 4);
	#ifdef _DEBUG
			LOG_DEBUG("Got Version Info from " + peer->toString() + " - Version: " + std::to_string(uint32tReceivedVersion), "NET");
	#endif
			// if the connected client has an older version than ours, we inform him about a new version and then destroy the connection
			if (g_cuint32tVersion > uint32tReceivedVersion)
			{
				{
					LOCK(peer->pcsvSend);
					peer->listSend.emplace_back(netMessage::SUBJECT::NET_VERSION_NEWER, (void *)&g_cuint32tVersion, sizeof(g_cuint32tVersion), true);
				}

				LOG("Connected client has a lower version than ours, inform and disconnect - " + peer->toString(), "NET");
				return false;
			}
			// if the connected client is newer than ours, we increment a ticker in the metachain and also destroy this connection
			else if (g_cuint32tVersion < uint32tReceivedVersion)
			{
				MetaChain::getInstance().incrementNewerVersionTicker();

				LOG("We have an older version than the connected client, increment the version ticker and disconnect - " + peer->toString(), "NET");
				return false;
			}
			// wenn the version number is the same, we don't do anything except we set a flag that the initial communication was made and the peer is ready for full service, as well as we ask them to send us their peer list
			else
			{
				// the connection is now valid
				peer->validConnection(true);

				LOCK(peer->pcsvSend);
				bool bTmp;
				peer->listSend.emplace_back(netMessage::SUBJECT::NET_VERSION, (void *)&g_cuint32tVersion, sizeof(g_cuint32tVersion), true);
				bTmp = MetaChain::getInstance().isTestNet();
				peer->listSend.emplace_back(netMessage::SUBJECT::NET_TESTNET, (void *)&bTmp, sizeof(bool), true);
				bTmp = MetaChain::getInstance().isFN();
				peer->listSend.emplace_back(netMessage::SUBJECT::NET_NODEMODE, (void *)&bTmp, sizeof(bool), true);
			}
		}
		break;

		case netMessage::SUBJECT::NET_NODEMODE:
		{
			// store the nodemode info for this peer. This is the last step in our handshake before the actual blockchain communication and that's why we request the peers from our partner now
			bool bFN = false;
			memcpy(&bFN, msg.getData(), sizeof(bool));
			peer->setNodeMode(bFN);
					
			LOCK(peer->pcsvSend);

			// if it's a full node, we want to know his listening port so that we can forward this information to all other connecting peers
			if (bFN)
				peer->listSend.emplace_back(netMessage::SUBJECT::NET_NODE_LISTENING_PORT_GET, (void*)NULL, 0, true);

			// request peers
			peer->listSend.emplace_back(netMessage::SUBJECT::NET_PEER_LIST_SEND, (void*)NULL, 0, true);
		}
		break;

		case netMessage::SUBJECT::NET_NODE_LISTENING_PORT_GET:
		{
			LOCK(peer->pcsvSend);
			// send back our listening port from our socket.
			unsigned short usTmp = m_pServiceLocal->GetPort();
			peer->listSend.emplace_back(netMessage::SUBJECT::NET_NODE_LISTENING_PORT_SEND, (void*)&usTmp, sizeof(unsigned short), true);
		}
		break;

		case netMessage::SUBJECT::NET_NODE_LISTENING_PORT_SEND:
		{
			// we get the listening port when the connected peer is a FN so that when the connection was incoming, we can forward the real port for the FN in our NET_PEER_LIST_SEND
			unsigned short usListeningPort;
			memcpy(&usListeningPort, msg.getData(), sizeof(unsigned short));
			peer->setListeningPort(usListeningPort);
		}
		break;

		case netMessage::SUBJECT::NET_TESTNET:
		{
			// we compare if the new connection is in the testnet or not and if that's the same as our node. if not, we remove the connection
			bool bTestNet = false;
			memcpy(&bTestNet, msg.getData(), sizeof(bool));

			if (MetaChain::getInstance().isTestNet() != bTestNet)
			{
				LOG("removing peer due to incompability with testnet", "NET");
				return false;
			}
		}
		break;

		case netMessage::SUBJECT::NET_VERSION_NEWER:
		{
			LOG("new version info received", "NET");

			// it looks like we have an older version than the connected node. We increment our ticker for future checking of newer versions
			MetaChain::getInstance().incrementNewerVersionTicker();
			// we dont need to disconnect the node since after this Message the node will automatically disconnect
		}
		break;

		case netMessage::SUBJECT::NET_PEER_LIST_SEND:
		{
			int iNumOfPeers = 0;
			unsigned int uiCount = 0;
			uint8_t *puint8tBuffer;
			// 18: ipv4 needs *4, ipv6 needs *16. So we use *16 for the IP and then *2 for the port
			static unsigned short susIpPortSize = 18;
			{
				LOCK(m_csPeerListOut);
				LOCK(m_csPeerListIn);

				// the connected node wants to know our peers for their reference. we build up some data, excluding the requesting peer itself and sending it over for them to process
				iNumOfPeers = m_lstPeerListOut.lstIP.size() + m_lstPeerListIn.lstIP.size() - 1;

				// if we don't have any peers, we simply quit this operation without sending anything
				if (iNumOfPeers == 0)
					break;

				// make us a buffer array. we multiply the number of peers by 18: ipv4 needs *4, ipv6 needs *16. So we use *16 for the IP and then *2 for the port			
				puint8tBuffer = new uint8_t[iNumOfPeers * susIpPortSize];
				memset(puint8tBuffer, '\0', iNumOfPeers * susIpPortSize);

				// now copy it into the buffer and skip all non full nodes
				for (std::list<netPeers>::iterator it = m_lstPeerListOut.lstIP.begin(); it != m_lstPeerListOut.lstIP.end(); it++)
				{
					if( (bInbound || it != peer) && (peer->isFN()) )
					{
						// copy the ip adress					
						memcpy((puint8tBuffer + uiCount * sizeof(uint8_t) * susIpPortSize), it->csAddress.GetBytes(), sizeof(uint8_t) * 16);

						// copy the port
						uint16_t port = it->getListeningPort();
						memcpy((puint8tBuffer + uiCount * sizeof(uint8_t) * susIpPortSize + susIpPortSize - 2 * sizeof(uint8_t)), &port, sizeof(port));

						uiCount++;
					}
				}
				for (std::list<netPeers>::iterator it = m_lstPeerListIn.lstIP.begin(); it != m_lstPeerListIn.lstIP.end(); it++)
				{
					if ( (!bInbound || it != peer) && (peer->isFN()) )
					{
						// copy the ip adress					
						memcpy((puint8tBuffer + uiCount * sizeof(uint8_t) * susIpPortSize), it->csAddress.GetBytes(), sizeof(uint8_t) * 16);

						// copy the port
						uint16_t port = it->getListeningPort();
						memcpy((puint8tBuffer + uiCount * sizeof(uint8_t) * susIpPortSize + susIpPortSize - 2 * sizeof(uint8_t)), &port, sizeof(port));

						uiCount++;
					}
				}
			}

			// and finally we sent our peer list on the way
			LOCK(peer->pcsvSend);
			peer->listSend.emplace_back(netMessage::SUBJECT::NET_PEER_LIST_GET, (void *)puint8tBuffer, uiCount * susIpPortSize, true);

			delete[] puint8tBuffer;
		}
		break;

		case netMessage::SUBJECT::NET_PEER_LIST_GET:
		{
			// make us a buffer array. the size of the buffer is defined by the payload and we can calculate backwards to the number of peers
			static unsigned short susIpPortSize = 18;
			int iNumOfPeers = msg.getHeader().ui32tPayloadSize / susIpPortSize;

			// get the individual peers, create the adress and add it to our outgoing list
			for (int i = 0; i < iNumOfPeers; i++)
			{
				uint16_t port = 0;
				uint8_t *puint8tBuffer = new uint8_t[susIpPortSize];

				memcpy(puint8tBuffer, msg.getData() + i*susIpPortSize, susIpPortSize);
				memcpy(&port, puint8tBuffer + (susIpPortSize - sizeof(uint16_t)), sizeof(uint16_t));

				netPeers tmp;
				tmp.csAddress.SetRaw(puint8tBuffer);
				tmp.csAddress.SetPort(port);

				CNetAddr addr = tmp.csAddress;

	#ifndef _DEBUG
				// when we're in production mode we don't accept new nodes that are on a local interface. it makes no sense to run multiple nodes on a single system
				if (addr.IsLocal())
					continue;
	#endif 
				// check if it's me
				if (*m_pServiceLocal == addr)
				{
	#ifdef _DEBUG
					LOG_DEBUG("Not adding new Peer since it's me!", "NET");
	#endif
					continue;
				}

				// check if we have this node already in our list
				{
					LOCK(m_csPeerListOut);
					if (m_lstBanList.entryExists(&addr))
						LOG_ERROR("Not adding new outbound peer - peer is banned - " + tmp.toString(), "NET");
					else if (!m_lstPeerListOut.entryExists(&tmp) )
					{
						m_lstPeerListOut.lstIP.push_back(tmp);
						LOG("Adding new outbound peer - " + tmp.toString(), "NET");
					}
	#ifdef _DEBUG
					else
						LOG_DEBUG("Not adding new Peer since it's already known - " + tmp.toString(), "NET");
	#endif
				}

				delete[] puint8tBuffer;
			}	
			break;
		}

		case netMessage::SUBJECT::NET_NODE_PING:
		{
			// we received a PING request, answer with PONG
			peer->listSend.emplace_back(netMessage::SUBJECT::NET_NODE_PONG, (void*)NULL, 0, true);
			break;
		}

		case netMessage::SUBJECT::NET_NODE_PONG:
		{
			// we received the answer from our PING command, so we update the flag of this node to be active and dismiss the command
			peer->bPingPong = true;
			break;
		}
		// the default statement as well as the ban list statements. We don't support sending ban lists, everyone has to build them theirselfs right now
		case netMessage::SUBJECT::NET_BAN_LIST_GET:
		case netMessage::SUBJECT::NET_BAN_LIST_SEND:
		default:
			return false;

		// todo: add random payload that is defined with MCP05 - pass it over to the instance of the subchain and let it handle itself. important: check for m_bSync == true. If m_bSync != true discard the message since we're not sync yet.
	}

	return true;
}

std::list< netPeers >::iterator NetworkManager::getNextOutNode(bool bConnected, bool bCheckTimeDelta)
{
	LOCK(m_csPeerListOut);
	for (std::list< netPeers >::iterator it = m_lstPeerListOut.lstIP.begin(); it != m_lstPeerListOut.lstIP.end(); it++ )
	{
		if (it->isConnected() != bConnected)
			continue;

		if (bCheckTimeDelta && (it->getTimeLastTry() != 0) && (GetTime() - it->getTimeLastTry() <= m_iTimeBetweenConnects))
			continue;

		return it;
	}
	return m_lstPeerListOut.lstIP.end();
}

void NetworkManager::DumpData()
{
	// write the ban list
	m_lstBanList.writeContents();

	// write the Peer List Out
	LOCK(m_csPeerListOut);
	LOCK(m_csPeerListIn);
	m_lstPeerListOut.writeContents();
	m_lstPeerListIn.writeContents(true);
}

/**
@brief 60 second scheduler that does maintaining

@detail This function gets called from a lightweight scheduler thread every 60 seconds. \n
This maintaining function consists of four tasks:\n
-# PingPong: send a PING command to all active connections which need to return PONG until the next Maintainer is called.\n
This helps us to filter out dead, overloaded, inactive or bad connectivity nodes. If PONG wasn't send since the last PING, the connection gets terminated.
-# Active connection tracking: if there are too few good active connections, NET_PEER_LIST_SEND gets triggered to all remaining connections to achieve more good connections. This is done up to a certain hysteresis
-# (one time only) Full Sync: as soon as enough good connections are established, this function triggers a fullsync. This is a one-time only function and deactivates itself after the sync is finished
-# Cleanup: cleanup functions that remove old, stale or inactive connections
*/
void NetworkManager::Maintainer()
{
	// Step 1: PingPong
	// we use m_lstPeerListOut for activity checking of our outgoing connections
	for (auto &it : m_lstPeerListOut.lstIP)
	{
		// if the node was already marked to destroy, we skip it
		if (it.toDestroy())
			continue;

		// check if the connection is valid
		if (it.bPingPong)
		{
			// send new PingPong cmd
			it.listSend.emplace_back(netMessage::SUBJECT::NET_NODE_PING, (void*)NULL, 0, true);

			// reset bPingPong
			it.bPingPong = false;
		}
		else
		{
			// when we reached this point the node we pinged didn't respond within a minute. we mark this connection to be destroyed
			it.markDestroy();
		}
	}

	unsigned short usActiveOutgoing = 0;
	// Step 2: connectivity tracking
	{
		// count the number of active outgoing connections of fullnodes
		for (auto &it : m_lstPeerListOut.lstIP)
		{
			if (it.validConnection() && it.isFN())
				usActiveOutgoing++;
		}

		if (usActiveOutgoing <= m_usHysteresisOutboundConnections)
		{
			// since we have less than our hysteresis requires, we're requesting the node lists from our peers
			for (auto &it : m_lstPeerListOut.lstIP)
			{
				if (it.validConnection())
					it.listSend.emplace_back(netMessage::SUBJECT::NET_PEER_LIST_SEND, (void*)NULL, 0, true);
			}
		}
	}

	// Step 3: full sync
	if (!m_bSync && usActiveOutgoing >= m_usMinOutboundConnections)
	{
		// todo: implement step 3 full sync

		m_bSync = true;
	}

	// Step 4: clean up and remove old nodes	
	{
		LOCK(m_csPeerListOut);
		for (std::list< netPeers >::iterator it = m_lstPeerListOut.lstIP.begin(); it != m_lstPeerListOut.lstIP.end(); )
		{
			if ((it->toDestroy() || ((it->hSocket == INVALID_SOCKET) && it->isConnected())) && !it->inUse())
				it = m_lstPeerListOut.lstIP.erase(it);
			else
				it++;
		}
	}
}

NetworkManager::~NetworkManager()
{
	// stop and delete the threads
	Interrupt();
	Stop();

	// dump the data one last time
	DumpData();
}

void NetworkManager::Interrupt()
{
	{
		boost::lock_guard<boost::mutex> lock(m_mutexMsgProc);
		m_abflagInterruptMsgProc = true;
	}
	m_condMsgProc.notify_all();

	m_interruptNet();
}

void NetworkManager::Stop()
{
	if (threadMessageHandler.joinable())
		threadMessageHandler.join();
	if (threadOpenConnections.joinable())
		threadOpenConnections.join();
	if (threadSocketHandler.joinable())
		threadSocketHandler.join();
}
