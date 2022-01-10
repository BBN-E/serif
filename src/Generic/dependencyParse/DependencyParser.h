// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEPENDENCY_PARSER_H
#define DEPENDENCY_PARSER_H

#include "Generic/common/IDGenerator.h"
#include <vector>
#include <string>

#include <boost/filesystem/operations.hpp>

class IDGenerator;
class DepNode;
class TokenSequence;

class DependencyParser {
public:

	DependencyParser();
	~DependencyParser(); 

	void resetForNewSentence();

	int convertFullParse(DependencyParseTheory **results, int max_dependency_parses, Parse *fullParse);
	int parse(DependencyParseTheory **results, int max_dependency_parses, Parse *fullParse, TokenSequence *tokenSequence, Symbol docName);


private:
	std::vector<DepNode *> _convertedDepNodes;
	std::vector<const SynNode *> _terminalSynNodes;
	IDGenerator _nodeIDGenerator;
	boost::filesystem::path _processingDirectory;
	std::string _modelFileName;
	std::string _maltParserJarFile;
	int _sentence_count;

	void collectDepNodes(const SynNode *node);
	void setChildren(std::vector<DepNode *> depNodes);
	void setIDsDepthFirst(DepNode* node);
	void setTokenSpans(std::vector<DepNode *> depNodes);
	int getStartToken(const DepNode *node);
	int getEndToken(const DepNode *node);

	static boost::filesystem::path createProcessingDirectory();
	Symbol fixPartOfSpeechTag(Symbol tag);


};

#endif
