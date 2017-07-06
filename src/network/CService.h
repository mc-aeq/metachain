#pragma once
/*
Copyright (c) 2009-2016 The Bitcoin Core developers
based on a version of 07/2017, heavily modified through the TCT developers
*/


/*
A combination of a network address (CNetAddr) and a (TCP) port
*/
class CService : public CNetAddr
{
	protected:
		unsigned short					port; // host order

	public:
										CService();
										CService(const CNetAddr& ip, unsigned short port);
										CService(const struct in_addr& ipv4Addr, unsigned short port);
										CService(const struct sockaddr_in& addr);
										CService(const struct in6_addr& ipv6Addr, unsigned short port);
										CService(const struct sockaddr_in6& addr);
										CService(const char *caHost, unsigned short usPort);

		void							Init();
		void							SetPort(unsigned short portIn);
		unsigned short					GetPort() const;
		bool							GetSockAddr(struct sockaddr* paddr, socklen_t *addrlen) const;
		bool							SetSockAddr(const struct sockaddr* paddr);

		friend bool						operator==(const CService& a, const CService& b);
		friend bool						operator!=(const CService& a, const CService& b);
		friend bool						operator<(const CService& a, const CService& b);

		std::vector<unsigned char>		GetKey() const;
		std::string						toString() const;
		std::string						toStringPort() const;
		std::string						toStringIPPort() const;
};