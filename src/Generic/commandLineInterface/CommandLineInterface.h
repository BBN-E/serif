// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COMMAND_LINE_INTERFACE_H
#define COMMAND_LINE_INTERFACE_H

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

int runSerifFromCommandLine(int argc, const char **argv);

// Run unit tests.
// EDL: In the long term, I don't think that the serif command line is the right place for this, but I guess it's ok for now. 
int runTests();

// Expose this for use in other entry points
class GenericTimer;
class DocumentDriver;
void reportProfilingInformation(const GenericTimer &totalLoadTimer,
								const GenericTimer &totalProcessTimer,
								const DocumentDriver &documentDriver);

/** Hook that can be used by feature modules to replace the 
 * command-line interface's default behavior. */
struct ModifyCommandLineHook {

	enum WhatNext {CONTINUE, SUCCESS, FAILURE, RESTART};
	
	/** Hook for overriding the default command-line "run" behavior.
	 * This is called just before the command-line would create a
	 * document driver and use it to process the specified documents.
	 *
	 * The hook's return value indicates what the command line
	 * interface should do next:
	 *
	 *  - CONTINUE: continue processing as normal (i.e., load a
	 *    document driver and process the specified documents).
	 *  - SUCCESS: exit with a zero returncode.
	 *  - FAILURE: exit with a nonzero returncode.
	 *  - RESTART: restart the process (using exec).
	 */
	virtual WhatNext run(int verbosity) { return CONTINUE; }

	virtual void cleanup(int verbosity) { }
};
typedef boost::shared_ptr<ModifyCommandLineHook> ModifyCommandLineHook_ptr;

// Add a new hook.  Multiple hooks may be added.
void addModifyCommandLineHook(ModifyCommandLineHook_ptr hook);

#endif
