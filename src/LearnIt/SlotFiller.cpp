#include "Generic/common/leak_detection.h"
#include <cassert>
#include <iostream>
#include "boost/algorithm/string/find.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/regex.hpp"
#include "boost/foreach.hpp"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueType.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/features/ValueMentionPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/common/TimexUtils.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/Seed.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/ObjectWithSlots.h"

using boost::dynamic_pointer_cast; using boost::make_shared;
using boost::make_shared;

double SlotFiller::GOOD_MATCH=0.0000001;
bool SlotFiller::_initialized = false;
bool SlotFiller::_useCoref = false;

Symbol SlotFiller::_curr_doc = Symbol();

std::map<const Entity*, std::set<std::wstring>, entityCompare > SlotFiller::entityToStringSet = 
	std::map<const Entity*, std::set<std::wstring>, entityCompare >();

SlotBase::SlotBase(SlotConstraints_ptr constraints) 
: _yymmdd_initialized(false), _yymmdd(), _slot_type(constraints->getMentionType()),
_is_yymmdd(constraints->getMentionType()==SlotConstraints::VALUE 
		   && constraints && constraints->getSeedType()==L"yymmdd") {}

SlotBase::SlotBase(SlotConstraints_ptr constraints, SlotConstraints::SlotType t) 
: _yymmdd_initialized(false), _yymmdd(), _slot_type(t),
_is_yymmdd(t == SlotConstraints::VALUE && constraints 
		   && constraints->getSeedType()==L"yymmdd") {}


SlotBase::SlotBase(const SlotBase& oth) : _yymmdd_initialized(oth._yymmdd_initialized),
_yymmdd(oth._yymmdd), _slot_type(oth._slot_type), _is_yymmdd(oth._is_yymmdd) {}

const std::vector<std::wstring>& SlotBase::asYYMMDD() const {
	if (!_yymmdd_initialized) {
		_initYYMMDD();
		_yymmdd_initialized = true;
	}
	return _yymmdd;
}

// parsing a string as a date is expensive, so we cache it
void SlotBase::_initYYMMDD() const {
	if (_is_yymmdd){ // otherwise, we don't need this information
		TimexUtils::parseYYYYMMDD(name(), _yymmdd);
	}
}

AbstractLearnItSlot::AbstractLearnItSlot(SlotConstraints_ptr constraints, 
	const std::wstring& nm) : SlotBase(constraints), _name(nm) {}

const std::wstring& AbstractLearnItSlot::name(bool normalize) const {
	if (normalize){
		if (_normalized_name.empty()){
			_normalized_name = MainUtilities::normalizeString(_name);
		}
		return _normalized_name;
	}
	else
		return _name;
}

SlotConstraints::SlotType SlotBase::getType() const { return _slot_type; }

// this needs to be kept in sync with two things because the IR 
// needs to emulate the matching behavior of the distill_readers
// as exactly as possible:
// make_date_query_advanced in generate_search_terms.py
// LearnItDocConverter::addBackedOffDates
double SlotBase::YYMMDDMatchAgainst(const SlotBase & other) const 
{
	// These values (which should be turned into class-level constants) determine how
	// much of a penalty we suffer if the seed has a concrete value, but we have 
	// XXXX (i.e., an unknown value).  
	static std::vector<float> piece_penalties;
	if (piece_penalties.empty()) {
		piece_penalties.push_back(0.5f); // year
		piece_penalties.push_back(0.9f); // month
		piece_penalties.push_back(0.8f); // day
	}

	// Parse the seed string and our own best name string.  If either of them is 
	// not a YYYYMMDD date, then return false.  Other formats we might 
	// encounter include: "thursday", "past_ref", "p1y", "p1d", etc.
	// Note that the SlotFiller's own bestname was parsed as a date at 
	// construction time and stored in _YYMMDDParts. If that parse failed, 
	// no need to parse the seed_string
	if (_yymmdd.empty() || other._yymmdd.empty())
		return 0.0;

	float penalty = 0;
	int num_seed_pieces = 0; // how many are concrete?
	int num_best_name_pieces = 0; // how many are concrete (& match)?

	// Evaluate each piece (year, month, day)
	for (int piece=0; piece<3; piece++) {
		const std::wstring& seed_piece = other._yymmdd[piece];
		const std::wstring& best_name_piece = _yymmdd[piece];
		if (seed_piece == L"xxxx" || seed_piece == L"xx" || seed_piece.empty()) {
			continue;
		} else if (best_name_piece == L"xxxx" || best_name_piece == L"xx" || best_name_piece.empty()) {
			penalty += piece_penalties[piece];
			num_seed_pieces += 1;
		} else if (seed_piece == best_name_piece) {
			num_best_name_pieces += 1;
			num_seed_pieces += 1;
		} else {
			return 0.0;
		}
	}

	// Unless the seed only contains a single piece, require that the best name
	// has more than one concrete piece.
	if ((num_seed_pieces>1) && (num_best_name_pieces<=1)) {
		penalty = 1.0f;
	}

	// Calculate the confidence.
	return std::max(0.0, 1.0 - penalty);
}

