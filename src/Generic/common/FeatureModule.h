// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FEATURE_MODULE_H
#define FEATURE_MODULE_H

#define FEATURE_MODULE_SEARCH_PATH_ENV_VAR "SERIF_FEATURE_MODULE_PATH"

#include <vector>
#include <string>

/**
 * A "feature module" is a plugin DLL that can be loaded by SERIF at
 * run time.  It modifies one or more aspects of SERIF's behavior by
 * registering new classes or functions with pre-defined hooks.
 *
 * Each feature module has a globally unique name, such as
 * "CipherStream", which is used to find the DLL file and to compute
 * the name of the feature module's setup function.
 * 
 * Currently, we do not support "unloading" feature modules.
 */
class FeatureModule {
public:
	/** Add any directories specified by the 'module_path'
	 * parameter to the feature module path; and then load any modules
	 * that are specified by the 'modules' parameter.  Both parameters
	 * should contain comma-separated lists.
	 *
	 * This method must only be called *after* the ParamReader is
	 * initialized. */
	static void load();

	/** Find and load the feature module with a given name.  The
	 * module is found by searching the "feature module path". */
	static void load(const char* name);

	/** Add a new directory to the feature module path. */
	static void addToPath(const char* directory, bool front=false);

	/** Unload all feature modules.  This should only be called
	 * after you are sure that the classes, functions, etc, that are
	 * defined by the feature modules will not be used any more.  At
	 * this point, it's probably safer to just never all this method,
	 * unless you're doing something like memory leak detection.*/
	static void cleanup();

	/** Type declaration for module setup functions */
	typedef void* (*module_setup_t)();

	/** Register a statically-linked feature module.  After this is
	 * called, any subsequent calls to FeatureModule::load(name) 
	 * will call the specified setup function, rather than searching
	 * for a DLL feature module. */
	static void registerModule(const char *name, 
							   module_setup_t setup_function,
							   bool required = false);
	
	/** Return a list of the names of all feature modules that have
	 * been loaded so far. */
	static std::vector<std::string> loadedFeatureModules();

	/** Return a list of the names of all feature modules that
	 * are required. */
	static std::vector<std::string> requiredFeatureModules();

	static std::vector<std::string> registeredFeatureModules();

	/** All setup functions should return this value; it is used to 
	 * perform a sanity check. */
	static void* setup_return_value();

private:
	FeatureModule(); // This class is not meant to be instantiated.
};

#endif
