// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "driver/DocTheory.h"
#include "common/OutputUtil.h"
#include "common/InternalInconsistencyException.h"
#include "sentences/Sentence.h"
#include "driver/SentenceTheory.h"
#include "edt/EntitySet.h"


DocTheory::DocTheory(Document *document)
	: _document(document), _sentences(0), _sentTheories(0), _entitySet(0)
{}

DocTheory::~DocTheory() {
	for (int i = 0; i < _n_sentences; i++) {
		delete _sentences[i];
		delete _sentTheories[i];
	}
	delete[] _sentences;
	delete[] _sentTheories;
	delete _entitySet;
}

void DocTheory::setSentences(int n_sentences, Sentence **sentences) {
	if (_sentences == 0) {
		_n_sentences = n_sentences;
		_sentences = _new Sentence*[n_sentences];
		_sentTheories = _new SentenceTheory*[n_sentences];
		for (int i = 0; i < n_sentences; i++) {
			_sentences[i] = sentences[i];
			_sentTheories[i] = 0;
		}
	} else {
		throw InternalInconsistencyException("DocTheory::setSentences()", "Sentences already set");
	}
}

Sentence* DocTheory::getSentence(int i) const {
	if ((unsigned) i < (unsigned) _n_sentences)
		return _sentences[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::getSentence()", _n_sentences, i);
}

void DocTheory::setSentenceTheory(int i, SentenceTheory *theory) {
	if ((unsigned) i < (unsigned) _n_sentences) {
		if (_sentTheories[i] == 0) {
			// assign sentence theory
			_sentTheories[i] = theory;

			// This assumes that theory is the newest sentence theory, but
			// update the doc theory's entity set with its entity set.
			if (theory->getEntitySet() != 0)
				_entitySet = _new EntitySet(*theory->getEntitySet());
		}
		else
			throw UnrecoverableException("DocTheory::setSentTheories()", "SentTheory already set");
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::setSentTheory()", _n_sentences, i);
	}
}

SentenceTheory* DocTheory::getSentenceTheory(int i) const {
	if ((unsigned) i < (unsigned) _n_sentences)
		return _sentTheories[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::getSentTheory()", _n_sentences, i);
}


void DocTheory::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Document Theory "
		<< "(document " << _document->getName().to_debug_string() << "):\n";
	
	for (int i = 0; i < _n_sentences; i++) {
		if (_sentTheories[i] == 0)
			out << "No theory returned!\n";
		else
			_sentTheories[i]->dump(out, indent + 2);
		out << "\n\n";
	}

	delete[] newline;
}
