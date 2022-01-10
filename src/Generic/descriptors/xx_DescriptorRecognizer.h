// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_DESCRIPTOR_RECOGNIZER_H
#define xx_DESCRIPTOR_RECOGNIZER_H

class Parse;
class MentionSet;
class NameTheory;
class NestedNameTheory;
class PartOfSpeechSequence;
class TokenSequence;

class GenericDescriptorRecognizer : public DescriptorRecognizer {
public:
	/**
	 * WARNING: A DescriptorRecognizer has not been defined if this constructor is being used.
	 */
	DescriptorRecognizer() 
	{
		SessionLogger::warn("descriptor_recognizer") << "<<<<<WARNING: Using unimplemented descriptor recognizer!>>>>>\n";
	}
	~DescriptorRecognizer() {}

	virtual void resetForNewSentence() {}


	virtual int getDescriptorTheories(MentionSet *results[],
									  int max_theories,
									  const PartOfSpeechSequence* partOfSpeechSeq,
									  const Parse *parse,
									  const NameTheory *nameTheory,
									  const NestedNameTheory *nestedNameTheory,
									  TokenSequence* tokenSequence,
									  int sentence_number) { return 0; }
};
#endif
