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
		m_pServiceLocal = new CService(iniFile->GetValue("network", "listening_ip", "*"), (unsigned short)iniFile->GetLongValue("network", "listening_port", 5634));
	}
	catch (exception e)
	{
		LOG_ERROR("That wasn't good. An error occured creating the local service listener. Please check your configuration file.", "NET");
		return false;
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

	vWhitelistedRange = connOptions.vWhitelistedRange;

	for (const auto& strDest : connOptions.vSeedNodes) {
		AddOneShot(strDest);
	}

	if (clientInterface) {
		clientInterface->InitMessage(_("Loading P2P addresses..."));
	}
	// Load addresses from peers.dat
	int64_t nStart = GetTimeMillis();
	{
		CAddrDB adb;
		if (adb.Read(addrman))
			LogPrintf("Loaded %i addresses from peers.dat  %dms\n", addrman.size(), GetTimeMillis() - nStart);
		else {
			addrman.Clear(); // Addrman can be in an inconsistent state after failure, reset it
			LogPrintf("Invalid or missing peers.dat; recreating\n");
			DumpAddresses();
		}
	}
	if (clientInterface)
		clientInterface->InitMessage(_("Loading banlist..."));
	// Load addresses from banlist.dat
	nStart = GetTimeMillis();
	CBanDB bandb;
	banmap_t banmap;
	if (bandb.Read(banmap)) {
		SetBanned(banmap); // thread save setter
		SetBannedSetDirty(false); // no need to write down, just read data
		SweepBanned(); // sweep out unused entries

		LogPrint(BCLog::NET, "Loaded %d banned node ips/subnets from banlist.dat  %dms\n",
			banmap.size(), GetTimeMillis() - nStart);
	}
	else {
		LogPrintf("Invalid or missing banlist.dat; recreating\n");
		SetBannedSetDirty(true); // force write
		DumpBanlist();
	}

	uiInterface.InitMessage(_("Starting network threads..."));

	fAddressesInitialized = true;

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


bool CConnman::InitBinds(const std::vector<CService>& binds, const std::vector<CService>& whiteBinds) {
	bool fBound = false;
	for (const auto& addrBind : binds) {
		fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR));
	}
	for (const auto& addrBind : whiteBinds) {
		fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR | BF_WHITELIST));
	}
	if (binds.empty() && whiteBinds.empty()) {
		struct in_addr inaddr_any;
		inaddr_any.s_addr = INADDR_ANY;
		fBound |= Bind(CService(in6addr_any, GetListenPort()), BF_NONE);
		fBound |= Bind(CService(inaddr_any, GetListenPort()), !fBound ? BF_REPORT_ERROR : BF_NONE);
	}
	return fBound;
}

bool CConnman::Bind(const CService &addr, unsigned int flags) {
	if (!(flags & BF_EXPLICIT) && IsLimited(addr))
		return false;
	std::string strError;
	if (!BindListenPort(addr, strError, (flags & BF_WHITELIST) != 0)) {
		if ((flags & BF_REPORT_ERROR) && clientInterface) {
			clientInterface->ThreadSafeMessageBox(strError, "", CClientUIInterface::MSG_ERROR);
		}
		return false;
	}
	return true;
}

bool CConnman::BindListenPort(const CService &addrBind, std::string& strError, bool fWhitelisted)
{
	strError = "";
	int nOne = 1;

	// Create socket for listening for incoming connections
	struct sockaddr_storage sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
	{
		strError = strprintf("Error: Bind address family for %s not supported", addrBind.ToString());
		LogPrintf("%s\n", strError);
		return false;
	}

	SOCKET hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (hListenSocket == INVALID_SOCKET)
	{
		strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %s)", NetworkErrorString(WSAGetLastError()));
		LogPrintf("%s\n", strError);
		return false;
	}
	if (!IsSelectableSocket(hListenSocket))
	{
		strError = "Error: Couldn't create a listenable socket for incoming connections";
		LogPrintf("%s\n", strError);
		return false;
	}


#ifndef WIN32
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
	if (!SetSocketNonBlocking(hListenSocket, true)) {
		strError = strprintf("BindListenPort: Setting listening socket to non-blocking failed, error %s\n", NetworkErrorString(WSAGetLastError()));
		LogPrintf("%s\n", strError);
		return false;
	}

	// some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
	// and enable it by default or not. Try to enable it, if possible.
	if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
#ifdef WIN32
		setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int));
#else
		setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int));
#endif
#endif
#ifdef WIN32
		int nProtLevel = PROTECTION_LEVEL_UNRESTRICTED;
		setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (const char*)&nProtLevel, sizeof(int));
#endif
	}

	if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
	{
		int nErr = WSAGetLastError();
		if (nErr == WSAEADDRINUSE)
			strError = strprintf(_("Unable to bind to %s on this computer. %s is probably already running."), addrBind.ToString(), _(PACKAGE_NAME));
		else
			strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"), addrBind.ToString(), NetworkErrorString(nErr));
		LogPrintf("%s\n", strError);
		CloseSocket(hListenSocket);
		return false;
	}
	LogPrintf("Bound to %s\n", addrBind.ToString());

	// Listen for incoming connections
	if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		strError = strprintf(_("Error: Listening for incoming connections failed (listen returned error %s)"), NetworkErrorString(WSAGetLastError()));
		LogPrintf("%s\n", strError);
		CloseSocket(hListenSocket);
		return false;
	}

	vhListenSocket.push_back(ListenSocket(hListenSocket, fWhitelisted));

	if (addrBind.IsRoutable() && fDiscover && !fWhitelisted)
		AddLocal(addrBind, LOCAL_BIND);

	return true;
}*/