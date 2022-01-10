// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_DOCUMENT_DRIVER_H
#define CA_DOCUMENT_DRIVER_H

#include "Generic/driver/DocumentDriver.h"
#include "Generic/common/UTF8OutputStream.h"

class Document;
class DocTheory;
class SentenceTheory;
class SentenceDriver;
class SessionLogger;
class SynNode;
class CAResultCollector;
class CorrectAnswers;
class CorrectDocument;

class CADocumentDriver : public DocumentDriver
{
public:

	CADocumentDriver(const SessionProgram *sessionProgram, 
		             ResultCollector *resultCollector);

private:

	CorrectAnswers *_correctAnswers;

	bool PRINT_AUGMENTED_PARSES;
	bool PRINT_NAME_TRAINING;
	bool PRINT_DESC_TRAINING;
	bool PRINT_VALUE_TRAINING;

	void printAugmentedParse(UTF8OutputStream &out, SentenceTheory *sentTheory, 
		CorrectDocument *correctDoc, const SynNode *node, int indent);
	void printDescriptorTraining(UTF8OutputStream &out, SentenceTheory *sentTheory);
	void printValueTraining(UTF8OutputStream &out, SentenceTheory *sentTheory);

protected:
	// support functions
	virtual Document *loadDocument(const wchar_t *document_file);

	virtual void loadDocEntityModels();
	virtual void loadDocRelationsEventsModels();

	virtual void loadCAMetadata(Document *doc);

	virtual void outputResults(const wchar_t *document_filename,
							   DocTheory *docTheory,
							   const wchar_t *output_dir);

	virtual void outputResults(std::wstring *results,
							   DocTheory *docTheory,
							   const wchar_t *output_dir);
	

	virtual void outputResults(std::wstring *results, DocTheory *docTheory) {
		DocumentDriver::outputResults(results, docTheory);
	}

	virtual void dumpDocumentTheory(DocTheory *docTheory, const wchar_t *document_filename);


};


#endif
