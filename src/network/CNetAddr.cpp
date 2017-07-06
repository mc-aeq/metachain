#include "../../stdafx.h"

static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

void CNetAddr::Init()
{
	memset(ip, 0, sizeof(ip));
	scopeId = 0;
}

void CNetAddr::SetIP(const CNetAddr& ipIn)
{
	memcpy(ip, ipIn.ip, sizeof(ip));
}

void CNetAddr::SetRaw(Network network, const uint8_t *ip_in)
{
	switch (network)
	{
	case NET_IPV4:
		memcpy(ip, pchIPv4, 12);
		memcpy(ip + 12, ip_in, 4);
		break;
	case NET_IPV6:
		memcpy(ip, ip_in, 16);
		break;
	default:
		assert(!"invalid network");
	}
}


CNetAddr::CNetAddr()
{
	Init();
}

CNetAddr::CNetAddr(const struct in_addr& ipv4Addr)
{
	SetRaw(NET_IPV4, (const uint8_t*)&ipv4Addr);
}

CNetAddr::CNetAddr(const struct in6_addr& ipv6Addr, const uint32_t scope)
{
	SetRaw(NET_IPV6, (const uint8_t*)&ipv6Addr);
	scopeId = scope;
}

unsigned int CNetAddr::GetByte(int n) const
{
	return ip[15 - n];
}

bool CNetAddr::IsIPv4() const
{
	return (memcmp(ip, pchIPv4, sizeof(pchIPv4)) == 0);
}

bool CNetAddr::IsIPv6() const
{
	return (!IsIPv4());
}

bool CNetAddr::IsRFC1918() const
{
	return IsIPv4() && (
		GetByte(3) == 10 ||
		(GetByte(3) == 192 && GetByte(2) == 168) ||
		(GetByte(3) == 172 && (GetByte(2) >= 16 && GetByte(2) <= 31)));
}

bool CNetAddr::IsRFC2544() const
{
	return IsIPv4() && GetByte(3) == 198 && (GetByte(2) == 18 || GetByte(2) == 19);
}

bool CNetAddr::IsRFC3927() const
{
	return IsIPv4() && (GetByte(3) == 169 && GetByte(2) == 254);
}

bool CNetAddr::IsRFC6598() const
{
	return IsIPv4() && GetByte(3) == 100 && GetByte(2) >= 64 && GetByte(2) <= 127;
}

bool CNetAddr::IsRFC5737() const
{
	return IsIPv4() && ((GetByte(3) == 192 && GetByte(2) == 0 && GetByte(1) == 2) ||
		(GetByte(3) == 198 && GetByte(2) == 51 && GetByte(1) == 100) ||
		(GetByte(3) == 203 && GetByte(2) == 0 && GetByte(1) == 113));
}

bool CNetAddr::IsRFC3849() const
{
	return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x0D && GetByte(12) == 0xB8;
}

bool CNetAddr::IsRFC3964() const
{
	return (GetByte(15) == 0x20 && GetByte(14) == 0x02);
}

bool CNetAddr::IsRFC6052() const
{
	static const unsigned char pchRFC6052[] = { 0,0x64,0xFF,0x9B,0,0,0,0,0,0,0,0 };
	return (memcmp(ip, pchRFC6052, sizeof(pchRFC6052)) == 0);
}

bool CNetAddr::IsRFC4380() const
{
	return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 && GetByte(12) == 0);
}

bool CNetAddr::IsRFC4862() const
{
	static const unsigned char pchRFC4862[] = { 0xFE,0x80,0,0,0,0,0,0 };
	return (memcmp(ip, pchRFC4862, sizeof(pchRFC4862)) == 0);
}

bool CNetAddr::IsRFC4193() const
{
	return ((GetByte(15) & 0xFE) == 0xFC);
}

bool CNetAddr::IsRFC6145() const
{
	static const unsigned char pchRFC6145[] = { 0,0,0,0,0,0,0,0,0xFF,0xFF,0,0 };
	return (memcmp(ip, pchRFC6145, sizeof(pchRFC6145)) == 0);
}

bool CNetAddr::IsRFC4843() const
{
	return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x10);
}

bool CNetAddr::IsLocal() const
{
	// IPv4 loopback
	if (IsIPv4() && (GetByte(3) == 127 || GetByte(3) == 0))
		return true;

	// IPv6 loopback (::1/128)
	static const unsigned char pchLocal[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 };
	if (memcmp(ip, pchLocal, 16) == 0)
		return true;

	return false;
}

bool CNetAddr::IsValid() const
{
	// Cleanup 3-byte shifted addresses caused by garbage in size field
	// of addr messages from versions before 0.2.9 checksum.
	// Two consecutive addr messages look like this:
	// header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
	// so if the first length field is garbled, it reads the second batch
	// of addr misaligned by 3 bytes.
	if (memcmp(ip, pchIPv4 + 3, sizeof(pchIPv4) - 3) == 0)
		return false;

	// unspecified IPv6 address (::/128)
	unsigned char ipNone6[16] = {};
	if (memcmp(ip, ipNone6, 16) == 0)
		return false;

	// documentation IPv6 address
	if (IsRFC3849())
		return false;
	
	if (IsIPv4())
	{
		// INADDR_NONE
		uint32_t ipNone = INADDR_NONE;
		if (memcmp(ip + 12, &ipNone, 4) == 0)
			return false;

		// 0
		ipNone = 0;
		if (memcmp(ip + 12, &ipNone, 4) == 0)
			return false;
	}

	return true;
}

