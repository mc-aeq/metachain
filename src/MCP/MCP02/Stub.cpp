/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Stub.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

// declare the name
const std::string MCP02::Stub::m_strName = "Stub";

// register this class for polymorphic exporting
BOOST_CLASS_EXPORT_GUID(MCP02::Stub, "MCP02::Stub")

namespace MCP02
{
}