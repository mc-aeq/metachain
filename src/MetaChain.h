#pragma once

using namespace std;
class NetworkManager;

/*
This is the main class that'll hold all information about the chain and handle all it's core parts.
*/
class MetaChain
{
private:
	NetworkManager		*m_pNetworkManager;

	void				LicenseInfo();

public:
						MetaChain();
	bool				initialize(CSimpleIniA* iniFile);
};