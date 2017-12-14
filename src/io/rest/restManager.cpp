/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

// it's important that the MetaChain.h file is included first before the cpprestsdk files due to incompabilities of boost and the U macro from cpprestsdk. If cpprestsdk is included before boost, boost will throw a syntax error C2187 in type_traits.hpp(757)
#include "../../MetaChain.h"
#include "restManager.h"
#include "../../logger.h"
#include "../../tinyformat.h"

// define a macro for faster string extraction of JSON parameters
#define getString(x) utility::conversions::to_utf8string(obj.at(U(x)).as_string())

restManager::restManager(CService IP, bool bSSL)
		: m_bInitialized(false),
		m_IP(IP),
		m_bSSL(bSSL),
		m_pSocket(nullptr)
{
}

restManager::~restManager()
{
	m_pSocket->close().wait();
	RELEASE(m_pSocket);
}

bool restManager::init()
{
	if (m_bInitialized)
	{
		LOG_ERROR("REST API already initialized", "REST");
		return false;
	}
	LOG("initializing REST API", "REST");

	// assembling listening uri
	std::string strURI = "http";
	try
	{
		if (m_bSSL)
			strURI += "s";
		strURI += "://" + m_IP.toStringIPPort() + "/api";

		// convert the std::string to a wstring
		std::wstringstream ws;
		ws << strURI.c_str();

		web::uri uriAddress(ws.str());

		m_pSocket = new web::http::experimental::listener::http_listener(uriAddress);
	}
	catch (...)
	{
		LOG_ERROR("Unable to initialize URI object and listener. Not launching REST Services", "REST");
		return false;
	}

	// connect functions
#ifdef _DEBUG
	LOG_DEBUG("adding REST Handlers", "REST");
#endif
	m_pSocket->support(web::http::methods::GET, std::bind(&restManager::handleRequest, this, std::placeholders::_1));
	m_pSocket->support(web::http::methods::POST, std::bind(&restManager::handleRequest, this, std::placeholders::_1));

	// open socket
	m_pSocket->open().wait();
	LOG("REST API initialized - listening at " + strURI, "REST");

	m_bInitialized = true;
	return true;
}

void restManager::handleRequest(web::http::http_request req)
{
#ifdef _DEBUG
	LOG_DEBUG(req.to_string(), "REST");
#endif

	// we split up the calling path into a vector for easier processing
	auto vecPaths = web::http::uri::split_path(web::http::uri::decode(req.relative_uri().path()));

	// check if we have atleast one parameter to work with
	if (vecPaths.size() < 1)
	{
		req.reply(web::http::status_codes::BadRequest, U("Missing Request URI"));
		return;
	}

	try
	{
		// handle GET calls
		if (req.method() == U("GET"))
		{
			// processing the request
			if (vecPaths[0] == U("version"))
				processVersion(&req);
			else if (vecPaths[0] == U("time"))
				processTime(&req);
			else if (vecPaths[0] == U("info"))
				processInfo(&req);
			else
				req.reply(web::http::status_codes::NotFound, U("API GET Method not found"));
		}
		// handle POST calls
		else if (req.method() == U("POST"))
		{
			// metachain and subchain relevant basic commands
			if (vecPaths[0] == U("metachain") )
			{
				if( vecPaths[1] == U("height"))
					processMetachainHeight(req);
				else
					req.reply(web::http::status_codes::NotFound, U("API POST Method not found"));
			}
			else
				req.reply(web::http::status_codes::NotFound, U("API POST Method not found"));
		}
		else
			req.reply(web::http::status_codes::BadRequest, U("Not supported http_request type"));
	}
	catch (web::json::json_exception e)
	{
		req.reply(web::http::status_codes::PreconditionFailed, strprintf("JSON Exception: %s", e.what()) );
	}
}

void restManager::handleError(pplx::task<void>& t)
{
	try
	{
		t.get();
	}
	catch (web::http::http_exception e)
	{
#ifdef _DEBUG
		LOG_DEBUG("Task cancelled: " + (std::string)e.what(), "REST");
#endif
	}
}

/**
@brief get the version number\n
@detail
\b Method: GET\n
\b URI: /version\n
@param none
@return version => version number
*/
void restManager::processVersion(web::http::http_request *req)
{
	// assemble json response
	web::json::value obj;
	obj[L"version"] = web::json::value::number(g_cuint32tVersion);

	// send response
	req->reply(web::http::status_codes::OK, obj);
}

/**
@brief get the current time\n
@detail
\b Method: GET\n
\b URI: /time\n
@param none
@return time => unix timestamp
*/
void restManager::processTime(web::http::http_request *req)
{
	// prepare response
	std::time_t ts = std::time(nullptr);

	// assemble json response
	web::json::value obj;
	obj[L"time"] = web::json::value::number(ts);

	// send response
	req->reply(web::http::status_codes::OK, obj);
}

/**
@brief get general information\n
@detail
\b Method: GET\n
\b URI: /info\n
@param none
@return version => version number
@return time => unix timestamp
*/
void restManager::processInfo(web::http::http_request *req)
{
	// prepare response
	std::time_t ts = std::time(nullptr);

	// assemble json response
	web::json::value obj;
	obj[L"version"] = web::json::value::number(g_cuint32tVersion);
	obj[L"time"] = web::json::value::number(ts);

	// send response
	req->reply(web::http::status_codes::OK, obj);
}

/**
@brief get block height of different chains\n
@detail
\b Method: POST\n
\b URI: /metachain/height\n
@param identifier => string identifier of the targeted chain
@return height => block number
*/
void restManager::processMetachainHeight(web::http::http_request req)
{
	req.extract_json().then([=](const web::json::value& obj)
	{
		MCP02::SubChain *ptr = MetaChain::getInstance().getStorageManager()->getSubChainManager()->getSubChain(getString("identifier"));
		if (ptr)
		{
			// assemble json response
			web::json::value ret;
			ret[L"height"] = web::json::value::number(ptr->getHeight());

			// send response
			req.reply(web::http::status_codes::OK, ret);
		}
		else
			req.reply(web::http::status_codes::BadRequest, U("Unable to find chain with the specified identifier"));
		
		// we're done processing, return needs to be here otherwise we create a second reply statement
		return;
	}).then([](pplx::task<void> t) { handleError(t); }).wait();

	// fallback with error message. we reach this line only when the json can't be decoded and the handleError(t) function was called
	req.reply(web::http::status_codes::UnprocessableEntity, U("Unable to extract JSON data from POST request"));
}