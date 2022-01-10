// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionConfidence.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/transliterate/Transliterator.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/edt/AbbrevMaker.h"
#include "Generic/edt/AcronymMaker.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"

#include <vector>
#include <string>
#include <math.h>
#include <boost/scoped_ptr.hpp>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

DebugStream &Entity::_debugOut = DebugStream::referenceResolverStream;

const char *Entity::getTypeString(EntityType type) {
	return type.getName().to_debug_string();
}	

Entity::Entity(int ID_, EntityType type_)
: _filters(), _is_generic(false), _GUID(NOT_ASSIGNED), 
	ID(ID_), type(type_), _canonical_name(0), _can_name_n_symbols(0), _hasName(UNSET), _hasDesc(UNSET)
{
	/*
	// +++ JJO 05 Aug 2011 +++
	// Word inclusion desc. linking
	_nStopWords = 315;
	Symbol stopWords[315] = {
			Symbol(L"a"), Symbol(L"about"), Symbol(L"above"), Symbol(L"across"), Symbol(L"after"), Symbol(L"afterwards"), Symbol(L"again"), Symbol(L"against"), Symbol(L"all"), Symbol(L"almost"), Symbol(L"alone"), Symbol(L"along"), Symbol(L"already"), Symbol(L"also"), Symbol(L"although"), Symbol(L"always"), Symbol(L"am"), Symbol(L"among"), Symbol(L"amongst"), Symbol(L"amoungst"), Symbol(L"amount"),  Symbol(L"an"), Symbol(L"and"), Symbol(L"another"), Symbol(L"any"), Symbol(L"anyhow"), Symbol(L"anyone"), Symbol(L"anything"), Symbol(L"anyway"), Symbol(L"anywhere"), Symbol(L"are"), Symbol(L"around"), Symbol(L"as"),  Symbol(L"at"), 
			Symbol(L"back"), Symbol(L"be"), Symbol(L"became"), Symbol(L"because"), Symbol(L"become"), Symbol(L"becomes"), Symbol(L"becoming"), Symbol(L"been"), Symbol(L"before"), Symbol(L"beforehand"), Symbol(L"behind"), Symbol(L"being"), Symbol(L"below"), Symbol(L"beside"), Symbol(L"besides"), Symbol(L"between"), Symbol(L"beyond"), Symbol(L"bill"), Symbol(L"both"), Symbol(L"bottom"), Symbol(L"but"), Symbol(L"by"), 
			Symbol(L"call"), Symbol(L"can"), Symbol(L"cannot"), Symbol(L"cant"), Symbol(L"co"), Symbol(L"con"), Symbol(L"could"), Symbol(L"couldn’t"), Symbol(L"cry"), 
			Symbol(L"de"), Symbol(L"describe"), Symbol(L"detail"), Symbol(L"do"), Symbol(L"done"), Symbol(L"down"), Symbol(L"due"), Symbol(L"during"), 
			Symbol(L"each"), Symbol(L"eg"), Symbol(L"eight"), Symbol(L"either"), Symbol(L"eleven"), Symbol(L"else"), Symbol(L"elsewhere"), Symbol(L"empty"), Symbol(L"enough"), Symbol(L"etc"), Symbol(L"even"), Symbol(L"ever"), Symbol(L"every"), Symbol(L"everyone"), Symbol(L"everything"), Symbol(L"everywhere"), Symbol(L"except"), 
			Symbol(L"few"), Symbol(L"fifteen"), Symbol(L"fifty"), Symbol(L"fill"), Symbol(L"find"), Symbol(L"fire"), Symbol(L"first"), Symbol(L"five"), Symbol(L"for"), Symbol(L"former"), Symbol(L"formerly"), Symbol(L"forty"), Symbol(L"found"), Symbol(L"four"), Symbol(L"from"), Symbol(L"front"), Symbol(L"full"), Symbol(L"further"), 
			Symbol(L"get"), Symbol(L"give"), Symbol(L"go"), 
			Symbol(L"had"), Symbol(L"has"), Symbol(L"hasn’t"), Symbol(L"have"), Symbol(L"he"), Symbol(L"hence"), Symbol(L"her"), Symbol(L"here"), Symbol(L"hereafter"), Symbol(L"hereby"), Symbol(L"herein"), Symbol(L"hereupon"), Symbol(L"hers"), Symbol(L"herself"), Symbol(L"him"), Symbol(L"himself"), Symbol(L"his"), Symbol(L"how"), Symbol(L"however"), Symbol(L"hundred"), 
			Symbol(L"ie"), Symbol(L"if"), Symbol(L"in"), Symbol(L"inc"), Symbol(L"indeed"), Symbol(L"interest"), Symbol(L"into"), Symbol(L"is"), Symbol(L"it"), Symbol(L"its"), Symbol(L"itself"), 
			Symbol(L"keep"), 
			Symbol(L"last"), Symbol(L"latter"), Symbol(L"latterly"), Symbol(L"least"), Symbol(L"less"), Symbol(L"ltd"), 
			Symbol(L"made"), Symbol(L"many"), Symbol(L"may"), Symbol(L"me"), Symbol(L"meanwhile"), Symbol(L"might"), Symbol(L"mill"), Symbol(L"mine"), Symbol(L"more"), Symbol(L"moreover"), Symbol(L"most"), Symbol(L"mostly"), Symbol(L"move"), Symbol(L"much"), Symbol(L"must"), Symbol(L"my"), Symbol(L"myself"), 
			Symbol(L"name"), Symbol(L"namely"), Symbol(L"neither"), Symbol(L"never"), Symbol(L"nevertheless"), Symbol(L"next"), Symbol(L"nine"), Symbol(L"no"), Symbol(L"nobody"), Symbol(L"none"), Symbol(L"nor"), Symbol(L"not"), Symbol(L"nothing"), Symbol(L"now"), Symbol(L"nowhere"), 
			Symbol(L"of"), Symbol(L"off"), Symbol(L"often"), Symbol(L"on"), Symbol(L"once"), Symbol(L"one"), Symbol(L"only"), Symbol(L"onto"), Symbol(L"or"), Symbol(L"other"), Symbol(L"others"), Symbol(L"otherwise"), Symbol(L"our"), Symbol(L"ours"), Symbol(L"ourselves"), Symbol(L"out"), Symbol(L"over"), Symbol(L"own"), 
			Symbol(L"part"), Symbol(L"per"), Symbol(L"perhaps"), Symbol(L"please"), Symbol(L"put"), 
			Symbol(L"rather"), Symbol(L"re"), 
			Symbol(L"same"), Symbol(L"see"), Symbol(L"seem"), Symbol(L"seemed"), Symbol(L"seeming"), Symbol(L"seems"), Symbol(L"serious"), Symbol(L"several"), Symbol(L"she"), Symbol(L"should"), Symbol(L"show"), Symbol(L"side"), Symbol(L"since"), Symbol(L"sincere"), Symbol(L"six"), Symbol(L"sixty"), Symbol(L"so"), Symbol(L"some"), Symbol(L"somehow"), Symbol(L"someone"), Symbol(L"something"), Symbol(L"sometime"), Symbol(L"sometimes"), Symbol(L"somewhere"), Symbol(L"still"), Symbol(L"such"), Symbol(L"system"), 
			Symbol(L"take"), Symbol(L"ten"), Symbol(L"than"), Symbol(L"that"), Symbol(L"the"), Symbol(L"their"), Symbol(L"them"), Symbol(L"themselves"), Symbol(L"then"), Symbol(L"thence"), Symbol(L"there"), Symbol(L"thereafter"), Symbol(L"thereby"), Symbol(L"therefore"), Symbol(L"therein"), Symbol(L"thereupon"), Symbol(L"these"), Symbol(L"they"), Symbol(L"thin"), Symbol(L"third"), Symbol(L"this"), Symbol(L"those"), Symbol(L"though"), Symbol(L"three"), Symbol(L"through"), Symbol(L"throughout"), Symbol(L"thru"), Symbol(L"thus"), Symbol(L"to"), Symbol(L"together"), Symbol(L"too"), Symbol(L"top"), Symbol(L"toward"), Symbol(L"towards"), Symbol(L"twelve"), Symbol(L"twenty"), Symbol(L"two"), 
			Symbol(L"un"), Symbol(L"under"), Symbol(L"until"), Symbol(L"up"), Symbol(L"upon"), Symbol(L"us"), 
			Symbol(L"very"), Symbol(L"via"), 
			Symbol(L"was"), Symbol(L"we"), Symbol(L"well"), Symbol(L"were"), Symbol(L"what"), Symbol(L"whatever"), Symbol(L"when"), Symbol(L"whence"), Symbol(L"whenever"), Symbol(L"where"), Symbol(L"whereafter"), Symbol(L"whereas"), Symbol(L"whereby"), Symbol(L"wherein"), Symbol(L"whereupon"), Symbol(L"wherever"), Symbol(L"whether"), Symbol(L"which"), Symbol(L"while"), Symbol(L"whither"), Symbol(L"who"), Symbol(L"whoever"), Symbol(L"whole"), Symbol(L"whom"), Symbol(L"whose"), Symbol(L"why"), Symbol(L"will"), Symbol(L"with"), Symbol(L"within"), Symbol(L"without"), Symbol(L"would"), Symbol(L"yet"), Symbol(L"you"), Symbol(L"your"), Symbol(L"yours"), Symbol(L"yourself"), Symbol(L"yourselves")
		};
	_stopWords = stopWords;
	// --- JJO 05 Aug 2011 ---
	*/
}

