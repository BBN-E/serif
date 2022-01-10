// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/events/patterns/EventPatternSet.h"
#include "Generic/events/patterns/EventPatternNode.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/ValueType.h"
#include <boost/scoped_ptr.hpp>

#if defined(_WIN32)
#define snprintf _snprintf
#endif

EventPatternSet::EventPatternSet(const char* filename) : _map(0)
{

	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if (stream.fail()) {
		std::stringstream errMsg;
		errMsg << "Unable to open the event patterns file:\n";
		errMsg << filename;
		throw UnexpectedInputException("EventPatternSet::EventPatternSet", errMsg.str().c_str());
	}

	_map = _new Map(500);

	Sexp entityTypeSet(stream);
	verifyEntityTypeSet(entityTypeSet);

	Sexp *patternSet = _new Sexp(stream);

	NodeSet ns;
	while (!patternSet->isVoid()) {
		//std::cerr << patternSet->to_debug_string() << std::endl;
		if (patternSet->isAtom() || patternSet->getFirstChild()->isList()) {
			char c[1000];
			snprintf(c, 999, "The event patterns file is invalid -- expected beginning of pattern set:\n%s", patternSet->to_debug_string().c_str());
			throw UnexpectedInputException("EventPatternSet::EventPatternSet", c);
		}
		Symbol setName = patternSet->getFirstChild()->getValue();
		int nkids = patternSet->getNumChildren();
		nkids--;
		EventPatternNode** kids = _new EventPatternNode*[nkids];
		for (int i = 0; i < nkids; i++)
			kids[i] = _new EventPatternNode(patternSet->getNthChild(i+1));
		delete patternSet;
		ns.n_nodes = nkids;
		ns.nodes = kids;
		(*_map)[setName] = ns;
		patternSet = _new Sexp(stream);
	}
	delete patternSet;
}

EventPatternSet::~EventPatternSet() {
	for (Map::iterator iter = _map->begin(); iter != _map->end(); ++iter) {
		NodeSet &ns = (*iter).second;
		for (int i = 0; i < ns.n_nodes; i++)
			delete ns.nodes[i];
		delete[] ns.nodes;
	}
	_map->clear();
	delete _map;
}

EventPatternNode** EventPatternSet::getNodes(const Symbol s, int& n_nodes) const {

	Map::iterator iter;

	iter = _map->find(s);
	if (iter == _map->end()) {
		n_nodes = 0;
		return static_cast<EventPatternNode**>(0);
	}
	n_nodes = (*iter).second.n_nodes;
	return (*iter).second.nodes;

}

void EventPatternSet::verifyEntityTypeSet(Sexp &sexp) {
	if (sexp.isVoid() || sexp.isAtom() || !sexp.getFirstChild()->isAtom() ||
		sexp.getFirstChild()->getValue() != Symbol(L"entity-types"))
	{
		throw UnexpectedInputException("EventPatternSet::verifyEntityTypeSet()",
			"The event patterns file does not contain an entity types list");
	}

	for (Sexp *etypeExp = sexp.getSecondChild();
		 etypeExp != 0; etypeExp = etypeExp->getNext())
	{
		if (!etypeExp->isAtom()) {
			throw UnexpectedInputException(
				"EventPatternSet::verifyEntityTypeSet()",
				"The event patterns file contains an invalid entity types list");
		}

		try {
			EntityType(etypeExp->getValue());
		}
		catch (UnexpectedInputException &) {
			try {
				ValueType(etypeExp->getValue());
			} 
			catch (UnexpectedInputException &) {
				std::string message = "";
				message += "The event patterns file refers to "
					"at least one unrecognized entity or value type: ";
				message += etypeExp->getValue().to_debug_string();
				throw UnexpectedInputException(
					"EventPatternSet::verifyEntityTypes()",
					message.c_str());
			}
		}
	}
}

