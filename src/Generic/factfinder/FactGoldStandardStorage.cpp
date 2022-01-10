// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/factfinder/FactGoldStandardStorage.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/SynNode.h"
#include "Generic/values/TemporalNormalizer.h"
#include "boost/foreach.hpp"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <boost/scoped_ptr.hpp>

using namespace std;

Symbol FactGoldStandardStorage::PER = Symbol(L"PER");
Symbol FactGoldStandardStorage::ORG = Symbol(L"ORG");
Symbol FactGoldStandardStorage::GPE = Symbol(L"GPE");
Symbol FactGoldStandardStorage::DATE = Symbol(L"DATE");
Symbol FactGoldStandardStorage::TEXT = Symbol(L"TEXT");

FactGoldStandardStorage::FactGoldStandardStorage(const char *gold_standard_file)
{
	_n_fact_entries = 0;
	_n_entity_entries = 0;

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(gold_standard_file));
	UTF8InputStream& in(*in_scoped_ptr);
	wstring line;

	while (in.getLine(line)) {
		// grab the guid
		size_t tab_pos;
		tab_pos = line.find(L'\t', 0);
		wstring guid = line.substr(0, tab_pos);

		// BrandyID EntID list
		in.getLine(line);
		size_t start_pos = 0;
		size_t tab_pos2;
		_entityData[_n_entity_entries] = 0;
		EntityData *last;
		do {
			tab_pos = line.find(L'\t', start_pos);
			if (tab_pos == wstring::npos)
				throw UnexpectedInputException("FactGoldStandardStorage::FactGoldStandardStorage", "Expected tab (1)");
			wstring docid = line.substr(start_pos, tab_pos - start_pos);

			tab_pos2 = line.find(L'\t', tab_pos + 1);

			wstring entIdString;
			if (tab_pos2 == wstring::npos) 
				entIdString = line.substr(tab_pos + 1);
			else 
				entIdString = line.substr(tab_pos + 1, tab_pos2 - tab_pos - 1);

			EntityData *ed = _new EntityData();
			ed->docId = Symbol(docid);
			ed->entity_id = _wtoi(entIdString.c_str());
			ed->next = 0;

			if (_entityData[_n_entity_entries] == 0) 
				_entityData[_n_entity_entries] = ed;
			else 
				last->next = ed;
			last = ed;

			start_pos = tab_pos2 + 1;

			//SessionLogger::info("BRANDY") << L"Found entity data for entry " << _n_entity_entries << L"\n";
			//SessionLogger::info("BRANDY") << ed->docId.c_str();
			//SessionLogger::info("BRANDY") << L" " << ed->entity_id << L"\n";


		} while (tab_pos2 != wstring::npos);

		while (1) {
			// relation instance id
			in.getLine(line);
			if (line.length() == 0) break;

			FactData *fd = _new FactData();
			fd->slot2Data = 0;
			fd->slot2EntityData = 0;
			fd->answerData = 0;
			fd->count_in_documents = 0;
			fd->entityData = _entityData[_n_entity_entries];
			fd->slot1Guid = guid;

			_factData[_n_fact_entries++] = fd;
			fd->id = _wtoi(line.c_str());

			// relation type
			in.getLine(line);
			fd->relationType = line;

			// slot 2 info
			in.getLine(line);
			tab_pos = line.find(L'\t', 0);
			if (tab_pos == wstring::npos)
				throw UnexpectedInputException("FactGoldStandardStorage::FactGoldStandardStorage", "Expected tab (2)");
			wstring type = line.substr(0, tab_pos);
			Symbol type2(type.c_str());
			if (type2 != PER && type2 != ORG && type2 != GPE && type2 != DATE && type2 != TEXT) 
				throw UnexpectedInputException("FactGoldStandardStorage::FactGoldStandardStorage", "slot type must be PER, ORG, GPE, DATE, or TEXT");

			fd->slot2SlotType = type2;

			//SessionLogger::info("BRANDY") << "Found slot type: " << fd->slot2SlotType.to_debug_string() << "\n";

			start_pos = tab_pos + 1;
			StringData *last;

			do {
				// store text
				tab_pos = line.find(L'\t', start_pos + 1);

				wstring name;
				if (tab_pos == wstring::npos) 
					name = line.substr(start_pos);
				else
					name = line.substr(start_pos, tab_pos - start_pos);

				start_pos = tab_pos + 1;

				if (name.substr(0, 5).compare(L"guid.") == 0)
					continue;

				if (fd->slot2SlotType == PER || fd->slot2SlotType == ORG || 
					fd->slot2SlotType == GPE ||fd->slot2SlotType == TEXT)
				{
					name = convertToLower(name);
					for (size_t l = 0; l < name.length(); l++) {
						if (!iswalpha(name.at(l)) && name.at(l) != L' ') {
							name.replace(l, 1, L"");
							l--;
						}
					}
				}
				if (name.length() < 2) {
					continue;
				}
				StringData *sd = _new StringData();

				sd->string = name;
				sd->next = 0;
				if (fd->slot2Data == 0) {
					fd->slot2Data = sd;
					last = sd;
				} else {
					last->next = sd;
					last = sd;
				}

				//SessionLogger::info("BRANDY") << "Found factdata for entry " << fd->id << "\n";
				//SessionLogger::info("BRANDY") << Symbol(sd->string.c_str()).to_debug_string() << "(" << sd->string.length() << ")\n";
			} while (tab_pos != wstring::npos);
			
			// Read in list of doc/entity id pairs for this slot2
			in.getLine(line);
			start_pos = 0;
			EntityData *lastEd;
			while ((tab_pos = line.find(L'\t', start_pos)) != wstring::npos) {
				wstring docIdString = line.substr(start_pos, tab_pos - start_pos);
				tab_pos2 = line.find(L'\t', tab_pos + 1);

				wstring entIdString;
				if (tab_pos2 == wstring::npos) 
					entIdString = line.substr(tab_pos + 1);
				else 
					entIdString = line.substr(tab_pos + 1, tab_pos2 - tab_pos - 1);

				EntityData *ed = _new EntityData();
				string docid_s(docIdString.begin(), docIdString.end());
				docid_s.assign(docIdString.begin(), docIdString.end()); 
				ed->docId = Symbol(docIdString);
				ed->entity_id = _wtoi(entIdString.c_str());
				ed->next = 0;

				if (fd->slot2EntityData == 0) {
					fd->slot2EntityData = ed;
					lastEd = ed;
				} else {
					lastEd->next = ed;
					lastEd = ed;
				}
				//SessionLogger::info("BRANDY") << "Found: " << ed->docId.c_str() << " " << ed->entity_id << " for fd id: " << fd->id << "\n";
				if (tab_pos2 == wstring::npos) break;
				start_pos = tab_pos2 + 1;
			}
		}
		_n_entity_entries++;
	}
}

