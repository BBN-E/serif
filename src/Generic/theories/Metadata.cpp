// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Span.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLTheoryElement.h"
#include <boost/foreach.hpp>

Metadata::SpanTree::~SpanTree()
{
	if (!_view)
	{
		delete _span;
	}
	if (_left != NULL)
	{
		delete _left;
	}
	if (_right != NULL)
	{
		delete _right;
	}
}

int Metadata::SpanTree::size() const
{
	int left_size = (_left == NULL) ? 0 : _left->size();
	int right_size = (_right == NULL) ? 0 : _right->size();
	return left_size + right_size;
}

void Metadata::SpanTree::find(const Metadata::SpanFilter *filter, Metadata::SpanList *result) const
{
	if (_left != NULL)
	{
		_left->find(filter, result);
	}
	if (filter->keep(_span))
	{
		result->add(_span);
	}
	if (_right != NULL)
	{
		_right->find(filter, result);
	}
}

void Metadata::SpanTree::find(EDTOffset offset, const Metadata::SpanFilter *filter, Metadata::SpanList *result) const
{
	if (_cmp.selectOffset(_span) == offset)
	{
		if (_left != NULL)
		{
			_left->find(offset, filter, result);
		}
		if (filter->keep(_span))
		{
			result->add(_span);
		}
		if (_right != NULL)
		{
			_right->find(offset, filter, result);
		}
	}
	else if (_cmp.lessThan(_span, offset))
	{
		if (_right != NULL)
		{
			_right->find(offset, filter, result);
		}
	}
	else
	{
		if (_left != NULL)
		{
			_left->find(offset, filter, result);
		}
	}
}

void Metadata::SpanTree::insert(Span *span)
{
	if (_cmp.lessThan(span, _span))
	{
		if (_left == NULL)
		{
			_left = _new SpanTree(span, _cmp, _view);
		}
		else
		{
			_left->insert(span);
		}
	}
	else
	{
		if (_right == NULL)
		{
			_right = _new SpanTree(span, _cmp, _view);
		}
		else
		{
			_right->insert(span);
		}
	}
}

Metadata::SpanTree *Metadata::SpanTree::headTree(EDTOffset maxOffset) const
{
	if (_cmp.lessThan(_span, maxOffset)) {
		SpanTree *left = (_left == NULL) ? NULL : _left->headTree(maxOffset);
		SpanTree *right = (_right == NULL) ? NULL : _right->headTree(maxOffset);
		return _new SpanTree(_span, _cmp, left, right, true);
	}
	else if (_left == NULL) {
		return NULL;
	}
	else {
		return _left->headTree(maxOffset);
	}
}

Metadata::SpanTree *Metadata::SpanTree::tailTree(EDTOffset minOffset) const
{
	if (!_cmp.lessThan(_span, minOffset)) {
		SpanTree *left = (_left == NULL) ? NULL : _left->tailTree(minOffset);
		SpanTree *right = (_right == NULL) ? NULL : _right->tailTree(minOffset);
		return _new SpanTree(_span, _cmp, left, right, true);
	}
	else if (_right == NULL) {
		return NULL;
	}
	else {
		return _right->tailTree(minOffset);
	}
}

bool Metadata::SpanTree::contains(Span *span) const
{
	if (_span == span) {
		return true;
	}
	else if (_cmp.lessThan(span, _span)) {
		return (_left == NULL) ? false : _left->contains(span);
	}
	else {
		return (_right == NULL) ? false : _right->contains(span);
	}
}

void Metadata::SpanTree::intersection(Metadata::SpanTree *other, const Metadata::SpanFilter *filter, Metadata::SpanList *result) const
{
	if (_left != NULL)
	{
		_left->intersection(other, filter, result);
	}
	if (other->contains(_span) && filter->keep(_span))
	{
		result->add(_span);
	}
	if (_right != NULL)
	{
		_right->intersection(other, filter, result);
	}
}

void Metadata::SpanTree::dump(std::ostream &out) const
{
	out << "(("
		<< _span->getStartOffset() << " . " << _span->getEndOffset()
		<< ") ";
	if (_left == NULL)
	{
		out << "null";
	}
	else
	{
		_left->dump(out);
	}
	out << " ";
	if (_right == NULL)
	{
		out << "null";
	}
	else
	{
		_right->dump(out);
	}
	out << ")";
}



const Metadata::StartComparator Metadata::startCmp;
const Metadata::EndComparator Metadata::endCmp;
const float Metadata::targetLoadingFactor = static_cast<float>(0.7);
const int initialTableSize = 5;

