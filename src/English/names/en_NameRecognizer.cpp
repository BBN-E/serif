// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "English/common/en_NationalityRecognizer.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/preprocessors/NamedSpan.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "English/names/en_IdFNameRecognizer.h"
#include "English/names/en_NameRecognizer.h"
#include "English/common/en_WordConstants.h"
#include "Generic/names/IdFListSet.h"
#include "Generic/common/InputUtil.h"
#include "English/common/en_EvalSpecificRules.h"
#include "Generic/names/PatternNameFinder.h"
#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/theories/Zoning.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


using namespace std;

Symbol EnglishNameRecognizer::_NONE_ST = Symbol(L"NONE-ST");
Symbol EnglishNameRecognizer::_NONE_CO = Symbol(L"NONE-CO");

Symbol EnglishNameRecognizer::_PER_ST = Symbol(L"PER-ST");
Symbol EnglishNameRecognizer::_PER_CO = Symbol(L"PER-CO");

EnglishNameRecognizer::EnglishNameRecognizer()
	: _debug_flag(false), _idfNameRecognizer(0), _pidfDecoder(0),
	  _tagSet(0), _num_names(0), _sent_no(0), _tokenSequence(0), _docTheory(0),
	  _patternNameFinder(0)
{
	if (ParamReader::isParamTrue("debug_name_recognizer"))
	{
		_debug_flag = true;
	}

	_skip_all_namefinding = ParamReader::isParamTrue("skip_all_namefinding");

	string nameFinderParam = ParamReader::getParam("name_finder");
	if (boost::iequals(nameFinderParam,"idf")) {
		_name_finder = IDF_NAME_FINDER;
	}
	else if (boost::iequals(nameFinderParam,"pidf")) {
		_name_finder = PIDF_NAME_FINDER;
	}
	else {
		throw UnexpectedInputException(
			"EnglishNameRecognizer::EnglishNameRecognizer()",
			"Parameter 'name_finder' must be set to 'idf' or 'pidf'");
	}

	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer = _new EnglishIdFNameRecognizer();
	}
	else {
		_pidfDecoder = _new PIdFModel(PIdFModel::DECODE);
		_tagSet = _pidfDecoder->getTagSet();
	}

	// This should only be turned on if sentence selection is the only thing you are doing!
	_print_sentence_selection_info = ParamReader::isParamTrue("print_sentence_selection_info");

	_select_decoder_case_by_sentence = ParamReader::getRequiredTrueFalseParam("select_pidf_decoder_case_by_sentence");
	_force_lowercase_sentence = false;
	
	_reduce_special_names = ParamReader::isParamTrue("reduce_special_names");
	std::string reduce_file = ParamReader::getParam("reduction_names");
	if (!reduce_file.empty()) {
		_reductionTable = _new IdFListSet(reduce_file.c_str());
	} else _reductionTable = 0;

	_use_atea_name_fixes = ParamReader::isParamTrue("use_atea_name_fixes");
	string wordsFile = ParamReader::getParam("per_subsumption_words_file");
	if (wordsFile.length() > 0)
		loadSubsumptionWords(wordsFile, _perSubsumptionWords);

	_misleading_capitalization_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("misleading_capitalization_threshold", 1.0);
	_check_misleading_capitalization = _misleading_capitalization_threshold > 0.0 && _misleading_capitalization_threshold < 1.0;

	_check_initial_sentence_capitalization = ParamReader::isParamTrue("check_initial_sentence_capitalization");

	_disallow_emails = ParamReader::isParamTrue("disallow_emails_as_names");

	_use_name_constraints_from_document = ParamReader::isParamTrue("use_name_constraints_from_document");
	if (!_use_name_constraints_from_document)
		_use_names_from_document = ParamReader::isParamTrue("use_names_from_document");
	else
		_use_names_from_document = false;

	if (ParamReader::hasParam("names_with_forced_entity_types")) {
		std::string forcedEntityTypesFile = ParamReader::getRequiredParam("names_with_forced_entity_types");
		typedef std::vector<std::wstring> wstring_vector_t;
		BOOST_FOREACH(wstring_vector_t vec, InputUtil::readColumnFileIntoSet(forcedEntityTypesFile, false, L"\t")) {
			if (vec.size() == 0)
				continue;
			std::wstring left = vec.at(0);
			if (left.size() == 0)
				continue;
			if (left.at(0) == L'#')
				continue;
			if (vec.size() != 2) {
				std::wstringstream msg;
				msg << L"In file: " << UnicodeUtil::toUTF16StdString(forcedEntityTypesFile) << " specified by parameter: 'names_with_forced_entity_types'\n";
				msg << L"Forced entity types rows must have exactly two elements per row (tab-separated): ";
				for (size_t i = 0; i < vec.size(); i++) {
					msg << vec.at(i) << "\t";
				}
				throw UnexpectedInputException("EnglishNameRecognizer::EnglishNameRecognizer()", msg);
			}
			std::wstring right = vec.at(1);
			boost::to_upper(right);
			if (_forcedTypeNames.find(right) != _forcedTypeNames.end()) {
				std::wstringstream msg;
				msg << L"In file: " << UnicodeUtil::toUTF16StdString(forcedEntityTypesFile) << " specified by parameter: 'names_with_forced_entity_types'\n";
				msg << L"Same entry appears multiple times in forced entity types list: " << left << L"\t" << right;
				throw UnexpectedInputException("EnglishNameRecognizer::EnglishNameRecognizer()", msg);
			}
			_forcedTypeNames[right] = EntityType(Symbol(left));	// This will throw an error if the entity type is invalid		
		}
	}

	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_name_finding_patterns", false))
		_patternNameFinder = _new PatternNameFinder();
}

