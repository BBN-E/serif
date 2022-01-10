// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/patterns/features/DocumentPFeature.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SentenceTheory.h"

void DocumentPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	out << L"    <focus type=\"document_rank\" ";
	out << L" val" << val_extra << "=\"" << _doc_rank << L"\"";
	out << L" />\n";

	out << L"    <focus type=\"document_type\" ";
	out << L" val" << val_extra << "=\"" << _docType << L"\"";
	out << L" />\n";

	out << L"    <focus type=\"document_score\" ";
	out << L" val" << val_extra << "=\"tfidf\" ";
	out << L" val" << val_extra_2 << "=\"" << _tf_idf_score << L"\"";
	out << L" />\n";

	out << L"    <focus type=\"document_score\" ";
	out << L" val" << val_extra << "=\"database\" ";
	out << L" val" << val_extra_2 << "=\"" << _db_score << L"\"";
	out << L" />\n";
}

bool DocumentPFeature::equals(PatternFeature_ptr other) {
	if (boost::shared_ptr<DocumentPFeature> f = 
			boost::dynamic_pointer_cast<DocumentPFeature>(other)) {
		return f->getDocType() == getDocType() &&
			f->getDocRank() == getDocRank() &&
			f->getTfIdfScore() == getTfIdfScore() &&
			f->getDbScore() == getDbScore();
	}
	return false;
}

void DocumentPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PseudoPatternFeature::saveXML(elem, idMap);

	if (!_docType.is_null())
		elem.setAttribute(X_doc_type, _docType);
	elem.setAttribute(X_doc_rank, _doc_rank);
	elem.setAttribute(X_tf_idf_score, _tf_idf_score);
	elem.setAttribute(X_db_score, _db_score);
}

DocumentPFeature::DocumentPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PseudoPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_doc_type))
		_docType = elem.getAttribute<Symbol>(X_doc_type);

	_doc_rank = elem.getAttribute<float>(SerifXML::X_doc_rank);
	_tf_idf_score =	elem.getAttribute<float>(SerifXML::X_tf_idf_score);
	_db_score = elem.getAttribute<float>(SerifXML::X_db_score);
}
