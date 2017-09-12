#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __METACHAIN_H__
#define __METACHAIN_H__ 1

#include <boost/thread/thread.hpp>

#include "network/NetworkManager.h"
#include "io/StorageManager.h"
#include "scheduler.h"
#include "MCP/MCP01/Account.h"

class NetworkManager;
class StorageManager;

/*
This is the main class that'll hold all information about the chain and handle all it's core parts.
It's also a singleton to provide direct access to the relevant metachain parts throughout the project. There can only be one MetaChain!
*/
class MetaChain
{
private:
	NetworkManager				*m_pNetworkManager;
	StorageManager				*m_pStorageManager;
	boost::thread_group			m_threadGroup;
	CScheduler					m_scheduler;

	// variables and functions for the autoupdate process
	int							m_iVersionTicksTillUpdate;
	bool						m_bAutoUpdate;
	unsigned short				m_usNewerVersionTicker;
	std::string					m_strCDNUrl;
	boost::filesystem::path		m_pathExecutable;
	boost::filesystem::path		m_pathTmp;
	bool						downloadFile(std::string strFrom, std::string strTo);
	bool						doAutoUpdate();

	// variables that define the functionality of this node
	bool						m_bModeFN;
	bool						m_bTestNet;
	MCP01::Account				m_mcpWallet;

	// constructor and operator
								MetaChain();
								MetaChain(MetaChain const& copy);	// not implemented
	MetaChain&					operator=(MetaChain const& copy);	// not implemented

public:
	static						MetaChain& getInstance();
	void						shutdown();
	bool						initialize(CSimpleIniA* iniFile, boost::filesystem::path pathExecutable);
	void						LicenseInfo();

	void						incrementNewerVersionTicker();

	// simple getter
	CScheduler*					getScheduler() { return &m_scheduler; };
	boost::thread_group*		getThreadGroup() { return &m_threadGroup; };
	NetworkManager*				getNetworkManager() { return m_pNetworkManager; };
	StorageManager*				getStorageManager() { return m_pStorageManager; };
	bool						isFN() { return m_bModeFN; };
	bool						isTestNet() { return m_bTestNet; };
};	

#endif