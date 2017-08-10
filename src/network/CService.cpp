#include "CService.h"

#include <assert.h>

#include "CNetAddr.h"
#include "../tinyformat.h"

void CService::Init()
{
	port = 0;
}

CService::CService()
{
	Init();
}

CService::CService(const CNetAddr& cip, unsigned short portIn) : CNetAddr(cip), port(portIn)
{
}

CService::CService(const struct in_addr& ipv4Addr, unsigned short portIn) : CNetAddr(ipv4Addr), port(portIn)
{
}

CService::CService(const struct in6_addr& ipv6Addr, unsigned short portIn) : CNetAddr(ipv6Addr), port(portIn)
{
}

CService::CService(const struct sockaddr_in& addr) : CNetAddr(addr.sin_addr), port(ntohs(addr.sin_port))
{
	assert(addr.sin_family == AF_INET);
}

CService::CService(const struct sockaddr_in6 &addr) : CNetAddr(addr.sin6_addr, addr.sin6_scope_id), port(ntohs(addr.sin6_port))
{
	assert(addr.sin6_family == AF_INET6);
}

CService::CService(const char *caHost, unsigned short usPort)
{
	init(caHost);
	port = usPort;
}

bool CService::SetSockAddr(const struct sockaddr *paddr)
{
	switch (paddr->sa_family) {
	case AF_INET:
		*this = CService(*(const struct sockaddr_in*)paddr);
		return true;
	case AF_INET6:
		*this = CService(*(const struct sockaddr_in6*)paddr);
		return true;
	default:
		return false;
	}
}

unsigned short CService::GetPort() const
{
	return port;
}

bool operator==(const CService& a, const CService& b)
{
	return (CNetAddr)a == (CNetAddr)b && a.port == b.port;
}

bool operator!=(const CService& a, const CService& b)
{
	return (CNetAddr)a != (CNetAddr)b || a.port != b.port;
}

bool operator<(const CService& a, const CService& b)
{
	return (CNetAddr)a < (CNetAddr)b || ((CNetAddr)a == (CNetAddr)b && a.port < b.port);
}

bool CService::GetSockAddr(struct sockaddr* paddr, socklen_t *addrlen) const
{
	if (IsIPv4()) {
		if (*addrlen < (socklen_t)sizeof(struct sockaddr_in))
			return false;
		*addrlen = sizeof(struct sockaddr_in);
		struct sockaddr_in *paddrin = (struct sockaddr_in*)paddr;
		memset(paddrin, 0, *addrlen);
		if (!GetInAddr(&paddrin->sin_addr))
			return false;
		paddrin->sin_family = AF_INET;
		paddrin->sin_port = htons(port);
		return true;
	}
	if (IsIPv6()) {
		if (*addrlen < (socklen_t)sizeof(struct sockaddr_in6))
			return false;
		*addrlen = sizeof(struct sockaddr_in6);
		struct sockaddr_in6 *paddrin6 = (struct sockaddr_in6*)paddr;
		memset(paddrin6, 0, *addrlen);
		if (!GetIn6Addr(&paddrin6->sin6_addr))
			return false;
		paddrin6->sin6_scope_id = scopeId;
		paddrin6->sin6_family = AF_INET6;
		paddrin6->sin6_port = htons(port);
		return true;
	}
	return false;
}

std::vector<unsigned char> CService::GetKey() const
{
	std::vector<unsigned char> vKey;
	vKey.resize(18);
	memcpy(&vKey[0], ip, 16);
	vKey[16] = port / 0x100;
	vKey[17] = port & 0x0FF;
	return vKey;
}

std::string CService::toStringPort() const
{
	return strprintf("%u", port);
}

std::string CService::toStringIPPort() const
{
	if (IsIPv4()) {
		return toStringIP() + ":" + toStringPort();
	}
	else {
		return "[" + toStringIP() + "]:" + toStringPort();
	}
}

std::string CService::toString() const
{
	return toStringIPPort();
}

void CService::SetPort(unsigned short portIn)
{
	port = portIn;
}