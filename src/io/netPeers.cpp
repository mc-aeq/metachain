#include "../../stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

netPeers::netPeers()
{

}

bool netPeers::init(string strEntry)
{
	// the format is <ip>:<port>\n
	vector< string > vecSplit;

	// split IP and Port
	boost::split(vecSplit, strEntry, boost::is_any_of(":"));

	// since we split IP and Port, the size of the vector must be 2, otherwise it's a mistake and we cant continue with this entry
	if (vecSplit.size() != 2)
	{
		LOG_ERROR("Can't parse peers entry. Must be wrong format: " + strEntry, "NETP");
		return false;
	}

	// entry 1 is the IP, entry 2 the port, so lets instantiate this
	m_CService = CService(vecSplit[0].c_str(), boost::lexical_cast<unsigned short>(vecSplit[1]));

	return true;
}

string netPeers::toString() const
{
	return m_CService.toString();
}