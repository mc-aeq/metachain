#pragma once

using namespace std;

/*
This is the main class that'll hold all information about the chain and handle all it's core parts.
*/
class NetworkManager
{
	private:
		MetaChain							*m_pMC;
		CService							*m_pServiceLocal;
		ipContainer< CNetAddr >				*m_pBanList;
		int									m_iNetConnectTimeout;
		int									m_iTimeBetweenConnects;
		bool								m_bActiveNetwork;

		// thread interrupts and message processing variables
		CThreadInterrupt					m_interruptNet;
		bool								m_bfMsgProcWake;
		condition_variable					m_condMsgProc;
		mutex								m_mutexMsgProc;
		atomic<bool>						m_abflagInterruptMsgProc;
		CSemaphore							*m_pSemOutbound;

		// socket functions
		bool								startListeningSocket();

		// thread functions and variables
		thread								threadSocketHandler;
		thread								threadOpenAddedConnections;
		thread								threadOpenConnections;
		thread								threadMessageHandler;
		void								startThreads();
		void								interruptSocks5(bool bInterrupt);
		void								ThreadSocketHandler();
		void								ThreadOpenAddedConnections();
		void								ThreadOpenConnections();
		void								ThreadMessageHandler();

		// functions to update the peers and ban lists
		void								DumpData();

		// functions and variables for our peers list and their management
		ipContainer< netPeers >				*m_pPeerList;
		vector< netPeers >::iterator		getNextNode(bool bConnected = true, bool bCheckTimeDelta = true);

		// destructor functions
		void								Interrupt();
		void								Stop();

	public:
											NetworkManager(MetaChain *mc);
											~NetworkManager();
		bool								initialize(CSimpleIniA* iniFile);
		int									getTimeout() { return m_iNetConnectTimeout; };
};