EnglishNameRecognizer::~EnglishNameRecognizer() {
	delete _idfNameRecognizer;
	delete _pidfDecoder;
	delete _patternNameFinder;
}

void EnglishNameRecognizer::resetForNewSentence(const Sentence *sentence) {
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->resetForNewSentence();
	} else if (_name_finder == PIDF_NAME_FINDER) {
		if (_patternNameFinder)
			_patternNameFinder->resetForNewSentence(_docTheory, sentence->getSentNumber());

		if (_select_decoder_case_by_sentence) {
			// Determine sentence case, selecting appropriate decoder and downcasing, and log it
			SessionLogger::updateContext(2, boost::lexical_cast<std::string>(sentence->getSentNumber()).c_str());
			int sent_case = sentence->getSentenceCase();
			if (sent_case == Sentence::LOWER) {
				_force_lowercase_sentence = false;
				_pidfDecoder->useLowercaseDecoder();
				SessionLogger::dbg("en_name_reset") << "Case is lower, using lowercase decoder";
			} else if (sent_case == Sentence::UPPER) {
				_force_lowercase_sentence = true;
				_pidfDecoder->useLowercaseDecoder();
				SessionLogger::dbg("en_name_reset") << "Case is upper, using lowercase decoder with downcasing";
			} else if (sent_case == Sentence::MIXED) {
				if (_check_initial_sentence_capitalization && hasAbruptCaseChange(sentence)) {
					// NCW: This more aggressively handles sentences where the
					//      sentence case changes abruptly mid-sentence
					_force_lowercase_sentence = true;
					_pidfDecoder->useLowercaseDecoder();
					SessionLogger::dbg("en_name_reset") << "Case is mixed, but changes mid-sentence, using lowercase decoder with downcasing";
				} else if (_check_misleading_capitalization && hasPotentiallyMisleadingCasing(sentence)) {
					// LB: This is an idea to help us deal with headline-style sentences or
					//     potentially weirdly-cased data.
					_force_lowercase_sentence = true;
					_pidfDecoder->useLowercaseDecoder();
					SessionLogger::dbg("en_name_reset") << "Case is mixed, but misleading, using lowercase decoder with downcasing";
				} else {
					_force_lowercase_sentence = false;
					_pidfDecoder->useDefaultDecoder();
					SessionLogger::dbg("en_name_reset") << "Case is mixed, using default decoder";
				}
			}
		}
	}
}

bool EnglishNameRecognizer::hasPotentiallyMisleadingCasing(const Sentence *sentence) {
	std::wstring sentence_string = sentence->getString()->toString();
	boost::trim(sentence_string);
	if (sentence_string.size() == 0)
		return false;

	// If the first character is lowercase, that's a sign of potential problems
	if (iswlower(sentence_string.at(0)))
		return true;

	std::vector<std::wstring> words;
	boost::split(words, sentence_string, boost::is_any_of(" \t\n"));
	int num_nocap_big_words = 0;
	int num_cap_big_words = 0;

	// We ignore words with three or fewer characters, or that don't start with a letter
	BOOST_FOREACH(std::wstring word, words) {
		if (word.size() <= 3)
			continue;
		if (iswupper(word.at(0)))
			num_cap_big_words++;
		else if (iswlower(word.at(0)))
			num_nocap_big_words++;		
	}

	if (num_cap_big_words + num_nocap_big_words == 0)
		return false;

	// If the ratio of capitalized words is more than a parameter, the casing here is potentially misleading; 0.6 is probably good
	if ((float)num_cap_big_words / (num_cap_big_words + num_nocap_big_words) > _misleading_capitalization_threshold)
		return true;

	return false;
}

bool EnglishNameRecognizer::hasAbruptCaseChange(const Sentence *sentence) {
	std::wstring sentence_string = sentence->getString()->toString();
	boost::trim(sentence_string);
	if (sentence_string.size() == 0)
		return false;

	std::vector<std::wstring> words;
	boost::split(words, sentence_string, boost::is_any_of(" \t\n"));
	bool changed = false;

	// We ignore words that don't start with a letter
	for (size_t w = 0; w < words.size(); w++) {
		// Determine case of this word
		std::wstring word = words.at(w);
		if (word.size() == 0)
			continue;
		int wordCase;
		if (iswupper(word.at(0)))
			wordCase = Sentence::UPPER;
		else if (iswlower(word.at(0)))
			wordCase = Sentence::LOWER;
		else
			continue;
		for (size_t c = 1; c < word.size(); c++) {
			if (wordCase != Sentence::UPPER && iswupper(word.at(c))) {
				wordCase = Sentence::MIXED;
				break;
			} else if (wordCase != Sentence::LOWER && iswlower(word.at(c))) {
				wordCase = Sentence::MIXED;
				break;
			}
		}

		// Need to start uppercase
		if (w == 0 && wordCase != Sentence::UPPER)
			return false;

		// Change must be after at least 2 words
		if (!changed && wordCase != Sentence::UPPER) {
			if (w >= 2)
				changed = true;
			else
				return false;
		}
	}

	// Made it through all words, note whether we saw the abrupt change
	return changed;
}

void EnglishNameRecognizer::cleanUpAfterDocument() {
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->cleanUpAfterDocument();
	}
	else {
		// copied from old name recognizer -- SRS

		// heap-allocated word lengths need cleanup.
		// everything else can persist, with just the counter reset
		int i;
		for (i=0; i < _num_names; i++)
			delete [] _nameWords[i].words;
#ifdef ENABLE_LEAK_DETECTION
		for (i=0; i < _num_names; i++)
			_names[i].swap(std::wstring()); // Acutally delete memory.
#endif
		_num_names = 0;
	}
}

