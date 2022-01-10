// Copyright 2015 by Raytheon BBN Technologies Corp.
// All Rights Reserved.
//
// Enable license-checking from command line

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/commandLineInterface/CommandLineInterface.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/FeatureModule.h"

#include "PerformanceTools/PerformanceToolsModule.h"

#include <gperftools/profiler.h>
#include <gperftools/heap-profiler.h>

struct PerformanceToolsCLIHook: public ModifyCommandLineHook {
	WhatNext run(int verbosity) {
		// Get output locations
		std::string experimentDir = ParamReader::getParam("experiment_dir");
		std::string cpuPerformanceFile = ParamReader::getParam("performance_tools_cpu_output_file", std::string(experimentDir + "/performance.cpu").c_str());
		std::string heapPerformancePrefix = ParamReader::getParam("performance_tools_heap_output_prefix", std::string(experimentDir + "/performance").c_str());
		std::cout << "PerformanceTools: cpu performance dump will be written to " << cpuPerformanceFile << std::endl;
		std::cout << "PerformanceTools: heap performance dump will be written to " << heapPerformancePrefix << std::endl;
		std::cout << std::endl;

		// Start profiling
		ProfilerStart(cpuPerformanceFile.c_str());
		HeapProfilerStart(heapPerformancePrefix.c_str());

		// Ready
		return CONTINUE;
	}

	void cleanup(int verbosity) {
		HeapProfilerStop();
		ProfilerStop();
	}
};

extern "C" DLL_PUBLIC void* setup_PerformanceTools() {
	addModifyCommandLineHook(boost::shared_ptr<PerformanceToolsCLIHook>(_new PerformanceToolsCLIHook()));
	return FeatureModule::setup_return_value();
}
