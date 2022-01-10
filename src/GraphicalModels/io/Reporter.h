#ifndef _REPORTER_H_
#define _REPORTER_H_

#include <iostream>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "Generic/common/ParamReader.h"
#include "../DataSet.h"

namespace GraphicalModel {

template <class GraphType, class Problem, class DataDumper, class ModDump>	
class Reporter {
public:
	Reporter(const std::string& dir, const boost::shared_ptr<Problem>& prob) 
		: _dir(dir), _prob(prob)
	{
		createDirectoryIfNeeded(dir);
	}

	void postE(unsigned int i, double ll, const DataSet<GraphType>& data) const {
		std::wcout << L"Iteration " << i << L", LL=" << ll << std::endl;
		if (ParamReader::isParamTrue("dump_after_each_em_iteration")) {
			postE(boost::lexical_cast<std::string>(i), data);
		}
	}

	void postE(const std::string& subdir, const DataSet<GraphType>& data) const {
		std::stringstream outputLoc;
		outputLoc << _dir << "/" << subdir;
		createDirectoryIfNeeded(outputLoc.str());

		const unsigned int BATCH_SIZE = 100;
		for (size_t batch = 0; 100*batch < data.nGraphs(); ++batch) {
			std::vector<boost::shared_ptr<GraphType> > toPrint;
			for (size_t graph = 100*batch; graph < data.nGraphs() && 
					graph < 100*(batch+1); ++graph) 
			{
				toPrint.push_back(data.graphs[graph]);
			}
			outputLoc << "/";

			std::stringstream outputFilename;
			outputFilename << "postE." << batch;

			DataDumper dumper(L"Unsupervised Events Dump", outputLoc.str(), 
					outputFilename.str(), _prob);
			dumper.dumpAll(toPrint);
		}
	}

	boost::shared_ptr<ModDump> modelDumper(unsigned int i) const {
		return modelDumper(boost::lexical_cast<std::string>(i));
	}

	boost::shared_ptr<ModDump> modelDumper(const std::string& subdir) const {
		std::stringstream outputLoc;
		outputLoc << _dir << "/" << subdir;
		createDirectoryIfNeeded(outputLoc.str());
		outputLoc << "/";

		return boost::make_shared<ModDump>(L"model dump", outputLoc.str(), "postM");
	}
private:
	std::string _dir;
	boost::shared_ptr<Problem> _prob;

	static void createDirectoryIfNeeded(const std::string& p) {
		if (!boost::filesystem::is_directory(p)) {
			boost::filesystem::create_directory(p);
		}
	}
};

};

#endif