Entity::~Entity() {
	if (_canonical_name != 0)
		delete[] _canonical_name;
}

/*
// +++ JJO 05 Aug 2011 +++
// Word inclusion desc. linking
bool Entity::isStopWord(Symbol word) {
	for (int i = 0; i < _nStopWords; i++) {
		if (word == _stopWords[i])
			return true;
	}
	return false;
}

bool Entity::hasWord(Symbol word) {
	return (_wordSet.find(word) != -1);
}

void Entity::addWords(Symbol *words, int nWords) {
	Symbol word;
	for (int i = 0; i < nWords && i < 512; i++) {
		if (!hasWord(words[i]) && !isStopWord(words[i]))
			_wordSet.add(words[i]);
	}
}
// --- JJO 05 Aug 2011 ---
*/

void Entity::addMention(MentionUID uid) {
	mentions.add(uid);
	clearHasNameDescCache();
}

/** removes specified mention from entity -- does not currently
 *  warn if you remove the last mention of an entity! 
 */
void Entity::removeMention(MentionUID uid) {
	mentions.remove(uid);
	clearHasNameDescCache();
}

bool Entity::hasNameMention(const EntitySet* entitySet) const {
	if (_hasName == YES) return true;
	if (_hasName == NO) return false;
	for (int i=0; i<mentions.length(); ++i) {
		if (entitySet->getMention(mentions[i])->getMentionType() == Mention::NAME){
			return true;
		}
	}
	return false;
}

