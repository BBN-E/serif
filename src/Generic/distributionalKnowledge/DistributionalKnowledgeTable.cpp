
#include "Generic/common/leak_detection.h"

#include <string>
#include <set>
#include <vector>

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolUtilities.h"
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "DistributionalKnowledgeTable.h"
#include "Generic/common/UnicodeUtil.h"
#include "time.h"

using namespace std;

bool DistributionalKnowledgeTable::_is_initialized = false;
Symbol DistributionalKnowledgeTable::_verbSym;
Symbol DistributionalKnowledgeTable::_nounSym;
Symbol DistributionalKnowledgeTable::_adjSym;
Symbol DistributionalKnowledgeTable::_advSym;
DistributionalKnowledgeTable::spScoreEntryMap* DistributionalKnowledgeTable::_vvScoreMap;
DistributionalKnowledgeTable::spScoreEntryMap* DistributionalKnowledgeTable::_nnScoreMap;
DistributionalKnowledgeTable::spScoreEntryMap* DistributionalKnowledgeTable::_vnScoreMap;
DistributionalKnowledgeTable::sIspMap* DistributionalKnowledgeTable::_nSubObjClusterMap;


void DistributionalKnowledgeTable::ensureInitialized() {
	if(_is_initialized)
		return;

	_verbSym = Symbol(L"VERB");
	_nounSym = Symbol(L"NOUN");
	_adjSym = Symbol(L"ADJ");
	_advSym = Symbol(L"ADV");

	DistributionalKnowledgeTable::_vvScoreMap = _new spScoreEntryMap(3872043);
	DistributionalKnowledgeTable::_nnScoreMap = _new spScoreEntryMap(2139718);
	DistributionalKnowledgeTable::_vnScoreMap = _new spScoreEntryMap(4498930);
	DistributionalKnowledgeTable::_nSubObjClusterMap = _new sIspMap(32993);

	std::cout << "Loading distributional knowledge resources" << std::endl;
	clock_t t = clock();


	// require .par : predicate_sub_score_file , predicate_obj_score_file
	std::string predicateSubScoreFile = ParamReader::getParam("predicate_sub_score_file");
	std::string predicateObjScoreFile = ParamReader::getParam("predicate_obj_score_file");
	std::cout << "- predicate_sub_score_file , predicate_obj_score_file" << std::endl;
	initPredicateSubObjScores(predicateSubScoreFile, predicateObjScoreFile);

	// require .par : predicate_sub_cluster_file , predicate_obj_cluster_file
	std::string predicateSubClusterFile = ParamReader::getParam("predicate_sub_cluster_file");
	std::string predicateObjClusterFile = ParamReader::getParam("predicate_obj_cluster_file");
	std::cout << "- predicate_sub_cluster_file , predicate_obj_cluster_file" << std::endl;
	initPredicateSubObjClusters(predicateSubClusterFile, predicateObjClusterFile);

	// require .par : vn_sim_file , nn_sim_file , vv_sim_file
	std::string vnSimFile = ParamReader::getParam("vn_sim_file");
	std::string nnSimFile = ParamReader::getParam("nn_sim_file");
	std::string vvSimFile = ParamReader::getParam("vv_sim_file");
	std::cout << "- (vn, nn, vv) sim files" << std::endl;
	readWWSimScores(vnSimFile, nnSimFile, vvSimFile);

	// require .par : vn_pmi_file , nn_pmi_file , vv_pmi_file
	std::string vnPmiFile = ParamReader::getParam("vn_pmi_file");
	std::string nnPmiFile = ParamReader::getParam("nn_pmi_file");
	std::string vvPmiFile = ParamReader::getParam("vv_pmi_file");
	std::cout << "- (vn, nn, vv) pmi files" << std::endl;
	readWWPmiScores(vnPmiFile, nnPmiFile, vvPmiFile);

	// require .par : causal_score_file
	std::string causalScoreFile = ParamReader::getParam("causal_score_file");
	std::cout << "- causal score file" << std::endl;
	readCausalScores(causalScoreFile);


	//std::cout << "cluster hash: #buckets=" << _nSubObjClusterMap->getNumBuckets() << " #nonEmptyBuckets=" << _nSubObjClusterMap->get_num_nonempty_buckets() << " #entries=" << _nSubObjClusterMap->getNumEntries() << std::endl;
	//std::cout << "vv hash: #buckets=" << _vvScoreMap->getNumBuckets() << " #nonEmptyBuckets=" << _vvScoreMap->get_num_nonempty_buckets() << " #entries=" << _vvScoreMap->getNumEntries() << std::endl;
	//std::cout << "nn hash: #buckets=" << _nnScoreMap->getNumBuckets() << " #nonEmptyBuckets=" << _nnScoreMap->get_num_nonempty_buckets() << " #entries=" << _nnScoreMap->getNumEntries() << std::endl;
	//std::cout << "vn hash: #buckets=" << _vnScoreMap->getNumBuckets() << " #nonEmptyBuckets=" << _vnScoreMap->get_num_nonempty_buckets() << " #entries=" << _vnScoreMap->getNumEntries() << std::endl;

	_is_initialized = true;

	t = clock() - t;

	std::cout << "DONE loading distributional knowledge resources in " << (((float)t)/CLOCKS_PER_SEC) << " seconds" << std::endl;
}


