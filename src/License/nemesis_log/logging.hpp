/*************************************************************************
 * Copyright 2008-2012 by Raytheon BBN Technologies.  All Rights Reserved
 *************************************************************************/

#ifndef CUBE2_NEMESIS_LOG_LOGGING_HPP
#define CUBE2_NEMESIS_LOG_LOGGING_HPP

#include "Generic/common/SessionLogger.h"

//////////////////////////////////////////////////////////
// Macros to wrap interface to SessionLogger
//////////////////////////////////////////////////////////

// -----------------------------------------------
// Map Nemesis log levels to SessionLogger levels
// -----------------------------------------------
#define logDEBUG dbg
#define logINFO info
#define logERROR err
#define logFATAL err

// -----------------------------------------------
// Log to a particular SessionLogger level
// -----------------------------------------------
#define NEMESIS_LOG(THE_LEVEL) SessionLogger::THE_LEVEL("license")

#endif