bool CNetAddr::IsRoutable() const
{
	return IsValid() && !(IsRFC1918() || IsRFC2544() || IsRFC3927() || IsRFC4862() || IsRFC6598() || IsRFC5737() || (IsRFC4193()) || IsRFC4843() || IsLocal());
}

enum Network CNetAddr::GetNetwork() const
{
	if (!IsRoutable())
		return NET_UNROUTABLE;

	if (IsIPv4())
		return NET_IPV4;
	
	return NET_IPV6;
}

std::string CNetAddr::ToStringIP() const
{
	CService serv(*this, 0);
	struct sockaddr_storage sockaddr;
	socklen_t socklen = sizeof(sockaddr);
	if (serv.GetSockAddr((struct sockaddr*)&sockaddr, &socklen)) {
		char name[1025] = "";
		if (!getnameinfo((const struct sockaddr*)&sockaddr, socklen, name, sizeof(name), NULL, 0, NI_NUMERICHOST))
			return std::string(name);
	}
	if (IsIPv4())
		return strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
	else
		return strprintf("%x:%x:%x:%x:%x:%x:%x:%x",
			GetByte(15) << 8 | GetByte(14), GetByte(13) << 8 | GetByte(12),
			GetByte(11) << 8 | GetByte(10), GetByte(9) << 8 | GetByte(8),
			GetByte(7) << 8 | GetByte(6), GetByte(5) << 8 | GetByte(4),
			GetByte(3) << 8 | GetByte(2), GetByte(1) << 8 | GetByte(0));
}

std::string CNetAddr::ToString() const
{
	return ToStringIP();
}

bool operator==(const CNetAddr& a, const CNetAddr& b)
{
	return (memcmp(a.ip, b.ip, 16) == 0);
}

bool operator!=(const CNetAddr& a, const CNetAddr& b)
{
	return (memcmp(a.ip, b.ip, 16) != 0);
}

bool operator<(const CNetAddr& a, const CNetAddr& b)
{
	return (memcmp(a.ip, b.ip, 16) < 0);
}

bool CNetAddr::GetInAddr(struct in_addr* pipv4Addr) const
{
	if (!IsIPv4())
		return false;
	memcpy(pipv4Addr, ip + 12, 4);
	return true;
}

bool CNetAddr::GetIn6Addr(struct in6_addr* pipv6Addr) const
{
	memcpy(pipv6Addr, ip, 16);
	return true;
}

uint64_t CNetAddr::GetHash() const
{
	uint256 hash = Hash(&ip[0], &ip[16]);
	uint64_t nRet;
	memcpy(&nRet, &hash, sizeof(nRet));
	return nRet;
}

// private extensions to enum Network, only returned by GetExtNetwork,
// and only used in GetReachabilityFrom
static const int NET_UNKNOWN = NET_MAX + 0;
static const int NET_TEREDO = NET_MAX + 1;
int static GetExtNetwork(const CNetAddr *addr)
{
	if (addr == NULL)
		return NET_UNKNOWN;
	if (addr->IsRFC4380())
		return NET_TEREDO;
	return addr->GetNetwork();
}

/** Calculates a metric for how reachable (*this) is from a given partner */
int CNetAddr::GetReachabilityFrom(const CNetAddr *paddrPartner) const
{
	enum Reachability {
		REACH_UNREACHABLE,
		REACH_DEFAULT,
		REACH_TEREDO,
		REACH_IPV6_WEAK,
		REACH_IPV4,
		REACH_IPV6_STRONG,
		REACH_PRIVATE
	};

	if (!IsRoutable())
		return REACH_UNREACHABLE;

	int ourNet = GetExtNetwork(this);
	int theirNet = GetExtNetwork(paddrPartner);
	bool fTunnel = IsRFC3964() || IsRFC6052() || IsRFC6145();

	switch (theirNet) {
	case NET_IPV4:
		switch (ourNet) {
		default:       return REACH_DEFAULT;
		case NET_IPV4: return REACH_IPV4;
		}
	case NET_IPV6:
		switch (ourNet) {
		default:         return REACH_DEFAULT;
		case NET_TEREDO: return REACH_TEREDO;
		case NET_IPV4:   return REACH_IPV4;
		case NET_IPV6:   return fTunnel ? REACH_IPV6_WEAK : REACH_IPV6_STRONG; // only prefer giving our IPv6 address if it's not tunnelled
		}
	case NET_TEREDO:
		switch (ourNet) {
		default:          return REACH_DEFAULT;
		case NET_TEREDO:  return REACH_TEREDO;
		case NET_IPV6:    return REACH_IPV6_WEAK;
		case NET_IPV4:    return REACH_IPV4;
		}
	case NET_UNKNOWN:
	case NET_UNROUTABLE:
	default:
		switch (ourNet) {
		default:          return REACH_DEFAULT;
		case NET_TEREDO:  return REACH_TEREDO;
		case NET_IPV6:    return REACH_IPV6_WEAK;
		case NET_IPV4:    return REACH_IPV4;
		}
	}
}