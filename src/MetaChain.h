#pragma once

using namespace std;
class NetworkManager;

/*
This is the main class that'll hold all information about the chain and handle all it's core parts.
*/
class MetaChain
{
private:
	NetworkManager				*m_pNetworkManager;
	boost::thread_group			m_threadGroup;
	CScheduler					m_scheduler;

	void						LicenseInfo();

public:
								MetaChain();
	bool						initialize(CSimpleIniA* iniFile);

	// simple getter
	CScheduler*					getScheduler() { return &m_scheduler; };
	boost::thread_group*		getThreadGroup() { return &m_threadGroup; };
};	