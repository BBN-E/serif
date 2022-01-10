// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <string>

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/GenericTimer.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/common/ParamReader.h"
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>

using namespace std;

DTTagSet::DTTagSet(const char *tag_set_file, bool generate_st_co_suffix,
				   bool add_start_end_tags, bool interleave_st_co_tags)
	: _noneTag(Symbol(L"NONE")), _noneSTTag(Symbol(L"NONE-ST")), _noneCOTag(Symbol(L"NONE-CO")),
	  _startTag(Symbol(L"START")), _endTag(Symbol(L"END")),
	  _linkTag1(Symbol(L"LINK")), _linkTag2(Symbol(L"o[link]")),
	  _start_tag_index(-1), _end_tag_index(-1),
	  _generate_st_co_suffix(generate_st_co_suffix),
	  _add_start_end_tags(add_start_end_tags),
	  _interleave_st_co_tags(interleave_st_co_tags)
{

	_use_nested_names = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_nested_names", false);
	if (_use_nested_names && !_generate_st_co_suffix)
		throw UnrecoverableException("DTTagSet::DTTagSet()", "Enabling nested names requires ST/CO tags to be enabled");

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(tag_set_file);
	if (in.fail()) {
		string message;
		message = "Unable to open tag set file: '";
		message += tag_set_file;
		message += "'";
		throw UnexpectedInputException("DTTagSet::DTTagSet()", message.c_str());
	}

	// determine the initial number of tags
	int n_reduced_tags;
	in >> n_reduced_tags;
	_reducedTags.reserve(n_reduced_tags);
	_tagArray.reserve(calcTotalNumTags(n_reduced_tags));

	// load the reduced tags
	//   START and END have to be inserted last, otherwise the ValueRecognizer misses some TIMEX values
	//   for an unknown side-effect reason. The order of succ/pred transitions shouldn't matter, but does
	//   for that particular use of DTTagSet. This shouldn't matter for name transitions added later when
	//   dynamic name spans are being used.
	addNoneTags();
	for (int i = 0; i < n_reduced_tags; i++) {
		UTF8Token token;
		in >> token;
		addTag(token.symValue());
	}
	addStartEndTags();
	setupSuccPred();
}

DTTagSet::~DTTagSet() {
	BOOST_FOREACH(DTTag* tag, _tagArray) {
		delete tag;
	}
}

int DTTagSet::getNRegularTags() const	{
	if (!_add_start_end_tags) // no START or END tags
		return static_cast<int>(_tagArray.size());
	else                              // START and END tag
		return static_cast<int>(_tagArray.size()) - 2;
}

const Symbol &DTTagSet::getSemiReducedTagSymbol(int index) const {
	if (index >= 0 && index < static_cast<int>(_tagArray.size())) {
		return _tagArray[index]->_semiReducedTag;
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::getSemiReducedTagSymbol()", _tagArray.size(), index);
	}
}

std::set<int> DTTagSet::getSuccessorTags(int index) const {
	if (index >= 0 && index < static_cast<int>(_tagArray.size())) {
		return _tagArray[index]->getSuccessorTags();
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::getSuccessorTags()", _tagArray.size(), index);
	}
}

std::set<int> DTTagSet::getPredecessorTags(int index) const {
	if (index >= 0 && index < static_cast<int>(_tagArray.size())) {
		return _tagArray[index]->getPredecessorTags();
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::getPredecessorTags()", _tagArray.size(), index);
	}
}

bool DTTagSet::isSTTag(int index) const {
	if (index >= 0 && index < static_cast<int>(_tagArray.size())) {
		return _tagArray[index]->_isST;
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::isSTTag()", _tagArray.size(), index);
	}
}

bool DTTagSet::isCOTag(int index) const{
	if (index >= 0 && index < static_cast<int>(_tagArray.size())) {
		return _tagArray[index]->_isCO;
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::isCOTag()", _tagArray.size(), index);
	}
}

bool DTTagSet::isNestedTag(int index) const {
	if (index >= 0 && index < static_cast<int>(_tagArray.size())) {
		return _tagArray[index]->_isNested;
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::isNestedTag()", _tagArray.size(), index);
	}
}

bool DTTagSet::isNoneTag(int index) const {
	// Sanity check: getTagIndex returns a valid index or -1 if none tags
	// were somehow not found; we do not want to treat -1 as a none tag
	if (index == -1)
		return false;
	return 
		(index == getTagIndex(_noneSTTag)) || 
		(index == getTagIndex(_noneCOTag)) ||
		(index == getTagIndex(_noneTag));
}