void EnglishNameRecognizer::resetForNewDocument(DocTheory *docTheory) {
	_docTheory = docTheory;
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->resetForNewDocument(docTheory);
	} else if (_name_finder == PIDF_NAME_FINDER) {
		_pidfDecoder->resetForNewDocument(docTheory);
	}

	_authorNameList.clear();
	if(docTheory->getDocument()->getZoning()){
		setAuthorNameList();
	}
}

void EnglishNameRecognizer::setAuthorNameList(){
	std::vector<Zone*> zones=_docTheory->getDocument()->getZoning()->getRoots();
	for(unsigned int i=0;i<zones.size();i++){
		addAuthorName(zones[i]);
	}

}

void EnglishNameRecognizer::addAuthorName(Zone* zone){
	if(zone->getAuthor()){
		addAuthorName(zone->getAuthor().get()->toSymbol().to_string());
	}
	for(unsigned int i=0;i<zone->getChildren().size();i++){
		addAuthorName(zone->getChildren()[i]);
	}
}

int EnglishNameRecognizer::getNameTheories(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence)
{
	_tokenSequence = tokenSequence;
	_sent_no = tokenSequence->getSentenceNumber();

	if (_skip_all_namefinding) {
		results[0] = _new NameTheory(tokenSequence, 0);
		return 1;
	}

	// don't do name recognition on POSTDATE region (turned into a timex elsewhere)
	if (_docTheory != 0 && _docTheory->isPostdateSentence(_sent_no)) {
		results[0] = _new NameTheory(tokenSequence, 0);
		return 1;
	}

	// make the contents of SPEAKER and RECEIVER regions automatically into person names
	if (_docTheory != 0 && 
		(_docTheory->isSpeakerSentence(_sent_no) || _docTheory->isReceiverSentence(_sent_no))) 
	{

		// although not if there is a comma or an email address involved 
		// (e.g. "BOB JONES, REPORTER FOR CNN", "name@yahoo.com")
		bool found_comma = false;
		bool found_prompt = false;
		int region_end = tokenSequence->getNTokens() - 1;
		for (int i = 0; i < tokenSequence->getNTokens(); i++) {
			if (tokenSequence->getToken(i)->getSymbol() == EnglishWordConstants::_COMMA_) {
				found_comma = true;
				break;
			}			
			if (tokenSequence->getToken(i)->getSymbol() == Symbol(L"prompt")) {
				found_prompt = true;
				break;
			}
			if (isEmailAddress(tokenSequence->getToken(i)->getSymbol())) {
				// Don't go past an email, which is probably at the end of the region
				region_end = i - 1;
				break;
			}
		}

		if (!found_comma && !found_prompt && region_end >= 0) {

			results[0] = _new NameTheory(tokenSequence);
			results[0]->takeNameSpan(_new NameSpan(0, region_end,
				EntityType::getPERType()));

			// faked
			if (_print_sentence_selection_info)
				results[0]->setScore(100000);

			if (_name_finder == PIDF_NAME_FINDER) {
				PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence);
				fixName(results[0]->getNameSpan(0), &sentence);

				if (!results[0]->getNameSpan(0)->type.isRecognized())
					results[0]->removeNameSpan(0);
			}

			return 1;

		}
	}

	int n_results;
	if (_name_finder == IDF_NAME_FINDER) {
		n_results = _idfNameRecognizer->getNameTheories(results, max_theories,
												   tokenSequence);
	}
	else { // PIDF_NAME_FINDER
		PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence, _force_lowercase_sentence);

		// Optionally pre-tag any name spans from the input document as constraints
		const Metadata* metadata = _docTheory->getDocument()->getMetadata();
		bool prev_tag_was_none = false;
		for (int t = 0; t < tokenSequence->getNTokens(); t++) {
			const Token* token = tokenSequence->getToken(t);
			const DataPreprocessor::NamedSpan* span = NULL;
			if (_use_name_constraints_from_document)
				span = dynamic_cast<DataPreprocessor::NamedSpan*>(metadata->getCoveringSpan(token->getStartEDTOffset(), Symbol(L"Name")));
			int tag = -1;
			if (span != NULL) {
				Symbol nameType = span->getOriginalType();
				if (!span->getMappedType().is_null())
					nameType = span->getMappedType();
				_pidfDecoder->getTagSet()->addTag(nameType);
				if (span->getStartOffset() == token->getStartEDTOffset())
					tag = _pidfDecoder->getTagSet()->getTagIndex(nameType + Symbol(L"-ST"));
				else
					tag = _pidfDecoder->getTagSet()->getTagIndex(nameType + Symbol(L"-CO"));
				prev_tag_was_none = false;
			} else {
				if (prev_tag_was_none)
					tag = _pidfDecoder->getTagSet()->getTagIndex(_pidfDecoder->getTagSet()->getNoneCOTag());
				else
					tag = _pidfDecoder->getTagSet()->getTagIndex(_pidfDecoder->getTagSet()->getNoneSTTag());
			}
			sentence.setTag(t, tag);
		}

		_pidfDecoder->constrainedDecode(sentence, _pidfDecoder->getTagSet());

		addUnfoundNationalities(sentence);
		reduceSpecialNames(sentence);

		if (_use_atea_name_fixes) {
			forceDoubleParenNames(sentence);
			joinDoubleParenNames(sentence);
			subsumeTokensInNames(sentence, _perSubsumptionWords, EntityType::getPERType());
		}

		if (_patternNameFinder) {
			_patternNameFinder->augmentPIdFSentence(sentence, _tagSet);
		}

		if (_use_names_from_document) {
			// Merge in any tags as long as they don't overlap with an existing tagged name
			for (int t = 0; t < tokenSequence->getNTokens(); t++) {
				// Only check for the start of name spans on untagged tokens
				if (_pidfDecoder->getTagSet()->isNoneTag(sentence.getTag(t))) {
					const DataPreprocessor::NamedSpan* span = dynamic_cast<DataPreprocessor::NamedSpan*>(metadata->getStartingSpan(tokenSequence->getToken(t)->getStartEDTOffset(), Symbol(L"Name")));
					if (span != NULL) {
						// Look ahead to make sure we don't overlap with any existing tags
						int end = t + 1;
						bool overlap = false;
						for (; end < tokenSequence->getNTokens(); end++) {
							if (!_pidfDecoder->getTagSet()->isNoneTag(sentence.getTag(end))) {
								// Hit an existing tag
								overlap = true;
								break;
							} else if (tokenSequence->getToken(end)->getEndEDTOffset() > span->getEndOffset()) {
								// Hit end of span
								break;
							}
						}

						// Merge in the tags if they don't overlap
						if (!overlap) {
							// Create a new tag if necessary
							Symbol nameType = span->getOriginalType();
							if (!span->getMappedType().is_null())
								nameType = span->getMappedType();
							_pidfDecoder->getTagSet()->addTag(nameType);

							// Get the start and continue tag indices
							int startTag = _pidfDecoder->getTagSet()->getTagIndex(nameType + Symbol(L"-ST"));
							int continueTag = _pidfDecoder->getTagSet()->getTagIndex(nameType + Symbol(L"-CO"));

							// Update the tags to store this span
							sentence.setTag(t, startTag);
							t++;
							for (; t < end; t++) {
								sentence.setTag(t, continueTag);
							}
							if (t < tokenSequence->getNTokens() && _pidfDecoder->getTagSet()->isNoneTag(sentence.getTag(t)))
								// Make sure we start NONE again after merging in the tags
								sentence.setTag(t, _pidfDecoder->getTagSet()->getTagIndex(_pidfDecoder->getTagSet()->getNoneSTTag()));
							t--; // Because we move on to the next outer loop iteration
						}
					}
				}
			}
		}

		results[0] = makeNameTheory(sentence);
		n_results = 1;
	}

	for (int i = 0; i < n_results; i++)
		postProcessNameTheory(results[i], tokenSequence);

	return n_results;
}

