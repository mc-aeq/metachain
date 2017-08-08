/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "MetaChain.h"

#include <atomic>

#include "external/SimpleIni.h"
#include "external/scheduler.h"
#include "functions.h"
#include "network/NetworkManager.h"
#include "logger.h"

extern std::atomic<bool> sabShutdown;

MetaChain::MetaChain() :
	m_pNetworkManager( NULL ),
	m_iVersionTicksTillUpdate(0),
	m_bAutoUpdate(true)
{
}

MetaChain& MetaChain::getInstance()
{
	static MetaChain instance;
	return instance;
}

bool MetaChain::initialize(CSimpleIniA* iniFile)
{
	// start the lightweight scheduling thread
	CScheduler::Function serviceLoop = boost::bind(&CScheduler::serviceQueue, &m_scheduler);
	m_threadGroup.create_thread(boost::bind(&TraceThread<CScheduler::Function>, "scheduler", serviceLoop));

	// read some ini settings
	m_iVersionTicksTillUpdate = iniFile->GetLongValue("autoupdate", "ticks_until_update_triggered", 10);
	m_bAutoUpdate = iniFile->GetBoolValue("autoupdate", "do_autoupdate", true);

	// create the block storage backends, check their integrity and check if no other instance is running
	m_pStorageManager = new StorageManager(this);
	if (!m_pStorageManager->initialize(iniFile))
		return false;

	// create a new network manager
	m_pNetworkManager = new NetworkManager(this);

	// initializing the network manager
	if (!m_pNetworkManager->initialize(iniFile))
		return false;

	// finally everything is initialized and we print our copyright info
	LicenseInfo();

	// everything is fine
	return true;
}

void MetaChain::LicenseInfo()
{
	LOG("-------------------------------", "");
	LOG("Copyright (C) 2017, TrustChainTechnologies.io", "");
	LOG("Please join our forums at https://forum.trustchaintechnologies.io and if you have questions you can also contact our support team via support@trustchaintechnologies.io", "");
	LOG("If you want to contribute to this development, you can always check out the sources at https://github.com/TrustChainTechnologies/metachain", "");
	LOG("This is an experimental software distributed under the GPL Version 3, read the LICENSE file for further information.", "");
	LOG("-------------------------------", "");
}

void MetaChain::incrementNewerVersionTicker()
{
	m_usNewerVersionTicker++;

	// after X ticks, we read the ini value if we auto update. if not, we close the node. if we have autoupdate configured, we connect to a CDN and download the latest binary and try to restart the node smoothly
	if (m_usNewerVersionTicker >= m_iVersionTicksTillUpdate)
	{
		if (!m_bAutoUpdate)
		{
			// if we don't want to auto update, we close the node due to incompability
			sabShutdown = true;
		}
		else
		{
			// todo: auto update
		}
	}
	
}