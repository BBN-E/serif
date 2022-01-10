// Copyright 2015 by Raytheon BBN Technologies Corp.
// All Rights Reserved.
//
// Enable license-checking from command line

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/commandLineInterface/CommandLineInterface.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/UTF8InputStream.h"

#ifndef _WIN32
#include <errno.h>
#endif

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "License/LicenseModule.h"
#include "License/nemesis_lic/license.hpp"

struct LicenseCLIHook: public ModifyCommandLineHook {
	WhatNext run(int verbosity) {
		FILE* license_fp = NULL;
		try {
			std::string license_file = ParamReader::getRequiredParam("license_file");
			license_fp = fopen(license_file.c_str(), "rb");
			if (license_fp == NULL) {
				std::stringstream err;
				err << "Could not open license file '" << license_file << "': " << std::strerror(errno);
				throw std::runtime_error(err.str());
			}
			std::string license_md5sum = nemesis_lic::verify_license(license_fp, "Serif");
			fclose(license_fp);
			UTF8InputStream::registerFileOpen(license_file.c_str());

			// Update restriction parameters if any
			std::string restrictions = nemesis_lic::get_restrictions();
			std::vector<std::string> lines;
			boost::split(lines, restrictions, boost::is_any_of("\n"));
			BOOST_FOREACH(std::string line, lines) {
				std::string key, value;
				if (ParamReader::parseParamLine(line.c_str(), key, value)) {
					ParamReader::setParam(key.c_str(), value.c_str());
				}
			}

			return CONTINUE;
		} catch (std::exception & e) {
			if (license_fp != NULL)
				fclose(license_fp);
			SessionLogger::err("license") << "Could not verify license: " << e.what();
			return FAILURE;
		}
	}
};

extern "C" DLL_PUBLIC void* setup_License() {
	addModifyCommandLineHook(boost::shared_ptr<LicenseCLIHook>(_new LicenseCLIHook()));
	return FeatureModule::setup_return_value();
}