void EnglishNameRecognizer::addUnfoundNationalities(PIdFSentence &sentence) {
	wchar_t natST_str[100];
	wcsncpy(natST_str,
			EntityType::getNationalityType().getName().to_string(), 90);
	wcscat(natST_str, L"-ST");
	Symbol natST(natST_str);
           
	for (int i = 0; i < sentence.getLength() - 1; i++) {
		Symbol currentReducedTag = _tagSet->getReducedTagSymbol(sentence.getTag(i));
		if (EnglishNationalityRecognizer::isNationalityWord(sentence.getWord(i)) && 
			currentReducedTag != EntityType::getNationalityType().getName())
		{
			if (_tagSet->isSTTag(sentence.getTag(i+1)) || currentReducedTag == _tagSet->getNoneTag())
			{
				sentence.setTag(i, _tagSet->getTagIndex(natST));
				if (sentence.getTag(i+1) == _tagSet->getTagIndex(_NONE_CO))
					sentence.setTag(i+1, _tagSet->getTagIndex(_NONE_ST));
			}
		}
	}
}



NameTheory *EnglishNameRecognizer::makeNameTheory(PIdFSentence &sentence) {
	//std::cout << sentence.getTokenSequence()->toDebugString()<<"\n";
	//check if the tokens in the sentence match any author/orig_author value, if so, set the tokens to be ST and CO
	if(_docTheory->getDocument()->getZoning()){

		if (sentence.getLength() == 0)
			return new NameTheory(_tokenSequence);
		
		const LocatedString * ls_sent=_docTheory->getSentence(sentence.getTokenSequence()->getSentenceNumber())->getString(); 
		const TokenSequence* sequence= sentence.getTokenSequence();
		std::string sequence_string=sentence.getTokenSequence()->toDebugString();
		int last_name_index=0;
		
			EDTOffset EDTstart=ls_sent->start<EDTOffset>(0);
			EDTOffset EDTend=ls_sent->end<EDTOffset>(sentence.getLength()-1);

			if(!_authorNameList.empty()){
				std::set<std::wstring>::iterator it;
				for (it = _authorNameList.begin(); it != _authorNameList.end(); ++it)
				{  
					std::vector<int> positions;
					int len=static_cast<int>(_authorNameList.size());
					std::wstring author_name = *it; // Note the "*" here
					int pos=ls_sent->indexOf(author_name.c_str(),0);
					while(pos!=-1){
						positions.push_back(pos);
						
						pos = ls_sent->indexOf(author_name.c_str(),pos+len);
				
					}
				
					for(size_t p=0;p<positions.size();p++){
						
						EDTOffset author_end_offset=ls_sent->end<EDTOffset>(positions[p]+static_cast<int>(author_name.size())-1);
						for(int i=0;i<sequence->getNTokens();i++){

							if(ls_sent->start<EDTOffset>(positions[p])==sequence->getToken(i)->getStartEDTOffset()){
								sentence.setTag(i, _tagSet->getTagIndex(_PER_ST));
								int j=i+1;
								if(j==sequence->getNTokens())
										break;
								while(sequence->getToken(j)->getEndEDTOffset()<=author_end_offset){
									sentence.setTag(j, _tagSet->getTagIndex(_PER_CO));
									j=j+1;
									if(j==sequence->getNTokens())
										break;
								}
								
							break;
							}
						}
					}
				}
			}
			
	}
	
	
	int NONE_ST_tag = _tagSet->getTagIndex(_NONE_ST);

	int n_name_spans = 0;
	for (int j = 0; j < sentence.getLength(); j++) {
		if (sentence.getTag(j) != NONE_ST_tag &&
			_tagSet->isSTTag(sentence.getTag(j)))
		{
			// EMB 10/17: manually disallow email addresses as names
			if (_disallow_emails &&
				(j +1 == sentence.getLength() || _tagSet->isSTTag(sentence.getTag(j+1))) &&
				isEmailAddress(sentence.getWord(j))) 
			{
				if (j == 0 || !_tagSet->isNoneTag(sentence.getTag(j-1)))
					sentence.setTag(j, NONE_ST_tag);
				else sentence.setTag(j, _tagSet->getTagIndex(_NONE_CO));
				continue;
			}
			n_name_spans++;
		}
	}

	NameTheory *nameTheory = _new NameTheory(_tokenSequence);
	
	int tok_index = 0;
	for (int i = 0; i < n_name_spans; i++) {
		while (!(sentence.getTag(tok_index) != NONE_ST_tag &&
				 _tagSet->isSTTag(sentence.getTag(tok_index))))
		{ tok_index++; }

		int tag = sentence.getTag(tok_index);

		int end_index = tok_index;
		while (end_index+1 < sentence.getLength() &&
			   _tagSet->isCOTag(sentence.getTag(end_index+1)))
		{ end_index++; }

		NameSpan *ns = _new NameSpan(tok_index, end_index, EntityType(_tagSet->getReducedTagSymbol(tag)));
		
		fixName(ns, &sentence);
		if (ns->type.isRecognized())
			nameTheory->takeNameSpan(ns);
		else delete ns;

		tok_index = end_index + 1;
	}

	return nameTheory;
}

