// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PDTB_H
#define PDTB_H

#include <wchar.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <string>
#include <map>
#include <iterator>
#include <vector>
#include "Generic/common/Symbol.h"
#include "Generic/discourseRel/TargetConnectives.h"


using namespace std;
class CrossSentRelation;

class PennDiscourseTreebank {
  public:
	typedef map<string, Symbol> ExplicitConnectiveTable;
	typedef map<string, vector<CrossSentRelation> > CrossSentRelationTable;

	PennDiscourseTreebank();
	~PennDiscourseTreebank();
	
	//load PDTB data for all explicit relations
	static void loadDataFromPDTBFileList (const char *listfile);
	static void loadDataFromPDTBFile (const wchar_t *filename);

	//load PDTB data for explicit relations for a set of connective words
	static void loadDataFromPDTBFileList (const char *listfile, TargetConnectives::ExplicitConnectiveDict *connectiveDict);
	static void loadDataFromPDTBFile (const wchar_t *filename, TargetConnectives::ExplicitConnectiveDict *connectiveDict);

	//load PDTB data for all cross-sentence relations
	static void loadCrossSentDataFromPDTBFileList (const char *listfile);
	static void loadCrossSentDataFromPDTBFile (const wchar_t *filename);
	
	//load PDTB data for selected cross-sentence relations (restricted by connectives and semTypes)
	//to be added later


	static void findConnLocation (const string fileName, const char *onelineOfPDTB);
	static void findConnLocation (const string fileName, const char *onelineOfPDTB, TargetConnectives::ExplicitConnectiveDict *connectiveDict);
	static void findCrossSentRelation (string docName, const char *onelineOfPDTB);

	static Symbol getLabelofExpConnective(string docName, string word, string sentIndex, string SynNodeId) ; 
	static Symbol getLabelofCrossSentRel(string docName, string sentIndex);
	static vector<CrossSentRelation>* getCrossSentRel(string docName, string sentIndex);

	static void finalize();

  private:
	static int _num_documents;
	static ExplicitConnectiveTable *_connective_table;
	static CrossSentRelationTable *_crossSentRel_table;
	//UTF8OutputStream _debugStream;
};

#endif
