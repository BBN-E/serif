// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h" // This must be the first #include

#include <string>
#include <vector>
#include <list>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <map>
#include <set>

#include "Generic/common/FeatureModule.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/linuxPort/serif_port.h"
#include "Generic/common/ParamReader.h"

#include "StaticFeatureModules/StaticFeatureModules.h"

// Define preprocessor macros that abstract (most of) the differences
// between how DLLs are loaded in windows and unix.
#if defined(_WIN32)
	#include <windows.h>
	typedef HMODULE FeatureModuleHandle;
	#define LOAD_LIBRARY(filename) LoadLibrary(filename)
	#define GET_LIBRARY_FUNCTION(handle, name) GetProcAddress(handle, name)
	#define FREE_LIBRARY(handle) FreeLibrary(handle)
	#define SHLIB_PREFIX ""
	#define SHLIB_EXTENSION ".dll"
#else
	#include <dlfcn.h>
	typedef void* FeatureModuleHandle;
	#define LOAD_LIBRARY(filename) dlopen(filename, RTLD_NOW)
	#define GET_LIBRARY_FUNCTION(handle, name) dlsym(handle, name)
	#define FREE_LIBRARY(handle) dlclose(handle)
	#define SHLIB_PREFIX "lib"
	#define SHLIB_EXTENSION ".so"
#endif

namespace {
	// Structure used to keep track of info about feature modules.
	struct FeatureModuleInfo {
		std::string name;
		FeatureModule::module_setup_t setup_function;
		bool is_static;
		bool is_required;
		bool loaded;
		FeatureModuleHandle dll_handle; // undefined if is_static=true.

		FeatureModuleInfo(std::string name, FeatureModule::module_setup_t setup, bool required = false)
			: name(name), setup_function(setup), is_static(true), is_required(required), loaded(false) {}
						  
		FeatureModuleInfo(std::string name, FeatureModuleHandle dll_handle,
						  FeatureModule::module_setup_t setup, bool required = false)
			: name(name), dll_handle(dll_handle), 
			  setup_function(setup), is_static(false), is_required(required), loaded(false) {}
		FeatureModuleInfo()
			: name(), setup_function(0), is_static(true), is_required(false), loaded(false) {}
	};

	std::map<std::string, FeatureModuleInfo> &_moduleInfo() {
		static std::map<std::string, FeatureModuleInfo> module_info;
		return module_info;
	}

	std::list<std::string> getInitialSearchPath() {
		std::list<std::string> path;
		const char* env_path = getenv(FEATURE_MODULE_SEARCH_PATH_ENV_VAR);
		if (env_path && env_path[0])
			boost::split(path, env_path, boost::is_any_of(":;"));
		return path;
	}

	std::list<std::string> &getFeatureModuleSearchPath() {
		static std::list<std::string> path = getInitialSearchPath();
		return path;
	}
	
	bool fileExists(const std::string& filename) {
		return (!std::ifstream(filename.c_str()).fail());
	}

	std::string findFeatureModule(const char* name) {
		BOOST_FOREACH(std::string directory, getFeatureModuleSearchPath()) {
			boost::algorithm::trim(directory);
			if (directory.empty())
				directory += ".";
			if (directory[directory.length()-1] != SERIF_PATH_SEP[0])
				directory += SERIF_PATH_SEP;
			std::string filename = (directory + SHLIB_PREFIX + "Serif" + 
									name + SHLIB_EXTENSION);
			//std::cout << "check [" << filename << "]\n";
			if (fileExists(filename)) return filename;
		}
		std::stringstream err;
		err << "Unable to find module [Serif" << name << "].  Looked in:";
		BOOST_FOREACH(std::string directory, getFeatureModuleSearchPath()) {
			boost::algorithm::trim(directory);
			err << "\n    " << directory;
		}
		throw UnexpectedInputException("FeatureModule::load", err.str().c_str());
	}

	std::string getDLLError() {
#ifdef _WIN32
		char lpMsgBuf[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
					  FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL, GetLastError(),
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  lpMsgBuf, 1023, NULL );
		std::string result(lpMsgBuf);
		return result;
#else
		return dlerror();
#endif
	}

}

