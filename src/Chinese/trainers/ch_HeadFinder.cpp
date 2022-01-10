// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/trainers/ch_HeadFinder.h"
#include "Generic/common/Symbol.h"
#include "Generic/trainers/Production.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SymbolConstants.h"
#include <boost/scoped_ptr.hpp>

const float targetLoadingFactor = static_cast<float>(0.7);

ChineseHeadFinder::ChineseHeadFinder() : headTable(0) {
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& instream(*instream_scoped_ptr);
	std::string table_file = ParamReader::getParam("head_index_table");
	if (!table_file.empty()) {
		instream.open(table_file.c_str());
		initializeTable(instream);
	}
}

ChineseHeadFinder::~ChineseHeadFinder() {
	if (headTable != 0) {
		ProductionHeadTable::iterator iter;
		for (iter = headTable->begin(); iter != headTable->end(); ++iter)
			delete (*iter).first;
		delete headTable;
	}
}

void ChineseHeadFinder::initializeTable(UTF8InputStream &stream) {
	int numEntries;
    int numBuckets;
    UTF8Token token;

	stream >> numEntries;
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);

	// numBuckets must be >= 1,
	//  but we set it to a minimum of 5, just to be safe
	// only relevant for very very small training sets, where
	//  there are no events of a certain type (e.g. left lexical)
	if (numBuckets < 5)
		numBuckets = 5;

	headTable = _new ProductionHeadTable(numBuckets);
    for (int i = 0; i < numEntries; i++) {

        Production *production = _new Production();

        stream >> token;

        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed head index rule record at entry: %d", i );
		  throw UnexpectedInputException("ChineseHeadFinder::()", c);
        }

        stream >> token;
        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed head index rule record at entry: %d", i );
          throw UnexpectedInputException("ChineseHeadFinder::()",c);
        }

		stream >> token;
		production->left = token.symValue();

		int N;
		stream >> N;
		production->number_of_right_side_elements = N;

        for (int j = 0; j < N; j++) {
            stream >> token;
            production->right[j] = token.symValue();
        }

        stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed head index rule record at entry: %d", i );
          throw UnexpectedInputException("ChineseHeadFinder::()",c);
        }

        int head_index;
        stream >> head_index;

        stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed head index rule record at entry: %d", i );
          throw UnexpectedInputException("ChineseHeadFinder::()",c);
        }

        (*headTable)[production] = head_index;
    }

}

int ChineseHeadFinder::get_head_index()
{

	// change all NPPs and NPAs to plain NPs before lookup
	if (production.left == ChineseSTags::NPP || production.left == ChineseSTags::NPA)
		production.left = ChineseSTags::NP;
	for (int i = 0; i < production.number_of_right_side_elements; i++)
		if (production.right[i] == ChineseSTags::NPP || production.right[i] == ChineseSTags::NPA)
			production.right[i] = ChineseSTags::NP;

	// if it's in the table, return stored index
	if (headTable != 0) {
		ProductionHeadTable::iterator iter;
		iter = headTable->find(&production);
		if (iter != headTable->end()) {
			return (*iter).second;
		}
	}

	// otherwise follow heuristics
	if (production.left == ChineseSTags::ADJP)
		return findHeadADJP();
	else if (production.left == ChineseSTags::ADVP)
		return findHeadADVP();
	else if (production.left == ChineseSTags::CLP)
		return findHeadCLP();
	else if (production.left == ChineseSTags::CP)
		return findHeadCP();
	else if (production.left == ChineseSTags::DNP)
		return findHeadDNP();
	else if (production.left == ChineseSTags::DP)
		return findHeadDP();
	else if (production.left == ChineseSTags::DVP)
		return findHeadDVP();
	else if (production.left == ChineseSTags::FRAG)
		return findHeadFRAG();
	else if (production.left == ChineseSTags::IP)
		return findHeadIP();
	else if (production.left == ChineseSTags::LCP)
		return findHeadLCP();
	else if (production.left == ChineseSTags::LST)
		return findHeadLST();
	else if (production.left == ChineseSTags::NP)
		return findHeadNP();
	else if (production.left == ChineseSTags::NPA)
		return findHeadNP();
	else if (production.left == ChineseSTags::NPP)
		return findHeadNP();
	else if (production.left == ChineseSTags::PP)
		return findHeadPP();
	else if (production.left == ChineseSTags::PRN)
		return findHeadPRN();
	else if (production.left == ChineseSTags::QP)
		return findHeadQP();
	else if (production.left == ChineseSTags::UCP)
		return findHeadUCP();
	else if (production.left == ChineseSTags::VCD)
		return findHeadVCD();
	else if (production.left == ChineseSTags::VCP)
		return findHeadVCP();
	else if (production.left == ChineseSTags::VNV)
		return findHeadVNV();
	else if (production.left == ChineseSTags::VP)
		return findHeadVP();
	else if (production.left == ChineseSTags::VPT)
		return findHeadVPT();
	else if (production.left == ChineseSTags::VRD)
		return findHeadVRD();
	else if (production.left == ChineseSTags::VSB)
		return findHeadVSB();
	else return 0;

}

