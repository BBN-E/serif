#include "Generic/common/leak_detection.h"
#include <fstream>
#include <vector>
#include <string>
#include <boost/foreach.hpp>
#include "ACEEventDumper.h"
#include "ACEEvent.h"
#include "ACEPassageDescription.h"
#include "ProblemDefinition.h"
#include "GraphDumper.h"
#include "ACEEntityFactorGroup.h"
#include "GraphicalModels/pr/Constraint.h"

using std::string;
using std::wstring;
using std::vector;
using std::wostream;
using std::wofstream;
using std::setprecision;
using std::endl;

ACEEventDumper::ACEEventDumper(const std::wstring& title, const std::string& directory,
		const std::string& filename, const ProblemDefinition_ptr& problem) 
	: GraphDumper<ACEEvent>(title, directory, filename) , _problem(problem)
{
	out << "Color key: ";
	if (problem->nClasses() > nColors()) {
		throw UnexpectedInputException("ACEEventDumper::ACEEventDumper",
				"More relation types than available colors");
	}
	out << "<h3>";
	for (unsigned int i=0; i<problem->nClasses(); ++i) {
		out << L"<font color='" << color(i) << L"'><b>"
			<< problem->className(i) << L"</b></font>, ";	
	}
	out << "</h3>";
	
	typedef GraphicalModel::Constraint<ACEEvent>::ptr Constraint_ptr;
	BOOST_FOREACH(const Constraint_ptr& con, problem->corpusConstraints()) {
		out << "<b>FOO[</b>" << con->expectation() << L"<="
			<< con->bound() << L"; " << con->weight() << L"<b>]</b>; ";
	}
}

void ACEEventDumper::dump(const ACEEvent& event) 
{
	event.passage().toHTML(out, *_problem);
}

ACEModelDumper::ACEModelDumper(const std::wstring & title, 
		const std::string& directory, const std::string& filename)
: ModelDumper<ACEEvent>(directory, filename)
{
}

