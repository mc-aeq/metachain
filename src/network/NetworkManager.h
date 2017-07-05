#pragma once

using namespace std;

/*
This is the main class that'll hold all information about the chain and handle all it's core parts.
*/
class NetworkManager
{
private:
	MetaChain				*m_pMC;
	CService				*m_pServiceLocal;

public:
							NetworkManager(MetaChain *mc);
	bool					initialize(CSimpleIniA* iniFile);
};