FactGoldStandardStorage::~FactGoldStandardStorage() 
{
	for (int i = 0; i < _n_fact_entries; i++) {
		StringData *temp = _factData[i]->slot2Data;
		while (temp != 0) {
			StringData *toBeDeleted = temp;
			temp = temp->next;
			delete toBeDeleted;
		}

		EntityData *temp2 = _factData[i]->slot2EntityData;
		while (temp2 != 0) {
			EntityData *toBeDeleted = temp2;
			temp2 = temp2->next;
			delete toBeDeleted;
		}

		delete _factData[i];
	}	

	for (int i = 0; i < _n_entity_entries; i++) {
		EntityData *temp = _entityData[i];
		while (temp != 0) {
			EntityData *toBeDeleted = temp;
			temp = temp->next;
			delete toBeDeleted;
		}
	}	
}

wstring FactGoldStandardStorage::convertToLower(wstring &str)
{
	boost::to_lower(str);
	return str;
}

void FactGoldStandardStorage::outputCounts(UTF8OutputStream &stream)
{
	for (int i = 0; i < _n_fact_entries; i++) {
		FactData *fd = _factData[i];
		if (fd->count_in_documents == 0)
			continue;
		stream << fd->id << L"\n";
		stream << fd->slot1Guid.c_str() << L"\n";
		stream << fd->relationType.to_string() << L"\n";
		Symbol slotType = fd->slot2SlotType;
		if (slotType == PER || slotType == GPE || slotType == ORG) 
			stream << L"NAME\n";
		else 
			stream << fd->slot2SlotType.to_string() << L"\n";
		stream << fd->count_in_documents << L"\n";
		StringData *ad = fd->answerData;
		while (ad != 0) {
			stream << ad->string.c_str() << L"\n";
			ad = ad->next;
		}
		stream << L"\n";
	}
}

void FactGoldStandardStorage::processDocuments(std::vector<DocTheory *>& docTheories) 
{
	for (int i = 0; i < _n_fact_entries; i++) {
		processFact(_factData[i], docTheories);
	}
}

