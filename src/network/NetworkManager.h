#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __NETWORKMANAGER_H__
#define __NETWORKMANAGER_H__ 1

#include <mutex>
#include <thread>
#include <atomic>
#include <list>
#include <boost/thread/condition_variable.hpp>

#include "../cThreadInterrupt.h"
#include "../cCriticalSection.h"
#include "../network/CService.h"
#include "../network/CNetAddr.h"
#include "../io/ipContainer.h"
#include "../io/netPeers.h"
#include "netMessage.h"
#include "../MetaChain.h"
#include "../SimpleIni.h"

class MetaChain;

/*
This is the main class that'll hold all information about the chain and handle all it's core parts.
*/
class NetworkManager
{
	private:
		MetaChain							*m_pMC;
		CService							*m_pServiceLocal;
		int									m_iNetConnectTimeout;
		int									m_iTimeBetweenConnects;
		int									m_iMaxOutboundConnections;
		int									m_iMaxInboundConnections;
		bool								m_bActiveNetwork;

		// thread interrupts and message processing variables
		CThreadInterrupt					m_interruptNet;
		std::atomic<bool>					m_abflagInterruptMsgProc;
		bool								m_bfMsgProcWake;
		boost::condition_variable			m_condMsgProc;
		boost::mutex						m_mutexMsgProc;
		void								WakeMessageHandler();

		// socket functions and variables
		SOCKET								m_hListenSocket;
		bool								startListeningSocket();

		// thread functions and variables
		std::thread							threadSocketHandler;
		std::thread							threadOpenConnections;
		std::thread							threadMessageHandler;
		void								startThreads();
		void								ThreadSocketHandler();
		void								ThreadOpenConnections();
		void								ThreadMessageHandler();

		// message handling
		inline void							handleMessage(ipContainer< netPeers> *peers, cCriticalSection *cs, bool bInbound);
		bool								ProcessMessage(netMessage msg, std::list< netPeers >::iterator peer, bool bInbound);

		// functions to update the peers and ban lists
		void								DumpData();

		// functions and variables for our peers list and their management
		ipContainer< netPeers >				m_lstPeerListOut;
		mutable cCriticalSection			m_csPeerListOut;
		cSemaphore							*m_pSemOutbound;
		ipContainer< netPeers >				m_lstPeerListIn;
		mutable cCriticalSection			m_csPeerListIn;
		cSemaphore							*m_pSemInbound;
		std::list< netPeers >::iterator		getNextOutNode(bool bConnected = true, bool bCheckTimeDelta = true);
		inline void							handlePeers(ipContainer< netPeers> *peers, cCriticalSection *cs);

		// functions and variables for our banned list and their management
		ipContainer< CNetAddr >				m_lstBanList;

		// destructor functions
		void								Interrupt();
		void								Stop();


	public:
											NetworkManager(MetaChain *mc);
											~NetworkManager();
		bool								initialize(CSimpleIniA* iniFile);
		int									getTimeout() { return m_iNetConnectTimeout; };
};

#endif