void SlotFiller::fillCache(const AlignedDocSet_ptr& doc_set){
	SlotFiller::entityToStringSet.clear();
	BOOST_FOREACH(LanguageVariant_ptr lv, doc_set->getLanguageVariants()) {
		const DocTheory* dt = doc_set->getDocTheory(lv);
		EntitySet* entity_set = dt->getEntitySet();
		for( int i = 0; i < entity_set->getNEntities(); ++i ){
			Entity* entity = entity_set->getEntity(i);
			for( int j = 0; j < entity->getNMentions(); ++j ){
				Mention* coref_ment = entity_set->getMention(entity->getMention(j));
				if(coref_ment->getMentionType() == Mention::NAME && coref_ment->getHead() != 0){
					SlotFiller::entityToStringSet[entity].insert(MainUtilities::normalizeString(
						dt->getSentenceTheory(coref_ment->getSentenceNumber())->getTokenSequence()->toString(
						coref_ment->getHead()->getStartToken(), coref_ment->getHead()->getEndToken())));

				}
			}
		}
	}
}

SlotFiller::SlotFiller(const Mention *mention, const DocTheory* dt, 
					   SlotConstraints_ptr slot_constraints, const LanguageVariant_ptr& languageVariant):
SlotBase(slot_constraints, SlotConstraints::MENTION), _mention(mention),
_value_mention(0), _doc(dt), _language_variant(languageVariant),
_sent_theory(dt->getSentenceTheory(mention->getSentenceNumber()))
{
	assert (_mention);
	_init(slot_constraints);
}


SlotFiller::SlotFiller(const ValueMention *value_mention, const DocTheory *dt, 
					   SlotConstraints_ptr slot_constraints, const LanguageVariant_ptr& languageVariant): 
SlotBase(slot_constraints, SlotConstraints::VALUE), _mention(0), 
_value_mention(value_mention), _doc(dt), _language_variant(languageVariant),
_sent_theory(dt->getSentenceTheory(value_mention->getSentenceNumber()))
{
	assert (_value_mention);
	_init(slot_constraints);
}

SlotFiller_ptr SlotFiller::fromPatternFeature(PatternFeature_ptr slot_feature,
			const DocTheory *doc, SlotConstraints_ptr slot_constraints) {
	return SlotFiller::fromPatternFeature(slot_feature,doc,slot_constraints,slot_feature->getLanguageVariant());
}

SlotFiller_ptr SlotFiller::fromPatternFeature(PatternFeature_ptr slot_feature,
			const DocTheory *doc, SlotConstraints_ptr slot_constraints, const LanguageVariant_ptr& languageVariant) 
{
	if (MentionPFeature_ptr smf = dynamic_pointer_cast<MentionPFeature>(slot_feature)) {
		return make_shared<SlotFiller>(smf->getMention(), doc, slot_constraints, languageVariant);
	} else if (ValueMentionPFeature_ptr svmf = 
			dynamic_pointer_cast<ValueMentionPFeature>(slot_feature)) {
		return make_shared<SlotFiller>(svmf->getValueMention(), doc, slot_constraints, languageVariant);
	} else if (MentionReturnPFeature_ptr srf = 
			dynamic_pointer_cast<MentionReturnPFeature>(slot_feature))
	{
		return make_shared<SlotFiller>(srf->getMention(), doc, slot_constraints, languageVariant);
	} else if (ValueMentionReturnPFeature_ptr svrf =
			dynamic_pointer_cast<ValueMentionReturnPFeature>(slot_feature)) 
	{
		return make_shared<SlotFiller>(svrf->getValueMention(), doc, slot_constraints, languageVariant);
	} else throw std::invalid_argument("Bad Snippet Feature Type");
}

void SlotFiller::_init(SlotConstraints_ptr slot_constraints) {
	_findBestName(slot_constraints);
}

SlotFiller::SlotFiller(SlotConstraints::SlotType slot_type):
SlotBase(SlotConstraints_ptr(), slot_type), _mention(NULL), _value_mention(NULL),
_language_variant(LanguageVariant::getLanguageVariant()),
_doc(NULL),  _sent_theory(NULL) 
{
	if (getType() == SlotConstraints::MENTION || getType() == SlotConstraints::VALUE) {
		throw std::invalid_argument("Can't create placeholder SlotFillers for Mentions or ValueMentions");
	}
}