void DTTagSet::dump() const {
	int i;
	cout << "========= All Tags: " << endl;
	for (i = 0; i < static_cast<int>(_tagArray.size()); i++) {
		if (getTagSymbol(i).is_null())
			break;
		cout << setw(3) << i;
		_tagArray[i]->dump();
	}
	cout << endl;
	cout << "========= Successors: " << endl;
	for (i = 0; i < static_cast<int>(_tagArray.size()); i++) {
		if (getTagSymbol(i).is_null())
			break;
		std::set<int> succs = getSuccessorTags(i);
		cout << getTagSymbol(i).to_debug_string() 
			<< ": "
			<< succs.size() << endl;
		cout << "\t ";
		BOOST_FOREACH(int succ, succs) {
			cout << getTagSymbol(succ).to_debug_string() << " ";
		}
		cout << endl;
	}
	cout << endl;
	cout << "========= Predecessors: " << endl;
	for (i = 0; i < static_cast<int>(_tagArray.size()); i++) {
		if (getTagSymbol(i).is_null())
			break;
		std::set<int> preds = getPredecessorTags(i);
		cout << getTagSymbol(i).to_debug_string()
			<< ": " 
			<< preds.size() << endl;
		cout << "\t ";
		BOOST_FOREACH(int pred, preds) {
			cout << getTagSymbol(pred).to_debug_string() << " ";
		}
		cout << endl;
	}
}

void DTTagSet::resetSuccessorTags() {
	BOOST_FOREACH(DTTag* tag, _tagArray) {
		tag->resetSuccessorTags();
	}
}
void DTTagSet::resetPredecessorTags() {
	BOOST_FOREACH(DTTag* tag, _tagArray) {
		tag->resetPredecessorTags();
	}
}

void DTTagSet::addTransition(const Symbol &prevTag, const Symbol &nextTag) {
	addTransition(getTagIndex(prevTag), getTagIndex(nextTag));
}

void DTTagSet::addTransition(int prevIndex, int nextIndex) {
	if (prevIndex >= 0 && prevIndex < static_cast<int>(_tagArray.size())) {
		_tagArray[prevIndex]->addSucc(nextIndex);
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::addTransition()", _tagArray.size(), prevIndex);
	}
	if (nextIndex >= 0 && nextIndex < static_cast<int>(_tagArray.size())) {
		_tagArray[nextIndex]->addPred(prevIndex);
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DTTagSet::addTransition()", _tagArray.size(), nextIndex);
	}
}

void DTTagSet::writeTransitions(const char* file) const {
	UTF8OutputStream out;
	out.open(file);
	int ntrans = 0;
	for (int j = 0; j < static_cast<int>(_tagArray.size()); j++){
		ntrans += static_cast<int>(getSuccessorTags(j).size());
	}
	out << ntrans << "\n";
	for (int i = 0; i < static_cast<int>(_tagArray.size()); i++) {
		std::set<int> succs = getSuccessorTags(i);
		BOOST_FOREACH(int succ, succs) {
			out << getTagSymbol(i) << " " << getTagSymbol(succ) << "\n";
		}
	}
	out.close();
}

void DTTagSet::readTransitions(const char* file) {	
	resetPredecessorTags();
	resetSuccessorTags();
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	uis.open(file);
	int ntrans;
	uis >> ntrans;
	UTF8Token tok;
	for (int i = 0; i < ntrans; i++) {
		uis >> tok;
		Symbol prevtag = tok.symValue();
		uis >> tok;
		Symbol nexttag = tok.symValue();
		addTransition(prevtag, nexttag);
	}
	uis.close();
}

int DTTagSet::calcTotalNumTags(int n_reduced_tags) const 
{
	int total = 0;
	int n_start_end = _add_start_end_tags ? 2 : 0;  // never nest, never take -ST or -CO
	int n_none = 1;  // reduced NONE

	// determine the number of semi-reduced tags (X=Y), not including completely reduced tags (X and Y)
	int n_semireduced_tags = _use_nested_names ? n_reduced_tags * n_reduced_tags : 0;	

	if (_generate_st_co_suffix) {
		// how many extra spaces for -ST and -CO tags?
		int n_none_stco = 2 * n_none;  // NONE-ST, NONE-CO
		int	n_reduced_stco = 2 * n_reduced_tags;  // X-ST, X-CO, Y-ST, Y-CO
		int n_nested_stco = 3 * n_semireduced_tags;  // nested tags take -ST, -CO, and -STST
		total = n_start_end + n_none_stco + n_reduced_stco + n_nested_stco;
	} else {
		total = n_start_end + n_none + n_reduced_tags + n_semireduced_tags;
	}
	//printf("tags in file: %d, st-co: %d, start-end: %d, nested: %d, return total number %d\n",
	//	n_reduced_tags, generate_st_co_suffix, add_start_end_tags, _use_nested_names, total);
	return total;
}

