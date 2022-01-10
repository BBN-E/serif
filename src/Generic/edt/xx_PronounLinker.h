// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_PRONOUNLINKER_H
#define xx_PRONOUNLINKER_H

class GenericPronounLinker : public PronounLinker {
private:
	friend class GenericPronounLinkerFactory;

public:
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, 
		EntityType linkType, LexEntitySet *results[], int max_results) { return 0; };
	virtual void addPreviousParse(const Parse *parse) {};
	virtual void resetPreviousParses() {};
	/*
	virtual int getLinkGuesses(EntitySet *currSolution, Mention *currMention, 
		LinkGuess results[], int max_results) { return 0; }
	*/
	
private:
	/**
	 * WARNING: A GenericPronounLinker has not been defined if this constructor is being used.
	 */
	GenericPronounLinker()
	{
		std::cerr << "<<<<<WARNING: Using unimplemented pronoun linker!>>>>>\n";
	};
};

class GenericPronounLinkerFactory: public PronounLinker::Factory {
	virtual PronounLinker *build() { return _new GenericPronounLinker(); }
};

#endif
