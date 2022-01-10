// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CLEANUP_HOOKS_H
#define CLEANUP_HOOKS_H

typedef void (*CleanupFunction)();

/** Register a cleanup function that should be called when 
 * callAllCleanupHooks is called.  
 *
 * Warning: it is up to the discression of the main binary to call
 * runCleanupHooks(), and there is no guarantee that these hooks will
 * ever be run.  In particular, they will probably only get run if
 * ENABLE_LEAK_DETECTION is turned on.
 */
void addCleanupHook(CleanupFunction hook);

/** Run all registered cleanup hooks. */
void runCleanupHooks();

#endif