/**
 * This regular expression determines whether or not a token
 * is likely an email address.
 *
 * We implement a subset of RFC 822's addr-spec syntax from section 6.1:
 *   http://www.ietf.org/rfc/rfc0822.txt
 *
 * This allows for arbitrary local-part and domain (unlike our old
 * simple string matching), but does not support IP addresses, quoting,
 * escaping, or other special cases.
 *
 * Inside of an atom ignore control characters and these special characters:
 *   ()<>@,;:\".[]
 *
 * @author nward@bbn.com
 * @date 2013.04.26
 **/
const boost::wregex EnglishNameRecognizer::email_address_re(
	// Allow one or more .-separated atoms
	L"((?<local_part>)"
	  L"[-_+a-zA-Z0-9]+"
	  L"(\\.[-_+a-zA-Z0-9]+)*"
    L")"

	// This is non-standard, but we allow a sequence of dots
	// because Craigslist (among others) uses this to obfuscate emails
	L"(\\.+)?"

	// As before, there has to be one and only one at symbol
	L"@"

	// Allow two or more .-separated atoms
	L"((?<domain>)"
	  L"[-_+a-zA-Z0-9]+"
	  L"(\\.[-_+a-zA-Z0-9]+)+"
	L")"
);

bool EnglishNameRecognizer::isEmailAddress(Symbol sym) 
{
	return boost::regex_match(sym.to_string(), email_address_re);
}

void EnglishNameRecognizer::fixName(NameSpan* span, PIdFSentence* sent)
{
	// LB: This seems to be a hack to deal with the first sentence of
	//     a document, starting with "Blah Blah, ...". Apparently
	//     we think that these are always GPEs and are forcing them to be so.
	if (_sent_no == 0 && span->start == 0 && !span->type.matchesGPE() &&
		sent->getLength() > span->start + 1 &&
		sent->getWord(span->start + 1) == EnglishWordConstants::_COMMA_)
	{
		span->type = EntityType::getGPEType();
		return;
	}

	// LB: This function in general appears to be designed to enforce name
	//     consistency across the document.

	// First, create a string for the entire name.
	// NOTE: this returns an uppercase string!
	wstring nameStr = getNameString(span->start, span->end, sent);

	// Force EU and European Union to be GPEs. The training data is inconsistent.
	if (nameStr.compare(L"EU") == 0 && span->type.matchesORG())
		span->type = EntityType::getGPEType();
	if (nameStr.compare(L"EUROPEAN UNION") == 0 && span->type.matchesORG())
		span->type = EntityType::getGPEType();

	_force_type_map_t::const_iterator iter = _forcedTypeNames.find(nameStr);
	if (iter != _forcedTypeNames.end()) {
		EntityType et = (*iter).second;
		if (et != span->type) {
			SessionLogger::info("fix_name_by_force") << L"Changing " << nameStr.c_str() << L" by force from "
					<< span->type.getName().to_string() << L" to "
					<< et.getName().to_string() << L"\n";
		}
		span->type = et;
		return;
	}

	int span_length = (span->end+1 - span->start);
	EntityType type;
	int i;
	bool foundMatch = false;
	bool exactMatch = false;
	// exact match phase
	for (i=0; i < _num_names; i++) {
		if (_names[i] == nameStr) {
			type = _types[i];
			foundMatch = true;
			exactMatch = true;
			break;
		}
	}
	// inexact match phase
	if (!foundMatch) {
		for (i=0; i < _num_names; i++) {
			// name must be longer to be inexact
			if (_nameWords[i].num_words <= span_length)
				continue;

			// GPE/LOC/NATIONALITY matching to non-GPE/LOC/NATIONALITY can 
			// only match to PER, and can only match as a terminating substring
			if ((span->type.matchesGPE() || span->type.matchesLOC()
					|| span->type.isNationality()) &&
				(!_types[i].matchesGPE() && !_types[i].matchesLOC()
					&& !_types[i].isNationality()))
			{
				if (!_types[i].matchesPER())
					continue;
				int start_idx = _nameWords[i].num_words-span_length;
				wstring subName = getNameString(start_idx,
								  _nameWords[i].num_words-1, _nameWords[i]);
				if (subName == nameStr) {
					type = _types[i];
					foundMatch = true;
					break;
				}
			} else if (_types[i].getName() == Symbol(L"SOFTWARE")) { 
				// Don't use previously seen SOFTWARE name to change other name's type
				break;
			}
			// for all others, we must check all substring-by-word combinations
			else {
				// consider each span of the proper length
				int j;
				for (j = 0; j <= _nameWords[i].num_words-span_length; j++) {
					wstring subName = getNameString(j, j+(span_length-1),
												   _nameWords[i]);
					if (subName == nameStr) {
						type = _types[i];
						foundMatch = true;
						break;
					}
				}
				if (foundMatch)
					break;
			}
		}
	}

	// matched the name - in some way
	if (foundMatch) {
		// fix name so it's consistent
		if (type != span->type) {
			if (_debug_flag) {
				SessionLogger::info("fix_name") << L"Changing " << nameStr.c_str() << L" from "
					<< span->type.getName().to_string() << L" to "
					<< type.getName().to_string() << L"\n";							
			}
			span->type = type;
		}
	}
	// didn't match perfectly, so we have a new name
	if (!exactMatch) {
		// can't save any more names, so warn, but continue
		if (_num_names >= MAX_DOC_NAMES) {
			SessionLogger::warn("document_name_limit") << "Document exceeds name limit of "
				   << MAX_DOC_NAMES << "\n";
		}
		else {
			_names[_num_names] = nameStr;
			_types[_num_names] = span->type;
			_nameWords[_num_names].num_words = span_length;
			_nameWords[_num_names].words = _new Symbol[span_length];
			int index = 0;
			for (i=span->start; i <= span->end; i++)
				_nameWords[_num_names].words[index++] = sent->getWord(i);
			_num_names++;
		}
	}
	return;
}

