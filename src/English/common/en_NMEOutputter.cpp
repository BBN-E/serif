// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "en_NMEOutputter.h"
#include <boost/scoped_ptr.hpp>

void NMEOutputter::outputfile(const char * source, const char * destdir, const char * filename, NMEResultVector * results) const{
	std::cout << "outputting with: " << source << ", " << filename << "\n";

	int tokstart=0;
	int tokend=0;
	char output_file_prefix[500];
	strcpy(output_file_prefix, destdir);
	strcat(output_file_prefix, "/");
	strncat(output_file_prefix, filename,499-strlen(output_file_prefix));
	strncat(output_file_prefix, ".xml",499-strlen(output_file_prefix));

	UTF8OutputStream resultstream;
	boost::scoped_ptr<UTF8InputStream> docStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& docStream(*docStream_scoped_ptr);
	std::wstring origstring;
	std::wstring resultstring;
	docStream.open(source);
	while (! docStream.eof()) {
		origstring += docStream.get();
	}
	docStream.close();
	
	//std::cout<<"\n\n\norig: ";
	//printf("%S",origstring.c_str());
	//std::cout<<"\n\nEND\n\n";

	const wchar_t * origtext=origstring.c_str();
	int last=0;
	for (NMEResultVector::iterator iter = results->begin();iter!=results->end();iter++) {
		for (int charpos=last;charpos<iter->getStart();charpos++) {
			//std::cout<<charpos<<"\n";
			resultstring+=origtext[charpos];
		}
		std::wstring * w=iter->getResult();
		resultstring+=w->c_str();
		last=iter->getEnd();
		//std::cout<<last<<"\n";
	}
	for (int charpos=last;charpos<wcslen(origtext);charpos++) {
		resultstring += origtext[charpos];
	}
	resultstream.open(output_file_prefix);
	resultstream << resultstring;
	resultstream.close();
	//printf("Result: \n%S\n\nEND\n\n",resultstring.c_str());	

}

