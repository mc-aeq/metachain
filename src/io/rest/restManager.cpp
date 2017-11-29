/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "restManager.h"
#include "../../logger.h"

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
		if (vecPaths[0] == U("test"))
			processPostExample(req);
		else
			req.reply(web::http::status_codes::NotFound, U("API POST Method not found"));
	}
	else
		req.reply(web::http::status_codes::BadRequest, U("Not supported http_request type"));
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

void restManager::processVersion(web::http::http_request *req)
{
	// assemble json response
	web::json::value obj;
	obj[L"version"] = web::json::value::number(g_cuint32tVersion);

	// send response
	req->reply(web::http::status_codes::OK, obj);
}

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

void restManager::processPostExample(web::http::http_request req)
{
	req.extract_json().then([=](const web::json::value& obj)
	{
		// posting back the same json as received for testing purposes
		req.reply(web::http::status_codes::OK, obj);

		// we're done processing, return needs to be here otherwise we create a second reply statement
		return;
	}).then([](pplx::task<void> t) { handleError(t); }).wait();

	// fallback with error message. we reach this line only when the json can't be decoded and the handleError(t) function was called
	req.reply(web::http::status_codes::UnprocessableEntity, U("Unable to extract JSON data from POST request"));
}