SlotFiller::SlotFiller(const SlotFiller& sf, double penalty) :
	SlotBase(sf), _mention(sf._mention), _value_mention(sf._value_mention), 
		_doc(sf._doc), _sent_theory(sf._sent_theory), 
		_language_variant(sf._language_variant),
		_bestName(sf._bestName, penalty), _normalizedBestName(sf._normalizedBestName)
{
	assert (_mention || _value_mention);
}

const std::wstring& SlotFiller::name(bool normalize) const {
	return getBestName(normalize);
}

const std::wstring& SlotFiller::getBestName(bool normalize) const {
	if (normalize){
		if (_normalizedBestName.empty()){
			_normalizedBestName = MainUtilities::normalizeString(_bestName.bestName());
		}
		return _normalizedBestName;
	}
	else
		return _bestName.bestName();
}

void SlotFiller::getBestNameMentionOffsets(EDTOffset &start, EDTOffset &end) const {
	start = _bestName.startOffset();
	end = _bestName.endOffset();
}

std::wstring SlotFiller::getText() const {
	assert (_mention || _value_mention);
	if (_mention)
		return _mention->getNode()->toCasedTextString(_sent_theory->getTokenSequence());
	else
		return _value_mention->toCasedTextString(_sent_theory->getTokenSequence());
}

std::wstring SlotFiller::getOriginalText() const {
	// Make sure we have a fully loaded document
	if (_doc == NULL)
		return L"";
	Document* doc = _doc->getDocument();
	std::wstring original_text;
	if (doc) {
		// Get the document substring by offsets
		const LocatedString* doc_text = doc->getOriginalText();
		LocatedString* located_substring = MainUtilities::substringFromEdtOffsets(doc_text, getStartOffset(), getEndOffset());
		original_text = located_substring->toWString();
		delete located_substring;
	} else {
		// Document not fully loaded, just use the tokenized text from the TokenSequence
		original_text = getText();
	}

	return original_text;
}

/*int SlotFiller::getSerializedObjectID() const {
	assert (_mention || _value_mention);
	if (_mention)
		return ObjectIDTable::getID(_mention);
	else
		return ObjectIDTable::getID(_value_mention);
}

int SlotFiller::getSerializedObjectIDForEntity() const {
	assert (_mention || _value_mention);
	if (_mention) {
		const EntitySet* entity_set = _doc->getEntitySet();
		return ObjectIDTable::getID(entity_set->getEntityByMention(_mention->getUID()));
	} else {
		return 0;
	}
}
*/

int SlotFiller::getStartToken() const {
	assert (_mention || _value_mention);
	return (_mention) ? _mention->getNode()->getStartToken() : _value_mention->getStartToken();
}

int SlotFiller::getEndToken() const {
	assert (_mention || _value_mention);
	return (_mention) ? _mention->getNode()->getEndToken() : _value_mention->getEndToken();
}

int SlotFiller::getHeadStartToken(const Target_ptr target, const int slot_num) const {
	assert (_mention || _value_mention);
	if (_value_mention) {
		return _value_mention->getStartToken();
	}
	else if (ParamReader::isParamTrue("always_use_parent_for_atomic_head") &&
			 (!target->getSlotConstraints(slot_num)->useBestName() || 
				 !getMentionEntity()->hasNameMention(_doc->getEntitySet())) &&
			 _mention->getAtomicHead()->getParent() != NULL)
	{
		// for special cases like "our receiver systems" or "the natual gas" we want to chop off the DT/PRP
		int start_token = _mention->getAtomicHead()->getParent()->getStartToken();
		static const int max_pos_results = 10;
		Symbol pos_sequence[max_pos_results]; 
		_mention->getAtomicHead()->getParent()->getPOSSymbols(pos_sequence,max_pos_results);
		Symbol::SymbolGroup POS_TO_CHOP = Symbol::makeSymbolGroup(L"DT PRP$");
		for(int i=0; i<max_pos_results; i++) {
			if (pos_sequence[i].isInSymbolGroup(POS_TO_CHOP)){
				start_token++;
//				SessionLogger::info("IF") << _mention->getAtomicHead()->getParent()->toTextString();
//				SessionLogger::info("IF") << "atomic head: " <<_sent_theory->getTokenSequence()->toString(start_token, getEndToken());
			}
			else { break; }
		}
		return start_token;
	}
	else {
		return _mention->getAtomicHead()->getStartToken();
	}
}

int SlotFiller::getHeadEndToken(const Target_ptr target, const int slot_num) const {
	assert (_mention || _value_mention);
	if (_value_mention) {
		return _value_mention->getEndToken();
	}
	else if (ParamReader::isParamTrue("always_use_parent_for_atomic_head") &&
			 (!target->getSlotConstraints(slot_num)->useBestName() || 
				 !getMentionEntity()->hasNameMention(_doc->getEntitySet())) &&
			 _mention->getAtomicHead()->getParent() != NULL)
	{
		return _mention->getAtomicHead()->getParent()->getEndToken();
	}
	else {
		return _mention->getAtomicHead()->getEndToken();
	}
}