int ChineseHeadFinder::findHeadADJP()
{
	Symbol set[5] = {ChineseSTags::ADJP, ChineseSTags::JJ, ChineseSTags::QP, ChineseSTags::NP, ChineseSTags::AD};

	return priority_scan_from_right_to_left(set, 5);
}

int ChineseHeadFinder::findHeadADVP()
{
	Symbol set[4] = {ChineseSTags::ADVP, ChineseSTags::AD, ChineseSTags::JJ, ChineseSTags::CS};
	return priority_scan_from_right_to_left (set, 4);
}

int ChineseHeadFinder::findHeadCLP()
{
	Symbol set[3] = {ChineseSTags::CLP, ChineseSTags::M, ChineseSTags::ADJP};
	return priority_scan_from_right_to_left(set, 3);
}

int ChineseHeadFinder::findHeadCP()
{
	Symbol set[7]={ChineseSTags::IP, ChineseSTags::CP, ChineseSTags::VP, ChineseSTags::DEC,
				ChineseSTags::ADVP, ChineseSTags::NP};
	return priority_scan_from_right_to_left(set, 7);
}

int ChineseHeadFinder::findHeadDNP()
{
	Symbol set[9]={ChineseSTags::NP, ChineseSTags::PP, ChineseSTags::LCP, ChineseSTags::QP,
				 ChineseSTags::ADJP, ChineseSTags::DP, ChineseSTags::UCP, ChineseSTags::DNP, ChineseSTags::DEG};
	return priority_scan_from_left_to_right(set, 9);
}

int ChineseHeadFinder::findHeadDP()
{
	Symbol set[6]={ChineseSTags::CLP, ChineseSTags::QP, ChineseSTags::DP, ChineseSTags::NP, ChineseSTags::DNP, ChineseSTags::DT};
	return priority_scan_from_right_to_left(set, 6);
}

int ChineseHeadFinder::findHeadDVP()
{
	Symbol set[3]={ChineseSTags::VP, ChineseSTags::ADVP, ChineseSTags::NP};
	return priority_scan_from_left_to_right(set, 3);
}

int ChineseHeadFinder::findHeadFRAG()
{
	return production.number_of_right_side_elements-1;
}

int ChineseHeadFinder::findHeadIP()
{
	Symbol set[4]={ChineseSTags::VP, ChineseSTags::IP, ChineseSTags::NP, ChineseSTags::PP};
	return priority_scan_from_right_to_left(set, 4);
}

int ChineseHeadFinder::findHeadLST()
{
	Symbol set[1]={ChineseSTags::CD};
	return priority_scan_from_right_to_left(set, 1);
}

int ChineseHeadFinder::findHeadLCP()
{
	Symbol set[4]={ChineseSTags::NP, ChineseSTags::IP, ChineseSTags::QP, ChineseSTags::DNP};
	return priority_scan_from_right_to_left(set, 4);
}

int ChineseHeadFinder::findHeadNP()
{
	Symbol set[16]={ChineseSTags::NP, ChineseSTags::NN, ChineseSTags::NR, ChineseSTags::NT,
					ChineseSTags::CP, ChineseSTags::QP, ChineseSTags::DP, ChineseSTags::DNP,
					ChineseSTags::CLP, ChineseSTags::PRN, ChineseSTags::LCP, ChineseSTags::IP,
					ChineseSTags::PP, ChineseSTags::ADJP, ChineseSTags::ADVP, ChineseSTags::UCP};
	return priority_scan_from_right_to_left(set, 16);
}

int ChineseHeadFinder::findHeadPP()
{
	Symbol set[7]={ChineseSTags::PP, ChineseSTags::P, ChineseSTags::NP, ChineseSTags::LCP,
					ChineseSTags::IP, ChineseSTags::ADVP, ChineseSTags::QP};
	return priority_scan_from_left_to_right(set, 7);
}

int ChineseHeadFinder::findHeadPRN()
{
	Symbol set[6]={ChineseSTags::IP, ChineseSTags::CP, ChineseSTags::VP, ChineseSTags::NP, ChineseSTags::PP, ChineseSTags::QP};
	return priority_scan_from_right_to_left(set, 6);
}

