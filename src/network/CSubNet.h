#pragma once
/*
Copyright (c) 2009-2016 The Bitcoin Core developers
based on a version of 07/2017, heavily modified through the TCT developers
*/

/*
Subnet class
*/
class CSubNet
{
	protected:		
		CNetAddr				network;		// Network (base) address		
		uint8_t					netmask[16];	// Netmask, in network byte order		
		bool					valid;			// Is this value valid? (only used to signal parse errors)

	public:
								CSubNet();
								CSubNet(const CNetAddr &addr, int32_t mask);
								CSubNet(const CNetAddr &addr, const CNetAddr &mask);
								explicit CSubNet(const CNetAddr &addr);	//constructor for single ip subnet (<ipv4>/32 or <ipv6>/128)


		bool					Match(const CNetAddr &addr) const;

		std::string				ToString() const;
		bool					IsValid() const;

		friend bool				operator==(const CSubNet& a, const CSubNet& b);
		friend bool				operator!=(const CSubNet& a, const CSubNet& b);
		friend bool				operator<(const CSubNet& a, const CSubNet& b);
};