EDTOffset SlotFiller::getStartOffset() const {
	if (getType() == SlotConstraints::UNFILLED_OPTIONAL ||
		getType() == SlotConstraints::UNMET_CONSTRAINTS) 
	{
                throw UnrecoverableException("SlotFiller::getStartOffset",
                        "Programming error: cannot retrieve offsets for placeholder SlotFiller types");
	}
	if (!_sent_theory)
            throw UnrecoverableException("SlotFiller::getStartOffSet", "SlotFiller::_sent_theory is null");
	return _sent_theory->getTokenSequence()->getToken(getStartToken())->getStartEDTOffset();
}

EDTOffset SlotFiller::getEndOffset() const {
	if (getType() == SlotConstraints::UNFILLED_OPTIONAL ||
            getType() == SlotConstraints::UNMET_CONSTRAINTS)
	{
            throw UnrecoverableException("SlotFiller::getEndOffset()",
		"Programming error: cannot retrieve offsets for placeholder SlotFiller types");
	}
	if (!_sent_theory)
            throw UnrecoverableException("SlotFiller::getEndOffset()",
                "SlotFiller::_sent_theory is null");
	return _sent_theory->getTokenSequence()->getToken(getEndToken())->getEndEDTOffset();
}

Mention::Type SlotFiller::getMentionType() const {
	assert (_mention || _value_mention);
	return (_mention) ? _mention->mentionType : Mention::NONE;
}

const Entity* SlotFiller::getMentionEntity() const {
	//SessionLogger::info("IF") << _language_variant->toString() << L"\n";
	assert (_mention || _value_mention);
	if (_mention) {
		EntitySet* entity_set = _doc->getEntitySet();
		return entity_set->lookUpEntityForMention(_mention->getUID());
	} else {
		return NULL;
	}
}

// initialize bestName.
void SlotFiller::_findBestName(SlotConstraints_ptr slot_constraints) 
{
	// For ValueMentions: 
	if (!_mention) {
		std::wstring best_name_string=L"NO_NAME";
		//SessionLogger::info("LEARNIT") << "debug _findBestName value =>" << getText() << "<= seedType=" << slot_constraints->getSeedType() << std::endl;
		if (slot_constraints->getSeedType() == L"yymmdd") {
			if (_value_mention && _value_mention->isTimexValue()) {
				const Value * docVal = _value_mention->getDocValue();
				Symbol timexVal = docVal->getTimexVal();
				if (timexVal.is_null()){
					// no good value is flagged this way -- don't ask for string
					best_name_string = getText();
				}else { 
					wchar_t const *timexString = timexVal.to_string();
					best_name_string = timexString;
				}
			}
		} else {
			best_name_string = getText();
		}
		double best_name_confidence = 1.0;
		Mention::Type best_name_mention_type = Mention::NONE;
		
		_bestName=BestName(best_name_string, best_name_confidence, 
			best_name_mention_type, EDTOffset(), EDTOffset());
	} else { 	// For Mentions:
		// The last argument, which specifies whether to take the full text of
		// a mention or just its head, depends on whether or not there are
		// any Brandy constraints
		if (slot_constraints->useBestName()) {
			_bestName=BestName::calcBestNameForMention(_mention, _sent_theory, 
			_doc, slot_constraints->getBrandyConstraints().empty(), true);
		} else {
			const SynNode* node = (_mention->getHead())?
									_mention->getHead():_mention->getNode();
			
			const std::wstring name = node->toCasedTextString(
										_sent_theory->getTokenSequence());
			_bestName = BestName(name, 1.0, _mention->getMentionType(), 
				_sent_theory->getTokenSequence()->getToken(
									node->getStartToken())->getStartEDTOffset(),
				_sent_theory->getTokenSequence()->getToken(
									node->getEndToken())->getEndEDTOffset());
		}
	}
}

// when comparing a seed slot to a mention, this is the function used
// to get the string representation of coreferent mentions
std::wstring SlotFiller::corefName(const DocTheory* dt, const Mention* coref_ment) {
	return MainUtilities::normalizeString(
			dt->getSentenceTheory(coref_ment->getSentenceNumber())->getTokenSequence()->toString( 
				coref_ment->getHead()->getStartToken(), 
				coref_ment->getHead()->getEndToken()));
}

// TODO: after testing that behavior is identical, make regular SlotFiller 
// methods use this as well
SlotFiller::EntityStringsIterator SlotFiller::getEntityStrings(const DocTheory* docTheory) const 
{
	if (_mention ==NULL) {
		return SlotFiller::entityToStringSet.end();
	}
	const Entity* entity = docTheory->getEntitySet()->lookUpEntityForMention(_mention->getUID());
	if (entity == 0) {
		return SlotFiller::entityToStringSet.end();
	}

	return SlotFiller::entityToStringSet.find(entity);
}