// concatenate symbols with spaces, and make them all uppercase

wstring EnglishNameRecognizer::getNameString(int start, int end,
									 PIdFSentence* sent)
{
	wchar_t nameStr[MAX_SENTENCE_LENGTH];
	int i;
	int offset = 0;
	for (i = start; i <= end; i++) {
		Symbol word = sent->getWord(i);
		offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
						   L"%ls", word.to_string());
		if (i < end)
			offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
							   L" ");
	}
	// transform to uppercase
	for (i = 0; i < offset; i++)
		nameStr[i] = towupper(nameStr[i]);
	return wstring(nameStr);
}

wstring EnglishNameRecognizer::getNameString(int start, int end, NameSet set)
{
	wchar_t nameStr[MAX_SENTENCE_LENGTH];
	int i;
	int offset = 0;
	for (i = start; i <= end; i++) {
		Symbol word = set.words[i];
		offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
						   L"%ls", word.to_string());
		if (i < end)
			offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
							   L" ");
	}
	// transform to uppercase
	for (i = 0; i < offset; i++)
		nameStr[i] = towupper(nameStr[i]);
	return wstring(nameStr);
}

void EnglishNameRecognizer::loadSubsumptionWords(string &wordsFile, vector<SymbolVector> &subsumptionWords) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(wordsFile));
	UTF8InputStream& in(*in_scoped_ptr);

	wstring line;
	while (!in.eof()) {
		in.getLine(line);

		std::vector<wstring> pieces;
		boost::algorithm::split(pieces, line, boost::is_any_of(L" "), boost::token_compress_on);

		if (pieces.size() > 0) {
			SymbolVector symVector;
			for (size_t i = 0; i < pieces.size(); i++) {
				symVector.push_back(Symbol(pieces[i]));
			}
			subsumptionWords.push_back(symVector);
		}
	}
}

// look for tokens (specified by a list) directly before or after a 
// name of a specified entity type. If those tokens are found, 
// extend the name over those tokens.
void EnglishNameRecognizer::subsumeTokensInNames(PIdFSentence &sentence, vector<SymbolVector> &subsumptionWords, EntityType entType) {

	Symbol entTypeSym = entType.getName();
	wstring entStr1 = entTypeSym.to_string();
	wstring entStr2 = entTypeSym.to_string();
	wstring entCOStr = entStr1.append(L"-CO");
	wstring entSTStr = entStr2.append(L"-ST");
	Symbol entCO = Symbol(entCOStr);
	Symbol entST = Symbol(entSTStr);

	for (int i = 0; i < sentence.getLength(); i++) {
		// look for start of name
		int tag = sentence.getTag(i);
		if (_tagSet->isSTTag(tag) && _tagSet->getReducedTagSymbol(tag) == entTypeSym) {
			int count = countSubsumptionWordsReverse(sentence, subsumptionWords, i - 1);
			if (count > 0) {
				sentence.setTag(i, _tagSet->getTagIndex(entCO));
				int j;
				for (j = i - 1; j > i - count; j--) 
					sentence.setTag(j, _tagSet->getTagIndex(entCO));
				sentence.setTag(j, _tagSet->getTagIndex(entST));
			}
		}
	}


	bool ent_name_previous = false;
	for (int i = 0; i < sentence.getLength(); i++) {
		// look for end of name
		int tag = sentence.getTag(i);
		if (_tagSet->isSTTag(tag) && ent_name_previous) {
			int count = countSubsumptionWordsForward(sentence, subsumptionWords, i);
			if (count > 0) {
				int j;
				for (j = i; j < i + count; j++)
					sentence.setTag(j, _tagSet->getTagIndex(entCO));
				if (j < sentence.getLength() &&_tagSet->getReducedTagSymbol(sentence.getTag(j)) != entTypeSym)
					sentence.changeToStartTag(j);
			}
		}
		if (_tagSet->getReducedTagSymbol(tag) == entTypeSym) 
			ent_name_previous = true;
		else
			ent_name_previous = false;
	}
}

