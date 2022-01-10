#include "Generic/common/leak_detection.h"
#include <fstream>
#include "PassageDumper.h"
#include "Passage.h"
#include "PassageDescription.h"
#include "ProblemDefinition.h"

using std::string;
using std::wstring;
using std::vector;
using std::wostream;
using std::wofstream;
using std::setprecision;
using std::endl;

void PassageDumper::dump(const Passage& passage) 
{
	PassageDescription_ptr desc = passage.description();
	
	if (desc) {
		out << L"\t\t<div class = 'passage'>" << endl;
		out << L"\t\t\t<h3>" << desc->docid() << L"( " << desc->startSentence() << L", "
			<< desc->endSentence() << L")</h3>" << endl;

		out << L"Type probs: ";
		outputMarginals(passage.relationProbs());
		out << endl;

		out << L"<div class='passageText'>" << desc->passage() << L"</div>" << endl;
		out << L"</div>" << endl;
	} else {
		out << "Cannot find passage description for dumping" << endl;
	}
}

void PassageDumper::outputMarginals(const std::vector<double>& marginals)
{
	assert(_varVals.size() == marginals.size());
	bool first = true;
	for (unsigned int i=0; i<marginals.size(); ++i) {
		if (!first) {
			out << L" / ";
		} else {
			first = false;
		}
		out << _varVals[i] << L"=" << setprecision(2) 
			<< L"<b>" << 100.0* marginals[i] << L"</b>; ";
	}
}

