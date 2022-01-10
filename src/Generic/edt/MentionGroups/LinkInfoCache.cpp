// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupConfiguration.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SentenceTheory.h"

#include "Generic/common/version.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

LinkInfoCache::LinkInfoCache(MentionGroupConfiguration *config) : 
	_alternateSpellings(_new HashTable()), _docTheory(0),
	_extractors(config->buildMentionExtractors()),
	_pairExtractors(config->buildMentionPairExtractors())
{
	loadAlternateSpellings(ParamReader::getParam(std::string("linker_alt_spellings")));
	loadAlternateSpellings(ParamReader::getParam(std::string("linker_nations")));
}

LinkInfoCache::~LinkInfoCache() {
	clearMentionFeatureTable();
	clearMentionPairFeatureTable();
}

std::vector<AttributeValuePair_ptr> LinkInfoCache::getMentionFeaturesByName(const Mention *mention, Symbol extractorName, Symbol featureName) const {
	std::vector<AttributeValuePair_ptr> nullResult;
	MentionFeatureTable::const_iterator it1 = _mentionFeatureTable.find(mention);
	if (it1 != _mentionFeatureTable.end()) {
		Symbol name(AttributeValuePairBase::getFullName(extractorName, featureName));
		AttributeValuePairMap map = (*it1).second;
		AttributeValuePairMap::iterator it2 = map.find(name);
		if (it2 != map.end())
			return (*it2).second;
	}
	return nullResult;
}

std::vector<AttributeValuePair_ptr> LinkInfoCache::getMentionFeaturesByName(const MentionGroup& group, Symbol extractorName, Symbol featureName) const {
	std::vector<AttributeValuePair_ptr> results;
	for (MentionGroup::const_iterator it = group.begin(); it != group.end(); ++it) {
		std::vector<AttributeValuePair_ptr> tmp = getMentionFeaturesByName(*it, extractorName, featureName);
		results.insert(results.end(), tmp.begin(), tmp.end());
	}
	return results;
}

std::vector<AttributeValuePair_ptr> LinkInfoCache::getMentionPairFeaturesByName(const Mention *ment1, const Mention *ment2, Symbol extractorName, Symbol featureName) {
	std::vector<AttributeValuePair_ptr> nullResult;
	MentionPair pair = (ment1->getUID() < ment2->getUID()) ? MentionPair(ment1, ment2) : MentionPair(ment2, ment1);
	MentionPairFeatureTable::const_iterator it1 = _mentionPairFeatureTable.find(pair);
	AttributeValuePairMap map;

	// If it doesn't exist, create it
	if (it1 != _mentionPairFeatureTable.end()) {
		map = (*it1).second;
	} else {
		populateMentionPairFeatureTable(pair, map);	
	}
	
	Symbol name(AttributeValuePairBase::getFullName(extractorName, featureName));
	AttributeValuePairMap::iterator it2 =  map.find(name);
	if (it2 != map.end())
		return (*it2).second;
	
	return nullResult;
}

std::vector<const Mention*> LinkInfoCache::getMentionsByFeatureValue(Symbol extractorName, Symbol featureName, Symbol featureValue) const {
	std::vector<const Mention*> nullResult;
	Symbol fullName(AttributeValuePairBase::getFullName(extractorName, featureName));
	FeatureToMentionsTable::const_iterator it1 = _featureToMentionsTable.find(fullName);
	if (it1 != _featureToMentionsTable.end()) {
		SymbolToMentionsMap map = (*it1).second;
		SymbolToMentionsMap::iterator it2 = map.find(featureValue);
		if (it2 != map.end())
			return (*it2).second;
	}
	return nullResult;
}

/**
  * LinkInfoCache::setDocTheory() must be called prior to calling this method.
  */
