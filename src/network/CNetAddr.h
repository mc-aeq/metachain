#pragma once
/*
Copyright (c) 2009-2016 The Bitcoin Core developers
based on a version of 07/2017, heavily modified through the TCT developers
*/

#ifndef __CNETADDR_H__
#define __CNETADDR_H__ 1

#include <stdint.h>
#include <string>

enum Network
{
	NET_UNROUTABLE = 0,
	NET_IPV4,
	NET_IPV6,

	NET_MAX,
};

/*
IP address (IPv6, or IPv4 using mapped IPv6 range (::FFFF:0:0/96))
*/
class CNetAddr
{
	protected:
		unsigned char					ip[16];		// in network byte order
		uint32_t						scopeId;	// for scoped/link-local ipv6 addresses

	public:
										CNetAddr();
										CNetAddr(const struct in_addr& ipv4Addr);
										CNetAddr(const struct in6_addr& pipv6Addr, const uint32_t scope = 0);
		bool							init();
		bool							init(std::string strEntry);
		void							SetIP(const CNetAddr& ip);

		/**
		* Set raw IPv4 or IPv6 address (in network byte order)
		* @note Only NET_IPV4 and NET_IPV6 are allowed for network.
		*/
		void							SetRaw(Network network, const uint8_t *data);
		void							SetRaw(const uint8_t *data);

		bool							IsIPv4() const;    // IPv4 mapped address (::FFFF:0:0/96, 0.0.0.0/0)
		bool							IsIPv6() const;    // IPv6 address (not mapped IPv4, not Tor)
		bool							IsRFC1918() const; // IPv4 private networks (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12)
		bool							IsRFC2544() const; // IPv4 inter-network communications (192.18.0.0/15)
		bool							IsRFC6598() const; // IPv4 ISP-level NAT (100.64.0.0/10)
		bool							IsRFC5737() const; // IPv4 documentation addresses (192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24)
		bool							IsRFC3849() const; // IPv6 documentation address (2001:0DB8::/32)
		bool							IsRFC3927() const; // IPv4 autoconfig (169.254.0.0/16)
		bool							IsRFC3964() const; // IPv6 6to4 tunnelling (2002::/16)
		bool 							IsRFC4193() const; // IPv6 unique local (FC00::/7)
		bool 							IsRFC4380() const; // IPv6 Teredo tunnelling (2001::/32)
		bool							IsRFC4843() const; // IPv6 ORCHID (2001:10::/28)
		bool							IsRFC4862() const; // IPv6 autoconfig (FE80::/64)
		bool							IsRFC6052() const; // IPv6 well-known prefix (64:FF9B::/96)
		bool							IsRFC6145() const; // IPv6 IPv4-translated address (::FFFF:0:0:0/96)
		bool							IsLocal() const;
		bool							IsRoutable() const;
		bool							IsValid() const;
		enum Network					GetNetwork() const;
		std::string						toString() const;
		std::string						toStringIP() const;
		unsigned char					GetByte(int n) const;
		unsigned char*					GetBytes() { return ip; };
		uint64_t						GetHash() const;
		bool							GetInAddr(struct in_addr* pipv4Addr) const;
		int								GetReachabilityFrom(const CNetAddr *paddrPartner = NULL) const;		
		bool							GetIn6Addr(struct in6_addr* pipv6Addr) const;

		bool							operator==(const CNetAddr& rhs) const { return (memcmp(this->ip, rhs.ip, 16) == 0); };
		friend bool						operator!=(const CNetAddr& a, const CNetAddr& b);
		friend bool						operator<(const CNetAddr& a, const CNetAddr& b);

		friend class CSubNet;
};

#endif