void FactGoldStandardStorage::processFact(FactData* factData, std::vector<DocTheory *>& docTheories) {

	EntityData *ed = factData->entityData;

	while (ed != 0) {
		BOOST_FOREACH(DocTheory *dt, docTheories) {
			if (dt->getDocument()->getName() == ed->docId) {
				int entity_id = ed->entity_id;
				countFactInstances(factData, dt, ed->docId, entity_id);
			}
		}
		ed = ed->next;
	}
}

void FactGoldStandardStorage::countFactInstances(FactData *factData, DocTheory *docTheory, Symbol docId, int entity_id)
{
	const EntitySet *entitySet = docTheory->getEntitySet();
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		//SessionLogger::info("BRANDY") << "Working on sentence: " << i << "\n";
		const SentenceTheory *st = docTheory->getSentenceTheory(i);
		// Check mentions in sentence to see if there's a slot1 in them
		const MentionSet *ms = st->getMentionSet();
		for (int j = 0; j < ms->getNMentions(); j++) {
			Mention *m = ms->getMention(j);
			Entity *e = entitySet->getEntityByMention(m->getUID());

			if (e != 0 && e->getID() == entity_id) {
				// We've found the slot1 entity in the sentence, now check whether slot2 is also
				// in the sentence
				if (slot2InSentence(docTheory, factData, st, docId, entity_id)) {
					factData->count_in_documents = factData->count_in_documents + 1;
				}
				break;
			}
		}
	}
}

bool FactGoldStandardStorage::slot2InSentence(DocTheory *docTheory, FactData *fd, const SentenceTheory *st, Symbol docId, int fact_entity_id)
{	
	//SessionLogger::info("BRANDY") << "Checking slot2insentence\n";
	if (fd->slot2SlotType == PER || fd->slot2SlotType == ORG || fd->slot2SlotType == GPE) {
		// check all entities in sentence to see if there's one with a name that matches 
		// any of the slot2 possibilities
		const EntitySet *es = docTheory->getEntitySet();
		const MentionSet *ms = st->getMentionSet();

		// for every mention in the sentence
		for (int i = 0; i < ms->getNMentions(); i++) {
			const Mention *m = ms->getMention(i);
			Entity *e = es->getEntityByMention(m->getUID());
			if (e == 0 || e->getID() == fact_entity_id) continue;

			// walk over the mentions in this particular entity
			for (int j = 0; j < e->getNMentions(); j++) {
				MentionUID mention_uid = e->getMention(j);
				const Mention *m = es->getMention(mention_uid);
				if (m->getMentionType() != Mention::NAME) continue;
				wstring name = getName(m, es);

				//SessionLogger::info("BRANDY") << "Found name: " << name.c_str() << "\n";
				StringData *sd = fd->slot2Data;
				while (sd != 0) {
					//SessionLogger::info("BRANDY") << "comparing: " << Symbol(sd->string.c_str()).to_debug_string() << " to: " << Symbol(name.c_str()).to_debug_string() << "\n";
					if (sd->string.compare(name) == 0) {
						//SessionLogger::info("BRANDY") << "same!\n";
						storeNameData(fd, docTheory, fact_entity_id, e);
						return true;
					}
					sd = sd->next;
				}
			}
		}

		// for every mention in the sentence
		for (int i = 0; i < ms->getNMentions(); i++) {
			const Mention *m = ms->getMention(i);
			Entity *e = es->getEntityByMention(m->getUID());
			if (e == 0 || e->getID() == fact_entity_id) continue;

			// walk over the slot2 docid/entid list
			EntityData *slot2ed = fd->slot2EntityData;
			while (slot2ed != 0) {
				if (slot2ed->docId == docId) {
					//SessionLogger::info("BRANDY") << "Testing for slot2 by entity id\n";
					if (slot2ed->entity_id == e->getID()) {
						//SessionLogger::info("BRANDY") << "Found!\n";
						storeNameData(fd, docTheory, fact_entity_id, e);
						return true;
					}
				}
				slot2ed = slot2ed->next;
			}
		}
	}

	if (fd->slot2SlotType == DATE) {
		// check all date value mentions in sentence, and see if there's one that normalizes to 
		// any of the slot 2 possibilities
		const ValueMentionSet *vms = st->getValueMentionSet();
		const ValueSet *vs = docTheory->getValueSet();

		for (int i = 0; i < vms->getNValueMentions(); i++) {
			const ValueMention *vm = vms->getValueMention(i);

			if (!vm->isTimexValue()) continue;
			const Value *v = vs->getValueByValueMention(vm->getUID());
			Symbol timexVal = v->getTimexVal();
			if (timexVal != Symbol()) {
				wstring timexValString(timexVal.to_string());
				StringData *sd = fd->slot2Data;
				while (sd != 0) {
					if (sd->string.compare(timexValString) == 0) {
						storeDateData(fd, docTheory, fact_entity_id, vm);
						return true;
					}
					sd = sd->next;
				}
			}
		}
	}

	if (fd->slot2SlotType == TEXT) {
		// check sentence to see if any of the slot2 text strings appear in it
		wstring sentenceStr = wstring(L" ");
		const TokenSequence *ts = st->getTokenSequence();
		for (int i = 0; i < ts->getNTokens(); i++) {
			wstring word = wstring(ts->getToken(i)->getSymbol().to_string());
			word = convertToLower(word);
			for (size_t l = 0; l < word.length(); l++) {
				if (!iswalpha(word.at(l))) {
					word.replace(l, 1, L"");
					l--;
				}
			}
			if (word.length() > 1) {
				if (sentenceStr.length() > 0) 
					sentenceStr.append(L" ");
				sentenceStr.append(word);
			}
		}
		sentenceStr.append(L" ");

		StringData *sd = fd->slot2Data;
		while (sd != 0) {
			wstring stringToSearchFor = L" ";
			stringToSearchFor.append(sd->string);
			stringToSearchFor.append(L" ");
			if (sentenceStr.find(stringToSearchFor) != wstring::npos) {
				storeTextData(fd, docTheory, fact_entity_id, sd->string);
				return true;
			}
			sd = sd->next;
		}
	}
	return false;
}

