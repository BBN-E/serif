#ifndef _ACE_EVENT_DUMPER_
#define _ACE_EVENT_DUMPER_ 

#include <string>
#include <vector>
#include <iostream>
#include "Generic/common/bsp_declare.h"
#include "GraphDumper.h"

class ACEEvent;
BSP_DECLARE(ProblemDefinition);

class ACEEventDumper : public GraphDumper<ACEEvent> {
	public:
		ACEEventDumper(const std::wstring& title, const std::string& directory,
				const std::string& filename, const ProblemDefinition_ptr& problem);
	private:
		void dump(const ACEEvent& passage);
		ProblemDefinition_ptr _problem;
};

class ACEModelDumper : public ModelDumper<ACEEvent> {
	public:
		ACEModelDumper(const std::wstring & title, const std::string& directory,
				const std::string& filename);
};

#endif