void DTTagSet::addStartEndTags() {
	if (_add_start_end_tags) {
		_start_tag_index = static_cast<int>(_tagArray.size());
		_tagArray.push_back(_new DTTag(_startTag, _startTag, _startTag));
		_tagMap[_startTag] = _start_tag_index;

		_end_tag_index = static_cast<int>(_tagArray.size());
		_tagArray.push_back(_new DTTag(_endTag, _endTag, _endTag));
		_tagMap[_endTag] = _end_tag_index;
	}
}

/* public convenience function that adds the specified reduced tag
   and updates nested tags if necessary */
void DTTagSet::addTag(const Symbol &reducedTag) {
	// don't allow reserved tag names
	if (reducedTag == _noneTag || reducedTag == _startTag || reducedTag == _endTag) {
		std::stringstream err;
		err << "Cannot add reserved tag name '" << reducedTag << "' to tag set";
		throw UnexpectedInputException("DTTagSet::addTag()", err.str().c_str());
	}

	// do nothing if the tag is already defined
	if (std::find(_reducedTags.begin(), _reducedTags.end(), reducedTag) != _reducedTags.end())
		return;

	// add the tag
	_reducedTags.push_back(reducedTag);
	addTag(reducedTag, reducedTag);

	// generate and add any nested tags if they're enabled
	if (_use_nested_names) {
		BOOST_FOREACH(Symbol otherTag, _reducedTags) {
			Symbol outerNestedTag = reducedTag + Symbol(L"=") + otherTag;
			addTag(reducedTag, outerNestedTag, true);
			if (otherTag != reducedTag) {
				// We're updating as we go, but we don't want to do the same reduced tag twice
				Symbol innerNestedTag = otherTag + Symbol(L"=") + reducedTag;
				addTag(otherTag, innerNestedTag, true);
			}
		}

		// we need to completely rebuild successors/predecessors when nested tags are being used (which is rare)
		setupSuccPred();
	}
}

/* adds both the tag X and the tag X-ST and X-CO (if using suffixes)
   or, if X=Y is the tag, makes X=Y-ST and X=Y-CO.
   the reducedTag and semiReduced tag are never calculated but instead passed in; no error checking
   is done to verify that they're correct or even in any way related */
void DTTagSet::addTag(const Symbol &reducedTag, const Symbol &semiReducedTag, bool makeSTST) {
	//std::cout<<"Reduced: "<<reducedTag.to_debug_string()<<"\tSemi: "<<semiReducedTag.to_debug_string()<<endl;

	if (_generate_st_co_suffix) {
		// make the -ST tag
		int st_index = static_cast<int>(_tagArray.size());
		Symbol tag_ST = semiReducedTag + Symbol(L"-ST");
		_tagArray.push_back(_new DTTag(tag_ST, reducedTag, semiReducedTag));
		_tagArray[st_index]->_isST = !makeSTST; // for nested tags, X=Y-ST is NOT a true ST tag
		_tagArray[st_index]->_isNested = makeSTST;
		_tagMap[tag_ST] = st_index;

		// make the -CO tag
		int co_index = static_cast<int>(_tagArray.size());
		Symbol tag_CO = semiReducedTag + Symbol(L"-CO");
		_tagArray.push_back(_new DTTag(tag_CO, reducedTag, semiReducedTag));
		_tagArray[co_index]->_isCO = true;
		_tagArray[co_index]->_isNested = makeSTST;
		_tagMap[tag_CO] = co_index;		

		_tagArray[st_index]->_coTag = co_index;
		_tagArray[co_index]->_coTag = st_index;

		// maybe make the -STST tag
		if (makeSTST) {
			int stst_index = static_cast<int>(_tagArray.size());
			Symbol tag_STST = semiReducedTag + Symbol(L"-STST");
			_tagArray.push_back(_new DTTag(tag_STST, reducedTag, semiReducedTag));
			_tagArray[stst_index]->_isST = true;
			_tagArray[stst_index]->_coTag = co_index;
			_tagArray[stst_index]->_isNested = true;
			_tagMap[tag_STST] = stst_index;
		} else {
			// when not nested, update the transitions
			addTransition(st_index, co_index);
			addTransition(co_index, co_index);
			if (_add_start_end_tags && _start_tag_index != -1 && _end_tag_index != -1) {
				addTransition(_start_tag_index, st_index);
				addTransition(st_index, _end_tag_index);
				addTransition(co_index, _end_tag_index);
			}
			for (int i = 0; i < static_cast<int>(_tagArray.size()) - 2; i++) {
				if (isSTTag(i)) {
					addTransition(i, st_index);
					addTransition(st_index, i);
					addTransition(co_index, i);
				} else if (isCOTag(i)) {
					addTransition(i, st_index);
				}
			}
		}
	} else {
		// don't generate -ST and -CO, so tag and reduced tag are same
		int index = static_cast<int>(_tagArray.size());
		_tagArray.push_back(_new DTTag(reducedTag, reducedTag, semiReducedTag));
		_tagArray[index]->_isNested = makeSTST;
		_tagMap[reducedTag] = index;
	}
}

