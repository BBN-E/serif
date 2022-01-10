// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/IDGenerator.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/DependencyParseTheory.h"
#include "Generic/theories/DepNode.h"
#include "Generic/dependencyParse/DependencyParser.h"

#pragma warning(push, 0)
#include <boost/date_time/posix_time/posix_time.hpp>
#pragma warning(pop)

#include <vector>
#include <boost/scoped_ptr.hpp>

DependencyParser::DependencyParser() : _sentence_count(0) {
	_nodeIDGenerator.reset();

	_processingDirectory = createProcessingDirectory();

	// Copy model file to processing direcotry
	std::string maltParserModelFile = ParamReader::getRequiredParam("malt_parser_model_file");
	boost::filesystem::path modelFile(maltParserModelFile);
	_modelFileName = BOOST_FILESYSTEM_PATH_GET_FILENAME(modelFile);
	boost::filesystem::path newModelFileLocation = _processingDirectory;
	newModelFileLocation /= _modelFileName;
	boost::filesystem::copy_file(maltParserModelFile, newModelFileLocation);

	_maltParserJarFile = ParamReader::getRequiredParam("malt_parser_jar_file");
}

DependencyParser::~DependencyParser() {
	//boost::filesystem::remove_all(_processingDirectory);
}

boost::filesystem::path DependencyParser::createProcessingDirectory() {
	// Make temporary directory for MaltParser input
	boost::posix_time::ptime currentTime(boost::posix_time::microsec_clock::local_time());
	std::string timeStamp = boost::posix_time::to_iso_string(currentTime);
	std::string parallelValue = ParamReader::getParam("parallel");
	if (parallelValue.empty())
		parallelValue = "000";
	std::string directoryName = "MaltParser-" + timeStamp + "-" + parallelValue;


#ifdef _WIN32
	boost::filesystem::path directory("C:\\scratch\\");
#else
	boost::filesystem::path directory("/export/u10/");
#endif

	bool rv1 = boost::filesystem::create_directory(directory);
	directory /= directoryName;
	bool rv2 = boost::filesystem::create_directory(directory);
	return directory;
}

void DependencyParser::resetForNewSentence() {

}

int DependencyParser::parse(DependencyParseTheory **results, int max_dependency_parses, Parse *fullParse, TokenSequence *tokenSequence, Symbol docName) {	
	std::wstringstream maltParserInputStream;
	int sent_no = tokenSequence->getSentenceNumber();

	int count = 1;
	const SynNode *node = fullParse->getRoot()->getFirstTerminal();
	if (!node) 
		throw InternalInconsistencyException("DependencyParser::parse", "No terminal nodes on Parse");
	std::wstringstream tokenStream;
	do {
		const SynNode *preterminal = node->getParent();
		Symbol posTag = preterminal->getTag();
		posTag = fixPartOfSpeechTag(posTag);

		tokenStream << count << L"\t" 
			        << tokenSequence->getToken(count - 1)->getSymbol().to_string() << L"\t_\t" 
			        << posTag.to_string() << L"\t" 
			        << posTag.to_string() << L"\t_\n";

		count++;
		node = node->getNextTerminal();
	} while (node);

	std::wstringstream filename;
	filename << docName.to_string() << L"-" << sent_no << "-" << _sentence_count++;
	boost::filesystem::path sentenceFile = _processingDirectory;
	sentenceFile /= OutputUtil::convertToUTF8BitString(filename.str().c_str());

	UTF8OutputStream maltParserInput(sentenceFile.string());
	maltParserInput << tokenStream.str();
	maltParserInput.close();

	boost::filesystem::current_path(_processingDirectory);

	std::stringstream ss;

	ss << "java -Xmx1024m -jar " << _maltParserJarFile 
	   << " -c " << _modelFileName 
	   << " -i " << sentenceFile.string() 
	   << " -o " << sentenceFile.string() << ".dep"
	   << " -m parse"
	   << " 2> " << sentenceFile.string() << ".err";

	//std::cout << ss.str() << "\n";
	
	system(ss.str().c_str());

	std::vector<DepNode *> depNodes;
	std::vector<int> depNodeParents;

	boost::scoped_ptr<UTF8InputStream> maltParserOutput_scoped_ptr(UTF8InputStream::build(sentenceFile.string() + ".dep"));
	UTF8InputStream& maltParserOutput(*maltParserOutput_scoped_ptr);
	UTF8Token token;
	count = 0;
	while (!maltParserOutput.eof()) {
		maltParserOutput >> token; // token number
		if (wcslen(token.chars()) == 0)
			break;

		maltParserOutput >> token; 
		std::wstring word = token.chars();

		maltParserOutput >> token; // blank

		maltParserOutput >> token; 
		std::wstring partOfSpeechTag = token.chars();

		maltParserOutput >> token; // pos tag
		maltParserOutput >> token; // blank

		maltParserOutput >> token; 
		int parent = _wtoi(token.chars());

		maltParserOutput >> token; 
		std::wstring dependencyRelationTag = token.chars();

		maltParserOutput >> token; // blank
		maltParserOutput >> token; // blank

		DepNode *depNode = new DepNode(_nodeIDGenerator.getID(), 0, Symbol(dependencyRelationTag.c_str()), 0, Symbol(word.c_str()), Symbol(partOfSpeechTag.c_str()), count++);
		depNodes.push_back(depNode);
		depNodeParents.push_back(parent);
	}
	maltParserOutput.close();

	DepNode *root = 0;
	bool found_root = false;
	// set parent for every depNode except the root
	for (size_t i = 0; i < depNodeParents.size(); i++) {
		int parent_index = depNodeParents[i];
		if (parent_index > 0) {
			depNodes[i]->setParent(depNodes[parent_index - 1]);
		} else {
			if (found_root) 
				throw UnexpectedInputException("DependencyParser::parse", "Found more than one root");
			found_root = true;
			root = depNodes[i];
		}
	}

	if (!found_root) 
		throw UnexpectedInputException("DependencyParser::parse", "Didn't find root");

	setChildren(depNodes);
	setTokenSpans(depNodes);

	results[0] = new DependencyParseTheory(tokenSequence);
	results[0]->setParse(root);

	return 1;

}

