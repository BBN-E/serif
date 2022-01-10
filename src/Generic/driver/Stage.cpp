// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/driver/Stage.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"

#include <stdio.h>
#include <string.h>
#include <vector>
#include <set>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "Generic/linuxPort/serif_port.h"

typedef const char* InitialStageInfoRecord[3];
InitialStageInfoRecord INITIAL_STAGE_INFO[] = {
	// name              state-saver?        description
	// --------------------------------------------------------------
	{"sent-break",           "y", "Sentence breaking"},
	{"tokens",               "y", "Tokenization"},
	{"part-of-speech",       "y", "Part-of-speech tagging"},
	{"names",                "y", "Named entity mention detection"},
	{"nested-names",         "y", "Nested named entity mention detection"},
	{"values",               "y", "Value mention detection"},
	{"parse",                "y", "Syntactic parsing"},
	{"npchunk",              "y", "Noun phrase chunking"},
	{"dependency-parse",     "y", "Dependency parsing"},
	{"mentions",             "y", "Entity mention detection"},
	{"actor-match",          "y", "Actor match detection"},
	{"props",                "y", "Proposition finding"},
	{"metonymy",             "y", "Metonymy detection"},
	{"entities",             "n", "Within-sentence entity co-reference"},
	{"events",               "n", "Within-sentence event detection"},
	{"relations",            "n", "Within-sentence relation finding"},
	{"sent-level-end",       "y", "End of sentence-level processing"}, // marker stage
	{"prop-status",          "y", "Proposition Status classifier"},
	{"doc-entities",         "y", "Whole-document entity coreference"},
	{"doc-relations-events", "y", "Whole-document relation and event detection"},
	{"doc-values",           "y", "Whole-document value coreference"},
	{"confidences",          "n", "Confidence estimation"},
	{"generics",             "n", "Generic detection"},
	{"doc-actors",           "n", "Create and score actors for Entities"},
	{"factfinder",			 "n", "Fact finding (for entity profiles)"},
	{"clutter",              "y", "Clutter removal"},
	{"xdoc",                 "n", "Cross-document coreference"},
	{"cause-effect",         "n", "Cause-Effect pattern matching"},
	{"output",               "n", "Output generation"},
	{"score",                "n", "Scoring"},
};

/*===================================================================
 * StageInfo
 *===================================================================*/

/** A private class used to record information about a single processing
  * stage.  Each "Stage" object contains a pointer to a StageInfo object,
  * and two Stage objects are equal iff they point at the same StageInfo.
  */
struct Stage::StageInfo {
	std::string name;
	std::string description;
	StageInfo(const char* name, const char* description, 
			  bool okToSaveStateAfterThisStage)
		: name(name), description(description), sequenceNum(-1),
		  okToSaveStateAfterThisStage(okToSaveStateAfterThisStage) {}
	boost::shared_ptr<StageInfo> nextStage;
	boost::shared_ptr<StageInfo> prevStage;
	int sequenceNum;
	bool okToSaveStateAfterThisStage; // can we use the state saver after this stage?
};

class Stage::StageSequence {
public:
	typedef std::vector<StageInfo_ptr>::iterator iterator;
	typedef std::vector<StageInfo_ptr>::const_iterator const_iterator;

