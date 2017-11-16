/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Trust.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

// declare the name
const std::string MCP02::Trust::m_strName = "TRUST";

// register this class for polymorphic exporting
BOOST_CLASS_EXPORT_GUID(MCP02::Trust, "MCP02::Trust")

namespace MCP02
{
}