void DistributionalKnowledgeTable::initPredicateSubObjClusters(const string& subFile, const string& objFile) {
	std::vector<std::wstring> tokens;
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(subFile.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	std::wstring line;
        // the 'all plausible verb cluster ids' returns too many associated verbs. 
        // to avoid generating too many features, keep only those verbs which are highly associated with current candidate argument
	float scoreThreshold = 0.8f;

	int runningCid = 1;

	tokens.clear();
	in.getLine(line);
	while(!in.eof()) {
		boost::split(tokens, line, boost::is_any_of(L" "));
		Symbol pred = Symbol(tokens[0]);
		for(unsigned j=2; j<tokens.size(); j++) {
			Symbol arg = Symbol(tokens[j]);
			float score = lookupPairInMap(pred, arg, ScoreEntry::PRED_SUB, _vnScoreMap);
			if(score >= scoreThreshold) 
				(*_nSubObjClusterMap)[arg].first.insert(runningCid);	
		}	

		tokens.clear();
		in.getLine(line);
		runningCid += 1;
	}
	in.close();

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr2(UTF8InputStream::build(objFile.c_str()));
	UTF8InputStream& in2(*in_scoped_ptr2);
	tokens.clear();
	in2.getLine(line);
	while(!in2.eof()) {
		boost::split(tokens, line, boost::is_any_of(L" "));
		Symbol pred = Symbol(tokens[0]);
		for(unsigned j=2; j<tokens.size(); j++) {
			Symbol arg = Symbol(tokens[j]);
			float score = lookupPairInMap(pred, arg, ScoreEntry::PRED_OBJ, _vnScoreMap);
			if(score >= scoreThreshold) 
				(*_nSubObjClusterMap)[arg].second.insert(runningCid);
		}	

		tokens.clear();
		in2.getLine(line);
		runningCid += 1;
	}
	in2.close();
}

int DistributionalKnowledgeTable::readSymbolPairFloatTable(const std::string& fname, spScoreEntryMap* table, int entrysize, int entryindex, int handle_order) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(fname.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	std::vector<std::wstring> tokens;
	std::wstring line;
	float scoreThreshold = 0.5f;
	spScoreEntryMap::iterator it;

	tokens.clear();
	int nlines = 0;
	in.getLine(line);
	while(!in.eof()) {
		boost::split(tokens, line, boost::is_any_of(L" "));

		if(tokens.size() != 3) {
			std::stringstream errmsg;
			errmsg << "Error reading line" << nlines << " from file: " << fname;
			throw UnrecoverableException("DistributionalKnowledgeTable::readSymbolPairFloatTable", errmsg.str().c_str());
		}

		Symbol w1 = Symbol(tokens[0]);
		Symbol w2 = Symbol(tokens[1]);
		float score = boost::lexical_cast<float>(tokens[2]);

		// the scores range from 0 to 1.0, and we are not interested in scores below 0.5
		if(score >= scoreThreshold) {
			if(handle_order == REORDER) {
				if(!(w1 < w2)) {
					Symbol temp = w1;
					w1 = w2;
					w2 = temp;
				}
			}
	
			SymbolPair sp = std::pair<Symbol, Symbol>(w1, w2);
			it = (*table).find(sp);
			if(it == (*table).end()) {
				for(int i=0; i<entrysize; i++){
					(*table)[sp].scores[i] = -1;
				}
				(*table)[sp].scores[entryindex] = score;
			}
			else {	// there was a prior entry. Check whether that, or current score is larger. Use the larger score.
				float priorScore = (*it).second.scores[entryindex];
				if(score > priorScore) {
					(*table)[sp].scores[entryindex] = score;
				}
			}
		}

		tokens.clear();
		in.getLine(line);
		nlines++;
	}

	in.close();
	return nlines;
}

void DistributionalKnowledgeTable::initPredicateSubObjScores(const string& subFile, const string& objFile) {
	int n_subj = readSymbolPairFloatTable(subFile, _vnScoreMap, VN_ENTRY_SIZE, ScoreEntry::PRED_SUB);
	int n_obj = readSymbolPairFloatTable(objFile, _vnScoreMap, VN_ENTRY_SIZE, ScoreEntry::PRED_OBJ);
}

void DistributionalKnowledgeTable::readWWSimScores(const string& vnFile, const string& nnFile, const string& vvFile) {
	int n_vn = readSymbolPairFloatTable(vnFile, _vnScoreMap, VN_ENTRY_SIZE, ScoreEntry::SIM);
	int n_nn = readSymbolPairFloatTable(nnFile, _nnScoreMap, NN_ENTRY_SIZE, ScoreEntry::SIM, REORDER);
	int n_vv = readSymbolPairFloatTable(vvFile, _vvScoreMap, VV_ENTRY_SIZE, ScoreEntry::SIM, REORDER);
}

void DistributionalKnowledgeTable::readWWPmiScores(const string& vnFile, const string& nnFile, const string& vvFile) {
	int n_vn = readSymbolPairFloatTable(vnFile, _vnScoreMap, VN_ENTRY_SIZE, ScoreEntry::PMI);
	int n_nn = readSymbolPairFloatTable(nnFile, _nnScoreMap, NN_ENTRY_SIZE, ScoreEntry::PMI, REORDER);
	int n_vv = readSymbolPairFloatTable(vvFile, _vvScoreMap, VV_ENTRY_SIZE, ScoreEntry::PMI, REORDER);
}

void DistributionalKnowledgeTable::readCausalScores(const std::string& filename) {
	int n_vv = readSymbolPairFloatTable(filename, _vvScoreMap, VV_ENTRY_SIZE, ScoreEntry::CAUSE, REORDER);
}

///////////////////////////

float DistributionalKnowledgeTable::lookupPairInMap(const Symbol& s1, const Symbol& s2, int entryindex, spScoreEntryMap* table, int handle_order) {
	spScoreEntryMap::iterator it;
	float score = -1;
	Symbol w1 = s1;
	Symbol w2 = s2;

	if(handle_order == REORDER) {
		if(!(w1 < w2)) {
			Symbol temp = w1;
			w1 = w2;
			w2 = temp;
		}
	}
	
	SymbolPair sp = std::pair<Symbol, Symbol>(w1, w2);
	it = (*table).find(sp);
	if(it != (*table).end()) {
		score = (*it).second.scores[entryindex];
	}

	return score;
}

float DistributionalKnowledgeTable::getCausalScore(const Symbol& v1, const Symbol& v2) {
	return lookupPairInMap(v1, v2, ScoreEntry::CAUSE, DistributionalKnowledgeTable::_vvScoreMap, REORDER);
}

float DistributionalKnowledgeTable::getVNPmi(const Symbol& v, const Symbol& n) {
	return lookupPairInMap(v, n, ScoreEntry::PMI, _vnScoreMap);
}

float DistributionalKnowledgeTable::getNNPmi(const Symbol& n1, const Symbol& n2) {
	return lookupPairInMap(n1, n2, ScoreEntry::PMI, _nnScoreMap, REORDER);
}

float DistributionalKnowledgeTable::getVVPmi(const Symbol& v1, const Symbol& v2) {
	return lookupPairInMap(v1, v2, ScoreEntry::PMI, _vvScoreMap, REORDER);
}

float DistributionalKnowledgeTable::getVNSim(const Symbol& v, const Symbol& n) {
	return lookupPairInMap(v, n, ScoreEntry::SIM, _vnScoreMap);
}

float DistributionalKnowledgeTable::getNNSim(const Symbol& n1, const Symbol& n2) {
	return lookupPairInMap(n1, n2, ScoreEntry::SIM, _nnScoreMap, REORDER);
}

float DistributionalKnowledgeTable::getVVSim(const Symbol& v1, const Symbol& v2) {
	return lookupPairInMap(v1, v2, ScoreEntry::SIM, _vvScoreMap, REORDER);
}

float DistributionalKnowledgeTable::getPredicateSubScore(const Symbol& p, const Symbol& w) {
	return lookupPairInMap(p, w, ScoreEntry::PRED_SUB, _vnScoreMap);
}

float DistributionalKnowledgeTable::getPredicateObjScore(const Symbol& p, const Symbol& w) {
	return lookupPairInMap(p, w, ScoreEntry::PRED_OBJ, _vnScoreMap);
}

std::set<int> DistributionalKnowledgeTable::getSubClusterIdsForArg(const Symbol& arg) {
	sIspMap::iterator it;
	std::set<int> result;

	it = (*_nSubObjClusterMap).find(arg);
	if(it != (*_nSubObjClusterMap).end()) {
		result = (*it).second.first;
	}

	return result;
}

std::set<int> DistributionalKnowledgeTable::getObjClusterIdsForArg(const Symbol& arg) {
	sIspMap::iterator it;
	std::set<int> result;

	it = (*_nSubObjClusterMap).find(arg);
	if(it != (*_nSubObjClusterMap).end()) {
		result = (*it).second.second;
	}

	return result;
}