void FactGoldStandardStorage::storeNameData(FactData *fd, DocTheory *docTheory, int fact_entity_id, const Entity *e) 
{
	// store doc/entity with FactData
	std::wstringstream answerDataString;	
	answerDataString << docTheory->getDocument()->getName().to_string() << L"\t";
	answerDataString << fact_entity_id << L"\t";
	answerDataString << e->getID();

	StringData *ad = _new StringData();

	ad->string = answerDataString.str();
	ad->next = 0;

	if (fd->answerData == 0) {
		fd->answerData = ad;
		return;
	}

	StringData *temp = fd->answerData;
	while (temp->next != 0) 
		temp = temp->next;
	temp->next = ad;
}

void FactGoldStandardStorage::storeDateData(FactData *fd, DocTheory *docTheory, int fact_entity_id, const ValueMention *vm) 
{	
	std::wstringstream answerDataString;	
	answerDataString << docTheory->getDocument()->getName().to_string() << L"\t";
	answerDataString << fact_entity_id << L"\t";
	answerDataString << vm->getUID().toInt();

	StringData *ad = _new StringData();

	ad->string = answerDataString.str();
	ad->next = 0;

	if (fd->answerData == 0) {
		fd->answerData = ad;
		return;
	}

	StringData *temp = fd->answerData;
	while (temp->next != 0) 
		temp = temp->next;
	temp->next = ad;
}

void FactGoldStandardStorage::storeTextData(FactData *fd, DocTheory *docTheory, int fact_entity_id, wstring &str) 
{

	std::wstringstream answerDataString;	
	answerDataString << docTheory->getDocument()->getName().to_string() << L"\t";
	answerDataString << fact_entity_id << L"\t";
	answerDataString << str;

	StringData *ad = _new StringData();
	ad->string = answerDataString.str();
	ad->next = 0;

	if (fd->answerData == 0) {
		fd->answerData = ad;
		return;
	}

	StringData *temp = fd->answerData;
	while (temp->next != 0) 
		temp = temp->next;
	temp->next = ad;
}

const wstring FactGoldStandardStorage::getName(const Mention *m, const EntitySet *es)
{
	const SynNode *headNode = m->getEDTHead();
	Symbol words[50];
	int num_words = headNode->getTerminalSymbols(words, 50);
	wstring name(L"");

	// put together lower case only-alphabetic name from head node words
	for (int k = 0; k < num_words; k++) {
		wstring word = wstring(words[k].to_string());
		word = convertToLower(word);
		for (size_t l = 0; l < word.length(); l++) {
			if (!iswalpha(word.at(l))) {
				word.replace(l, 1, L"");
				l--;
			}
		}
		if (word.length() > 1) {
			if (name.length() > 0) 
				name.append(L" ");
			name.append(word);
		}
	}
	return name;
}