SlotFiller::EntityStringsIterator SlotFiller::noEntityStrings() {
	return SlotFiller::entityToStringSet.end();
}


bool SlotFiller::matchSeedStringToEntity(const std::wstring& seed_string) const
{
	EntitySet* entity_set = _doc->getEntitySet();
	const Entity* entity = entity_set->lookUpEntityForMention(_mention->getUID());
	return matchSeedStringToEntity(seed_string, entity);	
}

bool SlotFiller::matchSeedStringToEntity(const std::wstring& seed_string,
										 const Entity* entity) const 
{
	if(entity == 0){
		return false;
	}

	EntityToStringSetMapping::const_iterator entityProbe=
		SlotFiller::entityToStringSet.find(entity);
	if (entityProbe!=SlotFiller::entityToStringSet.end()) {
		if (entityProbe->second.find(seed_string)!=entityProbe->second.end()) {
			return true;
		}
	}
	//MRF:  The cache only contains NAMES.  Not all entities have names and when matching non-ACE types (e.g. games), 
	//we return false, and try for partial matches.  
	return false;
}

bool SlotFiller::matchesArgument(Argument const *argument) const {
	if(argument->getType() == Argument::MENTION_ARG){
		const Mention* arg_mention = argument->getMention(_sent_theory->getMentionSet());
		if(_mention != 0){
			return contains(arg_mention);
		}
		else if(_value_mention != 0){
			const SynNode* arg_node = arg_mention->getNode();
			if( _value_mention->getStartToken() == arg_node->getStartToken() &&
				_value_mention->getEndToken() == arg_node->getEndToken()){
				return true;
			} 
			else if(argument->getRoleSym() == Argument::TEMP_ROLE &&
				 _value_mention->getStartToken() >= arg_node->getStartToken() &&
				 _value_mention->getEndToken() <= arg_node->getEndToken())
			{
				 return true;
			}
		}
	}
	return false;
}

void SlotFiller::initSlotFillerStatics() {
	if (!_initialized) {
		_initialized = true;
		_useCoref = !ParamReader::isParamTrue("no_coref");
	}
}

bool SlotFiller::useCoref() {
	SlotFiller::initSlotFillerStatics();
	return SlotFiller::_useCoref;
}