	StageSequence() {
		// Create the null stage
		_nullInfo = boost::make_shared<Stage::StageInfo>("NULL", "NULL Stage", false);
		_nullInfo->sequenceNum = -1;
		_nullInfo->nextStage = _nullInfo;
		_nullInfo->prevStage = _nullInfo;
		// Set up the special start/end marker stage
		_stageInfos.push_back(boost::make_shared<Stage::StageInfo>("start", "Start of pipeline", true));
		_stageInfos.push_back(boost::make_shared<Stage::StageInfo>("end", "End of pipeline", true));
		// Set up the initial stages
		size_t num_initial_stages = sizeof(INITIAL_STAGE_INFO)/sizeof(InitialStageInfoRecord);
		for (size_t i=0; i<num_initial_stages; ++i) {
			if (boost::iequals(INITIAL_STAGE_INFO[i][1], "y")) {
				appendStage(INITIAL_STAGE_INFO[i][0], INITIAL_STAGE_INFO[i][2], true);
			} else if (boost::iequals(INITIAL_STAGE_INFO[i][1], "n")) {
				appendStage(INITIAL_STAGE_INFO[i][0], INITIAL_STAGE_INFO[i][2], false);
			} else {
				throw InternalInconsistencyException("StageSequence::appendStage", "Bad INITIAL_STAGE_INFO row");
			}
		}
	}
	void appendStage(const char* name, const char* description, bool okToSaveStateAfterThisStage) {
		insertStage(_stageInfos.back(), name, description, okToSaveStateAfterThisStage, false);
	}
	void insertStage(StageInfo_ptr ref, const char* name, const char* description,
	                 bool okToSaveStateAfterThisStage, bool insert_after) {
		if (boost::iequals(ref->name, "start") && (!insert_after))
			throw InternalInconsistencyException("StageSequence::appendStage", "No stage may be inserted before 'start'");
		if (boost::iequals(ref->name, "end") && (insert_after))
			throw InternalInconsistencyException("StageSequence::appendStage", "No stage may be inserted after 'end'");
		if (_stageNames.find(name) != _stageNames.end())
			throw InternalInconsistencyException("StageSequence::appendStage", "A stage with this name already exists.");
		std::vector<StageInfo_ptr>::iterator it = std::find(_stageInfos.begin(), _stageInfos.end(), ref);
		if (it == _stageInfos.end())
			throw InternalInconsistencyException("StageSequence::insertStage", "Reference StageInfo not found!");
		if (insert_after) ++it;
		_stageInfos.insert(it, boost::make_shared<StageInfo>(name, description, okToSaveStateAfterThisStage));
		updateSequenceNumbersAndPointers();
		_stageNames.insert(name);
	}
	void swapStageOrder(StageInfo_ptr stage1, StageInfo_ptr stage2) {
		if (boost::iequals(stage1->name, "start") || boost::iequals(stage1->name, "end") ||
			boost::iequals(stage2->name, "start") || boost::iequals(stage2->name, "end"))
			throw InternalInconsistencyException("StageSequence::swapStageOrder", "start and end stage may not be re-ordered");
		std::vector<StageInfo_ptr>::iterator it1 = std::find(_stageInfos.begin(), _stageInfos.end(), stage1);
		std::vector<StageInfo_ptr>::iterator it2 = std::find(_stageInfos.begin(), _stageInfos.end(), stage2);
		if ((it1 == _stageInfos.end()) || (it2 == _stageInfos.end()))
			throw InternalInconsistencyException("StageSequence::swapStageOrder", "StageInfo not found!");
		std::swap(*it1, *it2);
		updateSequenceNumbersAndPointers();
	}
	StageInfo_ptr &startStage() { return _stageInfos.front(); }
	StageInfo_ptr &endStage() { return _stageInfos.back(); }
	iterator begin() { return _stageInfos.begin(); }
	iterator end() { return _stageInfos.end(); }
	StageInfo_ptr null() { return _nullInfo; }
private:
	std::set<std::string> _stageNames; // to ensure uniqueness.
	std::vector<StageInfo_ptr> _stageInfos;
	StageInfo_ptr _nullInfo;
	void updateSequenceNumbersAndPointers() {
		for (int i=0; i<static_cast<int>(_stageInfos.size()); ++i) {
			_stageInfos[i]->sequenceNum = i;
			_stageInfos[i]->prevStage = _stageInfos[std::max<int>(i-1, 0)];
			_stageInfos[i]->nextStage = _stageInfos[std::min<int>(i+1, static_cast<int>(_stageInfos.size())-1)];
		}
		// NULL comes just after the last stage.
		endStage()->nextStage = _nullInfo;
		_nullInfo->sequenceNum = endStage()->sequenceNum+1;
	}
};

Stage::StageSequence& Stage::getStageSequence() {
	static StageSequence stageSequence;
	return stageSequence;
}

/*===================================================================
 * Stage
 *===================================================================*/