bool Entity::hasDescMention(const EntitySet* entitySet) const {
	if (_hasDesc == YES) return true;
	if (_hasDesc == NO) return false;
	for (int i=0; i<mentions.length(); ++i) {
		Mention::Type mtype = entitySet->getMention(mentions[i])->getMentionType();
		if (mtype == Mention::DESC || mtype == Mention::PART){
			return true;
		}
	}
	return false;
}

bool Entity::hasNameOrDescMention(const EntitySet* entitySet) const {
	if (_hasName == YES) return true;
	if (_hasDesc == YES) return true;
	if (_hasName == NO) return false;
	if (_hasDesc == NO) return false;
	for (int i=0; i<mentions.length(); ++i) {
		Mention::Type mtype = entitySet->getMention(mentions[i])->getMentionType();
		if (mtype == Mention::NAME || mtype == Mention::DESC || mtype == Mention::PART){
			return true;
		}
	}
	return false;
}

EntitySubtype Entity::guessEntitySubtype(const DocTheory *docTheory) const {
	return docTheory->getEntitySet()->guessEntitySubtype(this);
}

void Entity::dump(std::ostream &out, int indent, const EntitySet* entitySet) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Entity " << ID << " (" << type.getName().to_debug_string() << "): "
		<< "mentions: ";

	for (int i = 0; i < mentions.length(); i++) {
		out << mentions[i] << " ";
		if (entitySet) {
			std::string mention_string = entitySet->getMention(mentions[i])->getHead()->toDebugTextString();
			out << "(" << mention_string.substr(0, mention_string.size()-1) << ") ";
		}
	}
	delete[] newline;
}