// this needs to be kept in sync with LearnIt doc generation in 
// LearnItDocConverter.cpp ~ RMG
// This also needs to be kept in sync with the canopy code for SlotsIdentityFeature
double SlotFiller::slotMatchScore(const SlotBase& slot, SlotConstraints_ptr slot_constraints,
                const std::set<Symbol>& overlapIgnoreWords) const
{
	// The the seed string is an exact match for the best name string, then we definitely match.
	//SessionLogger::info("IF") << slot.name();
	//SessionLogger::info("IF") << getBestName();
	if (slot_constraints->useBestName()) {
		if (getBestName() == slot.name()) {
			return 1.0;
		}
		
		if (ParamReader::isParamTrue("instance_finder_allow_best_name_overlap")) {
			if (getType() == SlotConstraints::MENTION && SlotFiller::useCoref() && matchSeedStringToEntity(slot.name())) {
				
				//Filter out overly common words to account for the potential of slight variations or trival overlap
				std::vector<std::wstring> slotToks;
				 boost::algorithm::split(slotToks, slot.name(), boost::algorithm::is_any_of(L" "));
				std::vector<std::wstring> slotToksFilt;
				for (std::vector<std::wstring>::iterator tok_iter = slotToks.begin(); tok_iter != slotToks.end(); ++tok_iter) {
					if (overlapIgnoreWords.find(Symbol(*tok_iter)) == overlapIgnoreWords.end()) {
						slotToksFilt.push_back(*tok_iter);
					}
				}
				
				std::vector<std::wstring> bestNameToks;
				boost::algorithm::split(bestNameToks, getBestName(), boost::algorithm::is_any_of(L" "));
				std::vector<std::wstring> bestNameToksFilt;
				for (std::vector<std::wstring>::iterator tok_iter = bestNameToks.begin(); tok_iter != bestNameToks.end(); ++tok_iter) {
					if (overlapIgnoreWords.find(Symbol(*tok_iter)) == overlapIgnoreWords.end()) {
						bestNameToksFilt.push_back(*tok_iter);
					}
				}
				
				//If they both have words left, compare overlap
				if (!slotToksFilt.empty() && !bestNameToksFilt.empty()) {
					std::wstring padded_seed_string = L" ";
					for (std::vector<std::wstring>::iterator tok_iter = slotToksFilt.begin(); tok_iter != slotToksFilt.end(); ++tok_iter) {
						padded_seed_string = padded_seed_string + *tok_iter + L" ";
					}
					
					std::wstring padded_best_name_string = L" ";
					for (std::vector<std::wstring>::iterator tok_iter = bestNameToksFilt.begin(); tok_iter != bestNameToksFilt.end(); ++tok_iter) {
						padded_best_name_string = padded_best_name_string + *tok_iter + L" ";
					}
					
					if (boost::algorithm::ifind_first(padded_best_name_string, padded_seed_string) ||
						boost::algorithm::ifind_first(padded_seed_string, padded_best_name_string))
					{
						return 1.0;
					}
				}
			}
		}
	} else {
		if(_sent_theory->getTokenSequence()->toString(getStartToken(), getEndToken()) == slot.name()) {
		    /*SessionLogger::info("IF") << "atomic head: " <<_sent_theory->getTokenSequence()->toString(getStartToken(), getEndToken()) << "\n";
			SessionLogger::info("IF") << "slot.name(): " << slot.name() << "\n";
			SessionLogger::info("IF") << "THEY MATCH!" << "\n";*/
			return 1.0;
		}
	}

	//If this option is set, then fail to match if best name didn't match
	if (slot_constraints->useBestName() && ParamReader::isParamTrue("only_name_seeds") && 
										 ParamReader::isParamTrue("only_match_best_name"))
	{
	return 0.0;
	}

	// If the slot filler contains a normalized date, then we use a special comparison
	// function that knows how to handle 'xxxx' (i.e., don't care/don't know) fields.
	// Note: if this does find a match, it may modify the best name confidence score.
	if (getType() == SlotConstraints::VALUE && slot_constraints->getSeedType() == L"yymmdd") {
		double date_match_score=YYMMDDMatchAgainst(slot);
		if (date_match_score>=GOOD_MATCH) {
			return date_match_score;		
		}
	}

	// If this slot filler is a mention and we have coref turned on, then check if the seed string matches any
	// of the mentions in its entity set.  If so, then apply a 0.8 penalty to the
	// best name confidence, and return a good match.
	if (getType() == SlotConstraints::MENTION && SlotFiller::useCoref() && matchSeedStringToEntity(slot.name())) {
		//setBestNameConfidence(getBestNameConfidence() * 0.8);
		//return true;
		return 0.8;
	}

	// Decide whether partial matches should be allowed.  Partial matches are 
	// currently only allowed for mention slots that are not ACE entities (e.g. games).
	// If a partial match is found, then we apply a penalty to the best name confidence
	// (based on how much extra material was added), and return a good match.
	bool partial_match_allowed = ((getType() == SlotConstraints::MENTION) && 
								  (slot_constraints->getBrandyConstraints().length() == 0)) ; 
	if (partial_match_allowed && _bestName.bestName().length() > slot.name().length()) {
		std::wstring padded_best_name_string = L" " + _bestName.bestName() + L" ";
		std::wstring padded_seed_string = L" " + slot.name() + L" ";
		if (boost::algorithm::ifind_first(padded_best_name_string, padded_seed_string)){
			return static_cast<double>(slot.name().length()) / _bestName.bestName().length();
			/*double penalty = seed->getSlot(slot_num).length() / _bestName.bestName().length();
			setBestNameConfidence(getBestNameConfidence() * penalty);
			return true;*/
		}
	}

	// Otherwise, the slot filler doesn't match the seed slot.
	return 0.0;
}



TargetToFillers SlotFiller::getAllSlotFillers(const AlignedDocSet_ptr doc_set, Symbol docid,
	int sentno, const std::vector<Target_ptr>& targets)
{
	TargetToFillers returnVal;
	if( SlotFiller::_curr_doc.is_null() || docid != SlotFiller::_curr_doc ){
		SlotFiller::fillCache(doc_set);
		SlotFiller::_curr_doc = docid;
	}
	BOOST_FOREACH(LanguageVariant_ptr languageVariant, doc_set->getLanguageVariants()) {
		//Check to see if this outdates the cache.
		for (std::size_t i = 0; i<targets.size(); i++) {
			Target_ptr target = targets[i];
			returnVal[target][languageVariant->toString()] = SlotFillersVector();
			SentenceTheory* stheory = doc_set->getDocTheory(languageVariant)->getSentenceTheory(sentno);
			getSlotFillers(doc_set, stheory, target, returnVal[target][languageVariant->toString()], languageVariant);
		}
	}
	return returnVal;
}

