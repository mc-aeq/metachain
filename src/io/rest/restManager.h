#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __RESTMANAGER_H__
#define __RESTMANAGER_H__ 1

/*
this class represents our REST api server. All requests that are sent to our REST api must be JSON encoded in order to get processed
*/

// remove U macro from cpprestsdk
//#define _TURN_OFF_PLATFORM_STRING 1
#define CPPREST_FORCE_HTTP_LISTENER_ASIO 1

#include <cpprest/uri.h>
#include <cpprest/uri_builder.h>
#include <cpprest/json.h>
#include <cpprest/http_listener.h>

#include "../../network/CService.h"

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

		// single command handlers _POST
		void													processPostExample(web::http::http_request req);

	public:
																restManager( CService IP, bool bSSL );
																~restManager();
		bool													init();
};
#endif