void Entity::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void Entity::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"Entity", this);

	stateSaver->saveInteger(ID);
	stateSaver->saveSymbol(type.getName());

	stateSaver->saveInteger(mentions.length());
	stateSaver->beginList(L"Entity::mentions");
	for (int i = 0; i < mentions.length(); i++)
		stateSaver->saveInteger(mentions[i].toInt());
	stateSaver->endList();

	if (stateSaver->getVersion() >= std::make_pair(1,7)) {
		stateSaver->saveInteger(_is_generic ? 1 : 0);
	}

	stateSaver->endList();
}

Entity::Entity(StateLoader *stateLoader) : _filters(), _is_generic(false), _GUID(NOT_ASSIGNED), 
	_canonical_name(0), _can_name_n_symbols(0), _hasName(UNSET), _hasDesc(UNSET)
{
	int id = stateLoader->beginList(L"Entity");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	ID = stateLoader->loadInteger();
	type = EntityType(stateLoader->loadSymbol());

	int n_mentions = stateLoader->loadInteger();
	mentions.setLength(n_mentions);
	stateLoader->beginList(L"Entity::mentions");
	for (int i = 0; i < n_mentions; i++)
		mentions[i] = MentionUID(stateLoader->loadInteger());
	stateLoader->endList();

	if (stateLoader->getVersion() >= std::make_pair(1,7)) {
		if (stateLoader->loadInteger())
			_is_generic = true;
	}

	stateLoader->endList();
}

void Entity::resolvePointers(StateLoader * stateLoader) {
}

void Entity::getCanonicalName(int & n_symbols, Symbol * symbols) {
	for (int i = 0; i < _can_name_n_symbols; i++) {
		symbols[i] = _canonical_name[i];
	}
	n_symbols = _can_name_n_symbols;
}

std::wstring Entity::getCanonicalNameOneWord(bool add_spaces) const {
        std::wstring name=L"";
        for (int i = 0; i < _can_name_n_symbols; i++) {
                if (i>0 && add_spaces)
                        name += L" ";
                name += _canonical_name[i].to_string();
        }
        return name;
}

void Entity::setCanonicalName(int n_symbols, Symbol * symbols) {
	if (_canonical_name != 0)
		delete[] _canonical_name;
	_canonical_name = _new Symbol[n_symbols];
	for (int i = 0; i < n_symbols; i++) {
		_canonical_name[i] = symbols[i];
	}
	_can_name_n_symbols = n_symbols;
}

MentionConfidenceAttribute Entity::getMentionConfidence(MentionUID uid) const {
	std::map<MentionUID, MentionConfidenceAttribute>::const_iterator mention_confidence_i = _mention_confidences.find(uid);
	if (mention_confidence_i != _mention_confidences.end()) {
		return (*mention_confidence_i).second;
	} else {
		return MentionConfidenceStatus::UNKNOWN_CONFIDENCE;
	}
}

void
Entity::applyFilter(const std::string& filterName, EntityClutterFilter *filter)
{
	double score (0.);
	if (filter->filtered(this, &score)) {
		_filters[filterName] = score;
	}
	else {
		_filters.erase(filterName);
	}
}

bool
Entity::isFiltered(const std::string& filterName) const
{
	std::map<std::string, double>::const_iterator filter (_filters.find(filterName));
	return filter != _filters.end();
}

double
Entity::getFilterScore(const std::string& filterName) const
{
	double score (0.);
	std::map<std::string, double>::const_iterator filter (_filters.find(filterName));
	if (filter != _filters.end()) {
		score = filter->second;
	}
	return score;
}