void FeatureModule::addToPath(const char* directory, bool front) {
	std::string dir_string(directory);
	boost::replace_all(dir_string, "/", SERIF_PATH_SEP);
	boost::replace_all(dir_string, "\\", SERIF_PATH_SEP);
	if (dir_string[dir_string.length()-1] != SERIF_PATH_SEP[0])
		dir_string += SERIF_PATH_SEP;
	if (front)
		getFeatureModuleSearchPath().push_front(dir_string);
	else
		getFeatureModuleSearchPath().push_back(dir_string);
}


void FeatureModule::load() {
	if (!ParamReader::isInitialized()) {
		throw InternalInconsistencyException("FeatureModule::load",
											 "ParamReader is not initialized!");
	}
	std::vector<std::string> path = 
		ParamReader::getStringVectorParam("module_path");
	BOOST_FOREACH(std::string dir, path) {
		addToPath(dir.c_str());
	}

	if (!ParamReader::hasParam("modules")) {
		std::ostringstream err;
		err << "Required param \"modules\" not defined.\n  "
			<< "Usually, this should at least contain a language "
			<< "module (eg English\n  or Arabic).  Use \"NONE\" "
			<< "if no feature modules should be loaded.";
		throw UnexpectedInputException("FeatureModule::load()", err.str().c_str());
	}
	
	// Get the basic modules list.
	std::vector<std::string> modules = 
		ParamReader::getStringVectorParam("modules");
	std::set<std::string> excludedModules;

	// Get any required modules (after making sure static modules are registered)
	load("none");
	std::vector<std::string> required_modules = requiredFeatureModules();
	modules.insert(modules.end(), required_modules.begin(), required_modules.end());

	// Individual parameters can be used for specific feature modules 
	// (eg "feature_module_BasicCipherStream: true").  These override
	// the "modules" list.
	typedef std::pair<std::string, std::string> StringPair;
	static const std::string feature_module_prefix("use_feature_module_");
	BOOST_FOREACH(const StringPair entry, ParamReader::getAllParams()) {
		if (boost::starts_with(entry.first, feature_module_prefix)) {
			std::string moduleName(entry.first.substr(feature_module_prefix.size()));
			if (ParamReader::isParamTrue(entry.first.c_str()))
				modules.push_back(moduleName);
			else if (std::find(required_modules.begin(), required_modules.end(), moduleName) == required_modules.end())
				excludedModules.insert(moduleName);
		}
	}

	BOOST_FOREACH(std::string module, modules) {
		if (!boost::iequals(module, "none")) {
			if (excludedModules.find(module) == excludedModules.end()) {
				//std::cerr << "  Using Module: [" << module << "]" << std::endl;
				load(module.c_str());
			}
		}
	}
}


void FeatureModule::load(const char* name) {
	// We automatically register any statically linked feature modules
	// the first time that load() is called.
	static bool static_feature_modules_registered = false;
	if (!static_feature_modules_registered) {
		registerStaticFeatureModules();
		static_feature_modules_registered = true;
	}

	if (boost::iequals(name, "none"))
		return;

	// If the module is not already registered, then search the path
	// for a DLL with the appropraite name.
	std::map<std::string, FeatureModuleInfo> &moduleInfo = _moduleInfo();
	if (moduleInfo.find(name) == moduleInfo.end()) {
		std::string filename = findFeatureModule(name);
		std::string setup_name = std::string("setup_")+name;
		module_setup_t setup = 0;       // pointer to setup method
		FeatureModuleHandle handle = 0; // handle for DLL

		try {
			// Load the library
			handle = LOAD_LIBRARY(filename.c_str());
			if (!handle) {
				std::stringstream err;
				err << "Cannot open library " << filename;
				throw UnexpectedInputException("FeatureModule::load",
											   err.str().c_str());
			}

			// Load the plugin's setup() function.
			setup = (module_setup_t) GET_LIBRARY_FUNCTION(handle, 
														  setup_name.c_str());

			if (!setup) {
				std::stringstream err;
				err << "Cannot load " << setup_name << " function in " 
					<< filename;
				throw UnexpectedInputException("FeatureModule::load",
											   err.str().c_str());
			}
		} catch(UnrecoverableException e) {
			if (handle) FREE_LIBRARY(handle);
			// Add any extra information we have about the error.
			std::ostringstream err;
			err << "While loading " << name << ": " << e.getMessage()
				<< ": " << getDLLError();
			throw UnrecoverableException(e.getSource(), err.str().c_str());
		}

		// Record the handle
		moduleInfo[name] = FeatureModuleInfo(name, handle, setup);
	}

	FeatureModuleInfo &module = moduleInfo[name];

	// If the module has already been loaded, then do nothing.
	if (module.loaded) return;

	// Otherwise, call its setup function, and set loaded=true.
	void *v = module.setup_function();
	//std::cout << "My v: " << (size_t)setup_return_value() << std::endl;
	//std::cout << "Your v: " << (size_t)v << std::endl;
	if (v != setup_return_value()) {
		std::ostringstream err;
		err << "While loading " << name << ": unexpected return value "
			<< "from setup function.  This probably indicates a "
			<< "linking issue.  In particular, feature module dlls must "
			<< "not include their own copy of the SERIF Generic library";
		throw UnrecoverableException("FeatureModule::load", err.str().c_str());
	}
	module.loaded = true;
}