void LinkInfoCache::populateMentionFeatureTable() {
	clearMentionFeatureTable();
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		MentionSet *mentionSet = _docTheory->getSentenceTheory(i)->getMentionSet();
		for (int m = 0; m < mentionSet->getNMentions(); m++) {
			Mention *ment = mentionSet->getMention(m);
			SessionLogger::dbg("LinkInfoCache_populate") << "Mention " << ment->getUID() << " [" << ment->toCasedTextString() << "]:";
			AttributeValuePairMap featureMap;
			BOOST_FOREACH(AttributeValuePairExtractor<Mention>::ptr_type extractor, _extractors) {
				std::vector<AttributeValuePair_ptr> features = extractor->extractFeatures(*ment, *this, _docTheory);
				BOOST_FOREACH(AttributeValuePair_ptr feature, features) {
					(featureMap[feature->getFullName()]).push_back(feature);
					SessionLogger::dbg("LinkInfoCache_populate") << "\t added " << feature->toString();
					// if feature has a Symbol value, add it to the _featureToMentionsTable
					boost::shared_ptr< AttributeValuePair<Symbol> > f = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(feature);
					if (f != 0) {
						((_featureToMentionsTable[f->getFullName()])[f->getValue()]).push_back(ment);
					}
				}
			}
			_mentionFeatureTable[ment] = featureMap;
		}
	}
}

void LinkInfoCache::populateMentionPairFeatureTable(MentionPair& pair, AttributeValuePairMap& featureMap) {
	BOOST_FOREACH(AttributeValuePairExtractor<MentionPair>::ptr_type extractor, _pairExtractors) {			
		std::vector<AttributeValuePair_ptr> features = extractor->extractFeatures(pair, *this, _docTheory);
		BOOST_FOREACH(AttributeValuePair_ptr feature, features) {
			(featureMap[Symbol(feature->getFullName())]).push_back(feature);
			SessionLogger::dbg("LinkInfoCache_populate") << "MentionPair " << pair.first->getUID() <<  " & " << pair.second->getUID() << ": added " << feature->toString();
		}
	}
	_mentionPairFeatureTable[pair] = featureMap;
}

void LinkInfoCache::loadAlternateSpellings(std::string filepath) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(filepath));
	UTF8InputStream& in(*in_scoped_ptr);
	// XXX put error checking here -- SRS
	std::wstring buf;
	std::wstring entry;
	while (!in.eof()) {
		in.getLine(buf);
		size_t begin = 0;
		size_t iter = buf.find(L';', 0);
		if (iter != std::string::npos) {
			entry = buf.substr(begin, iter - begin);
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			Symbol head(entry.c_str());
			iter += 2;
			begin = iter;
			while ((iter = buf.find(L';', iter)) != std::string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
				(*_alternateSpellings)[Symbol(entry.c_str())] = head;
				iter += 2;
				begin = iter;
			}
			entry = buf.substr(begin).c_str();
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			(*_alternateSpellings)[Symbol(entry.c_str())] = head;
		}
	}
}

Symbol LinkInfoCache::doHashTableLookup(HashTable *table, Symbol key) const {
	HashTable::iterator iter;
	iter = table->find(key);
	if (iter == table->end()) {
		return Symbol();
	}
	return (*iter).second;
}

void LinkInfoCache::setDocTheory(const DocTheory *docTheory) {
	_docTheory = docTheory; 
	clearMentionFeatureTable();
	clearMentionPairFeatureTable();
	clearFeatureToMentionsTable();

	BOOST_FOREACH(AttributeValuePairExtractor<Mention>::ptr_type extractor, _extractors) {
		extractor->resetForNewDocument(docTheory);
	}
}

void LinkInfoCache::clearMentionFeatureTable() {
	_mentionFeatureTable.clear();
}

void LinkInfoCache::clearMentionPairFeatureTable() {
	_mentionPairFeatureTable.clear();
}

void LinkInfoCache::clearFeatureToMentionsTable() {
	_featureToMentionsTable.clear();
}