Metadata::Metadata()
{
	creatorTable = _new SpanCreatorTable(initialTableSize);
	spanStartTree = NULL;
	spanEndTree = NULL;
	span_count = 0;
	creator_count = 0;
}

Metadata::~Metadata()
{
	SpanCreatorTable::iterator crIter;
	for (crIter = creatorTable->begin(); crIter != creatorTable->end(); ++crIter)
	{
		delete (*crIter).second;
	}
	delete creatorTable;

	if (span_count > 0)
	{
		delete spanStartTree;
		delete spanEndTree;
	}
}

void Metadata::initialize()
{
	if (span_count > 0)
	{
		delete spanStartTree;
		spanStartTree = NULL;
		delete spanEndTree;
		spanEndTree = NULL;
	}
	creatorTable->clear();
	span_count = 0;
	creator_count = 0;
}

void Metadata::addSpanCreator(Symbol type, SpanCreator* creator)
{
    SpanCreatorTable::iterator iter;
    iter = creatorTable->find(type);
    if (iter == creatorTable->end()) {
		(*creatorTable)[type] = creator;
		creator_count+= 1;
	}
	return;
}

SpanCreator* Metadata::lookupSpanCreator(Symbol type)
{
    SpanCreatorTable::iterator iter;
    iter = creatorTable->find(type);
    if (iter == creatorTable->end())
	{
		return 0;
	}
	return (*iter).second;
}

void Metadata::insert(Span *span)
{
	if (span_count == 0)
	{
		spanStartTree = _new SpanTree(span, startCmp, false);
		spanEndTree = _new SpanTree(span, endCmp, true);
	}
	else
	{
		spanStartTree->insert(span);
		spanEndTree->insert(span);
	}
	span_count++;
}

Span* Metadata::newSpan(Symbol type, EDTOffset start_offset, EDTOffset end_offset, const void *param)
{
	SpanCreator* creator = lookupSpanCreator(type);
	Span* span = (*creator).createSpan(start_offset, end_offset, param);
	insert(span);
	return span;
}

Span * Metadata::getSmallestSpan(const SpanList *list) const
{
	Span *result = NULL;
	for (int i = 0; i < list->length(); i++)
	{
		Span *span = (*list)[i];
		if ((result == NULL) || (result->length() > span->length()))
		{
			result = span;
		}
	}
	return result;
}

// ==================================================================
// Get all spans at an offset:
// ==================================================================

Metadata::SpanList * Metadata::getStartingSpans(EDTOffset start_offset) const
{
	KeepAllFilter filter;
	return getStartingSpans(start_offset, &filter);
}

Metadata::SpanList * Metadata::getSpans() const
{
	KeepAllFilter filter;
	return getSpans(&filter);
}

Metadata::SpanList * Metadata::getEndingSpans(EDTOffset end_offset) const
{
	KeepAllFilter filter;
	return getEndingSpans(end_offset, &filter);
}

Metadata::SpanList * Metadata::getCoveringSpans(EDTOffset offset) const
{
	KeepAllFilter filter;
	return getCoveringSpans(offset, &filter);
}

Metadata::SpanList * Metadata::getContainedSpans(EDTOffset start_offset, EDTOffset end_offset) const
{
	KeepAllFilter filter;
	return getContainedSpans(start_offset, end_offset, &filter);
}

// ==================================================================
// Get all spans at an offset of a given type:
// ==================================================================

Span * Metadata::getStartingSpan(EDTOffset start_offset, const Symbol type) const
{
	SpanTypeFilter filter(type);
	SpanList *list = getStartingSpans(start_offset, &filter);
	Span *result = getSmallestSpan(list);
	delete list;
	return result;
}

Span * Metadata::getEndingSpan(EDTOffset end_offset, const Symbol type) const
{
	SpanTypeFilter filter(type);
	SpanList *list = getEndingSpans(end_offset, &filter);
	Span *result = getSmallestSpan(list);
	delete list;
	return result;
}

Span * Metadata::getCoveringSpan(EDTOffset offset, const Symbol type) const
{
	SpanTypeFilter filter(type);
	SpanList *list = getCoveringSpans(offset, &filter);
	Span *result = getSmallestSpan(list);
	delete list;
	return result;
}

// ==================================================================
// Get the smallest span at a given offset:
// ==================================================================

Span * Metadata::getStartingSpan(EDTOffset start_offset) const
{
	KeepAllFilter filter;
	SpanList *list = getStartingSpans(start_offset, &filter);
	Span *result = getSmallestSpan(list);
	delete list;
	return result;
}