void SlotFiller::getSlotFillers(const AlignedDocSet_ptr doc_set,
			SentenceTheory* sent_theory, const Target_ptr target,
			SlotFillersVector & slotFillers, const LanguageVariant_ptr& languageVariant) 
{
	slotFillers.clear();
	//SessionLogger::info("LEARNIT") << L"SEED:\n";
	for( int slot_num = 0; slot_num < target->getNumSlots(); ++slot_num ){
		//SessionLogger::info("LEARNIT") << L"SLOT:\n";
		slotFillers.push_back(make_shared<SlotFillers>());
		Pattern_ptr slot_pattern = target->getSlotConstraints(slot_num)->getBrandyPattern();
		PatternMatcher_ptr matcher = PatternMatcher::makePatternMatcher(
				doc_set->getDocTheory(languageVariant), slot_pattern, false);
		
		std::vector<PatternFeatureSet_ptr> feature_sets = 
			matcher->getSentenceSnippets(sent_theory,
					0, true /* force use of multiMatchesSentence */ );
			//slot_pattern->multiMatchesSentence(doc_info, const_cast<SentenceTheory*>(sent_theory));
		SlotConstraints_ptr slot_constraints = target->getSlotConstraints(slot_num);
		for( size_t i=0; i < feature_sets.size(); ++i ){
			
			//SessionLogger::info("LEARNIT") << L"Found seed in " << languageVariant->getLanguageString() << L"\n";
			//SessionLogger::info("LEARNIT") << L"Feature language " << feature_sets[i]->getFeature(0)->getLanguageVariant()->getLanguageString() << L"\n";
			SlotFiller_ptr slot_filler = SlotFiller::fromPatternFeature(
				feature_sets[i]->getFeature(0), doc_set->getDocTheory(languageVariant), slot_constraints, languageVariant);
			//SessionLogger::info("LEARNIT") << slot_filler->name();
			slotFillers[slot_num]->push_back(slot_filler);	
		}
	}
}


std::vector<SlotFiller_ptr> SlotFiller::getSlotDistractors(const DocTheory *doc, SentenceTheory *sent_theory, 
                                                           Target_ptr target, Seed_ptr seed, int slot_num) 
{									   
	Pattern_ptr slot_pattern = 
		target->getSlotConstraints(slot_num)->getBrandyPattern();
	PatternMatcher_ptr matcher = 
		PatternMatcher::makePatternMatcher(doc, slot_pattern, false);
														
	//std::vector<PatternFeatureSet_ptr> feature_sets = slot_pattern->multiMatchesSentence(doc_info, sent_theory);
	std::vector<PatternFeatureSet_ptr> feature_sets = 
									matcher->getSentenceSnippets(sent_theory);
	SlotConstraints_ptr slot_constraints = target->getSlotConstraints(slot_num);
	std::vector<SlotFiller_ptr> slotDistractors;

	for (size_t i=0; i<feature_sets.size(); i++) {
		SlotFiller_ptr slot_filler = SlotFiller::fromPatternFeature(
                        feature_sets[i]->getFeature(0), doc, slot_constraints);

		bool good_distractor = true;
		for (int other_slot_num=0; other_slot_num < target->getNumSlots(); ++other_slot_num) {
			double match_score = slot_filler->slotMatchScore(
				*seed->getSlot(other_slot_num), 
				seed->getTarget()->getSlotConstraints(other_slot_num));
			if (match_score>SlotFiller::GOOD_MATCH) {
				good_distractor = false;
			}
		}
		if ((good_distractor) && slot_filler->satisfiesSlotConstraints(
									target->getSlotConstraints(slot_num))) 
		{
			slotDistractors.push_back(boost::make_shared<SlotFiller>(*slot_filler, 1.0));
		}
	}
	return slotDistractors;
}

bool SlotFiller::satisfiesSlotConstraints(SlotConstraints_ptr slot_constraints) const {
	if (_mention && ParamReader::isParamTrue("only_name_seeds") && !slot_constraints->allowDescTraining()) {
		if (!getMentionEntity() || !getMentionEntity()->hasNameMention(_doc->getEntitySet())) {
			return false;
		}
	}
	if (slot_constraints->getSeedType() == L"yymmdd") {
		static const boost::wregex date_re(L"\\A(xxxx|\\d\\d|\\d\\d\\d\\d)(?:-(xx|\\d\\d)(?:-(xx|\\d\\d))?)?\\z");
		if (!boost::regex_match(_bestName.bestName(), date_re))
			return false;
	}
	return true;
}
bool SlotFiller::isEquvialentSlot(const SlotFiller_ptr other_slot) const {
	if(_doc != other_slot->_doc)
		return false;
	if (_sent_theory != other_slot->_sent_theory)
		return false;
	if(getStartToken()== other_slot->getStartToken() && getEndToken() == other_slot->getEndToken())
		return true;
	if(other_slot->getMention() != NULL && _mention != NULL){
		if(other_slot->getMention()->getAtomicHead() == _mention->getAtomicHead()){
			return true;
		}
	}
	return false;
}

