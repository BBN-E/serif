// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnrecoverableException.h"

#include "Arabic/parse/ar_SplitTokenSequence.h"
#include "Arabic/parse/ar_WordSegment.h"

SplitTokenSequence::SplitTokenSequence(){
	_currToken =0;
	_numTokens =0;
}
SplitTokenSequence::~SplitTokenSequence(){}

bool SplitTokenSequence::readBuckwalterSentence(UTF8InputStream& stream) {
	UTF8Token token;
	int numTok = 0;

	if (stream.eof())
		return false;
	stream >> token;	//read in sentence paren
	if (stream.eof())
		return false;
	if(token.symValue() != Symbol(L"("))
		throw UnexpectedInputException("SplitTokenSequence::readBuckWalterSentence", 
		"ill-formed sentence");
	while (true) {
		try{
			stream >> token;	//read in word paren
			if(token.symValue()==Symbol(L")")){ //end of sentence
//				std::cout<<"found end of sentence!\n";
				break;
			}
			if(token.symValue() != Symbol(L"(")) 
				throw UnexpectedInputException("SplitTokenSequence::readBuckWalterSentence", 
				"ill-formed sentence");
		}
		catch(UnexpectedInputException &e){
			std::cerr<<e.getMessage() << std::endl;
			return false;
		}
		try{
			_possibleTokens[_currToken] = readBuckwalterWord(stream);
		}
		catch(UnexpectedInputException &e){
			std::cerr<<e.getMessage() << std::endl;
			return false;
		}

		_currToken++;
		
	}

	_numTokens = _currToken;
	return true;

}
WordSegment* SplitTokenSequence::readBuckwalterWord(UTF8InputStream& stream){
	UTF8Token token;
	int numTok = 0;

	WordSegment* thisWord = _new WordSegment();
	WordSegment::Segmentation possibilities[10];
	int num_pos =0;
	while (true) {
		stream >> token;	//read in possibility paren	
		if(token.symValue()==Symbol(L")"))	//end of word
			break;
		if(token.symValue() != Symbol(L"("))
			throw UnexpectedInputException("SplitTokenSequence::readBuckWalterWord", 
			"ill-formed sentence");

		stream >>token;		//segment paren or end of possibity paren

		if(token.symValue()==Symbol(L")"))	//end of word
			break;

		if(token.symValue() ==Symbol(L"-UNKNOWN-")){
			stream >> token;
			WordSegment* unanalyzed = _new WordSegment();
			unanalyzed->getSegments(token.symValue());
			stream >> token; //for convenience get the segment close paren

			if(token.symValue() != Symbol(L")"))
				throw UnexpectedInputException("SplitTokenSequence::readBuckWalterPossibility", 
				"ill-formed -UNKNOWN- segment");
			stream >> token; //for convenience get the segment close paren


			if(token.symValue() != Symbol(L")"))
				throw UnexpectedInputException("SplitTokenSequence::readBuckWalterPossibility", 
				"ill-formed -UNKNOWN- segment");

			return unanalyzed;
		}



		if(token.symValue() != Symbol(L"("))
			throw UnexpectedInputException("SplitTokenSequence::readBuckWalterWord", 
			"ill-formed sentence");
		bool is_analyzed =true;
		possibilities[num_pos++] = readBuckwalterPossibility(stream,is_analyzed);
		
		if(!is_analyzed){
			WordSegment* unanalyzed = _new WordSegment();
			unanalyzed->getSegments(possibilities[0].segments[0]);
			//take care of extra reading before returning
			stream >> token;
			if(token.symValue() != Symbol(L")"))
				throw UnexpectedInputException("SplitTokenSequence::readBuckWalterWord", 
				"ill-formed -UNKNOWN- word");
			return unanalyzed;
		}

	}
	//return new WordSegment;

	try{
//		std::cout<<"ADDED WORD"<<std::endl;
		WordSegment* new_word = _new WordSegment(possibilities, num_pos);
		new_word->printSegment();
		return new_word;
	}
	catch(UnrecoverableException &e){
		std::cerr<<"*******"<<e.getMessage() << std::endl;
		return _new WordSegment();
	}

	


}


WordSegment::Segmentation SplitTokenSequence::readBuckwalterPossibility(UTF8InputStream& stream, bool& is_analyzed){
		UTF8Token token;
		stream >> token;
		WordSegment::Segmentation segment_possibility;
		int num_seg =0;
		if(token.symValue() ==Symbol(L"-UNKNOWN-")){
			stream >> token;
			segment_possibility.segments[0]= token.symValue();
			segment_possibility.numSeg = 1;
			is_analyzed =false;
			stream >> token; //for convenience get the segment close paren
			if(token.symValue() != Symbol(L")"))
				throw UnexpectedInputException("SplitTokenSequence::readBuckWalterPossibility", 
				"ill-formed -UNKNOWN- segment");
			return segment_possibility;
		}
		else{			
			while(true){
				segment_possibility.segments[num_seg++] = token.symValue();
				stream >> token; //close segement

				if(token.symValue() != Symbol(L")"))
					throw UnexpectedInputException("SplitTokenSequence::readBuckWalterWord", 
						"word segment not closed");
				stream >> token; //either open segment or close word option

				if(token.symValue()==Symbol(L")"))
					break;
				if(token.symValue() != Symbol(L"("))
					throw UnexpectedInputException("SplitTokenSequence::readBuckWalterWord", 
					"ill-formed segment");
				stream >> token;	//word segment

			}
			segment_possibility.numSeg = num_seg;
			return segment_possibility;
		}


}