Span * Metadata::getEndingSpan(EDTOffset end_offset) const
{
	KeepAllFilter filter;
	SpanList *list = getEndingSpans(end_offset, &filter);
	Span *result = getSmallestSpan(list);
	delete list;
	return result;
}

Span * Metadata::getCoveringSpan(EDTOffset offset) const
{
	KeepAllFilter filter;
	SpanList *list = getCoveringSpans(offset, &filter);
	Span *result = getSmallestSpan(list);
	delete list;
	return result;
}

// ==================================================================
// Get selected spans at an offset:
// ==================================================================

Metadata::SpanList * Metadata::getStartingSpans(EDTOffset start_offset, const Metadata::SpanFilter *filter) const
{
	SpanList *result = _new SpanList();
	if (span_count > 0)
	{
		spanStartTree->find(start_offset, filter, result);
	}
	return result;
}

Metadata::SpanList * Metadata::getSpans(const Metadata::SpanFilter *filter) const
{
	SpanList *result = _new SpanList();
	if (span_count > 0)
	{
		spanStartTree->find(filter, result);
	}
	return result;
}

Metadata::SpanList * Metadata::getEndingSpans(EDTOffset end_offset, const Metadata::SpanFilter *filter) const
{
	SpanList *result = _new SpanList();
	if (span_count > 0)
	{
		spanEndTree->find(end_offset, filter, result);
	}
	return result;
}

Metadata::SpanList * Metadata::getCoveringSpans(EDTOffset offset, const Metadata::SpanFilter *filter) const
{
	SpanList * result = _new SpanList();
	if (span_count > 0)
	{
		SpanTree *startBefore = spanStartTree->headTree(offset);
		if (startBefore != NULL)
		{
			SpanTree *endAfter = spanEndTree->tailTree(offset);
			if (endAfter != NULL)
			{
				startBefore->intersection(endAfter, filter, result);
				delete endAfter;
			}
			delete startBefore;
		}
	}
	return result;
}

Metadata::SpanList * Metadata::getContainedSpans(EDTOffset start_offset, EDTOffset end_offset, const Metadata::SpanFilter *filter) const
{
	SpanList * result = _new SpanList();
	if (span_count > 0)
	{
		SpanTree *startAfter = spanStartTree->tailTree(start_offset);
		if (startAfter != NULL)
		{
			SpanTree *endBefore = spanEndTree->headTree(end_offset);
			if (endBefore != NULL)
			{
				startAfter->intersection(endBefore, filter, result);
				delete endBefore;
			}
			delete startAfter;
		}
	}
	return result;
}

int Metadata::get_creator_count() const
{ 
	return creator_count; 
}
	
int Metadata::get_span_count() const
{ 
	return span_count; 
}

void Metadata::dump(std::ostream &out) const
{
	if (spanStartTree != NULL)
	{
		out << "Span start tree: ";
		spanStartTree->dump(out);
		out << std::endl;
		out << "Span end tree: ";
		spanEndTree->dump(out);
		out << std::endl;
	}
}

void Metadata::updateObjectIDTable() const {
	throw InternalInconsistencyException("Metadata::updateObjectIDTable()",
										"Using unimplemented method.");
}

void Metadata::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("Metadata:::saveState()",
										"Using unimplemented method.");

}

void Metadata::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("Metadata::resolvePointers()",
										"Using unimplemented method.");

}

const wchar_t* Metadata::XMLIdentifierPrefix() const {
	return L"metadata";
}

void Metadata::saveXML(SerifXML::XMLTheoryElement metadataElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Metadata::saveXML", "Expected context to be NULL");
	Metadata::SpanList *spans = const_cast<Metadata*>(this)->getSpans();
	for (int i=0; i<spans->length(); ++i)
		metadataElem.saveChildTheory(X_Span, (*spans)[i]);
	delete spans;
	// Note: we do not serialize span creators.  (Should we?)
}

Metadata::Metadata(SerifXML::XMLTheoryElement metadataElem)
: creatorTable(_new SpanCreatorTable(initialTableSize)),
  spanStartTree(NULL), spanEndTree(NULL), span_count(0), creator_count(0)
{
	using namespace SerifXML;

	BOOST_FOREACH(XMLTheoryElement spanElem, metadataElem.getChildElementsByTagName(X_Span)) {
		insert(Span::loadSpanFromXML(spanElem));
	}
}