void FeatureModule::cleanup() {
	std::map<std::string, FeatureModuleInfo> &moduleInfo = _moduleInfo();
	typedef std::pair<std::string, FeatureModuleInfo> FMIPair;
	BOOST_FOREACH(FMIPair fmi_pair, _moduleInfo()) {
		if (fmi_pair.second.loaded && !fmi_pair.second.is_static) {
			FREE_LIBRARY(fmi_pair.second.dll_handle);
			fmi_pair.second.loaded = false;
		}
	}
}

void FeatureModule::registerModule(const char *name, 
								   module_setup_t setup_function,
								   bool required) {
	std::map<std::string, FeatureModuleInfo> &moduleInfo = _moduleInfo();
	if (moduleInfo.find(name) == moduleInfo.end()) {
		//std::cout << "Registering module: \"" << name << "\"" << std::endl;
		moduleInfo[name] = FeatureModuleInfo(name, setup_function, required);
	} 
	else if (moduleInfo[name].setup_function != setup_function) {
		std::ostringstream err;
		err << "Attempt to register multiple (different) setup "
			<< "functions for feature module \"" << name << "\"";
		throw InternalInconsistencyException("FeatureModule::registerModule",
											 err.str().c_str());
	}
}

std::vector<std::string> FeatureModule::requiredFeatureModules() {
	std::vector<std::string> result;

	std::map<std::string, FeatureModuleInfo> &moduleInfo = _moduleInfo();
	typedef std::pair<std::string, FeatureModuleInfo> FMIPair;
	BOOST_FOREACH(FMIPair fmi_pair, _moduleInfo()) {
		if (fmi_pair.second.is_required)
			result.push_back(fmi_pair.second.name);
	}
	return result;
}

std::vector<std::string> FeatureModule::loadedFeatureModules() {
	std::vector<std::string> result;

	std::map<std::string, FeatureModuleInfo> &moduleInfo = _moduleInfo();
	typedef std::pair<std::string, FeatureModuleInfo> FMIPair;
	BOOST_FOREACH(FMIPair fmi_pair, _moduleInfo()) {
		if (fmi_pair.second.loaded)
			result.push_back(fmi_pair.second.name);
	}
	return result;
}

std::vector<std::string> FeatureModule::registeredFeatureModules() {
	load("none"); // ensures that static modules have been registered.
	std::vector<std::string> result;
	std::map<std::string, FeatureModuleInfo> &moduleInfo = _moduleInfo();
	typedef std::pair<std::string, FeatureModuleInfo> FMIPair;
	BOOST_FOREACH(FMIPair fmi_pair, _moduleInfo())
		result.push_back(fmi_pair.second.name);
	return result;
}

/** Return the address of a new (function-local) static variable.  The
 * FeatureModule::load() function uses this to do a sanity check that
 * makes sure that the feature module doesn't have its own copy of the
 * generic library. */
void* FeatureModule::setup_return_value() {
	static int v = 0;
	return &v;
}