int EnglishNameRecognizer::countSubsumptionWordsReverse(PIdFSentence &sentence, vector<SymbolVector> &subsumptionWords, int index) {
	size_t max_count = 0;

	for (size_t i = 0; i < subsumptionWords.size(); i++) {
		size_t vector_index = 0;
		SymbolVector symVector = subsumptionWords[i];
		for (int j = index; j >= 0 && j > index - (int)(symVector.size()); j--) {
			if (sentence.getWord(j) != symVector[symVector.size() - 1 - vector_index])
				break;
			vector_index++;
		}
		if (vector_index == symVector.size() && symVector.size() > max_count) {
			max_count = symVector.size();
		}
	}
	return static_cast<int>(max_count);
}

int EnglishNameRecognizer::countSubsumptionWordsForward(PIdFSentence &sentence, vector<SymbolVector> &subsumptionWords, int index) {
	size_t max_count = 0;

	for (size_t i = 0; i < subsumptionWords.size(); i++) {
		size_t vector_index = 0;
		SymbolVector symVector = subsumptionWords[i];
		for (int j = index; j < sentence.getLength() && j < index + (int)(symVector.size()); j++) {
			if (sentence.getWord(j) != symVector[vector_index])
				break;
			vector_index++;
		}
		if (vector_index == symVector.size() && symVector.size() > max_count) {
			max_count = symVector.size();
		}
	}
	return static_cast<int>(max_count);
}


// If PIdF finds "Smith, John" as two names, combine them into one
void EnglishNameRecognizer::fixReversedNames(PIdFSentence &sentence) {

	Symbol perSymbol = EntityType::getPERType().getName();
	wstring perStr = perSymbol.to_string();
	wstring perCOStr = perStr.append(L"-CO");
	Symbol perCO = Symbol(perCOStr);

	for (int i = 0; i < sentence.getLength() - 2; i++) {
		// check for one word person name, followed by a comma
		
		int tag = sentence.getTag(i);
		if (!_tagSet->isSTTag(tag)) 
			continue;
		
		Symbol reducedTag = _tagSet->getReducedTagSymbol(tag);
		if (reducedTag != perSymbol) 
			continue;

		int next_tag = sentence.getTag(i + 1);
		if (!_tagSet->isSTTag(next_tag) || !_tagSet->isNoneTag(next_tag))
			continue;
		
		Symbol nextWord = sentence.getWord(i + 1);
		if (nextWord != EnglishWordConstants::_COMMA_) 
			continue;

		// after the comma, count how many words make up a person name, 
		// starting at the word after the comma
		int per_word_count = 0;
		for (int j = i + 2; j < sentence.getLength(); j++) {
			int tag2 = sentence.getTag(j);
			Symbol reducedTag2 = _tagSet->getReducedTagSymbol(tag2);

			if (reducedTag2 != perSymbol)
				break;

			if (j != i + 2 && _tagSet->isSTTag(tag2))
				break;

			per_word_count++;
		}

		if (per_word_count == 0) 
			continue;

		if (!inProbableList(sentence, i)) 
			for (int j = i + 1; j <= i + 1 + per_word_count; j++)
				sentence.setTag(j, _tagSet->getTagIndex(perCO));
	}
}


bool EnglishNameRecognizer::inProbableList(PIdFSentence &sentence, int name_start) {
	// walk over the rest of sentence until you hit a
	// token that's not a person and not a comma, look for "and"
	
	for (int i = name_start + 1; i < sentence.getLength(); i++) {
		Symbol word = sentence.getWord(i);
		wstring wordStr = word.to_string();
		std::transform(wordStr.begin(), wordStr.end(), wordStr.begin(), towlower);
		if (wordStr.compare(L"and") == 0)
			return true;

		int tag = sentence.getTag(i);
		Symbol reducedTag = _tagSet->getReducedTagSymbol(tag);

		if (word != EnglishWordConstants::_COMMA_ && reducedTag != EntityType::getPERType().getName())
			break;
	}
	
	return false;
}

// If we see John ((Smith)) make sure everything up including the last double 
// paren occurs in the name
void EnglishNameRecognizer::joinDoubleParenNames(PIdFSentence &sentence) {

	Symbol perSymbol = EntityType::getPERType().getName();
	wstring perStr = perSymbol.to_string();
	wstring perCOStr = perStr.append(L"-CO");
	Symbol perCO = Symbol(perCOStr);

	// look for PER names surrounded by -LDB- and -RDB-, turn all of it to PER
	for (int i = 0; i < sentence.getLength() - 2; i++) {
		if (WordConstants::isOpenDoubleBracket(sentence.getWord(i))) {
			bool found_per = false;
			bool found_rdb = false;
			int j;
			for (j = i + 1; j < i + 4 && j < sentence.getLength(); j++) {
				Symbol reducedTag = _tagSet->getReducedTagSymbol(sentence.getTag(j));
				Symbol word = sentence.getWord(j);
				if (reducedTag == perSymbol) found_per = true;
				if (WordConstants::isClosedDoubleBracket(word)) {
					found_rdb = true;
					break;
				}
			}

			// Found PER surrounded by -LDB- and -RDB-. i is index of -LDB-, j is index of -RDB-
			if (found_per && found_rdb) {
				for (int k = i; k <= j; k++) {
					sentence.setTag(k, _tagSet->getTagIndex(perCO));
				}
				if (i == 0 || _tagSet->getReducedTagSymbol(sentence.getTag(i - 1)) != perSymbol) {
					sentence.changeToStartTag(i);
				}
				if (j + 1 < sentence.getLength() && _tagSet->getReducedTagSymbol(sentence.getTag(j + 1)) == perSymbol && _tagSet->isSTTag(sentence.getTag(j + 1))) {
					sentence.changeToContinueTag(j + 1);
				}
				if (j + 1 < sentence.getLength() && _tagSet->getReducedTagSymbol(sentence.getTag(j + 1)) != perSymbol) {
					sentence.changeToStartTag(j + 1);
				}
			}
		}
	}
}

