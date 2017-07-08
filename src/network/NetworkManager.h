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
		ipContainer< netPeers >				*m_pPeerList;

		// thread interrupts and message processing variables
		CThreadInterrupt					m_interruptNet;
		bool								m_bfMsgProcWake;
		condition_variable					m_condMsgProc;
		mutex								m_mutexMsgProc;
		atomic<bool>						m_abflagInterruptMsgProc;

		// socket functions
		bool								startListeningSocket();
		bool								SetSocketNonBlocking(SOCKET& hSocket, bool fNonBlocking);
		string								NetworkErrorString(int err);
		bool								CloseSocket(SOCKET& hSocket);

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

		// destructor functions
		void								Interrupt();
		void								Stop();

	public:
											NetworkManager(MetaChain *mc);
											~NetworkManager();
		bool								initialize(CSimpleIniA* iniFile);
};