int DependencyParser::convertFullParse(DependencyParseTheory **results, int max_dependency_parses, Parse *fullParse) {	
	// For every word in the parse, make a DepNode and store it 
	// in a vector
	_convertedDepNodes.clear();
	_convertedDepNodes.resize(fullParse->getTokenSequence()->getNTokens());
	_terminalSynNodes.clear(); 
	_terminalSynNodes.resize(fullParse->getTokenSequence()->getNTokens());
	collectDepNodes(fullParse->getRoot());

	// Set a parent for every DepNode, except the future root 
	// of the tree, which will be stored.
	DepNode *root = 0;
	for (size_t i = 0; i < _terminalSynNodes.size(); i++) {
		const SynNode *synNode = _terminalSynNodes[i];
		const SynNode *highestHead = synNode->getHighestHead();
		if (highestHead->getParent() == 0) {
			if (root != 0) {
				SessionLogger::info("SERIF") << fullParse->getRoot()->toDebugString(0) << "\n";
				throw InternalInconsistencyException("DependencyParser::convertFullParse", "Found more than one dependency tree root.");
			}
			root = _convertedDepNodes[i];
		} else {
			const SynNode *parent = highestHead->getParent();
			const SynNode *headPreterm = parent->getHeadPreterm();
			_convertedDepNodes[i]->setParent(_convertedDepNodes[headPreterm->getStartToken()]);
		}
	}

	if (root == 0) {
		SessionLogger::info("SERIF") << fullParse->getRoot()->toDebugString(0) << "\n";
		throw InternalInconsistencyException("DependencyParser::convertFullParse", "Could not find dependency tree root.");
	}

	setChildren(_convertedDepNodes);
	setTokenSpans(_convertedDepNodes);

	// Fix our IDs to mimic what the Parse IDs do (depth first, 0-indexed per sentence)
	_nodeIDGenerator.reset();
	setIDsDepthFirst(root);

	// Create theory and return
	results[0] = new DependencyParseTheory(fullParse->getTokenSequence());
	results[0]->setParse(root);

	return 1;
}

// Fill in children vectors for each DepNode
// Requires that the parent variable is already set
void DependencyParser::setChildren(std::vector<DepNode *> depNodes) {
	for (size_t i = 0; i < depNodes.size(); i++) {
		DepNode *depNode = depNodes[i];
		std::vector<DepNode *> children;
		for (size_t j = 0; j < depNodes.size(); j++) {
			if (i == j) continue;
			DepNode *possibleChild = depNodes[j];
			if (possibleChild->getParent() == depNode)
				children.push_back(possibleChild);
		}
		if (children.size() > 0) {
			DepNode** childrenArray = new DepNode*[children.size()];
			for (size_t k = 0; k < children.size(); k++) {
				childrenArray[k] = children[k];
			}
			depNode->setChildren(static_cast<int>(children.size()), 0, reinterpret_cast<SynNode**>(childrenArray));
		}
	}
}

void DependencyParser::setIDsDepthFirst(DepNode* node) {
	node->setID(_nodeIDGenerator.getID());
	for (int i = 0; i < node->getNChildren(); i++) {
		const DepNode* child = (const DepNode*) node->getChild(i);
		setIDsDepthFirst(const_cast<DepNode*>(child));
	}
}

void DependencyParser::setTokenSpans(std::vector<DepNode *> depNodes) {
	// Set token spans
	for (size_t i = 0; i < depNodes.size(); i++) {
		DepNode *depNode = depNodes[i];
		int start_token = getStartToken(depNode); 
		int end_token = getEndToken(depNode);
		depNode->setTokenSpan(start_token, end_token);
	}
}

int DependencyParser::getStartToken(const DepNode *node) {
	int min = node->getTokenNumber();
	for (int i = 0; i < node->getNChildren(); i++) {
		int start_token = getStartToken(static_cast<const DepNode*>(node->getChild(i)));
		if (start_token < min)
			min = start_token;
	}
	return min;
}

int DependencyParser::getEndToken(const DepNode *node) {
	int max = node->getTokenNumber();
	for (int i = 0; i < node->getNChildren(); i++) {
		int end_token = getEndToken(static_cast<const DepNode*>(node->getChild(i)));
		if (end_token > max)
			max = end_token;
	}
	return max;
}

void DependencyParser::collectDepNodes(const SynNode *node) {

	if (node->isPreterminal()) {
		DepNode *depNode = new DepNode(_nodeIDGenerator.getID(), 0, Symbol(L"UNK"), 0, node->getHeadWord(), node->getTag(), node->getStartToken());
		_convertedDepNodes[node->getStartToken()] = depNode;
		_terminalSynNodes[node->getStartToken()] = node->getFirstTerminal();
	}

	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		collectDepNodes(child);
	}	
}

Symbol DependencyParser::fixPartOfSpeechTag(Symbol tag) {
	if (tag == Symbol(L"DATE-NNP"))
		return Symbol(L"NNP");
	return tag;
}
