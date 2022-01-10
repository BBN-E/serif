// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_BWCHART_DECODER_H
#define ar_BWCHART_DECODER_H

#include <cstddef>
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/PriorProbTable.h"
#include "Generic/parse/HeadProbs.h"
#include "Generic/parse/ModifierProbs.h"
#include "Generic/parse/LexicalProbs.h"
#include "Generic/parse/PartOfSpeechTable.h"
#include "Generic/parse/VocabularyTable.h"
#include "Arabic/parse/ar_WordFeatures.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/SequentialBigrams.h"
#include "Arabic/parse/ar_SignificantConstitOracle.h"
#include "Generic/parse/Constraint.h"
#include "Generic/common/limits.h" // SRS
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/parse/ChartDecoder.h"
#include "Generic/theories/Entity.h"
#include "Arabic/parse/ar_WordSegment.h"
#include "Arabic/parse/ar_ChartEntry.h"
#include "Arabic/BuckWalter/ar_ParseSeeder.h"

//#include <io.h>

#define MAX_SENTENCE_LENGTH MAX_SENTENCE_TOKENS // SRS
#define MAX_TAGS_PER_WORD 50

class BWArabicChartDecoder : public ChartDecoder {
protected:
	UTF8OutputStream _debugStream;
	//UTF8OutputStream _parseStream;
	bool DEBUG1;
	bool DEBUG2;
	static const bool DEBUG3 = false;
public:
		BWArabicChartDecoder(const char* model_prefix, double frag_prob) :
			ChartDecoder(model_prefix, frag_prob)
			{
				std::string parse_buffer = ParamReader::getParam("parse_debug");
				DEBUG1 = (!parse_buffer.empty());
				if (DEBUG1)
					_debugStream.open(parse_buffer.c_str());

				//std::string parsenode_buffer = ParamReader::getParam("parsenode_debug");
				//DEBUG2 = (!parsenode_buffer.empty());
				//if (DEBUG2)
				//	_debugStream.open(parsenode_buffer.c_str());
				DEBUG2 = false;

		};
	virtual ParseNode* decode(ArabicParseSeeder::SameSpanParseToken** init_segments, int length, std::vector<Constraint> & constraints,
							  bool collapseNPlabels);
	virtual ParseNode* decode(Symbol* sentence, int length, std::vector<Constraint> & constraints,
							  bool collapseNPlabels = true, Symbol* pos_constraints = 0 ) {
		return ChartDecoder::decode(sentence, length, constraints, collapseNPlabels, pos_constraints);
	}
	virtual ParseNode* decode(Symbol* sentence, int length, std::vector<Constraint> & constraints, 
		                      bool collapseNPlabels, const PartOfSpeechSequence* pos_constraints) { 
		return ChartDecoder::decode(sentence, length, constraints, collapseNPlabels, pos_constraints); 
	}
	void setMaxParserSeconds(int maxsecs) { MAX_CLOCKS = maxsecs * CLOCKS_PER_SEC; }



protected:
	static const bool std_dbg = false;
	ParseNode* _makeFlatParse(ArabicParseSeeder::SameSpanParseToken** init_segments,
		int length, Constraint* _constraints, int _numConstraints);
	Symbol _cleanWords[MAX_SENTENCE_LENGTH][MAX_SENTENCE_LENGTH];
	Symbol _nameWords[MAX_SENTENCE_LENGTH];
	int _numNameWords;
	int _chartEnd;
	//post processing overwrites only name words
	void postprocessParse(ParseNode* node,	std::vector<Constraint> & constraints, bool collapseNPlabels);

	void addNameToNode(ParseNode* node, int& currentPosition);
	int initChart(ArabicParseSeeder::SameSpanParseToken** init_segments, Symbol* split_sentence, int length, std::vector<Constraint> & constraints);
	void initChartWord(ArabicParseSeeder::SameSpanParseToken *segment,int startIndex, int endIndex, bool firstWord);
	void addConstraintEntry(int left, int right, Symbol right_text, Symbol type,
									  EntityType entityType);
	//change to not write over preterminals with spans
	virtual void transferTheoriesToChart(int start, int end);
	virtual void cleanupChart(int length);

	//replace headWord in non name preterminals with cleanWord of same span
	void postProcessChart();



	void addWordNode(ParseNode* currNode, ArabicParseSeeder::SameSpanParseToken* t);
	//int addNPPNode(ParseNode* currNode, Constraint constraint, ArabicParseSeeder::SameSpanParseToken** sent, int length);

	void addNameToParse(ParseNode* node, int& currentPosition);
	void addNameToPremods(ParseNode* node, int& currentPosition);
	void addNameToPostmods(ParseNode* node, int& currentPosition);


};
#endif
