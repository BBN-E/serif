// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// PIdFSimulatedActiveLearning.cpp : Defines the entry point for the console application.
//

#include "common/leak_detection.h"

#include <stdio.h>
#include <crtdbg.h>

#include "common/UnrecoverableException.h"
#include "common/ParamReader.h"
#include "common/HeapChecker.h"
#include "names/discmodel/PIdFActiveLearning.h"




int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "PIdfSimActiveLearningTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		//return -1;
	}
	
/*
sample annotation
<SENTENCE ID="ABC20001001.1830.0973-0">
<DISPLAY_TEXT>like many heartland states, iowa has had trouble keeping young people down on the farm or anywhere within state lines.</DISPLAY_TEXT>
<ANNOTATION TYPE=”GPE” START_OFFSET=”10” END_OFFSET=”25” SOURCE=”Example1”/>
<ANNOTATION TYPE=”GPE” START_OFFSET=”28” END_OFFSET=”31” SOURCE=”Example2”/>
</SENTENCE>
<SENTENCE ID="ABC20001001.1830.0973-1">
<DISPLAY_TEXT>with population waning, the state is looking beyond its borders for newcomers.</DISPLAY_TEXT>
<ANNOTATION TYPE=”PER” START_OFFSET=”5” END_OFFSET=”14” SOURCE=”Example1”/>
<SENTENCE>
<SENTENCE ID="ABC20001001.1830.0973-2">
<DISPLAY_TEXT>
as abc's jim sciutto reports, one little town may provide a big lesson.</DISPLAY_TEXT>
<ANNOTATION TYPE=”ORG” START_OFFSET=”3” END_OFFSET=”5” SOURCE=”Example1”/>
<ANNOTATION TYPE=”PER” START_OFFSET=”9” END_OFFSET=”19” SOURCE=”Example2”/>
</SENTENCE>
<SENTENCE ID="ABC20001001.1830.0973-3">
<DISPLAY_TEXT>on homecoming night postville feels like hometown, usa, but a look around this town of 2,000 shows it's become a miniature ellis island.</DISPLAY_TEXT>
<ANNOTATION TYPE=”LOC” START_OFFSET=”20” END_OFFSET=”28” SOURCE=”Example1”/>
<ANNOTATION TYPE=”GPE” START_OFFSET=”51” END_OFFSET=”53” SOURCE=”Example2”/>
<ANNOTATION TYPE=”LOC” START_OFFSET=”133” END_OFFSET=”144” SOURCE=”Example3”/>
</SENTENCE>
*/
	try {
		char outstr[5000];

		std::cout<<"****New: "<<std::endl;
		char* paramfile = "c:\\serif\\src\\PIdFQuickLearn\\English_Release\\eng-active-learning.par";
		if (argc == 2) {
			paramfile = argv[1];
		}
		ParamReader::readParamFile(paramfile);
		PIdFActiveLearning* trainer = _new PIdFActiveLearning();

		std::cout<<"\n\n***Initialize: "<<std::endl;
		wstring str;
		str = trainer->Initialize(paramfile);
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***ReadCorpus: "<<std::endl;
		str = trainer->ReadCorpus();
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***Save:"<<std::endl;
		//str = trainer->Save();
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;


		/*
		wchar_t* training = 
			L"<SENTENCE ID=\"ABC20001001.1830.0973-0\">\n
			<DISPLAY_TEXT>like many heartland states, iowa has had trouble keeping young people down on 
			the farm or anywhere within state lines.</DISPLAY_TEXT>\n
			<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"10\" END_OFFSET=\"25\" SOURCE=\"Example1\"/>\n
			<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"28\" END_OFFSET=\"31\" SOURCE=\"Example2\"/>\n
			</SENTENCE>";
		wchar_t* test =	L"<SENTENCE ID=\"ABC20001001.1830.0973-0\">\n<DISPLAY_TEXT>like many heartland states, iowa has had trouble keeping young people down on the farm or anywhere within state lines.</DISPLAY_TEXT>\n</SENTENCE>";
		*/
		
		wchar_t* training = 
			L"<SENTENCE ID=\"ABC20001001.1830.0973-0\">\n<DISPLAY_TEXT>like many heartland states, iowa has had trouble keeping young people down on the farm or anywhere within state lines.</DISPLAY_TEXT>\n<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"10\" END_OFFSET=\"25\" SOURCE=\"Example1\"/>\n<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"28\" END_OFFSET=\"31\" SOURCE=\"Example2\"/>\n</SENTENCE>\n<SENTENCE ID=\"ABC20001001.1830.0973-1\">\n<DISPLAY_TEXT>iowa is a state.</DISPLAY_TEXT>\n<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"0\" END_OFFSET=\"3\" SOURCE=\"Example3\"/>\n<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"10\" END_OFFSET=\"14\" SOURCE=\"Example3\"/>\n</SENTENCE>";

		wchar_t* test =	L"<SENTENCE ID=\"ABC20001001.1830.0973-0\">\n<DISPLAY_TEXT>like many heartland states, iowa has had trouble keeping young people down on the farm or anywhere within state lines.</DISPLAY_TEXT>\n</SENTENCE>\n<SENTENCE ID=\"ABC20001001.1830.0973-1\">\n<DISPLAY_TEXT>iowa is a state.</DISPLAY_TEXT>\n</SENTENCE>";
/*
		wchar_t* training = 
			L"<SENTENCE ID=\"ABC20001001.1830.0973-1\">\n<DISPLAY_TEXT>iowa is a state.</DISPLAY_TEXT>\n<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"0\" END_OFFSET=\"3\" SOURCE=\"Example3\"/>\n<ANNOTATION TYPE=\"GPE\" START_OFFSET=\"10\" END_OFFSET=\"14\" SOURCE=\"Example3\"/>\n</SENTENCE>";
*/
//		wchar_t* test =	L"<SENTENCE ID=\"ABC20001001.1830.0973-1\">\n<DISPLAY_TEXT>iowa is a state.</DISPLAY_TEXT>\n</SENTENCE>";

		std::cout<<"\n\n***Train:"<<std::endl;
		str = trainer->Train(training, 5, false);;
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;
		trainer->Save();

		std::cout<<"\n\n***Decode:"<<std::endl;
		str = trainer->Decode(test);
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***Select Sentences:"<<std::endl;
		str = trainer->SelectSentences(100, 5, 6, 0);
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

//		trainer->Save();

		std::cout<<"\n\n***GetCorpusPointer:"<<std::endl;
		str = trainer->GetCorpusPointer();
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***ChangeCorpus:"<<std::endl;		
		str = trainer->ChangeCorpus("C:\\SERIF\\src\\StandaloneSentenceBreaker\\newcorpus.txt");
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***Select Sentences:"<<std::endl;
		str = trainer->SelectSentences(100, 5, 6, 0);
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***Close:"<<std::endl;		
		str = trainer->Close();
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***Initialize: "<<std::endl;
		str = trainer->Initialize(paramfile);
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***ReadCorpus: "<<std::endl;
		str = trainer->ReadCorpus();
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

		std::cout<<"\n\n***Decode:"<<std::endl;
		str = trainer->Decode(test);
		wcstombs(outstr, str.c_str(), 5000);
		std::cout<<"result: \n"<<outstr<<std::endl;

	
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

#ifdef _DEBUG
		cerr << "Press enter to exit....\n";
		getchar();
#endif

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

#if 0
	ParamReader::finalize();

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();
#endif

#ifdef _DEBUG
	cerr << "Press enter to exit....\n";
	getchar();
#endif

	return 0;
}
