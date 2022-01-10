// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.
//
// Serif HTTP Server command-line interface

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/commandLineInterface/CommandLineInterface.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/FeatureModule.h"
#include "SerifHTTPServer/SerifHTTPServerModule.h"
#include "SerifHTTPServer/SerifHTTPServer.h"
#include "SerifHTTPServer/IOConnection.h"

#include <boost/foreach.hpp>

#include <iostream>
#include <fstream>

struct SerifHTTPServerCLIHook: public ModifyCommandLineHook {
	WhatNext run(int verbosity) { 
		bool is_http_server = 
			ParamReader::isParamTrue("run_http_server");
		bool is_subprocess_server = 
			ParamReader::isParamTrue("run_subprocess_server");
		if (is_http_server || is_subprocess_server) {
			int http_server_port = ParamReader::getOptionalIntParamWithDefaultValue("http_server_port", -1);
			std::string logdir = ParamReader::getParam("http_server_logdir");
			// No batch file for server mode.
			ParamReader::setParam("batch_file", ""); 
			// Run the server
			SerifHTTPServer server(http_server_port, verbosity+1, 
								   is_subprocess_server,
								   (logdir.empty()?0:logdir.c_str()));
			SerifHTTPServer::SERVER_EXIT_STATUS status = server.run();
			if (status == SerifHTTPServer::RESTART) {
				return RESTART;
			} else if (status == SerifHTTPServer::FAILURE) {
				return FAILURE;
			} else {
				return SUCCESS;
			}
		} else {
			return CONTINUE;
		}
	}

private:
	int http_server_port;
	std::string http_server_logdir;
	std::string parfile;
	bool http_server;
	bool subprocess_server;
};

extern "C" DLL_PUBLIC void* setup_SerifHTTPServer() {
	IOConnection::dupStdinAndStdout();
	addModifyCommandLineHook(boost::shared_ptr<SerifHTTPServerCLIHook>(
			_new SerifHTTPServerCLIHook()));
	return FeatureModule::setup_return_value();
}
