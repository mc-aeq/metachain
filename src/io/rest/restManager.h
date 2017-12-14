#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __RESTMANAGER_H__
#define __RESTMANAGER_H__ 1

// force async IO to enable the http listener
#define CPPREST_FORCE_HTTP_LISTENER_ASIO 1

#include <cpprest/uri.h>
#include <cpprest/uri_builder.h>
#include <cpprest/json.h>
#include <cpprest/http_listener.h>

#include "../../network/CService.h"

/**
@brief This is the REST API server. More development information are in the <em>Detailed Descriptions</em>

This class handles all external requests. The default listening URI is <em>http://127.0.0.1:10016/api/</em>\n
Every function either uses POST or GET to retrieve possible parameters. Use the corresponding request method, otherwise your request will result in an error.\n
All parameters must be JSON encoded arrays and return values will also always be JSON encoded arrays.\n\n

@note For standardized documentation purposes, every function call will be documented as follows:\n
\b Method: POST or GET\n
\b URI: <em>path that needs to be called</em>\n
\b Parameters: <em>list of params</em>\n
\b Returns: <em>list of return values on success, key => value</em>\n

@warning If a request method doesn't match the documented standard (POST, GET), params are missing or invalid, the status code will contain the corresponding error number and the body of the document will be a description of the error itself. This value is not JSON encoded, it's sent plain.

*/
class restManager
{
	private:
		bool													m_bInitialized;

		CService												m_IP;
		bool													m_bSSL;
		web::http::experimental::listener::http_listener		*m_pSocket;

		// function handlers
		void													handleRequest(web::http::http_request req);
		static void												handleError(pplx::task<void>& t);

		// single command handlers _GET
		void													processVersion(web::http::http_request *req);
		void													processTime(web::http::http_request *req);
		void													processInfo(web::http::http_request *req);
		void													processGetAllSubchains(web::http::http_request *req);

		// single command handlers _POST
		void													processMetachainHeight(web::http::http_request req);

		// don't allow copy constructors
																restManager(restManager const& copy);	// not implemented
		restManager&											operator=(restManager const& copy);		// not implemented

	public:
																restManager( CService IP, bool bSSL );
																~restManager();
		bool													init();
};
#endif