NMEResultVector * NMEOutputter::to_enamex_sgml_string(SentenceTheory *theory,int offset) const{
	NMEResultVector * allresults = _new NMEResultVector();
	std::wstring result=L"";
	const TokenSequence *sentence=theory->getTokenSequence();
	const NMEResult * nmeresult;
	const NameTheory * nm=theory->getNameTheory();

//	std::wstring * result;

	//for each name span
	for (int i = 0; i < nm->n_name_spans; i++) {
		bool first_in_namespan=true;
		bool last_in_namespan=false;
		NameSpan * ent = nm->nameSpans[i];
		
		Symbol tag=ent->type;

		for (int tokennum=ent->start;tokennum<=ent->end;tokennum++) {
			if (tokennum==ent->end) {last_in_namespan=true;}
			const Token * tok=sentence->getToken(tokennum);
			//Symbol tag = getReducedTagSymbol(theory->getTag(i));
			// SERIF hack
			if (tag == EntityConstants::PER)
				tag = Symbol(L"PERSON");
			else if (tag == EntityConstants::ORG)
				tag = Symbol(L"ORGANIZATION");
			else if (tag == EntityConstants::LOC)
				tag = Symbol(L"LOCATION");

			if (first_in_namespan) {
				//printf("FIRST\n");
				if (tag == Symbol(L"PERSON") ||
					tag == Symbol(L"ORGANIZATION") ||
					tag == Symbol(L"LOCATION") ||
					tag == Symbol(L"FAC") ||
					tag == Symbol(L"GPE") ||
					tag == Symbol(L"ANIMAL") ||
					tag == Symbol(L"CONTACT_INFO") ||
					tag == Symbol(L"DISEASE") ||
					tag == Symbol(L"EVENT") ||
					tag == Symbol(L"GAME") ||
					tag == Symbol(L"LANGUAGE") ||
					tag == Symbol(L"LAW") ||
					tag == Symbol(L"NATIONALITY") ||
					tag == Symbol(L"PLANT") ||
					tag == Symbol(L"PRODUCT") ||
					tag == Symbol(L"SUBSTANCE") ||
					tag == Symbol(L"SUBSTANCE:NUCLEAR") ||
					tag == Symbol(L"WORK_OF_ART"))
					result += L"<ENAMEX TYPE=\"";
				else if (tag == Symbol(L"TIME") ||
					tag == Symbol(L"DATE") ||
					tag == Symbol(L"DATE:duration") ||
					tag == Symbol(L"DATE:age") ||
					tag == Symbol(L"DATE:date") ||
					tag == Symbol(L"DATE:other"))
					result += L"<TIMEX TYPE=\"";
				else if (tag == Symbol(L"MONEY") ||
					tag == Symbol(L"PERCENT") ||
					tag == Symbol(L"CARDINAL") ||
					tag == Symbol(L"ORDINAL") ||
					tag == Symbol(L"QUANTITY"))
					result += L"<NUMEX TYPE=\"";
				else result += L"<";
				result += tag.to_string();
				result += L"\">";
			}//end if first tag in namespan
			//add a space between words
			if (!first_in_namespan) {
				result+=L" ";
			}
			result += tok->getSymbol().to_string();
			//	sentence->getToken(i)->getSymbol().to_string();
			//printf(" res2: %S\n",result.c_str());

			//if we're at the start or end of a sentence, close the last tag
			if (last_in_namespan) {
				//printf("LAST\n");
				if (tag == Symbol(L"PERSON") ||
					tag == Symbol(L"ORGANIZATION") ||
					tag == Symbol(L"LOCATION") ||
					tag == Symbol(L"FAC") ||
					tag == Symbol(L"GPE") ||
					tag == Symbol(L"ANIMAL") ||
					tag == Symbol(L"CONTACT_INFO") ||
					tag == Symbol(L"DISEASE") ||
					tag == Symbol(L"EVENT") ||
					tag == Symbol(L"GAME") ||
					tag == Symbol(L"LANGUAGE") ||
					tag == Symbol(L"LAW") ||
					tag == Symbol(L"NATIONALITY") ||
					tag == Symbol(L"PLANT") ||
					tag == Symbol(L"PRODUCT") ||
					tag == Symbol(L"SUBSTANCE") ||
					tag == Symbol(L"SUBSTANCE:NUCLEAR") ||
					tag == Symbol(L"WORK_OF_ART"))
					result += L"</ENAMEX>";
				else if (tag == Symbol(L"TIME") ||
					tag == Symbol(L"DATE") ||
					tag == Symbol(L"DATE:duration") ||
					tag == Symbol(L"DATE:age") ||
					tag == Symbol(L"DATE:date") ||
					tag == Symbol(L"DATE:other"))
					result += L"</TIMEX>";
				else if (tag == Symbol(L"MONEY") ||
					tag == Symbol(L"PERCENT") ||
					tag == Symbol(L"CARDINAL") ||
					tag == Symbol(L"ORDINAL") ||
					tag == Symbol(L"QUANTITY"))
					result += L"</NUMEX>";
				else {
					result += L"</";
					result += tag.to_string();
					result += L">";
				}//end else
			}//end if we're at the start of a sentence or a new name span close the tag
			first_in_namespan=false;
		}//end for each token in the name span
		
		tokstart=sentence->getToken(ent->start)->getPosInFileStart();
		tokend=sentence->getToken(ent->end)->getPosInFileStart()+sentence->getToken(ent->end)->getLength();
		
		/*
		const Token * lasttok=sentence->getToken(ent->end);
		std::cout<<" last token: ";
		printf("%S\n",sentence->getToken(ent->end)->getSymbol().to_string());
		*/

		nmeresult = _new NMEResult(_new std::wstring(result),tokstart,tokend);
		allresults->insert(allresults->end(),*nmeresult);
		result.clear();
	}//end for each name span
	//printf("result: %S",result.c_str());
	
	return allresults;
}