// Any time you see one or two tokens surrounded by double parens, make it a person name
void EnglishNameRecognizer::forceDoubleParenNames(PIdFSentence &sentence) {

	Symbol perSymbol = EntityType::getPERType().getName();
	wstring perStr = perSymbol.to_string();
	wstring perCOStr = perStr.append(L"-CO");
	Symbol perCO = Symbol(perCOStr);
	perStr = perSymbol.to_string();
	wstring perSTStr = perStr.append(L"-ST");
	Symbol perST = Symbol(perSTStr);

	// look for PER names surrounded by -LDB- and -RDB-, turn all of it to PER
	for (int i = 0; i < sentence.getLength() - 2; i++) {
		if (WordConstants::isOpenDoubleBracket(sentence.getWord(i)) && 
			WordConstants::isClosedDoubleBracket(sentence.getWord(i + 2)) && 
			isForceableToPerson(sentence, i + 1, i + 1)) 
		{
			if (sentence.getTag(i + 1) != _tagSet->getTagIndex(perCO)) {
				sentence.setTag(i + 1, _tagSet->getTagIndex(perST));
			}
			if (sentence.getTag(i + 2) != _tagSet->getTagIndex(perCO)) {
				sentence.changeToStartTag(i + 2);
			}
		}

		if (i + 3 < sentence.getLength() && 
			WordConstants::isOpenDoubleBracket(sentence.getWord(i)) && 
			WordConstants::isClosedDoubleBracket(sentence.getWord(i + 3)) && 
			isForceableToPerson(sentence, i + 1, i + 2)) 
		{
			if (sentence.getTag(i + 1) != _tagSet->getTagIndex(perCO)) {
				sentence.setTag(i + 1, _tagSet->getTagIndex(perST));
			}
			sentence.setTag(i + 2, _tagSet->getTagIndex(perCO));
			
			if (sentence.getTag(i + 3) != _tagSet->getTagIndex(perCO)) {
				sentence.changeToStartTag(i + 3);
			}
		}
	}
}

bool EnglishNameRecognizer::isForceableToPerson(PIdFSentence &sentence, int start, int end) {
	for (int i = start; i <= end; i++) {
		// check for other type
		int tag = sentence.getTag(i);
		Symbol reducedTagSym = _tagSet->getReducedTagSymbol(tag);
		Symbol tagSym = _tagSet->getTagSymbol(tag);
		if (tagSym != _NONE_ST && tagSym != _NONE_CO  && reducedTagSym != EntityType::getPERType().getName())
			return false;
		
		// check for digit in words
		Symbol word = sentence.getWord(i);
		const wchar_t *wordStr = word.to_string();
		for (size_t i = 0; i < wcslen(wordStr); i++) {
			if (iswdigit(wordStr[i]))
				return false;
		}
	}
	return true;
}

void EnglishNameRecognizer::reduceSpecialNames(PIdFSentence &sentence) {
	if (!_reduce_special_names || _reductionTable == 0)
		return;

	for (int k = 0; k < sentence.getLength(); k++) {
		int tag = sentence.getTag(k);
		if (_tagSet->isSTTag(tag) && !_tagSet->isNoneTag(tag)) {
			Symbol sym = _tagSet->getReducedTagSymbol(tag);

			// start-end [inclusive]
			int start = k;
			int end = 0;
			for (end = k+1; end < sentence.getLength(); end++) {
				if (_tagSet->isSTTag(sentence.getTag(end)))
					break;
			}
			end--;

			if (start == end)
				continue;

			for (int special = start; special <= end; special++) {
				int name_found = _reductionTable->isListMember(&sentence, special);
				if (name_found == 0)
					continue;

				// punctuation makes this too dangerous
				bool stop = false;
				for (int n = start; n < special; n++) {
					if (WordConstants::isPunctuation(sentence.getWord(n)))
						stop = true;
				}
				for (int p = special + name_found; p <= end; p++) {
					if (WordConstants::isPunctuation(sentence.getWord(p)))
						stop = true;
				}

				if (stop)
					continue;

				if (special != start)
					sentence.setTag(start, _tagSet->getTagIndex(_tagSet->getNoneSTTag()));
				for (int m = start + 1; m < special; m++)
					sentence.setTag(m, _tagSet->getTagIndex(_tagSet->getNoneCOTag()));
				sentence.setTag(special, tag);
				if (special + name_found <= end)
					sentence.setTag(special + name_found, _tagSet->getTagIndex(_tagSet->getNoneSTTag()));
				for (int q = special + name_found + 1; q <= end; q++)
					sentence.setTag(q, _tagSet->getTagIndex(_tagSet->getNoneCOTag()));
				k = end;
				break;

			}

		}
	}

}