int ChineseHeadFinder::findHeadQP()
{
	Symbol set[10]={ChineseSTags::QP, ChineseSTags::CLP, ChineseSTags::NP, ChineseSTags::M,
					ChineseSTags::CD, ChineseSTags::OD, ChineseSTags::PRN, ChineseSTags::NT,
					ChineseSTags::NN, ChineseSTags::DNP};
	return priority_scan_from_right_to_left(set, 10);
}

int ChineseHeadFinder::findHeadUCP()
{
	Symbol set[9]={ChineseSTags::IP, ChineseSTags::NP, ChineseSTags::LCP, ChineseSTags::ADJP,
				ChineseSTags::UCP, ChineseSTags::DNP, ChineseSTags::DP, ChineseSTags::ADJP, ChineseSTags::ADVP};
	return priority_scan_from_right_to_left(set, 9);
}

int ChineseHeadFinder::findHeadVCD()
{
	Symbol set[2]={ChineseSTags::VV, ChineseSTags::VA};
	return priority_scan_from_right_to_left(set, 2);
}

int ChineseHeadFinder::findHeadVCP()
{
	Symbol set[3]={ChineseSTags::VV, ChineseSTags::VA, ChineseSTags::VC};
	return priority_scan_from_right_to_left(set, 3);
}

int ChineseHeadFinder::findHeadVNV()
{
	Symbol set[3]={ChineseSTags::VV, ChineseSTags::VA, ChineseSTags::VC};
	return priority_scan_from_right_to_left(set, 3);
}

int ChineseHeadFinder::findHeadVP()
{
	Symbol set[22]={ChineseSTags::SB, ChineseSTags::VSB, ChineseSTags::BA, ChineseSTags::VA,
					ChineseSTags::VV, ChineseSTags::VP, ChineseSTags::VE, ChineseSTags::VC,
					ChineseSTags::VRD, ChineseSTags::VCD, ChineseSTags::VPT, ChineseSTags::DVP,
					ChineseSTags::QP, ChineseSTags::ADVP, ChineseSTags::IP, ChineseSTags::PP,
					ChineseSTags::CP, ChineseSTags::LCP, ChineseSTags::NP, ChineseSTags::LB};
	return priority_scan_from_right_to_left(set, 22);
}

int ChineseHeadFinder::findHeadVPT()
{
	Symbol set[4]={ChineseSTags::VV, ChineseSTags::VE, ChineseSTags::VA, ChineseSTags::VC};
	return priority_scan_from_right_to_left(set, 4);
}

int ChineseHeadFinder::findHeadVRD()
{
	Symbol set[5]={ChineseSTags::VV, ChineseSTags::VA, ChineseSTags::VE, ChineseSTags::VC, ChineseSTags::VCD};
	return priority_scan_from_right_to_left(set, 5);
}

int ChineseHeadFinder::findHeadVSB()
{
	Symbol set[4]={ChineseSTags::VV, ChineseSTags::VA, ChineseSTags::VCD, ChineseSTags::VE};
	return priority_scan_from_left_to_right(set, 4);
}


// priority_scan searches go through the tags in "set" one by one to see if
//		they match the elements in right[]
// scan searches go through the elements of right[] one by one to see if
//		they match any of the tags in "set"
//
// i.e. in priority_scan, the order of the tags in the set matters,
//   and in scan it doesn't
//
// left_to_right / right_to_left obviously refers to the starting place for
//   searches through the elements of right[], a.k.a. the children
//   of the node whose head we're trying to find

int ChineseHeadFinder::priority_scan_from_left_to_right (Symbol* set, int size)
{
	for (int j = 0; j < size; j++)
	{
		for (int k = 0; k < production.number_of_right_side_elements; k++)
		{
			if (production.right[k] == set[j])
				return k;
		}
	}
	return 0;
}

int ChineseHeadFinder::priority_scan_from_right_to_left (Symbol* set, int size)
{
	for (int j = 0; j < size; j++)
	{
		for (int k = production.number_of_right_side_elements - 1; k >= 0; k--)
		{
			if (production.right[k] == set[j])
				return k;
		}
	}
	return production.number_of_right_side_elements - 1;
}

int ChineseHeadFinder::scan_from_left_to_right (Symbol* set, int size)
{
	for (int j = 0; j < production.number_of_right_side_elements; j++) {
		for (int k = 0; k < size; k++) {
			if (production.right[j] == set[k])
				return j;
		}
	}
	return -1;
}

int ChineseHeadFinder::scan_from_right_to_left (Symbol* set, int size)
{
	for (int j = production.number_of_right_side_elements - 1; j >= 0; j--) {
		for (int k = 0; k < size; k++) {
			if (production.right[j] == set[k])
				return j;
		}
	}
	return -1;
}
