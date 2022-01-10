// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <vector>
#include <boost/algorithm/string.hpp> 
#include <boost/foreach.hpp> 
#include <boost/scoped_ptr.hpp>

#include "Generic/common/limits.h"
#include "Generic/sentences/SentenceBreaker.h"
#include "Generic/sentences/DefaultSentenceBreaker.h"
#include "Generic/sentences/SentenceBreakerFactory.h"
#include "Generic/sentences/StatSentenceBreaker.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Sentence.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/reader/ProseDocumentZoner.h"

#include <wchar.h>

int SentenceBreaker::_max_sentence_chars = 3000;


SentenceBreaker::SentenceBreaker() : _do_language_id_check(false), _commonInLanguageWords(NULL), _commonNameTokens(NULL)
{
	if (ParamReader::isParamTrue("remove_foreign_sentences")) { 
		std::string input_file = ParamReader::getRequiredParam("common_in_language_words");
		boost::scoped_ptr<UTF8InputStream> input_file_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& input_stream(*input_file_scoped_ptr);
		input_stream.open(input_file.c_str());
		if (input_stream.fail()) {
			throw UnexpectedInputException("SentenceBreaker::SentenceBreaker", "Could not read language_id_check file");
		} else {	
			_commonInLanguageWords = _new Symbol::HashSet(1000);

			while (!input_stream.eof()) {
				wstring word;
				input_stream.getLine(word);
				_commonInLanguageWords->insert(Symbol(word));
			}

			_do_language_id_check = true;
		}
		input_file = ParamReader::getRequiredParam("common_in_language_name_words");
		boost::scoped_ptr<UTF8InputStream> input_file_scoped_ptr2(UTF8InputStream::build());
		UTF8InputStream& input_stream2(*input_file_scoped_ptr2);
		input_stream2.open(input_file.c_str());
		if (input_stream2.fail()) {
			throw UnexpectedInputException("SentenceBreaker::SentenceBreaker", "Could not read keep_tables file");
		} else {	
			_commonNameTokens = _new Symbol::HashSet(10000);

			while (!input_stream2.eof()) {
				wstring word;
				input_stream2.getLine(word);
				_commonNameTokens->insert(Symbol(word));
			}
		}
	}
}

SentenceBreaker::~SentenceBreaker() {
	delete _commonInLanguageWords;
	delete _commonNameTokens;
}

SentenceBreaker* SentenceBreaker::build() { 
	return _factory()->build();
}

void SentenceBreaker::setFactory(boost::shared_ptr<Factory> factory) {
	_factory() = factory;
}

int SentenceBreaker::getSentences(Sentence **results, int max_sentences,
										 const Region* const* regions,
										 int num_regions) 
{
											 
	const Region** regionsToProcess = _new const Region*[num_regions];
	int region_count = 0;
	for (int i = 0; i < num_regions; i++) {
		if (regions[i]->getRegionTag() != ProseDocumentZoner::NON_PROSE_TAG)
			regionsToProcess[region_count++] = regions[i];
	}
	int n_results = getSentencesRaw(results, max_sentences, regionsToProcess, region_count);

	// remove empty sentences
    int sent = 0;
	float percentage_in_lang = 0;

	if (_do_language_id_check) {
		percentage_in_lang = percentageInLangDoc(results, n_results);
	}

    while (sent < n_results) {
		const LocatedString *string = results[sent]->getString();
		
		bool nonspace_char_found = false;
        for (int i = 0; i < string->length(); i++) {
            if (!iswspace(string->charAt(i))) {
                nonspace_char_found = true;
                break;
            }
        }

		bool in_lang_sentence = true;
		if (_do_language_id_check) {
			in_lang_sentence = isInLangSentence(results[sent], percentage_in_lang);
		}

        if (!nonspace_char_found || !in_lang_sentence) {
            // remove empty sentence
            n_results--;
            for (int i = sent; i < n_results; i++) {
                results[i] = results[i+1];
                results[i]->reorderAs(i);
            }
        }
        else {
            // move on to next sentence
            sent++;
        }
    }
	delete[] regionsToProcess;
	return n_results;
}