std::wstring Entity::getBestName(const DocTheory* docTheory) const {
	return getBestNameWithSourceMention(docTheory).first;
}

std::pair<std::wstring, const Mention*> Entity::getBestNameWithSourceMention(const DocTheory *docTheory) const {
	// inefficient...
	// gets longest name
	// also, should deal with nationalities
	//if there is no name, get the first nominal mention
	std::wstring longestName = L"";
	const Mention *sourceMention = 0;
	for (int mentno = 0; mentno < getNMentions(); mentno++) {
		const Mention *ment = docTheory->getEntitySet()->getMention(getMention(mentno));
		TokenSequence* t = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();
		const SynNode* atomicHead = ment->getAtomicHead();
		if(ment->getMentionType() == Mention::NAME && atomicHead != 0){
			std::wstring temp = t->toString(atomicHead->getStartToken(), atomicHead->getEndToken());
			if(temp.length() > longestName.length()){
				longestName = temp;
				sourceMention = ment;
			}
		}
	}	
	if (longestName.length() > 0){
		return std::make_pair(longestName, sourceMention);
	}
	//get the first nominal mention
	int nsent = docTheory->getNSentences();
	while (nsent > 1 && 
		   docTheory->getSentenceTheory(nsent - 1)->getTokenSequence()->getNTokens() == 0)
	{
		nsent--;
	}
	TokenSequence* toks = docTheory->getSentenceTheory(nsent - 1)->getTokenSequence();
	if (toks->getNTokens() == 0)
		throw UnexpectedInputException("Entity::getBestNameWithSourceMention", "Empty TokenSequence");
	EDTOffset last_offset = toks->getToken(toks->getNTokens()-1)->getEndEDTOffset();
	int sent_num = nsent-1;
	const SynNode* firstNomNode = 0;
	EDTOffset mentOffset = last_offset;
	for (int mentno = 0; mentno < getNMentions(); mentno++) {
		const Mention *ment = docTheory->getEntitySet()->getMention(getMention(mentno));
		const SynNode* atomicHead = ment->getAtomicHead();
		// This looks buggy -- firstNomNode will never get set to anything, since we already
		// know there aren't any names.  Note: It looks like the entire method was copied from
		// DistillUtilities.  -JSM 6/8/11
		if(ment->getMentionType() == Mention::NAME && atomicHead != 0){
			TokenSequence* t = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();
			EDTOffset s = t->getToken(atomicHead->getStartToken())->getStartEDTOffset();
			if(s < mentOffset){
				mentOffset = s;
				firstNomNode = atomicHead;
				sent_num = ment->getSentenceNumber();
				sourceMention = ment;
			}
		// prefer mentions of type NAME over NEST
		} else if (ment->getMentionType() == Mention::NEST && atomicHead != 0 && !sourceMention){
			sent_num = ment->getSentenceNumber();
			firstNomNode = atomicHead;
			sourceMention = ment;
			// leave offset at last offset in case there is a mention of type NAME earlier in the doc
		}
	}
	toks = docTheory->getSentenceTheory(sent_num)->getTokenSequence();
	if(firstNomNode !=0){
		return std::make_pair(toks->toString(firstNomNode->getStartToken(), firstNomNode->getEndToken()), sourceMention);
	}
	else return std::make_pair(std::wstring(L"NO_NAME"), sourceMention);	
}

std::pair<std::wstring, const Mention*> Entity::getBestNameWithSourceMention(const EntitySet *entitySet) const {
	// inefficient...
	// gets longest name
	// also, should deal with nationalities
	std::wstring longestName = L"";
	const Mention *sourceMention = 0;
	for (int mentno = 0; mentno < getNMentions(); mentno++) {
		const Mention *ment = entitySet->getMention(getMention(mentno));
		if (ment->getMentionType() == Mention::NAME && ment->getAtomicHead() != 0){
			std::wstring temp = ment->toAtomicCasedTextString();
			if (temp.length() > longestName.length()) {
				longestName = temp;
				sourceMention = ment;
			}
		}
	}	
	if (longestName.length() > 0)
		return std::make_pair(longestName, sourceMention);
	else return std::make_pair(std::wstring(L"NO_NAME"), sourceMention);	
}

