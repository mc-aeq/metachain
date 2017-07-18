#pragma once

using namespace std;
class NetworkManager;

/*
This is the main class that'll hold all information about the chain and handle all it's core parts.
It's also a singleton to provide direct access to the relevant metachain parts throughout the project. There can only be one MetaChain!
*/
class MetaChain
{
private:
	NetworkManager				*m_pNetworkManager;
	boost::thread_group			m_threadGroup;
	CScheduler					m_scheduler;

	unsigned short				m_usNewerVersionTicker;

	void						LicenseInfo();

	// constructor and operator
								MetaChain();
								MetaChain(MetaChain const& copy);	// not implemented
	MetaChain&					operator=(MetaChain const& copy);	// not implemented

public:
	static						MetaChain& getInstance();
	bool						initialize(CSimpleIniA* iniFile);

	void						incrementNewerVersionTicker();

	// simple getter
	CScheduler*					getScheduler() { return &m_scheduler; };
	boost::thread_group*		getThreadGroup() { return &m_threadGroup; };
	NetworkManager*				getNetworkManager() { return m_pNetworkManager; };
};	