float SentenceBreaker::percentageInLangDoc(Sentence **results, int n_results) {
	
	float num_not_in_lang = 0, total = 0;
	for (int i = 0; i < n_results; i++) {
		std::wstring str = results[i]->getString()->toWString();
		std::vector<std::wstring> split_string;
		split(split_string, str, boost::is_any_of(L" \t\n"));

		BOOST_FOREACH(std::wstring w, split_string) {
			std::wstring no_punct = L"";
	
			for (unsigned i = 0; i < w.length(); i++) {
				if (!iswpunct(w [i])) no_punct+=w[i]; 
			}

			if (!iswupper(no_punct[0]) && iswalpha(no_punct[0])) {
				total++; 
				if (_commonInLanguageWords->find(Symbol(no_punct)) == _commonInLanguageWords->end()) {
					num_not_in_lang++;
				} 
			}
		}
	}
		
	if (total == 0) return 0;
	else return 1 - (num_not_in_lang / total);
}


bool SentenceBreaker::isInLangSentence(Sentence *sent, float percentage_in_lang) {

	std::wstring str = sent->getString()->toWString();
	std::vector<std::wstring> split_string;
	split(split_string, str, boost::is_any_of(L" \t\n"));

	float num_not_in_lang = 0, total = 0;
	BOOST_FOREACH(std::wstring w, split_string) {
		std::wstring no_punct = L"";

		//strip punctuation
		for (unsigned i = 0; i < w.length(); i++) {
			if (iswalpha(w[i])) no_punct+=w[i]; 
		}

		//Count all lowercase non-numeric words towards total
		if (!iswupper(no_punct[0]) && iswalpha(no_punct[0])) {
			total++; 

			//Count all words not in dictionary towards not_in_lang sum
			if (_commonInLanguageWords->find(Symbol(no_punct)) == _commonInLanguageWords->end()) {
				num_not_in_lang++;
			} else {	//delete debug
			}
		}

		//Count any uppercase words that are in the dictionary towards total 
		if (iswupper(no_punct[0])) {			
			std::wstring lower = L"";
			for (unsigned i = 0; i < no_punct.length(); i++) {
				if(!iswspace(no_punct[i])) {
					lower += towlower(no_punct[i]);
				}
			}
			if (_commonInLanguageWords->find(Symbol(lower)) != _commonInLanguageWords->end()) {
				total++;
			}
		}
	}

	if (total > 3) {
		if (num_not_in_lang / total <= 0.5 && percentage_in_lang >= 0.9) {
			return true;
		} else if (num_not_in_lang / total <= 0.4 && percentage_in_lang >= 0.8) {
			return true;
		} else if (num_not_in_lang / total < 0.2) {
			return true;
		} 
	} else if (percentage_in_lang >= 0.8) {
		return true;
	}

	//keep tables and lists composed of names
	float total_names = 0, total_tokens = 0;
	BOOST_FOREACH(std::wstring w, split_string) {
		if (isalpha(w[0])) {
			total_tokens++;

			std::wstring lower = L"";
			for (unsigned i = 0; i < w.length(); i++) {
				if(!iswspace(w[i])) {
					lower += towlower(w[i]);
				}
			}

			if (_commonNameTokens->find(Symbol(lower)) != _commonNameTokens->end()) {
				total_names++;
			}
		}
	}
	if (total_tokens > 0 && total_names / total_tokens >= .5) {
		return true;
	}

	return false;
}

/*int SentenceBreaker::getSentences(Sentence **results, int max_sentences,
										 const Zone* const* zones,
										 int num_zones) {
	int n_results = getSentencesRaw(results, max_sentences, zones, num_zones);

	// remove empty sentences
    int sent = 0;
    while (sent < n_results) {
		const LocatedString *string = results[sent]->getString();
		
		bool nonspace_char_found = false;
        for (int i = 0; i < string->length(); i++) {
            if (!iswspace(string->charAt(i))) {
                nonspace_char_found = true;
                break;
            }
        }

        if (!nonspace_char_found) {
            // remove empty sentence
            n_results--;
            for (int i = sent; i < n_results; i++) {
                results[i] = results[i+1];
                results[i]->reorderAs(i);
            }
        }
        else {
            // move on to next sentence
            sent++;
        }
    }
	return n_results;
}*/

boost::shared_ptr<SentenceBreaker::Factory> &SentenceBreaker::_factory() {
	static boost::shared_ptr<SentenceBreaker::Factory> factory(new SentenceBreakerFactory<DefaultSentenceBreaker>());
	return factory;
}

void SentenceBreaker::setMaxSentenceChars(int max_sentence_chars) {
	_max_sentence_chars = max_sentence_chars;
}