void DTTagSet::setupSuccPred() {
	int i, j;

	if (!_generate_st_co_suffix)
		return;

	resetPredecessorTags();
	resetSuccessorTags();

	if (_add_start_end_tags) {
		// START {any ST tag}
		if (_start_tag_index != -1) {
			for (i = 0; i < static_cast<int>(_tagArray.size()); i++) {
				if (isSTTag(i)) {
					addTransition(_start_tag_index, i);
				}
			}
		}

		// {any tag except END} END
		if (_end_tag_index != -1) {
			for (j = 0; j < static_cast<int>(_tagArray.size()); j++) {
				if (j != _end_tag_index) {
					addTransition(j, _end_tag_index);
				}
			}
		}
	}
	
	for (i = 0; i < static_cast<int>(_tagArray.size()); i++) {
		// -ST TAGS
		if (isSTTag(i)) {
			// TAG-ST TAG-CO
			int co_index = _tagArray[i]->_coTag;
			addTransition(i, co_index);

			for (j = 0; j < static_cast<int>(_tagArray.size()); j++) {
				// TAG-ST {any tag except START or the -CO of another tag}
				if (isSTTag(j)) {
					addTransition(i, j);
				}

				// {any tag except END} TAG-ST
				if ((i != j) && (j != _end_tag_index)) {
					addTransition(j, i);
				}
			}
		}

		// TAG-CO TAG-CO
		if (isCOTag(i)) {
			addTransition(i, i);
		}

		// X=Y-STST or X=Y-CO X-Y-ST
		if (isNestedTag(i)) {
			int outerCOIndex = getOuterCOIndex(i);
			int outerSTIndex = getOuterSTIndex(i);
			int co_index = _tagArray[i]->_coTag;

			// Y-CO X=Y-ST and X=Y-STST Y-CO
			addTransition(i, outerCOIndex);
			
			// X=Y-STST X=Y-ST
			if (isSTTag(i)) {
				_tagArray[i]->addSucc(_tagArray[co_index]->_coTag);
			}

			// X=Y-ST X=Y-CO and Y-ST X=Y-ST and X=Y-ST X=Y-ST
			if (!isCOTag(i) && !isSTTag(i)) {
				addTransition(i, co_index);
				addTransition(outerSTIndex, i);
				addTransition(outerCOIndex, i);
				addTransition(i, i);
			}

			// X=Y-STST Z=Y-ST and X=Y-ST Z=Y-ST and Z=Y-CO X=Y-ST
			for (j = 0; j < static_cast<int>(_tagArray.size()); j++){
				int j_outerCOIndex = getOuterCOIndex(j);
				int j_outerSTIndex = getOuterSTIndex(j);
				int j_co_index = _tagArray[j]->_coTag; 
				if (j_outerCOIndex == outerCOIndex) {
					if (isNestedTag(j)) {
						_tagArray[i]->addPred(j);
						if (!isCOTag(j) && !isSTTag(j)) {
							_tagArray[i]->addSucc(j);
						}
					}
				}
			}
		}
	}
}

int DTTagSet::getOuterCOIndex(int index) const{
	wstring tempStr = getSemiReducedTagSymbol(index).to_string();
	size_t eqIndex = tempStr.find(L"=");
	wstring	outerTag= tempStr.substr(eqIndex+1, tempStr.length()-eqIndex-1);
	wstring endTag = L"-CO";
	outerTag.append(endTag);
	const wchar_t *s = _new wchar_t[100];
	s = outerTag.c_str();	

	return getTagIndex(Symbol(s));

}

int DTTagSet::getOuterSTIndex(int index) const{
	wstring tempStr = getSemiReducedTagSymbol(index).to_string();
	size_t eqIndex = tempStr.find(L"=");
	wstring	outerTag= tempStr.substr(eqIndex+1, tempStr.length()-eqIndex-1);
	wstring endTag = L"-ST";
	outerTag.append(endTag);
	const wchar_t *s = _new wchar_t[100];
	s = outerTag.c_str();	

	return getTagIndex(Symbol(s));
}

int DTTagSet::getLinkTagIndex() const {
	int link_index = getTagIndex(_linkTag1);
	return (link_index == -1) ? getTagIndex(_linkTag2) : link_index;
}
