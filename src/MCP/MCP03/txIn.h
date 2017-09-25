#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TXIN_H__
#define __MCP03_TXIN_H__ 1

#include <string>
#include "txOutRef.h"

namespace MCP03
{
	class txIn
	{
		private:
			txOutRef									m_txPrev;
			std::string									m_strSignature;

		public:

														txIn();
														~txIn();
				
			// simple getter and setter
			std::string									toString();
			txOutRef*									getTxOutRef() { return &m_txPrev; };

			// operators
			bool										operator==(const txIn& ref);
			bool										operator!=(const txIn& ref);
	};
}
#endif