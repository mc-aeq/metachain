#include "../../stdafx.h"
#include <boost/algorithm/string.hpp>

ipContainer::ipContainer()
{
}

ipContainer::ipContainer(string strFileName) :
	m_strFileName(strFileName)
{
}

ipContainer::~ipContainer()
{

}

void ipContainer::readContents()
{
	ifstream streamFile(m_strFileName);
#ifdef _DEBUG
	LOG_DEBUG("reading contents of file: " + m_strFileName, "IPC");
#endif

	for (string strLine; getline(streamFile, strLine); )
	{
		// renmove unwanted spaces from beginning and end
		boost::trim(strLine);

		// don't process empty lines
		if (strLine.length() == 0 || strLine.empty())
			continue;

		// if the first character is a # it's a comment and we skip this line
		if (strLine[0] == '#')
			continue;

#ifdef _DEBUG
		LOG_DEBUG("line content: " + strLine, "IPC");
#endif
		// convert those IP adresses to CNetAddr and add them to our vector
		struct addrinfo aiHint;
		memset(&aiHint, 0, sizeof(struct addrinfo));

		aiHint.ai_socktype = SOCK_STREAM;
		aiHint.ai_protocol = IPPROTO_TCP;
		aiHint.ai_family = AF_UNSPEC;
#ifdef _WINDOWS
		aiHint.ai_flags = 0;
#else
		aiHint.ai_flags = AI_ADDRCONFIG;
#endif

		struct addrinfo *aiRes = NULL;
		int nErr = getaddrinfo(strLine.c_str(), NULL, &aiHint, &aiRes);
		if (nErr)
		{
			LOG_ERROR("Error resolving hostname: " + strLine + " - Error No: " + to_string(nErr), "CService");
			
			// since we can't proceed with this entry, we skip it and go to the next one
			continue;
		}

		struct addrinfo *aiTrav = aiRes;
		while (aiTrav != NULL)
		{
			if (aiTrav->ai_family == AF_INET)
			{
				assert(aiTrav->ai_addrlen >= sizeof(sockaddr_in));
				CNetAddr tmp(((struct sockaddr_in*)(aiTrav->ai_addr))->sin_addr);
				m_vecIP.push_back(tmp);
			}

			if (aiTrav->ai_family == AF_INET6)
			{
				assert(aiTrav->ai_addrlen >= sizeof(sockaddr_in6));
				struct sockaddr_in6* s6 = (struct sockaddr_in6*) aiTrav->ai_addr;

				CNetAddr tmp(s6->sin6_addr, s6->sin6_scope_id);
				m_vecIP.push_back(tmp);
			}

			aiTrav = aiTrav->ai_next;
		}

		freeaddrinfo(aiRes);
	}

#ifdef _DEBUG
	LOG_DEBUG("done reading contents of file: " + m_strFileName, "IPC");
	LOG_DEBUG("elements in the vector: " + to_string(m_vecIP.size()), "IPC");
#endif
}

void ipContainer::writeContents()
{
#ifdef _DEBUG
	LOG_DEBUG("writing contents to file: " + m_strFileName, "IPC");
#endif

	ofstream streamOut;
	streamOut.open(m_strFileName, ios_base::out | ios_base::trunc);

	// write standard header
	streamOut << "# this is the bans file" << endl;
	streamOut << "# it will be automatically updated through the TCT blockchain" << endl;
	streamOut << "# any manual changes will be overridden!" << endl << endl;

	// TODO: add date and time of generation of this file
	for (vector<CNetAddr>::iterator it = m_vecIP.begin(); it != m_vecIP.end(); it++)
		streamOut << (*it).ToStringIP() << endl;

	streamOut.close();

#ifdef _DEBUG
	LOG_DEBUG("done writing contents to file: " + m_strFileName, "IPC");
#endif
}