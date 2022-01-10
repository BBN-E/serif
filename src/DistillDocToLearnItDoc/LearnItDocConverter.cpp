#include "common/leak_detection.h" // must be the first #include
#include "Generic/common/SessionLogger.h"
#include "LearnItDocConverter.h"


#include <ctype.h>
#include <iostream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "theories/DocTheory.h"
#include "theories/Value.h"
#include "LearnIt/MainUtilities.h"

using namespace std;
using namespace boost;

wstring LearnItDocConverter::SENTENCE=L"<s>"; 
wstring LearnItDocConverter::_SENTENCE=L"</s>";

LearnItDocConverter::LearnItDocConverter(UTF8OutputStream& output) 
	: 
	 _output(output) {}

void LearnItDocConverter::addSentenceProcessor(LearnItSentenceConverter_ptr sp) {
	_processors.push_back(sp);
}

void LearnItDocConverter::process(const DocTheory* dt,
									const string& docname) 
{
	SessionLogger::info("LEARNIT") << "Processing " << docname << endl;

	wstringstream out;

	out << L"<DOC>\n\t<DOCNO>"
		<<wstring(docname.begin(), docname.end()) <<L"</DOCNO>\n" <<
		"\t<TEXT>\n";

	for (int sn=0; sn<dt->getNSentences(); ++sn) {
		SentenceTheory* st=dt->getSentenceTheory(sn);

		out << L"\t\t" << SENTENCE << L"\n";

		BOOST_FOREACH(LearnItSentenceConverter_ptr& sp, _processors) {
			sp->processSentence(dt, sn, out);
		}

		out << L"\t\t" << _SENTENCE << L"\n";
	}

	out << L"\n\t</TEXT>\n</DOC>\n";
	_output << out.str();
}

void LearnItDocConverter::process(const std::vector<const DocTheory*> dts,
								  const std::string docname,
								  std::vector<std::string> variations) 
{
	SessionLogger::info("LEARNIT") << "Processing " << docname << endl;

	wstringstream out;

	out << L"<DOC>\n\t<DOCNO>"
		<<wstring(docname.begin(), docname.end()) <<L"</DOCNO>\n" <<
		"\t<TEXT>\n";

	const DocTheory* first = dts.front();

	for (int sn=0; sn < first->getNSentences(); ++sn) {
		out << L"\t\t" << SENTENCE << L"\n";

		for (size_t i=0; i < dts.size(); i++) {

			BOOST_FOREACH(LearnItSentenceConverter_ptr& sp, _processors) {
				out << L"\t\t<" << wstring(variations[i].begin(),variations[i].end()) << L">\n";
				sp->processSentence(dts[i], sn, out);
				out << L"\t\t</" << wstring(variations[i].begin(),variations[i].end()) << L">\n";

			}

		}


		out << L"\t\t" << _SENTENCE << L"\n";
	}

	out << L"\n\t</TEXT>\n</DOC>\n";
	_output << out.str();
}

void LearnItDocConverter::indent(int n, wostream& output) {
	for (int i=0; i<n; ++i) {
		output << L"\t";
	}
}