bool Entity::containsMention(const Mention* ment) const {
	for(int ent_no = 0; ent_no < getNMentions(); ent_no++){
		if(ment->getUID() == getMention(ent_no)){
			return true;
		}
	}
	return false;
}

const wchar_t* Entity::XMLIdentifierPrefix() const {
	return L"entity";
}

namespace {
	inline bool mention_uid_less_than(const Theory* lhs, const Theory* rhs) {
		return dynamic_cast<const Mention*>(lhs)->getUID() < dynamic_cast<const Mention*>(rhs)->getUID(); 
	}
}

void Entity::saveXML(SerifXML::XMLTheoryElement entityElem, const Theory *context) const {
	using namespace SerifXML;
	const EntitySet *entitySet = dynamic_cast<const EntitySet*>(context);
	if (context == 0)
		throw InternalInconsistencyException("Entity::saveXML", "Expected context to be an EntitySet");
	entityElem.setAttribute(X_is_generic, _is_generic);
	if (hasGUID())
		entityElem.setAttribute(X_entity_guid, _GUID);
	entityElem.setAttribute(X_entity_type, type.getName());
	
	std::wstring cname = getCanonicalNameOneWord(true);
	if (!cname.empty())
		entityElem.setAttribute(X_canonical_name, cname);

	EntitySubtype subtype = entitySet->guessEntitySubtype(this);
	if (subtype.getParentEntityType() != type ||
		subtype == EntitySubtype::getUndetType())
		subtype = EntitySubtype::getDefaultSubtype(type);
	entityElem.setAttribute(X_entity_subtype, subtype.getName());

	XMLIdMap *idMap = entityElem.getXMLSerializedDocTheory()->getIdMap();
	std::vector<const Theory*> mentionList;
	for (int i = 0; i < getNMentions(); i++) {
		MentionUID mentionId = getMention(i);
		const Mention *mention = entitySet->getMention(getMention(i));
		// In the current design, we can sometimes make copies of mentions, 
		// rather using pointers to them.  Therefore, it's possible that the
		// mention we just got isn't actually a mention that's been
		// serialized, so we won't have a pointer to it.  But there should
		// be an identical mention with the same MentionUID, so look that 
		// up.  [XX] Note: this would break if we ever used a beam with 
		// more than one MentionSet as a sentence subtheory.  But we 
		// currently never do.
		if (!idMap->hasId(mention)) {
			const Mention *mentionFromSentence = entityElem.getXMLSerializedDocTheory()->lookupMentionById(mention->getUID());
			assert(mention->isIdenticalTo(*mentionFromSentence));
			mention = mentionFromSentence;
		}
		mentionList.push_back(mention);
		if (entityElem.getOptions().include_mentions_as_comments)
			entityElem.addComment(mention->toCasedTextString());
	}
	std::sort(mentionList.begin(), mentionList.end(), mention_uid_less_than);
	entityElem.saveTheoryPointerList(X_mention_ids, mentionList);

	// Mention confidences as enum indices (optional)
	if (entityElem.getOptions().include_mention_confidences) {
		// Accumulate the mention confidences in the same sorted Mention order
		std::vector<std::wstring> confidenceStrings;
		BOOST_FOREACH(const Theory* mention, mentionList) {
			confidenceStrings.push_back(getMentionConfidence(static_cast<const Mention*>(mention)->getUID()).toString());
		}
		entityElem.setAttribute(X_mention_confidences, boost::algorithm::join(confidenceStrings, " "));
	}

	// Canonical name mention and transliteration (optional)
	if (entityElem.getOptions().include_canonical_names) {
		std::pair<std::wstring, const Mention*> bestName = getBestNameWithSourceMention(entitySet);
		const Mention *canonicalMention = bestName.second;
		if (canonicalMention != 0) {
			if (!idMap->hasId(canonicalMention)) {
				const Mention *mentionFromSentence = entityElem.getXMLSerializedDocTheory()->lookupMentionById(canonicalMention->getUID());
				assert(canonicalMention->isIdenticalTo(*mentionFromSentence));
				canonicalMention = mentionFromSentence;
			}
			entityElem.saveTheoryPointer(X_canonical_name_mention_id, canonicalMention); 
			if (entityElem.getOptions().include_name_transliterations) {
				boost::scoped_ptr<Transliterator> transliterator(Transliterator::build());
				entityElem.setAttribute(X_canonical_name_transliteration, transliterator->getTransliteration(canonicalMention));
			}
		}
	}
}

