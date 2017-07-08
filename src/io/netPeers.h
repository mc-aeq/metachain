#pragma once

/*
this class is used as template type for ipContainer. Especially to store and retrieve all relevant information about peers
*/
class netPeers
{
private:
	CService						m_CService;

public:
	netPeers();
	bool							init(string strEntry);
	std::string						toString() const;
};