Stage::Stage() : _info(getStageSequence().null()) {}
Stage::Stage(const Stage &other): _info(other._info) {}
Stage& Stage::operator=(const Stage &other) { 
	_info = other._info; return *this; }

Stage::Stage(const char *name): _info() {
	std::string nameStr(name);
	// Check if there's an offset -- eg "tokens+1"
	int offset = 0;
	static const boost::regex OFFSET_RE("^(.*)([+-])([0-9]+)$");
	boost::smatch match;
	if (boost::regex_match(nameStr, match, OFFSET_RE)) {
		if (match[2] == "-")
			offset = -boost::lexical_cast<int>(match[3]);
		else
			offset = boost::lexical_cast<int>(match[3]);
		nameStr = match[1];
	}

	// Look up the stage by name.
	for (std::vector<StageInfo_ptr>::const_iterator iter = getStageSequence().begin(); iter != getStageSequence().end(); ++iter) {
		StageInfo_ptr stageInfo = (*iter);
		if (boost::iequals(nameStr, stageInfo->name)) {
			_info = stageInfo;
			break;
		}
	}
	if (!_info) {
		std::ostringstream err;
		err << "Unknown stage: \"" << name << "\"";
		throw UnexpectedInputException("Stage::Stage()", err.str().c_str());
	}

	// Apply the offset if there was one.
	for (int i=0; i<offset; ++i)
		_info = _info->nextStage;
	for (int i=0; i<(-offset); ++i)
		_info = _info->prevStage;

}

void Stage::swapStageOrder(Stage stage1, Stage stage2) {
	getStageSequence().swapStageOrder(stage1._info, stage2._info);
}

Stage Stage::addNewStageAfter(Stage refStage, const char* name, const char* description, bool okToSaveStateAfterThisStage) {
	getStageSequence().insertStage(refStage._info, name, description, okToSaveStateAfterThisStage, true);
	return refStage.getNextStage();
}

Stage Stage::addNewStageBefore(Stage refStage, const char* name, const char* description, bool okToSaveStateAfterThisStage) {
	getStageSequence().insertStage(refStage._info, name, description, okToSaveStateAfterThisStage, false);
	return refStage.getPrevStage();
}

int Stage::getSequenceNumber() const { return _info->sequenceNum; }
const char *Stage::getName() const { return _info->name.c_str(); }
const char* Stage::getDescription() const { return _info->description.c_str(); }
bool Stage::okToSaveStateAfterThisStage() const { return _info->okToSaveStateAfterThisStage; }

Stage &Stage::operator++() {
	_info = _info->nextStage;
	return *this;
}
Stage &Stage::operator--() {
	_info = _info->prevStage;
	return *this;
}

Stage Stage::getPrevStage() {
	return Stage(_info->prevStage);
}

Stage Stage::getNextStage() {
	return Stage(_info->nextStage);
}

bool Stage::operator==(const Stage &other) const {
	return _info->sequenceNum == other._info->sequenceNum;
}
bool Stage::operator!=(const Stage &other) const {
	return _info->sequenceNum != other._info->sequenceNum;
}
bool Stage::operator<(const Stage &other) const {
	return _info->sequenceNum < other._info->sequenceNum;
}
bool Stage::operator<=(const Stage &other) const {
	return _info->sequenceNum <= other._info->sequenceNum;
}
bool Stage::operator>(const Stage &other) const {
	return _info->sequenceNum > other._info->sequenceNum;
}
bool Stage::operator>=(const Stage &other) const {
	return _info->sequenceNum >= other._info->sequenceNum;
}

Stage Stage::getLastSentenceLevelStage() {
	return Stage("sent-level-end").getPrevStage();
}

Stage Stage::getStartStage() {
	return Stage(getStageSequence().startStage());
}

Stage Stage::getEndStage() {
	return Stage(getStageSequence().endStage());
}

Stage Stage::getFirstStage() {
	return getStartStage().getNextStage();
}

Stage Stage::getLastStage() {
	return getEndStage().getPrevStage();
}

size_t Stage::hash_code() const {
	return _info->sequenceNum;
}
