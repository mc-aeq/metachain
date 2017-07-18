#include "../stdafx.h"

MetaChain::MetaChain() :
	m_pNetworkManager( NULL )
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

	// todo: auto update
	// after X ticks, we read the ini value if we auto update. if not, we close the node. if we have autoupdate configured, we connect to a CDN and download the latest binary and try to restart the node smoothly
}