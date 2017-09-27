/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "mcTransaction.h"

namespace MCP04
{
	namespace MetaChain
	{
		mcTransaction::mcTransaction()
			: Transaction(CURRENT_MC_TRANSACTION_VERSION)
		{
		}
	}
}