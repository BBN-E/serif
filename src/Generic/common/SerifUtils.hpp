// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_UTILS_H
#define SERIF_UTILS_H

#include "Generic/driver/SessionProgram.h"

#if defined(_WIN32)
	#include <direct.h>
#endif

#include "Generic/linuxPort/serif_port.h"
#include "Generic/common/OutputUtil.h"

namespace serif {

class SerifUtils
{
public:

	static void createNecessarySerifDirectories(SessionProgram& session)
    {
      // make sure parent experiment dir, experiment dir, and subdirs exist
		
      wstring experimentDir(session.getExperimentDir());
      size_t index = experimentDir.find_last_of(LSERIF_PATH_SEP);
	  OutputUtil::makeDir(experimentDir.substr(0, index).c_str());
      OutputUtil::makeDir(experimentDir.c_str());
	  if (session.getDumpDir() != 0)
		OutputUtil::makeDir(session.getDumpDir());
      if (session.getOutputDir() != 0)
        OutputUtil::makeDir(session.getOutputDir());
    }

};

};
#endif