Entity::Entity(SerifXML::XMLTheoryElement entityElem, int entity_id)
: _filters(), _is_generic(false), _GUID(NOT_ASSIGNED), 
  _canonical_name(0), _can_name_n_symbols(0), _hasName(UNSET), _hasDesc(UNSET)
{
	using namespace SerifXML;
	entityElem.loadId(this);
	_is_generic = entityElem.getAttribute<bool>(X_is_generic);
	ID = entity_id;
	_GUID = entityElem.getAttribute<int>(X_entity_guid, NOT_ASSIGNED);
	type = EntityType(entityElem.getAttribute<Symbol>(X_entity_type));

	// Canonical name.  Note: std::vector is guaranteed by the standard to be
	// implemented as an array, so taking &cname_words[0] to get an array is safe, *UNLESS* cname_words.size() is zero.
	if (entityElem.hasAttribute(X_canonical_name)) {
		std::wstringstream cname(entityElem.getAttribute<std::wstring>(X_canonical_name));
		std::vector<Symbol> cname_words;
		do { 
			std::wstring word;
			cname >> word;
			cname_words.push_back(Symbol(word.c_str()));
		} while (cname);
		size_t num_words = cname_words.size();
		Symbol* word_array = (num_words==0)?0:(&cname_words[0]);
		setCanonicalName(static_cast<int>(num_words), word_array);
	}

	// Mentions
	std::vector<const Mention*> mentionList = entityElem.loadTheoryPointerList<Mention>(X_mention_ids);
	int n_mentions = static_cast<int>(mentionList.size());
	mentions.setLength(n_mentions);
	for (int i=0; i<n_mentions; ++i) {
		mentions[i] = mentionList[i]->getUID();
	}
	
	// Mention confidences
	if (entityElem.hasAttribute(X_mention_confidences)) {
		std::wstring confidenceAttribute = entityElem.getAttribute<std::wstring>(X_mention_confidences);
		std::vector<std::wstring> confidenceStrings;
		boost::algorithm::split(confidenceStrings, confidenceAttribute, boost::is_any_of(L" "));
		for (int m = 0; m < n_mentions; m++) {
			try {
				setMentionConfidence(mentions[m], MentionConfidenceAttribute::getFromString(confidenceStrings.at(m).c_str()));
			} catch (UnexpectedInputException &) {
				setMentionConfidence(mentions[m], MentionConfidenceStatus::UNKNOWN_CONFIDENCE);
			}
		}
	}

	// Note: we're not loading the canonical_name_mention_id and 
	// canonical_name_transliteration, since they both get generated 
	// on the fly.
}

void Entity::initializeHasNameDescCache(const EntitySet* entitySet){
	for (int i=0; i<mentions.length(); ++i) {
		if (entitySet->getMention(mentions[i])->getMentionType() == Mention::NAME){
			_hasName = YES;
			break;
		}
	}
	if (_hasName == UNSET) {
		_hasName = NO;
	}
	for (int i=0; i<mentions.length(); ++i) {
		if (entitySet->getMention(mentions[i])->getMentionType() == Mention::DESC || 
			entitySet->getMention(mentions[i])->getMentionType() == Mention::PART){
			_hasDesc = YES;
			break;
		}
	}
	if (_hasDesc == UNSET) {
		_hasDesc = NO;
	}
	return;
}

void Entity::clearHasNameDescCache(){
	_hasName = UNSET;
	_hasDesc = UNSET;
}

