#ifndef _PASSAGE_DUMPER_H_
#define _PASSAGE_DUMPER_H_

#include <string>
#include <vector>
#include <iostream>
#include "Generic/common/bsp_declare.h"
#include "GraphDumper.h"

class Passage;

class PassageDumper : public GraphDumper<Passage> {
	public:
		PassageDumper(const std::wstring& title, const std::string& directory,
				const std::string& filename,
				const std::vector<std::wstring>& varVals) 
			: GraphDumper<Passage>(title, directory, filename), _varVals(varVals) {}
	private:
		std::vector<std::wstring> _varVals;
		void dump(const Passage& passage);
		void outputMarginals(const std::vector<double>& marginals);
};

#endif