void SlotFiller::filterBySeed(
	const SlotFillers& fillerList, SlotFillers& output,
	const SlotBase& slot, SlotConstraints_ptr slotConstraints, 
	bool update_score, const std::set<Symbol>& overlapIgnoreWords)
{
	output.clear();
	BOOST_FOREACH(SlotFiller_ptr filler, fillerList) {
		double match_score = filler->slotMatchScore(slot, slotConstraints, overlapIgnoreWords);
		if (match_score > SlotFiller::GOOD_MATCH) {
			if (update_score) {
				output.push_back(boost::make_shared<SlotFiller>(*filler, match_score));
			} else {
				output.push_back(filler);
			}
		}
	}
}

bool SlotFiller::isGoodSlotMatch(
	SlotFiller_ptr filler, const SlotBase& slot, SlotConstraints_ptr slotConstraints)
{
	return filler->slotMatchScore(slot, slotConstraints)
			> SlotFiller::GOOD_MATCH;
}

bool SlotFiller::findAllCombinations(Target_ptr target,
	const SlotFillersVector& slotFillerPossibilities,
	std::vector<SlotFillerMap>& output) 
{
	// Find all combinations of the slot fillers.  Each combination is encoded as a slotnum->slotfiller
	// map.  Thus, the list of all combinations is a vector of SlotFillerMap.  To find
	// all combinations, we begin with an empty SlotFillerMap.  Then, for each slotnum, we
	// form all combinations of that slotnum with the combinations we've already found.
	//
	// If any slotnum has no fillers listed, then we assume that it is an optional slot
	// filler, and we just skip it.
	std::vector<SlotFillerMap> combos;
	combos.push_back(SlotFillerMap());
	for (size_t slotnum=0; slotnum < slotFillerPossibilities.size(); slotnum++) {
		if (slotFillerPossibilities[slotnum]->size() > 0) {
			std::vector<SlotFillerMap> newCombinations;
			BOOST_FOREACH(SlotFillerMap partialSlotFillerMap, combos) {
				for (std::vector<SlotFiller_ptr>::iterator sf_iter = slotFillerPossibilities[slotnum]->begin(); 
					sf_iter != slotFillerPossibilities[slotnum]->end(); ++sf_iter)
				{
					SlotFiller_ptr slotFiller = *sf_iter;
					SlotFillerMap slotFillers(partialSlotFillerMap); // Make a copy.
					slotFillers[static_cast<int>(slotnum)] = slotFiller;
					
					if (target->checkCoreferentArguments(slotFillers)) {
						newCombinations.push_back(slotFillers);
					}
				}
			}
			// Swap in the new value for slotFillerCombinations.
			combos.swap(newCombinations);
		}
	}

	output.insert(output.end(), combos.begin(), combos.end());
	return !combos.empty();
}

bool SlotFiller::unmetConstraints() const {
	return getType() == SlotConstraints::UNMET_CONSTRAINTS;
}

bool SlotFiller::unfilledOptional() const {
	return getType() == SlotConstraints::UNFILLED_OPTIONAL;
}

bool SlotFiller::filled() const {
	return getType() != SlotConstraints::UNFILLED_OPTIONAL 
		&& getType() !=SlotConstraints::UNMET_CONSTRAINTS;
}

bool SlotFiller::sameReferent(const SlotFiller& rhs) const {
	if (!filled() || !rhs.filled()) return false;
	const Entity* a_entity = getMentionEntity();
	const Entity* b_entity = rhs.getMentionEntity();

	if (!a_entity && !b_entity) {
		const ValueMention* a_vm = getValueMention();
		const ValueMention* b_vm = rhs.getValueMention();

		if (!a_vm && !b_vm) {
			const Mention* a_m = getMention();
			const Mention* b_m = rhs.getMention();

			return a_m == b_m;
		} else {
			return a_vm == b_vm;
		}
	} else {
		return a_entity == b_entity;
	}
}

bool SlotFiller::operator==(const SlotFiller& rhs) const {
	const Mention* lhs_m = getMention();
	const Mention* rhs_m = rhs.getMention();

	if (!lhs_m && !rhs_m) {
		const ValueMention* lhs_vm = getValueMention();
		const ValueMention* rhs_vm = getValueMention();

		return lhs_vm == rhs_vm;
	} else {
		return lhs_m == rhs_m;
	}
}

bool SlotFiller::operator<(const SlotFiller& rhs) const {
	const Mention* lhs_m = getMention();
	const Mention* rhs_m = rhs.getMention();

	if (lhs_m) {
		if (rhs_m) {
			return lhs_m->getUID() < rhs_m->getUID();
		} else {
			return true;
		}
	} else {
		if (rhs_m) {
			return false;
		} else {
			const ValueMention* lhs_m = getValueMention();
			const ValueMention* rhs_m = rhs.getValueMention();
			
			if (lhs_m) {
				if (rhs_m) {
					return lhs_m->getUID() < rhs_m->getUID();
				} else {
					return true;
				}
			} else {
				return false;
			}
		}
	}
}
