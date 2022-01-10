// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include <string> 
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include "Generic/common/leak_detection.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/Value.h"
#include "English/timex/TimePoint.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/TimeObj.h"
#include "English/timex/TimeAmount.h"
#include "English/timex/Year.h"
#include "English/timex/Month.h"
#include "English/timex/Day.h"
#include "English/timex/Hour.h"
#include "English/timex/Min.h"
#include "English/timex/TemporalString.h"
#include "English/timex/Strings.h"
#include "English/timex/en_TemporalParser.h"
#include "English/timex/en_TemporalNormalizer.h"
#include "English/common/en_WordConstants.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Zone.h"
#include "Generic/theories/Zoning.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Offset.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"

#include "Generic/theories/Parse.h"
#include "English/parse/en_STags.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/reader/DefaultDocumentReader.h"

using namespace std;

const wstring EnglishTemporalNormalizer::timeUnitCodes[EnglishTemporalNormalizer::num_tuc] = { L"Y", L"M", L"D", L"W", L"H", L"N", L"S", L"E", L"C", L"C", L"L", L"L" };
const wstring EnglishTemporalNormalizer::pureTimeUnits[EnglishTemporalNormalizer::num_ptu] = { L"year", L"month", L"day", L"week", L"hour", L"minute", L"second", 
L"decade", L"century", L"centuries", L"millennia", L"millennium"};
const wstring EnglishTemporalNormalizer::exTimeUnits[EnglishTemporalNormalizer::num_etu] = {  L"morning", L"afternoon", L"evening", L"night", L"spring",
L"summer", L"fall", L"autumn", L"winter", L"weekend"};

const wstring EnglishTemporalNormalizer::tenseStrings[EnglishTemporalNormalizer::num_tenses] = { L"unknown", L"past", L"present", L"future" };

const wstring EnglishTemporalNormalizer::_HALF_ = L"+half";
const wstring EnglishTemporalNormalizer::_QUARTER_ = L"+quarter";
const wstring EnglishTemporalNormalizer::_THREE_QUARTERS_ = L"+threequarters";

EnglishTemporalNormalizer::EnglishTemporalNormalizer() 
{
	_table = new SubstitutionTable(MAX_KEYS, hasher, eqTester);
	initializeSubstitutionTable();
}

EnglishTemporalNormalizer::~EnglishTemporalNormalizer() {
	delete _table;
}

/**
 * Looks at the last 6 characters of a string that that is >6 characters long.
 * If this substring looks like "+nnsxx" or "-nnsxx", where "nn" represents
 * a number between 01 and 24 (and "s" presumably is a time separator, though
 * this is not checked), return "+nn" or "-nn", respectively.
 **/
wstring EnglishTemporalNormalizer::getAdditionalDocTime(const wstring & originalDtString) {

	wstring dtString = boost::algorithm::trim_copy(originalDtString);
	size_t length = dtString.length();

	if (length > 6 &&
			(dtString.at(length - 6) == L'+' || dtString.at(length - 6) == L'-'))
	{
		int hours = Strings::parseInt(dtString.substr(length - 5, 2));
		if (hours > 0 && hours < 25)
			return dtString.substr(length - 6, 3);
	}

	return L"";
}

/** Not used within normalizeTimexValues, but useful for normalization of mentions *after* the 
 * main normalization pass.
 *
wstring EnglishTemporalNormalizer::normalizeStringToVal( wstring & timeString ){

	// JSG special case for explicit dates
	wstring expDate = parseExplicitDateTime( timeString );
	if( expDate.length() > 0 ){
			
		TimePoint * tp = new TimePoint( expDate );
		wstring additionalTimeOffset = getAdditionalDocTime( timeString );

		if( additionalTimeOffset.length() > 0 )
			tp->setAdditionalTime(additionalTimeOffset);
	
		wcerr << L"POST EXP timex string: \"" << timeString << L"\" ==> " << tp->toString() << endl;
		
		return tp->toString();
	}
		
	//cout << "Normalizing " << Symbol(timeString.c_str()) << "\n";
	TimeObj *to = normalize(timeString);
	//cout << Symbol(to->toString().c_str()).to_debug_string() << "\n\n";

	wstring result;
	
	if (to->isDuration()) {
		TimeAmount *ta = to->getDuration();
		result = L"P" + ta->toString();
	} else if (to->isGeneric()) {
		result = to->getGeneric();
	} else if (to->isSpecificDate() && !to->isEmpty()) {
		TimePoint *tp = to->getDateAndTime();
		result = tp->toString();
	}

	// JSG
	wcerr << L"timex string: \"" << timeString << L"\" ==> " << result << endl;

	return result;
}
*/

void EnglishTemporalNormalizer::normalizeDocumentDate(const DocTheory *docTheory, wstring &docDateTime, wstring &additionalDocTime)
{
	Document *doc = docTheory->getDocument();

	const char *no_date_warning =
		"The system could not identify a publication date for this document. "
		"A publication date is required to resolve relative date references "
		"(e.g. 'yesterday' or 'last January'). "
		"If this is not important to you (or you have no publication date), "
		"you can disable this warning by including 'no_document_date' in the "
		"list of warnings specified by the log_ignore parameter. "
		"If you would like to specify a document date, this is typically done "
		"via XML or SGML metadata (please see Input Specifications).";

	boost::optional<boost::posix_time::time_period> documentTimePeriod = doc->getDocumentTimePeriod();
	if (!documentTimePeriod) {
		SessionLogger::warn_user("no_document_date") << no_date_warning;
		docDateTime = L"XXXX-XX-XX";
		return;
	}
	
	docDateTime = doc->normalizeDocumentDate();

	if (docDateTime ==  L"XXXX-XX-XX") {
		SessionLogger::warn_user("no_document_date") << no_date_warning;
		return;
	}

	// Deal with time zone -- store hour offset as "+nn" or "-nn"
	boost::optional<boost::local_time::posix_time_zone> documentTimeZone = doc->getDocumentTimeZone();
	if (documentTimeZone) {
		std::wstringstream wss;
		wss << (documentTimeZone->base_utc_offset().hours() > 0 ? L"+" : L"-")
	        << std::setfill(L'0') << std::setw(2) 
	        << abs(documentTimeZone->base_utc_offset().hours());
		additionalDocTime = wss.str();
	}
}


void EnglishTemporalNormalizer::normalizeZoneDocumentDate(const wstring & originalDtString, wstring &docDateTime, wstring &additionalDocTime){
	
	std::vector<std::string> tokens;
	boost::algorithm::split(tokens, originalDtString, boost::algorithm::is_any_of("T")); 
	std::wstring date;
	date.assign(tokens[0].begin(), tokens[0].end());
	docDateTime=date;
	std::wstring time;
	std::string narrow_time=tokens[1].substr(0,2);
	time.assign(narrow_time.begin(),narrow_time.end());
	additionalDocTime=time;

}


void EnglishTemporalNormalizer::normalizeTimexValues(DocTheory* docTheory) 
{
	wstring docDateTime = L"";
	wstring additionalDocTime = L"";

	normalizeDocumentDate(docTheory, docDateTime, additionalDocTime);

	//cout << "Context = " << Symbol(_context->toString().c_str()).to_debug_string() << "\n";
	//wcout << "doc time: " << docDateTime << endl;
#ifdef BOOST
	temporal_timex2 timex2;
	if (docDateTime.length() != 0) {
		timex2.set_anchor(docDateTime);
	}
#endif

#if 0
	{	
		boost::scoped_ptr<UTF8InputStream> ifs_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream &ifs(*ifs_scoped_ptr);

		ifs.open("ace2007_timex2b.txt");
		if (ifs.is_open()) {
			wchar_t line[BUFSIZ];
			while (!ifs.eof()) {
				ifs.getline(line, BUFSIZ);

				wchar_t *p0 = wcsstr (line, L"\t");
				if (p0 == 0)
					continue;

				*p0 = 0;
				wstring expr0 = line;

				wchar_t *p1 = wcsstr (++p0, L"\t");
				*p1 = 0;
				wstring expr1 = p0;
				wstring expr2 = p1+1; // anchor date

				wcout << expr0 << L"\t";
				_context = new TimePoint(expr2);

				wstring expDate = parseExplicitDateTime(expr0);
				if( expDate.length() > 0 ){
			
					TimePoint * tp = new TimePoint( expDate );
					wstring additionalTimeOffset = getAdditionalDocTime( expr0 );
	
					if( additionalTimeOffset.length() > 0 )
						tp->setAdditionalTime(additionalTimeOffset);
				
					wcout << L"VAL=" << tp->toString() 
						<< L" //" << expr1 << L"\t" << expr2 << endl;
					continue;
				}

				TimeObj *to = normalize(expr0);

				if (to->isDuration()) {
					TimeAmount *ta = to->getDuration();
					wstring fullDuration = L"P" + ta->toString();
					//v->setTimexVal(Symbol(fullDuration.c_str()));
					wcout << L"VAL=" << fullDuration;
				} else if (to->isGeneric()) {
					//v->setTimexVal(Symbol(to->getGeneric().c_str()));
					wcout << L"VAL=" << to->getGeneric();
				} else if (to->isSpecificDate() && !to->isEmpty()) {
					TimePoint *tp = to->getDateAndTime();
					//v->setTimexVal(Symbol(tp->toString().c_str()));
					wcout << L"VAL=" << tp->toString();
				}
				if (to->getMod().length() > 0) {
					//v->setTimexMod(Symbol(to->getMod().c_str()));
					wcout << L" MOD=" << to->getMod();
				}
				if (to->isSet()) {
					//v->setTimexSet(Symbol(L"YES"));
					wcout << L" SET=YES";
				}

				// make anchor guesses
				if (to->isGeneric()) {
					wstring gen = to->getGeneric();
					if (!gen.compare(L"PAST_REF")) {
						//v->setTimexAnchorDir(Symbol(L"BEFORE"));
						//v->setTimexAnchorVal(Symbol(_context->toString().c_str()));
						wcout << L" ANCHOR_DIR=BEFORE ANCHOR_VAL=" << _context->toString();
					}
					if (!gen.compare(L"PRESENT_REF")) {
						//v->setTimexAnchorDir(Symbol(L"AS_OF"));
						//v->setTimexAnchorVal(Symbol(_context->toString().c_str()));
						wcout << L" ANCHOR_DIR=AS_OF ANCHOR_VAL=" << _context->toString();
					}
					if (!gen.compare(L"FUTURE_REF")) {
						//v->setTimexAnchorDir(Symbol(L"AFTER"));	
						wcout << L" ANCHOR_DIR=AFTER";
					}
				}
				wcout << L" //" << expr1 << L"\t" << expr2 << endl;
			}
		}
		ifs.close();
		exit(0);
	}
#endif
	
	
	ValueSet *vs = docTheory->getValueSet();
	for (int i = 0; i < vs->getNValues(); i++) {
		bool foundDateTimeInZone=false;
		Value *v = vs->getValue(i);
		if (!v->isTimexValue()) continue;
		if(docTheory->getDocument()->getZoning()){
			int start = v->getStartToken();
			int end = v->getEndToken();
			int sent_number = v->getSentenceNumber();
			SentenceTheory *st = docTheory->getSentenceTheory(sent_number);
			TokenSequence *ts = st->getTokenSequence();
			EDTOffset EDTstart=ts->getToken(start)->getStartEDTOffset();
			EDTOffset EDTend=ts->getToken(start)->getEndEDTOffset();

			boost::optional<Zone*> closetZone=docTheory->getDocument()->getZoning()->findZone(EDTstart);
			if(closetZone){
				LSPtr	datetime=closetZone.get()->getDatetime();
				if(datetime){
					foundDateTimeInZone=true;
					wstring zoneDocDateTime = L"";
					wstring zoneAdditionalDocTime = L"";
					normalizeZoneDocumentDate(datetime.get()->toWString(), zoneDocDateTime, zoneAdditionalDocTime);
					_context = new TimePoint(zoneDocDateTime);
					if (zoneAdditionalDocTime.length() > 0)
					_context->setAdditionalTime(zoneAdditionalDocTime);
				}
			}
		}
	
		if(!foundDateTimeInZone){
			_context = new TimePoint(docDateTime);
			
			if (additionalDocTime.length() > 0)
				_context->setAdditionalTime(additionalDocTime);
		}
		int start = v->getStartToken();
		int end = v->getEndToken();
		if (start == -1)
			start = 0;
		if (end == -1)
			end = 0;	

		int sent_number = v->getSentenceNumber();
		SentenceTheory *st = docTheory->getSentenceTheory(sent_number);
		TokenSequence *ts = st->getTokenSequence();
		wstring timeString;
		for (int i = start; i <= end; i++) {
			wstring token = wstring(ts->getToken(i)->getSymbol().to_string());
			timeString += token;
			if (i < end) timeString += L" ";			
		}

		_contextVerbTense = getContextVerbTense(st->getPrimaryParse(), start, end);

#ifdef BOOST
		if (normalize_temporal (timex2, timeString)) {
			v->setTimexVal(Symbol(timex2.VAL.c_str()));
			if (timex2.ANCHOR_VAL.length() > 0) {
				v->setTimexAnchorDir(Symbol(timex2.ANCHOR_DIR.c_str()));
				v->setTimexAnchorVal(Symbol(timex2.ANCHOR_VAL.c_str()));
			}
			if (timex2.SET.length() > 0) {
				v->setTimexSet(Symbol(timex2.SET.c_str()));
			}
			if (timex2.MOD.length() > 0) {
				v->setTimexMod(Symbol(timex2.MOD.c_str()));
			}

			//continue;
		}
#endif
		// JSG special case for explicit dates
		wstring expDate = parseExplicitDateTime( timeString );
#ifdef BOOST
		if (timex2.VAL.length() > 0) {
			// do nothing here
		}
		else
#endif
		if( expDate.length() > 0 ){
			
			TimePoint * tp = new TimePoint( expDate );
			wstring additionalTimeOffset = getAdditionalDocTime( timeString );

			//Don't do anything else if the time string is already YYYY-MM-DD
			if( expDate.compare(timeString) && additionalTimeOffset.length() > 0 )
				tp->setAdditionalTime(additionalTimeOffset);
				
			v->setTimexVal(Symbol((tp->toString()).c_str()));

			// JSG
			//wcerr << L"EXP timex string: \"" << timeString << L"\" ==> " << v->getTimexVal().to_string() << endl;

			delete tp;
			delete _context;
			continue;
		}
		
		//cout << "Normalizing " << Symbol(timeString.c_str()) << "\n";
		TimeObj *to = normalize(timeString);
		//cout << Symbol(to->toString().c_str()).to_debug_string() << "\n\n";

#ifdef BOOST
		if (timex2.VAL.length() > 0) {
		}
		else
#endif
		if (to->isDuration()) {
			TimeAmount *ta = to->getDuration();
			wstring fullDuration = L"P" + ta->toString();
			v->setTimexVal(Symbol(fullDuration.c_str()));
		} else if (to->isGeneric()) {
			v->setTimexVal(Symbol(to->getGeneric().c_str()));
		} else if (to->isSpecificDate() && !to->isEmpty()) {
			TimePoint *tp = to->getDateAndTime();
			v->setTimexVal(Symbol(tp->toString().c_str()));
		}
		if (to->getMod().length() > 0)
			v->setTimexMod(Symbol(to->getMod().c_str()));
		if (to->isSet()) 
			v->setTimexSet(Symbol(L"YES"));

		// make anchor guesses
		if (!to->getAnchor()->isEmpty()) 
			v->setTimexAnchorVal(Symbol(to->getAnchor()->toString().c_str()));
		if (to->getAnchorDir().length() > 0)
			v->setTimexAnchorDir(Symbol(to->getAnchorDir()));

		delete to;
		delete _context;
		// JSG
		//wcerr << L"timex string: \"" << timeString << L"\" ==> " << ( (!v->getTimexVal().is_null()) ? v->getTimexVal().to_string() : L"(no val)") << endl;
	}			
}


wstring EnglishTemporalNormalizer::parseExplicitDateTime(const wstring & instr ){

	// remove leading and trailing whitespace
	wstring str = boost::algorithm::trim_copy(instr);

	if (str.length() == 0) 
		return L"";
	
	// 20030808_13:28:00
	if (str.length() == 17 && str[8] == '_' ){
		wstring year  = str.substr(0,4);
		wstring month = str.substr(4,2);
		wstring day   = str.substr(6,2);
		
		wstring hour  = str.substr(9,2);
		wstring min   = str.substr(12,2);
		wstring sec   = str.substr(15,2);

		if( allDigits(year) && allDigits(month) && allDigits(day) &&
			allDigits(hour) && allDigits(min) && allDigits(sec) )
		{
			wstring results = 
				year + L"-" + month + L"-" + day + L"T" +
				hour + L":" + min + L":" + sec;
	
			return results;
		}

	}

	// 20030304
	if (str.length() == 8 && allDigits(str)) {
		wstring year = str.substr(0, 4);
		wstring month = str.substr(4, 2);
		wstring day = str.substr(6, 2);

		wstring results = year + L"-" + month + L"-" + day;
		return results;
	}

	// 05/14/2004 11:23:01.34
	if (str.length() == 22 || str.length() == 21){
		wstring month = str.substr(0,2);
		wstring day   = str.substr(3,2);
		wstring year  = str.substr(6,4);

		wstring hour,min,sec;
		
		// long form
		if( allDigits(str.substr(12,1)) ){
			hour = str.substr(11,2);
			min  = str.substr(14,2);
			sec  = str.substr(17,2);
		}
		else{ // short form
			hour = str.substr(11,1);
			min  = str.substr(13,2);
			sec  = str.substr(16,2);
		}

		if( allDigits(year) && allDigits(month) && allDigits(day) &&
			allDigits(hour) && allDigits(min) && allDigits(sec) )
		{
			wstring results = 
				year + L"-" + month + L"-" + day + L"T" +
				hour + L":" + min + L":" + sec;
	
			//wcerr << "NEW FORM " << str << " to " << results << endl;
			
			return results;
		}

		//wcerr << "FAILED FORM " << str << " : " << month << day << year << endl;
	}


	// 2003-03-06 or (2004-11-01T11:44:00 or 2003-03-03T19:00:00-05:00 or 2003-03-06 08:36:54)
	if (str.length() == 10 || str.length() == 19 || str.length() == 25) {
		wstring year = str.substr(0, 4);
		wstring month = str.substr(5, 2);
		wstring day = str.substr(8, 2);
		if (str.length() == 10) {
			if (allDigits(year) && allDigits(month) && allDigits(day)&&
				str.at(4) == L'-' && str.at(7) == L'-') {
				return str;
			}
			else {
				return L"";
			}
		} 

		wstring hour = str.substr(11, 2);
		wstring min = str.substr(14, 2);
		wstring sec = str.substr(17, 2);

		if (allDigits(year) && allDigits(month) &&
			allDigits(day) && allDigits(hour) &&
			allDigits(min) && allDigits(sec) &&
			(str.at(10) == L'T' || str.at(10) == L' ') &&
			str.at(4) == L'-' && str.at(7) == L'-' &&
			str.at(13) == L':' && str.at(16) == L':')
		{
			//cout << "first substring " << Symbol(str.substr(0, 10).c_str()).to_debug_string() << "\n";
			//cout << "second substring " << Symbol(str.substr(11).c_str()).to_debug_string() << "\n";
			str = str.substr(0, 10) + L"T" + str.substr(11);
			return str.substr(0, 19);
		}
	}

	// 20041222-15:23:17
	if (str.length() == 17) {
		wstring year = str.substr(0, 4);
		wstring month = str.substr(4, 2);
		wstring day = str.substr(6,2);
		wstring hour = str.substr(9, 2);
		wstring min = str.substr(12, 2);
		wstring sec = str.substr(15, 2);

		if (allDigits(year) && allDigits(month) &&
			allDigits(day) && allDigits(hour) &&
			allDigits(min) && allDigits(sec) &&
			str.at(8) == L'-' && str.at(11) == L':' &&
			str.at(14) == L':')
		{
			wstring results = 
				year + L"-" + month + L"-" + day + L"T" +
				hour + L":" + min + L":" + sec;
			return results;
		}
	}

	// Tue, 10 Oct 2000 23:46:00 -0700 (PDT)
	if (str.length() == 37) {
		wstring dow = str.substr(0, 3);
		wstring day = str.substr(5, 2);
		wstring month = str.substr(8, 3);
		wstring year = str.substr(12, 4);
		wstring hour = str.substr(17, 2);
		wstring min = str.substr(20, 2);
		wstring sec = str.substr(23, 2);

		boost::to_lower(dow);
		boost::to_lower(month);

		if (allDigits(year) && 
			allDigits(day) && allDigits(hour) &&
			allDigits(min) && allDigits(sec) &&
			Month::isMonth(month) && Day::isDayOfWeek(dow) &&
			str.at(3) == L',' && str.at(19) == L':' &&
			str.at(22) == L':')
		{
			Month monthObj(Month::month2Num(month));

			wstring results = 
				year + L"-" + monthObj.toString() + L"-" + day + L"T" +
				hour + L":" + min + L":" + sec;
			return results;
		}
	}

	// Fri, 2 Nov 2001 08:36:59 -0800 (PST)
	if (str.length() == 36) {
		wstring dow = str.substr(0, 3);
		wstring day = str.substr(5, 1);
		wstring month = str.substr(7, 3);
		wstring year = str.substr(11, 4);
		wstring hour = str.substr(16, 2);
		wstring min = str.substr(19, 2);
		wstring sec = str.substr(22, 2);

		boost::to_lower(dow);
		boost::to_lower(month);

		if (allDigits(year) && 
			allDigits(day) && allDigits(hour) &&
			allDigits(min) && allDigits(sec) &&
			Month::isMonth(month) && Day::isDayOfWeek(dow) &&
			str.at(3) == L',' && str.at(18) == L':' &&
			str.at(21) == L':')
		{
			Month monthObj(Month::month2Num(month));
			Day dayObj(day);

			wstring results = 
				year + L"-" + monthObj.toString() + L"-" + 
				dayObj.toString() + L"T" +
				hour + L":" + min + L":" + sec;
			return results;
		}
	}


	// unrecognized format
	return L"";
}

/* Return the most likely verb tense for the context around the 
 * timex expression beginning at start_tok and ending at end_tok.
 */
EnglishTemporalNormalizer::Tense EnglishTemporalNormalizer::getContextVerbTense(Parse *parse, int start_tok, int end_tok) {
	
	if (parse != 0) {

		const SynNode * timexNode = parse->getRoot()->getNodeByTokenSpan(start_tok, end_tok);
		wstring tag_chain = L"";
		wstring word_chain = L"";

		// find the first VP, S or SBAR containing timex expression
		const SynNode * vpNode = timexNode;
		while (vpNode != 0 && vpNode->getTag() != EnglishSTags::VP && vpNode->getTag() != EnglishSTags::S && vpNode->getTag() != EnglishSTags::SBAR) {
			vpNode = vpNode->getParent();
		}
		addHeadWordAndTagToChain(vpNode, tag_chain, word_chain);

		// while there are nested VPs, continue up tree
		while (vpNode != 0 && vpNode->getParent() != 0 && vpNode->getParent()->getTag() == EnglishSTags::VP) {		
			vpNode = vpNode->getParent();
			addHeadWordAndTagToChain(vpNode, tag_chain, word_chain);
		}

		if (vpNode != 0) {
			//wcout << L"Chain before call: " << tag_chain << L" " << word_chain << L"\n";
			if (LanguageSpecificFunctions::isPastTenseVerbChain(tag_chain, word_chain)) {
				//wcout << L"Past Tense\n";
				return PAST_TENSE;
			}
			else if (LanguageSpecificFunctions::isPresentTenseVerbChain(tag_chain, word_chain)) {
				//wcout << L"Present Tense\n";
				return PRESENT_TENSE;
			}
			else if (LanguageSpecificFunctions::isFutureTenseVerbChain(tag_chain, word_chain)) {
				//wcout << L"Future Tense\n";
				return FUTURE_TENSE;
			}
		}
	}	
	return UNK_TENSE;
}

/* Find the preterminal head node of 'node' and append its tag and head
 * word to tag_chain and word_chain respectively.
 */
void EnglishTemporalNormalizer::addHeadWordAndTagToChain(const SynNode *node, wstring& tag_chain, wstring& word_chain) {
	while (node != 0 && !node->isPreterminal()) {
		node = node->getHead();
	}
	if (node != 0) {
		if (tag_chain.length() != 0) 
			tag_chain = L":" + tag_chain;
		if (word_chain.length() != 0) 
			word_chain = L":" + word_chain;
		tag_chain = node->getTag().to_string() + tag_chain;
		word_chain = node->getHeadWord().to_string() + word_chain;
	}
}

bool EnglishTemporalNormalizer::allDigits(const wstring &str) {
	for (int i = 0; i < (int)str.length(); i++) {
		if (!iswdigit(str.at(i)))
			return false;
	}
	return true;
}

/* returns true if str is contained in EnglishTemporalNormalizer.pureTimeUnits */
bool EnglishTemporalNormalizer::isPureTimeUnit(const wstring &str) {
	for (int i = 0; i < num_ptu; i++) 
		if (!str.compare(pureTimeUnits[i]) || !str.compare(pureTimeUnits[i] + L"s")) return true;
	return false;
}

/* returns true if str is contained in EnglishTemporalNormalizer.pureTimeUnits or 
EnglishTemporalNormalizer.exTimeUnits or Month.monthNames or Day.daysOfWeek  */
bool EnglishTemporalNormalizer::isExTimeUnit(const wstring &str) {
	for (int i = 0; i < num_etu; i++)
		if (!str.compare(exTimeUnits[i]) || !str.compare(exTimeUnits[i] + L"s")) return true;
	return (isPureTimeUnit(str) || Month::isMonth(str) || Day::isDayOfWeek(str));
}

// Runs unit tests
void EnglishTemporalNormalizer::test() {
	EnglishTemporalNormalizer etn;
	assert(TimexUtils::extractDateFromDocId(L"ar-nw-en-hierdec_v3-XIA20070929.0122") == L"20070929");
	assert(TimexUtils::extractDateFromDocId(L"en-bc-en-stt-EUA20091020_MSNBC_HARDBALL_ENG_20091020_165802.22") == L"20091020");
}

/* returns the code used to access the given time unit (str) in a TimeObj */ 
wstring EnglishTemporalNormalizer::timeUnit2Code(const wstring &str) {
	for (int i = 0; i < num_ptu; i++)
		if (Strings::startsWith(str, pureTimeUnits[i])) 
			return timeUnitCodes[i];
	return L"";
}

TimeObj* EnglishTemporalNormalizer::normalize(wstring str) {
	std::wstring original_str = str;

	// special case to remember 'past' and 'last', etc, as they'll be normalized out
	// 	we use them *only* if we can't glean other information out
	bool has_weak_past_ref = ( str.find( L"last" ) != std::wstring::npos ||
							   str.find( L"past" ) != std::wstring::npos ||
							   str.find( L"ago"  ) != std::wstring::npos ||
							   str.find( L"ex"   ) != std::wstring::npos );
	
	str = canonicalizeString(str); // "September 10th" -> "sep 10"
	TemporalString *ts = new TemporalString(str);

	dealWithFractions(ts);
	combineAdjacentNumbers(ts);
	//cout << "Temporal string: " << Symbol(ts->toString().c_str()).to_debug_string() << "\n";

	TimeObj *t = highProbabilityMatch(ts);

	// no match found, check for generic past, present or future refs
	if (t->isEmpty()) {
	
		// special case part 2 - if it contained past/last, set to PAST_REF but allow matchGenericRef to over-ride
		if( has_weak_past_ref ) {
			t->setGeneric(wstring(L"PAST_REF"));
			t->setAnchorDir(wstring(L"BEFORE"));
		}

		matchGenericRefs(ts->toString(), t);
		t->addAnchorContext(_context);
		t->normalizeAnchor();
	} 
	// match was found, fill in context and modifiers 
	else {
		matchModifiers(ts->toString(), t);

		// Handle September 11th.
		// The "September 11th" form gets handled in highProbabilityMatch().  We handle "9/11" here.
		if (original_str.find(L"9/11") != std::wstring::npos) {
			if ((t->getDateAndTime()->get(L"YYYY")->isEmpty()) &&
				(t->getDateAndTime()->get(L"MM")->value() == 9) &&
				(t->getDateAndTime()->get(L"DD")->value() == 11) &&
				(_context->get(L"YYYY")->value() >= 2001)) {
			t->getDateAndTime()->set(L"YYYY", L"2001");
			}
		}
		t->addContext(_context);
		t->normalizeTime();
	}
	delete ts;
	return t;
}

wstring EnglishTemporalNormalizer::canonicalizeString(wstring str) {
	int i;
	str = L" " + str + L" ";
	boost::to_lower(str);

	for (i = 0; i < _num_keys; i++) {

		Symbol key = _keys[i];
		Symbol value = (*_table)[key];

		int index = 0;
		while (str.find(key.to_string(), index) != std::wstring::npos) {
			int old_index = (int)str.find(key.to_string(), index);
			index = old_index + (int)wstring(value.to_string()).length() - 1;
			str = str.substr(0, old_index) + 
				value.to_string() + 
				str.substr(old_index + wstring(key.to_string()).length());
		}
	}

	/* remove intermediate commas in numbers */
	size_t comma = 0;
	while ((comma = str.find(L",", comma)) != std::wstring::npos) {
		if (comma >= 1 && (comma + 3) < str.length()) 
			if (iswdigit(str.at(comma - 1)) &&
				iswdigit(str.at(comma + 1)) &&
				iswdigit(str.at(comma + 2)) &&
				iswdigit(str.at(comma + 3)))
				str = str.substr(0, comma) + str.substr(comma + 1);
		comma++;
	}

	/* replace all remaining commas with spaces */
	comma = 0;
	while ((comma = str.find(L",", comma)) != std::wstring::npos) {
		str = str.substr(0, comma) + L" " + str.substr(comma + 1);
		comma++;
	}

	/* Ordinals */
	wstring ords[4] = { L"st ", L"nd ", L"rd ", L"th " };
	for (i = 0; i < 4; i++) {
		size_t  ind = 0;
		while ((ind = str.find(ords[i], ind)) != std::wstring::npos) {
			size_t j = 1;
			while (ind >= j  && !(isalpha(str.at(ind-j)) || iswdigit(str.at(ind-j)))) j++;

			if ((ind >= j)  && iswdigit(str.at(ind-j))) 
				str = str.substr(0, ind) + L" " + str.substr(ind + ords[i].length());
			ind++;
		}
	}

	return str;
}


TimeObj* EnglishTemporalNormalizer::highProbabilityMatch(TemporalString *ts) { 
	TimeObj *temp;
	TimeObj *result = new TimeObj();

	int i, t1, t2, t3;
	float f1;

	while (true) {

		//wcout << L"currently, " << ts->toString() << L": " << Symbol(result->toString().c_str()).to_string() << L"\n";
		//wcout << L"with verb tense: " << tenseStrings[_contextVerbTense] << L"\n";

		//  x century (where x > 1) - assumed to mean xth century
		if ((i = ts->containsWord(L"century")) > -1 && ((t1 = Strings::parseInt(ts->tokenAt(i-1))) > 1)) {    
			result->setTime(L"YYYY", Strings::valueOf(t1-1) + L"XX");
			ts->remove(i-1, i);

		// Today, Tomorrow, Yesterday, Tonight
		} else if ((i = ts->containsWord(L"today")) > -1) {
			result->setTime(L"YYYY", _context->get(L"YYYY")->clone());
			result->setTime(L"MM", _context->get(L"MM")->clone());
			result->setTime(L"DD", _context->get(L"DD")->clone());
			ts->remove(i); 
		} else if ((i = ts->containsWord(L"yesterday")) > -1) {
			result->setTime(L"YYYY", _context->get(L"YYYY")->clone());
			result->setTime(L"MM", _context->get(L"MM")->clone());
			result->setTime(L"DD", (_context->get(L"DD"))->sub(1));
			ts->remove(i);
		} else if ((i = ts->containsWord(L"tomorrow")) > -1) {
			result->setTime(L"YYYY", _context->get(L"YYYY")->clone());
			result->setTime(L"MM", _context->get(L"MM")->clone());
			result->setTime(L"DD", (_context->get(L"DD"))->add(1));
			ts->remove(i);
		} else if ((i = ts->containsWord(L"tonight")) > -1) {
			result->setTime(L"YYYY", _context->get(L"YYYY")->clone());
			result->setTime(L"MM", _context->get(L"MM")->clone());
			result->setTime(L"DD", (_context->get(L"DD"))->clone());
			if (result->getTime(L"HH")->isEmpty()) result->setTime(L"HH", L"NI");
			else if (result->getTime(L"HH")->value() <= 12) 
				result->setTime(L"HH", result->getTime(L"HH")->add(12)); 
			ts->remove(i);

		// Numeric pattens involving hyphens, slashes, etc.
		} else if ((temp = getAndParseDateString(ts)) != NULL) {
			delete result;
			result = temp;

		// each, every
		} else if  ((i = ts->containsWord(L"every")) > -1 || (i = ts->containsWord(L"each")) > -1) {
			if (isExTimeUnit(ts->tokenAt(i+1))) {
				result->setSet(ts->tokenAt(i+1));
				if (Month::isMonth(ts->tokenAt(i+1)) && (t1 = Strings::parseInt(ts->tokenAt(i+2))) > -1 && t1 <= 31) {
					result->setTime(L"DD", new Day(t1));
					result->setGranularity(L"D", 1);
					ts->remove(i+2);
				}
				ts->remove(i, i+1);
			} else ts->remove(i);

		// this
		} else if ((i = ts->containsWord(L"this")) > -1) {
			if (isExTimeUnit(ts->tokenAt(i+1))) {
				result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L"this"), ts->tokenAt(i+1), 1);
				if (Month::isMonth(ts->tokenAt(i+1)) && 
					(t1 = Strings::parseInt(ts->tokenAt(i+2))) >= 1 && t1 <= 31) 
				{
					result->setTime(L"DD", new Day(t1));
					ts->remove(i+2);
				}
				ts->remove(i, i+1);
			} else ts->remove(i);

		// upcoming, coming
		} else if ((i = ts->containsWord(L"upcoming")) > -1 || (i = ts->containsWord(L"coming")) > -1) {
			if (isExTimeUnit(ts->tokenAt(i+1))) {
				if (Strings::endsWith(ts->tokenAt(i+1), wstring(L"s")) || !ts->tokenAt(i+1).compare(L"millennia")) {
					// unit is plural 
					result->setDuration(timeUnit2Code(ts->tokenAt(i+1)).c_str(), TimeAmount::MAX_VALUE);
					result->setRelativeAnchorTime(_context, tenseStrings[_contextVerbTense], wstring(L""), ts->tokenAt(i+1), 0);
					result->setAnchorDir(wstring(L"AFTER"));
				}
				else {
					result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L"next"), ts->tokenAt(i+1), 1);
					if (Month::isMonth(ts->tokenAt(i+1)) && 
						(t1 = Strings::parseInt(ts->tokenAt(i+2))) >= 1 && t1 <= 31) 
					{
						result->setTime(L"DD", new Day(t1));
						ts->remove(i+2);
					}
				}
				ts->remove(i, i+1);
			} else ts->remove(i);

		// recent
		} else if ((i = ts->containsWord(L"recent")) > -1 && isExTimeUnit(ts->tokenAt(i+1))) {
			if (Strings::endsWith(ts->tokenAt(i+1), wstring(L"s")) || !ts->tokenAt(i+1).compare(L"millennia")) {
				// unit is plural 
				result->setDuration(timeUnit2Code(ts->tokenAt(i+1)).c_str(), TimeAmount::MAX_VALUE);
				result->setRelativeAnchorTime(_context, tenseStrings[_contextVerbTense], wstring(L""), ts->tokenAt(i+1), 0);
				result->setAnchorDir(wstring(L"BEFORE"));
			}
			ts->remove(i, i+1);

		// next, last
		} else if (((i = ts->containsWord(L"next")) > -1) || ((i = ts->containsWord(L"last")) > -1)) {
			//cout << "contains next\n";
			wstring mods[2] = { L"next", L"last" };
			for (int j = 0; j < 2; j++) {
				if ((i = ts->containsWord(mods[j])) > -1) {
					if (isExTimeUnit(ts->tokenAt(i+1))) {
						result->setRelativeTime(_context, tenseStrings[_contextVerbTense], mods[j], ts->tokenAt(i+1), 1);
						if (Month::isMonth(ts->tokenAt(i+1)) && 
							(t1 = Strings::parseInt(ts->tokenAt(i+2))) >= 1 && t1 <= 31) 
						{
							result->setTime(L"DD", new Day(t1));
							ts->remove(i, i+2);
						}
						// e.g. "May last year"
						else if (!ts->tokenAt(i+1).compare(L"year") && Month::isMonth(ts->tokenAt(i-1))) {
							result->setTime(L"MM", new Month(Month::month2Num(ts->tokenAt(i-1))));
							ts->remove(i-1, i+1);
						}
						// e.g. "May 16 last year"
						else if (!ts->tokenAt(i+1).compare(L"year") && Month::isMonth(ts->tokenAt(i-2)) &&
								(t1 = Strings::parseInt(ts->tokenAt(i-1))) >= 1 && t1 <= 31) 
						{
							result->setTime(L"MM", new Month(Month::month2Num(ts->tokenAt(i-2))));
							result->setTime(L"DD", new Day(t1));
							ts->remove(i-2, i+1);
						}
						else {
							ts->remove(i, i+1);
						}
					}
					else if (isPureTimeUnit(ts->tokenAt(i+2))) {
						if ((t1 = Strings::parseInt(ts->tokenAt(i+1))) > 0) {
							result->setDuration(timeUnit2Code(ts->tokenAt(i+2)).c_str(), t1);
							ts->remove(i, i+2);
						} else if ((f1 = Strings::parseFloat(ts->tokenAt(i+1))) > 0) {
							result->setDuration(timeUnit2Code(ts->tokenAt(i+2)).c_str(), f1);
							ts->remove(i, i+2);
						} else ts->remove(i);
					} else {
						ts->remove(i);
					}

					break;
				}
			}

		// previous, past
		} else if (((i = ts->containsWord(L"previous")) > -1) || ((i = ts->containsWord(L"past")) > -1)) {
			wstring mods[2] = { L"previous", L"past"};
			for (int j = 0; j < 2; j++) {
				if ((i = ts->containsWord(mods[j])) > -1) {
					if (isPureTimeUnit(ts->tokenAt(i+1))) {
						if (Strings::endsWith(ts->tokenAt(i+1), wstring(L"s")))
							result->setDuration(timeUnit2Code(ts->tokenAt(i+1)).c_str(), TimeAmount::MAX_VALUE);
						else 
							result->setDuration(timeUnit2Code(ts->tokenAt(i+1)).c_str(), 1);
						ts->remove(i, i+1);
					}
					else if (isExTimeUnit(ts->tokenAt(i+1))) {
						result->setRelativeTime(_context, tenseStrings[_contextVerbTense], mods[j], ts->tokenAt(i+1), 1);
						if (Month::isMonth(ts->tokenAt(i+1)) && 
							(t1 = Strings::parseInt(ts->tokenAt(i+2))) >= 1 && t1 <= 31) 
						{
							result->setTime(L"DD", new Day(t1));
							ts->remove(i+2);
						}
						ts->remove(i, i+1);
					}
					else if (isPureTimeUnit(ts->tokenAt(i+2))) {
						if ((t1 = Strings::parseInt(ts->tokenAt(i+1))) > 0) {
							result->setDuration(timeUnit2Code(ts->tokenAt(i+2)).c_str(), t1);
							ts->remove(i, i+2);
						} else if ((f1 = Strings::parseFloat(ts->tokenAt(i+1))) > 0) {
							result->setDuration(timeUnit2Code(ts->tokenAt(i+2)).c_str(), f1);
							ts->remove(i, i+2);
						} else ts->remove(i);
					} else ts->remove(i);
					break;
				}
			}

		// ago, away
		} else if (((i = ts->containsWord(L"ago")) > -1) || ((i = ts->containsWord(L"away")) > -1)) { 
			wstring mods[2] = { L"ago", L"away" };
			for (int j = 0; j < 2; j++) {
				if ((i = ts->containsWord(mods[j])) > -1) {
					if ((t1 = Strings::parseInt(ts->tokenAt(i-2))) > -1) {
						result->setRelativeTime(_context, tenseStrings[_contextVerbTense], mods[j], ts->tokenAt(i-1), t1);
						ts->remove(i-2, i); 
					} else if ((f1 = Strings::parseFloat(ts->tokenAt(i-2))) > -1 && isPureTimeUnit(ts->tokenAt(i-1))) {
						result->setRelativeTime(_context, tenseStrings[_contextVerbTense], mods[j], ts->tokenAt(i-1), f1);
						ts->remove(i-2, i);
					}
					else ts->remove(i);
				}
			}

		// from now 
		} else if ((i = ts->containsWord(L"from")) > -1 && !ts->tokenAt(i+1).compare(L"now")) {
			if (isPureTimeUnit(ts->tokenAt(i-1))) {
				if  ((t1 = Strings::parseInt(ts->tokenAt(i-2))) > -1) {
					result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L"away"), ts->tokenAt(i-1), t1);
					ts->remove(i-2, i+1);
				} else if ((f1 = Strings::parseFloat(ts->tokenAt(i-2))) > -1) {
					result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L"away"), ts->tokenAt(i-1), f1);
					ts->remove(i-2, i+1);
				} else {
					ts->remove(i, i+1);
				}
			} else ts->remove(i, i+1);

		// Explicitly named months 
		} else if ((i = ts->containsMonth()) > -1) {
			bool unknown_year_on_entry = result->getDateAndTime()->isEmpty(L"YYYY");
			result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L""), ts->tokenAt(i), 0);
			if (Strings::parseInt(ts->tokenAt(i-1)) >= 1 && 
				Strings::parseInt(ts->tokenAt(i-1)) <= Month::monthDays[Month::month2Num(ts->tokenAt(i))]) {
			    //wcout << "setting day of month to be " << Strings::parseInt(ts->tokenAt(i-1)) << "\n";
				result->setTime(L"DD", ts->tokenAt(i-1));
				if (Strings::parseInt(ts->tokenAt(i+1)) > -1 && ts->tokenAt(i+1).length() == 4) {
					//wcout << "setting year to be " << Strings::parseInt(ts->tokenAt(i+1)) << "\n";
					result->setTime(L"YYYY", ts->tokenAt(i+1));
					ts->remove(i-1, i+1);
				} else { 
					ts->remove(i-1, i);
				}
			} else if (Strings::parseInt(ts->tokenAt(i+1)) >= 1 && 
				Strings::parseInt(ts->tokenAt(i+1)) <= Month::monthDays[Month::month2Num(ts->tokenAt(i))]) {
				//wcout << "setting day of month to be " << Strings::parseInt(ts->tokenAt(i+1)) << "\n";
				result->setTime(L"DD", ts->tokenAt(i+1));
				if (Strings::parseInt(ts->tokenAt(i+2)) > -1 && ts->tokenAt(i+2).length() == 4) {
					//wcout << "setting year to be " << Strings::parseInt(ts->tokenAt(i+2)) << "\n";
					result->setTime(L"YYYY", ts->tokenAt(i+2));
					ts->remove(i, i+2);
				} else {

					// If we get here we have had a pattern like "Month ##" with no year following it.
					// Handle September 11th
					// Note we can't rely on addContext to do this because setRelativeTime above already called addContext
					// when we knew the month but not the day, so it couldn't test for 9/11
					if (unknown_year_on_entry &&
						!_context->get(L"YYYY")->isEmpty() && (_context->get(L"YYYY")->value() >= 2001) &&
						result->getTime(L"MM")->value() == 9 &&
						result->getTime(L"DD")->value() == 11) {
						result->setTime(L"YYYY", L"2001");
					}
					ts->remove(i, i+1);
				}
			} else if (Strings::parseInt(ts->tokenAt(i+1)) > -1 && ts->tokenAt(i+1).length() == 4) {
				//wcout << "setting year to be " << Strings::parseInt(ts->tokenAt(i+1)) << "\n";
				result->setTime(L"YYYY", ts->tokenAt(i+1));
				ts->remove(i, i+1);
			} else {
				ts->remove(i);
			}
			//wcout << L"now, " << ts->toString() << L": " << Symbol(result->toString().c_str()).to_string() << L"\n";
		// Quarters, Halves 
		} else if ((i = ts->containsWord(L"quarter")) > -1) {
			if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) >= 1) && (t1 <= 4)) {
				result->setTime(L"MM", L"Q" + Strings::valueOf(t1));
				ts->remove(i-1, i);
			} else ts->remove(i);
		} else if ((i = ts->containsWord(L"half")) > -1) {
			if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) >= 1) && (t1 <= 2)) {
				result->setTime(L"MM", L"H" + Strings::valueOf(t1));
				ts->remove(i-1, i);
			} else ts->remove(i);

		// Apostrophes (') and apostrophe-s ('s) 
		} else if ((i = ts->containsWord(L"'s")) > -1 || (i = ts->containsWord(L"s")) > -1) {
			if ((t1 = Strings::parseInt(ts->tokenAt(i-1))) > -1) {
				if ( t1 != 0 && (t1%100) == 0) {
					wstring t = Year::canonicalizeYear(t1);
					result->setTime(L"YYYY", t.substr(0,2) + L"XX"); 
				} else if ( t1 != 0 && (t1%10) == 0) {
					wstring t = Year::canonicalizeYear(t1);
					result->setTime(L"YYYY", t.substr(0,3) + L"X");
				}
				ts->remove(i-1, i);
			} else ts->remove(i);
		} else if ((i = ts->containsWord(L"'")) > -1) {
			if ((t1 = Strings::parseInt(ts->tokenAt(i+1))) > -1) {
				result->setTime(L"YYYY", Year::canonicalizeYear(t1));
				ts->remove(i, i+1);
			} else ts->remove(i);

		// time indicator phrases 
		} else if ((temp = getAndParseTimeString(ts)) != NULL) {
			result->setTime(L"HH", temp->getTime(L"HH")->clone());
			result->setTime(L"MN", temp->getTime(L"MN")->clone());
			delete temp;
			temp = 0;

		// colon (:), am and pm, o'clock, GMT
		} else if (((i = ts->containsWord(L"gmt")) > -1) && 
			((t1 = Strings::parseInt(ts->tokenAt(i-1))) > -1) && t1 <= 2400) 
		{
			result->getDateAndTime()->setIsGmt(true);
			if ((ts->tokenAt(i-1).length() == 4) || (ts->tokenAt(i-1).length() == 3)) {
				result->setTime(L"HH", new Hour((int)(t1/100)));
				result->setTime(L"MN", new Min((int)(t1%100)));
				ts->remove(i-1, i);
			} else ts->remove(i);
		} else if (((i = ts->containsWord(L"hours")) > -1) && (ts->tokenAt(i-1).length() > 2) &&
			((t1 = Strings::parseInt(ts->tokenAt(i-1))) > -1) && t1 <= 2400) 
		{
			if ((ts->tokenAt(i-1).length() == 4) || (ts->tokenAt(i-1).length() == 3)) {
				result->setTime(L"HH", new Hour((int)(t1/100)));
				result->setTime(L"MN", new Min((int)(t1%100)));
				ts->remove(i-1, i);
			} else ts->remove(i);
		} else if ((i = ts->containsWord(L"o'clock")) > -1) {
			if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) > -1) && (t1 <= 12)) {
				if (!ts->tokenAt(i+1).compare(L"pm")) {
					t1 += 12;
					ts->remove(i+1);
				} else if (!result->getTime(L"HH")->toString().compare(L"AF") || 
					!result->getTime(L"HH")->toString().compare(L"EV") || !result->getTime(L"HH")->toString().compare(L"NI")) 
				{
					t1 += 12;
				}
				result->setTime(L"HH", new Hour(t1));
				result->setTime(L"MN", new Min(0));
				ts->remove(i-1, i);
			} else ts->remove(i);
		} else if ((i = ts->containsWord(L":")) > -1) {
			if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) >= 0) && (t1 <= 24) &&
				((t2 = Strings::parseInt(ts->tokenAt(i+1))) >= 0) && (t2 < 60))  
			{
				//cout << "Found hours and minutes\n";
				//cout << Symbol(ts->toString().c_str()).to_debug_string() << "\n";
				if (!ts->tokenAt(i+2).compare(L":") && (t3 = Strings::parseInt(ts->tokenAt(i+3))) >= 0 && t3 < 60) {
					//cout << "Found seconds\n";
					result->setTime(L"SS", new Min(t3));

					// deal with "- 0300" after seconds, additionalTime should be set to "-03"
					wstring additionalMarker = ts->tokenAt(i+4);
					wstring additional = ts->tokenAt(i+5);
					if ((!additionalMarker.compare(L"-") || !additionalMarker.compare(L"+")) &&
						(Strings::parseInt(additional.substr(0, 2)) > 0 && Strings::parseInt(additional.substr(0, 2)) < 25)) 
					{
						result->getDateAndTime()->setAdditionalTime(additionalMarker.append(additional.substr(0, 2)));
						ts->remove(i+4);
					}

					ts->remove(i+2, i+3);
				}
				if (t1 <= 12 && !ts->tokenAt(i+2).compare(L"pm")) {
					t1 += 12;
					ts->remove(i+2);
				} else if (t1 <= 12 && (!result->getTime(L"HH")->toString().compare(L"AF") || 
					!result->getTime(L"HH")->toString().compare(L"EV") || !result->getTime(L"HH")->toString().compare(L"NI"))) 
				{
					t1 +=12;
				}
				result->setTime(L"HH", new Hour(t1));
				result->setTime(L"MN", new Min(t2));
				ts->remove(i-1, i+1);

			}
			else if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) >= 0) && (t1 < 60) &&
					((t2 = Strings::parseInt(ts->tokenAt(i+1))) >= 0) && (t2 < 60)) 
			{
				result->setTime(L"MN", new Min(t1));
				result->setTime(L"SS", new Min(t2));
				ts->remove(i-1, i+1);
			}
			else ts->remove(i);
		} else if ((i = ts->containsWord(L"am")) > -1) {
			if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) > 0) && t1 <= 12) {
				if (t1 == 12) t1 -= 12; // 12 am == midnight
				result->setTime(L"HH", new Hour(t1));
				result->setTime(L"MN", new Min(0));
				ts->remove(i-1,i);
			} else if ((t1 = Strings::parseInt(ts->tokenAt(i-2))) > 0 && t1 <= 12 &&
				(t2 = Strings::parseInt(ts->tokenAt(i-1))) >= 0 && t2 < 60) 
			{
				result->setTime(L"HH", new Hour(t1));
				result->setTime(L"MN", new Min(t2));
				ts->remove(i-2, i);
			}
			ts->remove(i);
		} else if ((i = ts->containsWord(L"pm")) > -1) {
			if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) > 0) && t1 <= 12) {
				if (t1 < 12) t1 += 12;
				result->setTime(L"HH", new Hour(t1));
				result->setTime(L"MN", new Min(0));
				ts->remove(i-1, i);
			} else if (((t1 = Strings::parseInt(ts->tokenAt(i-1))) > 12) && t1 <= 24) {
				result->setTime(L"HH", new Hour(t1));
				result->setTime(L"MN", new Min(0));
				ts->remove(i-1, i);
			} else if ((t1 = Strings::parseInt(ts->tokenAt(i-2))) > 0 && t1 <= 12 &&
					(t2 = Strings::parseInt(ts->tokenAt(i-1))) >= 0 && t2 < 60) 
			{
				if (t1 < 12) t1 += 12;
				result->setTime(L"HH", new Hour(t1));
				result->setTime(L"MN", new Min(t2));
				ts->remove(i-2, i);
			}
			ts->remove(i);

		// Times of Day: Midnight, Noon, Mid-day, Daytime, etc.
		} else if ((i = ts->containsWord(L"midnight")) > -1) {
			result->setTime(L"HH", new Hour(24));
			result->setTime(L"MN", new Min(0));
			ts->remove(i);
		} else if ((i = ts->containsWord(L"noon")) > -1) {
			result->setTime(L"HH", new Hour(12));
			result->setTime(L"MN", new Min(0));
			ts->remove(i);
		} else if (((i = ts->containsWord(L"mid")) > -1) && 
			(!ts->tokenAt(i+1).compare(L"-")) && !ts->tokenAt(i+2).compare(L"day")) 
		{
			if (result->getTime(L"HH")->isEmpty()) result->setTime(L"HH", L"MI");
			ts->remove(i, i+2);
		} else if ((i = ts->containsWord(L"midday")) > -1) {
			if (result->getTime(L"HH")->isEmpty()) result->setTime(L"HH", L"MI");
			ts->remove(i);
		} else if ((i = ts->containsWord(L"daytime")) > -1) {
			if (result->getTime(L"HH")->isEmpty()) result->setTime(L"HH", L"DT");
			ts->remove(i);
		} else if (((i = ts->containsWord(L"morning")) > -1) || ((i = ts->containsWord(L"dawn")) > -1)) {
			if (result->getTime(L"HH")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) {
					result->setTime(L"HH", L"MO");
				}
				else {
					result->setSet(L"morning");	
				}
			}
			ts->remove(i);
		} else if ((i = ts->containsWord(L"afternoon")) > -1) {
			if (result->getTime(L"HH")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"HH", L"AF");
				else result->setSet(wstring(L"afternoon"));
			} else if (result->getTime(L"HH")->value() <= 12) {
				result->setTime(L"HH", result->getTime(L"HH")->add(12));
			}
			ts->remove(i);
		} else if (((i = ts->containsWord(L"evening")) > -1) || ((i = ts->containsWord(L"dusk")) > -1)) {
			if (result->getTime(L"HH")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"HH", L"EV");
				else result->setSet(L"evening");
			} else if (result->getTime(L"HH")->value() <= 12) { 
					result->setTime(L"HH", result->getTime(L"HH")->add(12));
			}
			ts->remove(i);
		} else if (((i = ts->containsWord(L"night")) > -1) || ((i = ts->containsWord(L"overnight")) > -1)) {
			if (result->getTime(L"HH")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"HH", L"NI");
				else result->setSet(wstring(L"night"));
			} else if (result->getTime(L"HH")->value() >= 6 && result->getTime(L"HH")->value() <= 12) {
				result->setTime(L"HH", result->getTime(L"HH")->add(12));
			}
			ts->remove(i);

		// weekend without modifiers
		} else if ((i = ts->containsWord(L"weekend")) > -1) {
			if (result->getTime(L"DD")->isEmpty()) {
				// singular
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) {
					if (result->isWeekMode()) {
						result->setTime(L"DD", L"WE");
					} else {
						// assume we mean this weekend, unless preceded by '1' (originally was the indefinite 'a')
						if (Strings::parseInt(ts->tokenAt(i-1)) == 1) {
							result->setTime(L"MM", L"WXX");
							result->setTime(L"DD", L"WE");
						} else result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L"this"), wstring(L"weekend"), 0);
					}
				// plural
				} else { result->setSet(L"weekend"); }
			}
			ts->remove(i);

		// Seasons without modifiers: spring, summer, fall, autumn, winter
		} else if ((i = ts->containsWord(L"spring")) > -1) {
			if (result->getTime(L"MM")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"MM", L"SP");
				else result->setSet(L"spring");
			}
			ts->remove(i);
		} else if ((i = ts->containsWord(L"summer")) > -1) {
			if (result->getTime(L"MM")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"MM", L"SU");
				else result->setSet(L"summer");
			}
			ts->remove(i);
		} else if ((i = ts->containsWord(L"fall")) > -1) { 
			if (result->getTime(L"MM")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"MM", L"FA");
				else result->setSet(wstring(L"fall"));
			}
			ts->remove(i);
		} else if ((i = ts->containsWord(L"autumn")) > -1) {
			if (result->getTime(L"MM")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"MM", L"FA");
				else result->setSet(L"autumn");
			}
			ts->remove(i);
		} else if ((i = ts->containsWord(L"winter")) > -1) {
			if (result->getTime(L"MM")->isEmpty()) {
				if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) result->setTime(L"MM", L"WI");
				else result->setSet(L"winter");
			}
			ts->remove(i);

		// Days of week without modifiers
		} else if ((i = ts->containsDayOfWeek()) > -1) {
			if (!Strings::endsWith(ts->tokenAt(i), wstring(L"s"))) {
				if (!result->isSet()) 
					result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L""), ts->tokenAt(i), 0);
				else { 
					result->setTime(L"MM", L"WXX"); 
					result->setTime(L"DD", new Day(Day::dayOfWeek2Num(ts->tokenAt(i)),true));
				}
				//wcout << L"now, " << Symbol(result->toString().c_str()).to_string() << L"\n";
			}
			else result->setSet(ts->tokenAt(i));
			ts->remove(i);

		// Time units without modifiers (durations) - number defaults to one if singular, X if plural
		} else if ((i = ts->containsPureTimeUnit()) > -1) {
			if (result->isEmpty()) {
				if ((t1 = Strings::parseInt(ts->tokenAt(i-1))) > -1) {
					result->setDuration(timeUnit2Code(ts->tokenAt(i)).c_str(), t1);
					ts->remove(i-1, i);
				} else if (!ts->tokenAt(i-1).compare(L"-") && (t1 = Strings::parseInt(ts->tokenAt(i-2))) > -1) {
					result->setDuration(timeUnit2Code(ts->tokenAt(i)).c_str(), t1);
					ts->remove(i-2, i);
				} else if ((f1 = Strings::parseFloat(ts->tokenAt(i-1))) > -1) {
					result->setDuration(timeUnit2Code(ts->tokenAt(i)).c_str(), f1);
					ts->remove(i-1, i);
				} else if (Strings::endsWith(ts->tokenAt(i), wstring(L"s")) || !ts->tokenAt(i).compare(L"millennia")) {
					// unit is plural 
					result->setDuration(timeUnit2Code(ts->tokenAt(i)).c_str(), TimeAmount::MAX_VALUE);
					ts->remove(i);
				// the end of the month
				} else if (!ts->tokenAt(i-1).compare(L"the")) {
					result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L"this"), ts->tokenAt(i), 0);
					ts->remove(i-1, i);
				// L"Year 2000"
				} else if (!ts->tokenAt(i).compare(L"year") && (t1 = Strings::parseInt(ts->tokenAt(i+1))) > 1) { 
					result->setTime(L"YYYY", new Year(t1));
					ts->remove(i, i+1);
				} else {
					result->setDuration(timeUnit2Code(ts->tokenAt(i)).c_str(), 1);
					ts->remove(i);
				}
			} else {
				if ((t1 = Strings::parseInt(ts->tokenAt(i-1))) > -1) {
					result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L""), ts->tokenAt(i), t1);
					ts->remove(i-1, i);
				}
				else {
					ts->remove(i);
				}
			}
		} else {

			if (!result->isDuration()) {
				// Guess at numbers with strong or medium indicators as to their type
				wstring numbers[10];
				int length = ts->getNums(numbers, 10);
				for (int j = 0; j < length; j++) { 
					wstring n = numbers[j];
					if (n.length() == 4) {
						if (result->isEmpty(L"YYYY") && n.at(0) == '0' && Strings::parseInt(n.substr(2)) < 60) {
							result->setTime(L"HH", new Hour(n.substr(0,2)));
							result->setTime(L"MN", new Min(n.substr(2)));
						} else if (result->isEmpty(L"YYYY") && 
							(Strings::parseInt(n.substr(0,2)) >= 10 || Strings::parseInt(n.substr(0,2)) <= 20)) 
						{
							result->setTime(L"YYYY", new Year(n));
						} else if (result->isEmpty(L"HH") && result->isEmpty(L"MN") && 
							Strings::parseInt(n.substr(0,2)) <= 24 && Strings::parseInt(n.substr(2)) < 60) 
						{
							result->setTime(L"HH", new Hour(n.substr(0,2)));
							result->setTime(L"MN", new Min(n.substr(2)));
						} else if (result->isEmpty(L"YYYY")) result->setTime(L"YYYY", new Year(n)); 
					}
					else if (n.length() == 1 || n.length() == 2) {
						if (result->isEmpty(L"DD") && (i = ts->containsWord(n)) && Strings::parseInt(n) >= 1 && Strings::parseInt(n) <= 31) {
							// Handle cases like "the 29th"
							if (!ts->tokenAt(i-1).compare(L"the") && ts->size() == (i+1)) {
								result->setTime(L"DD", new Day(Strings::parseInt(n)));
								// Try to determine the month and year based on context
								if (result->getTime(L"YYYY")->isEmpty() && result->getTime(L"MM")->isEmpty()) {
									TimeUnit * contextDay = _context->get(L"DD");
									if (!contextDay->isEmpty()) { 
										if (contextDay->compareTo(result->getTime(L"DD")) > 0 && _contextVerbTense == FUTURE_TENSE) {
											result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L""), wstring(L"month"), 1);
										} else if (contextDay->compareTo(result->getTime(L"DD")) < 0 && _contextVerbTense == PAST_TENSE) {
											result->setRelativeTime(_context, tenseStrings[_contextVerbTense], wstring(L""), wstring(L"month"), -1);
										} else {
											result->addContext(_context);
										}
									} else result->addContext(_context);
								}
							}
						} else if (result->isEmpty(L"YYYY") && Strings::parseInt(n) >= 40 && Strings::parseInt(n) <= 99) {
							result->setTime(L"YYYY", Year::canonicalizeYear(Strings::parseInt(n)));
						}
					}
					// dd dd -> set Month, Day?
				}	
			}
			//wcout << L"now, " << Symbol(result->toString().c_str()).to_string() << L"\n";
			return result;
		}
	}
}

void EnglishTemporalNormalizer::matchGenericRefs(const wstring &s, TimeObj *time) {
	
	wstring s_exp = wstring(L" ") + s + L" ";
	
	wstring past[14] = { L" past ", L" former ", L" formerly ", L" recent ", L" recently ",  
		L" previous ", L" previously ", L" onetime ", L" once ", L" ago ", L" last ", L" earlier ",
		L" onetime ", L" 1 - time "};
	wstring pres[12] = { L" now ", L" current ", L" currently ",  L" present ", L" presently ",
		L" nowadays ", L" this point in time ", L" this time ", L" next ", L" immediately ", L" modern ", L" lately " };
	wstring future[1] = { L" future "};

	for (int i = 0; i < 14; i++) 
		if (s_exp.find(past[i]) != std::wstring::npos) { time->setGeneric(wstring(L"PAST_REF")); time->setAnchorDir(wstring(L"BEFORE")); return; }
	for (int j = 0; j < 12; j++)
		if (s_exp.find(pres[j]) != std::wstring::npos) { time->setGeneric(wstring(L"PRESENT_REF")); time->setAnchorDir(wstring(L"AS_OF")); return; }
	for (int k = 0; k < 1; k++)
		if (s_exp.find(future[k]) != std::wstring::npos) { time->setGeneric(wstring(L"FUTURE_REF")); time->setAnchorDir(wstring(L"AFTER")); return; }
}

void EnglishTemporalNormalizer::matchModifiers(const wstring &s, TimeObj *time) {
	int i;
	
	const int sm_length = 4;
	wstring startMods[sm_length] = { L" start ", L" early ", L" beginning ", L" dawn of " };
	const int mm_length = 3;
	wstring midMods[mm_length] = { L" middle ", L" mid-", L" mid " };
	const int em_length = 3;
	wstring endMods[em_length] = { L" end ", L" late ", L" latter part " };
	const int am_length = 4;
	wstring approxMods[am_length] = { L" about ", L" around ", L" roughly ", L" or so " };

	const int lm_length = 3;
	wstring lessMods[lm_length] = { L" less than ", L" nearly ", L" almost " };
	const int morem_length = 3;
	wstring moreMods[morem_length] = { L" more than ", L" barely ", L" over " };
	const int elm_length = 2;
	wstring equalLessMods[elm_length] = { L" no more than ", L" up to " };
	const int emm_length = 2;
	wstring equalMoreMods[emm_length] = { L" no less than ", L" at least "};

	for (i = 0; i < sm_length; i++)
		if (s.find(startMods[i]) != std::wstring::npos) { time->setMod(L"START"); return; }
	for (i = 0; i < mm_length; i++)
		if (s.find(midMods[i]) != std::wstring::npos) { time->setMod(L"MID"); return; }
	for (i = 0; i < em_length; i++)
		if (s.find(endMods[i]) != std::wstring::npos) { time->setMod(L"END"); return; }
	for (i = 0; i < am_length; i++)
		if (s.find(approxMods[i]) != std::wstring::npos) { time->setMod(L"APPROX"); return; }

	if (time->isDuration()) {
		for (i = 0; i < elm_length; i++)
			if (s.find(equalLessMods[i]) != std::wstring::npos) { time->setMod(L"EQUAL_OR_LESS"); return; }
		for (i = 0; i < emm_length; i++)
			if (s.find(equalMoreMods[i]) != std::wstring::npos) { time->setMod(L"EQUAL_OR_MORE"); return; }
		for (i = 0; i < lm_length; i++)
			if (s.find(lessMods[i]) != std::wstring::npos) { time->setMod(L"LESS_THAN"); return; }
		for (i = 0; i < morem_length; i++)
			if (s.find(moreMods[i]) != std::wstring::npos) { time->setMod(L"MORE_THAN"); return; }
	} else {
		if (s.find(L" before ") != std::wstring::npos) {
			time->setMod(L"BEFORE");
			return;
		}
		if (s.find(L" after ") != std::wstring::npos) {
			time->setMod(L"AFTER");
			return;
		}
		if (time->compareTo(_context) == -1) {
			for (i = 0; i < elm_length; i++)
				if (s.find(equalLessMods[i]) != std::wstring::npos) { time->setMod(L"ON_OR_AFTER"); return; }
			for (i = 0; i < emm_length; i++)
				if (s.find(equalMoreMods[i]) != std::wstring::npos) { time->setMod(L"ON_OR_BEFORE"); return; }
			for (i = 0; i < lm_length; i++)
				if (s.find(lessMods[i]) != std::wstring::npos) { time->setMod(L"AFTER"); return; }
			for (i = 0; i < morem_length; i++)
				if (s.find(moreMods[i]) != std::wstring::npos) { time->setMod(L"BEFORE"); return; }
		} else if (time->compareTo(_context) == 1) {
			for (i = 0; i < elm_length; i++)
				if (s.find(equalLessMods[i]) != std::wstring::npos) { time->setMod(L"ON_OR_BEFORE"); return; }
			for (i = 0; i < emm_length; i++)
				if (s.find(equalMoreMods[i]) != std::wstring::npos) { time->setMod(L"ON_OR_AFTER"); return; }
			for (i = 0; i < lm_length; i++)
				if (s.find(lessMods[i]) != std::wstring::npos) { time->setMod(L"BEFORE"); return; }
			for (i = 0; i < morem_length; i++)
				if (s.find(moreMods[i]) != std::wstring::npos) { time->setMod(L"AFTER"); return; }
		}
	}
}

/* searches ts for dates in the form mm-dd or mm-dd-yyyy or mm-yyyy or
mm/dd or mm/dd/yyyy or mm/yyyy, etc. and returns a time object set to
contain the value of the first such date pattern found. */
TimeObj* EnglishTemporalNormalizer::getAndParseDateString(TemporalString* ts) {
	TimeObj *result = new TimeObj();

	// 20040912
/*	for (int i = 0; i < ts->size(); i++) {
		wstring date = ts->tokenAt(i);
		
		if (date.length() == 8 && Strings::parseInt(date) > -1) {
			wstring year = date.substr(0, 4);
			wstring month = date.substr(4, 2);
			wstring day = date.substr(6, 2);
			if (Strings::parseInt(month) >= 1 && Strings::parseInt(month) <= 12 &&
				Strings::parseInt(day) >= 1 && Strings::parseInt(day) <= 31)
			{
				result->setTime(L"YYYY", year);
				result->setTime(L"MM", month);
				result->setTime(L"DD", day);
				ts->remove(i);
				return result;
			}	
		}
	}*/

	wstring delims[3] = { L"-", L"/", L"\\" };
	wstring m, d, y, rest, previous;
	for (int i = 0; i < 3; i++) {
		for (int j = 1; j < ts->size() - 1; j++) {   // search through i - (ts.size()-1)
			if (!ts->tokenAt(j).compare(delims[i])) {
				m = ts->tokenAt(j - 1);
				d = ts->tokenAt(j + 1);
				previous = ts->tokenAt(j - 2);
				if (Strings::parseInt(m) >= 1 && Strings::parseInt(m) <= 12 &&
					Strings::parseInt(d) >= 1 && Strings::parseInt(d) <= 31 &&
					previous.compare(L":"))
				{
					result->setTime(L"MM", m);
					result->setTime(L"DD", d);
					if (!ts->tokenAt(j+2).compare(delims[i])) {
						y = ts->tokenAt(j+3);
						if (Strings::parseInt(y) != -1) {
							result->setTime(L"YYYY", Year::canonicalizeYear(Strings::parseInt(y)));
							ts->remove(j-1, j+3);
							return result;
						}
					}
					ts->remove(j-1, j+1);
					return result;
				}
				else if (Strings::parseInt(m) >= 1 && Strings::parseInt(m) <= 12 && 
					     Strings::parseInt(d) > -1 && previous.compare(L":")) 
				{
					result->setTime(L"MM", m);
					result->setTime(L"YYYY", Year::canonicalizeYear(Strings::parseInt(d)));
					ts->remove(j-1, j+1);
					return result;
				}
				else if (Strings::parseInt(d) >= 1 && Strings::parseInt(d) <= 12 && 
					     Strings::parseInt(m) > -1 && previous.compare(L":")) 
				{
					result->setTime(L"MM", d);
					result->setTime(L"YYYY", Year::canonicalizeYear(Strings::parseInt(m)));
					ts->remove(j-1, j+1);
					return result;
				}
			}
		}
	}
	delete result;
	return NULL;
}

/* Find time indicator like 'ten til three' and 'five after four' in a 
TemporalString and return a TimeObj representing that time */
TimeObj* EnglishTemporalNormalizer::getAndParseTimeString(TemporalString *ts) {

	const int b_length = 2;
	const int a_length = 2;
	wstring befores[b_length] = { L"til", L"before" };
	wstring afters[a_length] = { L"after", L"past" };
	TimeObj *t = new TimeObj();
	int t1, t2, i, j;

	for (j = 0; j < b_length; j++) {
		if ((i = ts->containsWord(befores[j])) > -1) {
			if (( t1 = Strings::parseInt(ts->tokenAt(i-1))) > 0 && t1 < 60 &&
				( t2 = Strings::parseInt(ts->tokenAt(i+1))) > 0 && t1 <= 12) {
					t->setTime(L"HH", new Hour(t2-1));
					t->setTime(L"MN", new Min(60 - t1));
					ts->remove(i-1, i+1);
					return t;
				}
		}
	}

	for (j = 0; j < a_length; j++) {
		if ((i = ts->containsWord(afters[j])) > -1) {
			if (( t1 = Strings::parseInt(ts->tokenAt(i-1))) > 0 && t1 < 60 &&
				( t2 = Strings::parseInt(ts->tokenAt(i+1))) > 0 && t1 <= 12) {
					t->setTime(L"HH", new Hour(t2));
					t->setTime(L"MN", new Min(t1));
					ts->remove(i-1, i+1);
					return t;
				}
		}
	}	   
	delete t;
	return NULL;
}

/* Combine adjacent number tokens in the TemporalString into a single number
if possible (for example, nineteen eighty three - 19 80 3 - would 
become 1983) */
void EnglishTemporalNormalizer::combineAdjacentNumbers(TemporalString *t) {
	int first, second;

	for (int i = 0; i < t->size(); i++) {
		if ((Strings::parseInt(t->tokenAt(i)) > -1) && (Strings::parseInt(t->tokenAt(i + 1)) > -1)) {
			first = Strings::parseInt(t->tokenAt(i));
			second = Strings::parseInt(t->tokenAt(i+1));
			if ( second == 1000 && first > 0 && first < 1000) {
				t->setToken(i, Strings::valueOf(first * second));
				t->remove(i+1);
				i--;
			}
			if ( second == 100 && first > 0 && first < 20) {
				t->setToken(i, Strings::valueOf(first * second));
				t->remove(i+1);
				i--;
			}
			else if (first > 10 && first <= 20 && second >= 10 && second < 100) {
				t->setToken(i, Strings::valueOf((first*100) + second));
				t->remove(i+1);
				i--;
			}
			else if ( (first%100) == 0 && second > 0 && second < 10) {
				t->setToken(i, Strings::valueOf((int)(first/100)) + L"0" + Strings::valueOf(second));
				t->remove(i+1);
				i--;
			}
			else if ( (first%100) == 0 && second > 0 && second < 100) {
				t->setToken(i, Strings::valueOf((int)(first/100)) + Strings::valueOf(second));
				t->remove(i+1);
				i--;
			}
			else if ( (first%10) == 0 && second > 0 && second < 10) {
				t->setToken(i, Strings::valueOf((int)(first/10)) + Strings::valueOf(second));
				t->remove(i+1);
				i--;
			}
		}
	}
}

/* Translate separate adjacent tokens such as '1 _HALF_' into one token - 1.5 */
void EnglishTemporalNormalizer::dealWithFractions(TemporalString *t) {
	int j;
	for (int i = 0; i < t->size(); i++) {
		if (!t->tokenAt(i).compare(_HALF_) || !t->tokenAt(i).compare(_QUARTER_) || 
			!t->tokenAt(i).compare(_THREE_QUARTERS_)) 
		{

			float n = (float)0;
			if (!t->tokenAt(i).compare(_HALF_))           n = (float).5;
			if (!t->tokenAt(i).compare(_QUARTER_))        n = (float).25;
			if (!t->tokenAt(i).compare(_THREE_QUARTERS_)) n = (float).75;

			if (Strings::parseInt(t->tokenAt(i+1)) == 1)
				t->remove(i+1);  // remove article -> half of a year -> _HALF_ 1 year -> .5 year

			if ((j = Strings::parseInt(t->tokenAt(i-1))) > -1) {
				t->setToken(i-1, Strings::valueOf((float)j + n));
				t->remove(i);
				i--;
				// cases like: 'a year and a half' (1 year 0.5)  
			} else if (isPureTimeUnit(t->tokenAt(i-1)) && (j = Strings::parseInt(t->tokenAt(i-2))) > -1) {
				t->setToken(i-2, Strings::valueOf((float)j + n));
				t->remove(i);
			} else t->setToken(i, Strings::valueOf(n));
		}
	}
}

void EnglishTemporalNormalizer::initializeSubstitutionTable()
{
	_num_keys = 0;
	(*_table)[Symbol(L"january")] = Symbol(L"jan");
	_keys[_num_keys++] = Symbol(L"january");
	(*_table)[Symbol(L"february")] = Symbol(L"feb");
	_keys[_num_keys++] = Symbol(L"february");
	(*_table)[Symbol(L"march")] = Symbol(L"mar");
	_keys[_num_keys++] = Symbol(L"march");
	(*_table)[Symbol(L"april")] = Symbol(L"apr");
	_keys[_num_keys++] = Symbol(L"april");
	(*_table)[Symbol(L"june")] = Symbol(L"jun");
	_keys[_num_keys++] = Symbol(L"june");
	(*_table)[Symbol(L"july")] = Symbol(L"jul");
	_keys[_num_keys++] = Symbol(L"july");
	(*_table)[Symbol(L"august")] = Symbol(L"aug");
	_keys[_num_keys++] = Symbol(L"august");
	(*_table)[Symbol(L"september")] = Symbol(L"sep");
	_keys[_num_keys++] = Symbol(L"september");
	(*_table)[Symbol(L"sept")] = Symbol(L"sep");
	_keys[_num_keys++] = Symbol(L"sept");
	(*_table)[Symbol(L"october")] = Symbol(L"oct");
	_keys[_num_keys++] = Symbol(L"october");
	(*_table)[Symbol(L"november")] = Symbol(L"nov");
	_keys[_num_keys++] = Symbol(L"november");
	(*_table)[Symbol(L"december")] = Symbol(L"dec");
	_keys[_num_keys++] = Symbol(L"december");
	(*_table)[Symbol(L"sunday")] = Symbol(L"sun");
	_keys[_num_keys++] = Symbol(L"sunday");
	(*_table)[Symbol(L"monday")] = Symbol(L"mon");
	_keys[_num_keys++] = Symbol(L"monday");
	(*_table)[Symbol(L"tuesday")] = Symbol(L"tue");
	_keys[_num_keys++] = Symbol(L"tuesday");
	(*_table)[Symbol(L"tues")] = Symbol(L"tue");
	_keys[_num_keys++] = Symbol(L"tues");
	(*_table)[Symbol(L"wednesday")] = Symbol(L"wed");
	_keys[_num_keys++] = Symbol(L"wednesday");
	(*_table)[Symbol(L"thursday")] = Symbol(L"thu");
	_keys[_num_keys++] = Symbol(L"thursday");
	(*_table)[Symbol(L"thurs")] = Symbol(L"thu");
	_keys[_num_keys++] = Symbol(L"thurs");
	(*_table)[Symbol(L"thur")] = Symbol(L"thu");
	_keys[_num_keys++] = Symbol(L"thur");
	(*_table)[Symbol(L"friday")] = Symbol(L"fri");
	_keys[_num_keys++] = Symbol(L"friday");
	(*_table)[Symbol(L"saturday")] = Symbol(L"sat");
	_keys[_num_keys++] = Symbol(L"saturday");
	(*_table)[Symbol(L"a.m.")] = Symbol(L"am");
	_keys[_num_keys++] = Symbol(L"a.m.");
	(*_table)[Symbol(L"p.m.")] = Symbol(L"pm");
	_keys[_num_keys++] = Symbol(L"p.m.");

	(*_table)[Symbol(L" a half ")] = Symbol(wstring(L" " + _HALF_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" a half ");
	(*_table)[Symbol(L" one half ")] = Symbol(wstring(L" " + _HALF_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" one half ");
	(*_table)[Symbol(L" half a ")] = Symbol(wstring(L" " + _HALF_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" half a ");
	(*_table)[Symbol(L"-and-a-half")] = Symbol(wstring(L" " + _HALF_).c_str());
	_keys[_num_keys++] = Symbol(L"-and-a-half");
	(*_table)[Symbol(L" 1/2 ")] = Symbol(wstring(L" " + _HALF_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" 1/2 ");
	(*_table)[Symbol(L".5 ")] = Symbol(wstring(L"  " + _HALF_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L".5 ");

	(*_table)[Symbol(L" a quarter ")] = Symbol(wstring(L" " + _QUARTER_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" a quarter ");
	(*_table)[Symbol(L" one quarter ")] = Symbol(wstring(L" " + _QUARTER_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" one quarter ");
	(*_table)[Symbol(L" 1/4 ")] = Symbol(wstring(L" " + _QUARTER_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" 1/4 ");
	(*_table)[Symbol(L".25 ")] = Symbol(wstring(L" " + _QUARTER_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L".25 ");
	(*_table)[Symbol(L" three quarters ")] = Symbol(wstring(L" " + _THREE_QUARTERS_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" three quarters ");
	(*_table)[Symbol(L" 3/4 ")] = Symbol(wstring(L" " + _THREE_QUARTERS_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L" 3/4 ");
	(*_table)[Symbol(L".75 ")] = Symbol(wstring(L" " + _THREE_QUARTERS_ + L" ").c_str());
	_keys[_num_keys++] = Symbol(L".75 ");

	(*_table)[Symbol(L" twenty-")] = Symbol(L" 2-");
	_keys[_num_keys++] = Symbol(L" twenty-");
	(*_table)[Symbol(L" thirty-")] = Symbol(L" 3-");
	_keys[_num_keys++] = Symbol(L" thirty-");
	(*_table)[Symbol(L" forty-")] = Symbol(L" 4-");
	_keys[_num_keys++] = Symbol(L" forty-");
	(*_table)[Symbol(L" fifty-")] = Symbol(L" 5-");
	_keys[_num_keys++] = Symbol(L" fifty-");
	(*_table)[Symbol(L" sixty-")] = Symbol(L" 6-");
	_keys[_num_keys++] = Symbol(L" sixty-");
	(*_table)[Symbol(L" seventy-")] = Symbol(L" 7-");
	_keys[_num_keys++] = Symbol(L" seventy-");
	(*_table)[Symbol(L" eighty-")] = Symbol(L" 8-");
	_keys[_num_keys++] = Symbol(L" eighty-");
	(*_table)[Symbol(L" ninety-")] = Symbol(L" 9-");
	_keys[_num_keys++] = Symbol(L" ninety-");
	(*_table)[Symbol(L"-one ")] = Symbol(L"1 ");
	_keys[_num_keys++] = Symbol(L"-one ");
	(*_table)[Symbol(L"-two ")] = Symbol(L"2 ");
	_keys[_num_keys++] = Symbol(L"-two ");
	(*_table)[Symbol(L"-three ")] = Symbol(L"3 ");
	_keys[_num_keys++] = Symbol(L"-three ");
	(*_table)[Symbol(L"-four ")] = Symbol(L"4 ");
	_keys[_num_keys++] = Symbol(L"-four ");
	(*_table)[Symbol(L"-five ")] = Symbol(L"5 ");
	_keys[_num_keys++] = Symbol(L"-five ");
	(*_table)[Symbol(L"-six ")] = Symbol(L"6 ");
	_keys[_num_keys++] = Symbol(L"-six ");
	(*_table)[Symbol(L"-seven ")] = Symbol(L"7 ");
	_keys[_num_keys++] = Symbol(L"-seven ");
	(*_table)[Symbol(L"-eight ")] = Symbol(L"8 ");
	_keys[_num_keys++] = Symbol(L"-eight ");
	(*_table)[Symbol(L"-nine ")] = Symbol(L"9 ");
	_keys[_num_keys++] = Symbol(L"-nine ");
	(*_table)[Symbol(L"-first ")] = Symbol(L"1 ");
	_keys[_num_keys++] = Symbol(L"-first ");
	(*_table)[Symbol(L"-second ")] = Symbol(L"2 ");
	_keys[_num_keys++] = Symbol(L"-second ");
	(*_table)[Symbol(L"-third ")] = Symbol(L"3 ");
	_keys[_num_keys++] = Symbol(L"-third ");
	(*_table)[Symbol(L"-fourth ")] = Symbol(L"4 ");
	_keys[_num_keys++] = Symbol(L"-fourth ");
	(*_table)[Symbol(L"-fifth ")] = Symbol(L"5 ");
	_keys[_num_keys++] = Symbol(L"-fifth ");
	(*_table)[Symbol(L"-sixth ")] = Symbol(L"6 ");
	_keys[_num_keys++] = Symbol(L"-sixth ");
	(*_table)[Symbol(L"-seventh ")] = Symbol(L"-7 ");
	_keys[_num_keys++] = Symbol(L"-seventh ");
	(*_table)[Symbol(L"-eighth ")] = Symbol(L"8 ");
	_keys[_num_keys++] = Symbol(L"-eighth ");
	(*_table)[Symbol(L"-ninth ")] = Symbol(L"9 ");
	_keys[_num_keys++] = Symbol(L"-ninth ");
	(*_table)[Symbol(L"-")] = Symbol(L" - ");
	_keys[_num_keys++] = Symbol(L"-");
	(*_table)[Symbol(L". ")] = Symbol(L" ");
	_keys[_num_keys++] = Symbol(L". ");
	(*_table)[Symbol(L" twenties ")] = Symbol(L" 20s ");
	_keys[_num_keys++] = Symbol(L" twenties ");
	(*_table)[Symbol(L" thirties ")] = Symbol(L" 30s ");
	_keys[_num_keys++] = Symbol(L" thirties ");
	(*_table)[Symbol(L" forties ")] = Symbol(L" 40s ");
	_keys[_num_keys++] = Symbol(L" forties ");
	(*_table)[Symbol(L" fifties ")] = Symbol(L" 50s ");
	_keys[_num_keys++] = Symbol(L" fifties ");
	(*_table)[Symbol(L" sixties ")] = Symbol(L" 60s ");
	_keys[_num_keys++] = Symbol(L" sixties ");
	(*_table)[Symbol(L" seventies ")] = Symbol(L" 70s ");
	_keys[_num_keys++] = Symbol(L" seventies ");
	(*_table)[Symbol(L" eighties ")] = Symbol(L" 80s ");
	_keys[_num_keys++] = Symbol(L" eighties ");
	(*_table)[Symbol(L" nineties ")] = Symbol(L" 90s ");
	_keys[_num_keys++] = Symbol(L" nineties ");
	(*_table)[Symbol(L" hundreds ")] = Symbol(L" 100s ");
	_keys[_num_keys++] = Symbol(L" hundreds ");
	(*_table)[Symbol(L" one ")] = Symbol(L" 1 ");
	_keys[_num_keys++] = Symbol(L" one ");
	(*_table)[Symbol(L" two ")] = Symbol(L" 2 ");
	_keys[_num_keys++] = Symbol(L" two ");
	(*_table)[Symbol(L" three ")] = Symbol(L" 3 ");
	_keys[_num_keys++] = Symbol(L" three ");
	(*_table)[Symbol(L" four ")] = Symbol(L" 4 ");
	_keys[_num_keys++] = Symbol(L" four ");
	(*_table)[Symbol(L" five ")] = Symbol(L" 5 ");
	_keys[_num_keys++] = Symbol(L" five ");
	(*_table)[Symbol(L" six ")] = Symbol(L" 6 ");
	_keys[_num_keys++] = Symbol(L" six ");
	(*_table)[Symbol(L" seven ")] = Symbol(L" 7 ");
	_keys[_num_keys++] = Symbol(L" seven ");
	(*_table)[Symbol(L" eight ")] = Symbol(L" 8 ");
	_keys[_num_keys++] = Symbol(L" eight ");
	(*_table)[Symbol(L" nine ")] = Symbol(L" 9 ");
	_keys[_num_keys++] = Symbol(L" nine ");
	(*_table)[Symbol(L" ten ")] = Symbol(L" 10 ");
	_keys[_num_keys++] = Symbol(L" ten ");
	(*_table)[Symbol(L" eleven ")] = Symbol(L" 11 ");
	_keys[_num_keys++] = Symbol(L" eleven ");
	(*_table)[Symbol(L" twelve ")] = Symbol(L" 12 ");
	_keys[_num_keys++] = Symbol(L" twelve ");
	(*_table)[Symbol(L" thirteen ")] = Symbol(L" 13 ");
	_keys[_num_keys++] = Symbol(L" thirteen ");
	(*_table)[Symbol(L" fourteen ")] = Symbol(L" 14 ");
	_keys[_num_keys++] = Symbol(L" fourteen ");
	(*_table)[Symbol(L" fifteen ")] = Symbol(L" 15 ");
	_keys[_num_keys++] = Symbol(L" fifteen ");
	(*_table)[Symbol(L" sixteen ")] = Symbol(L" 16 ");
	_keys[_num_keys++] = Symbol(L" sixteen ");
	(*_table)[Symbol(L" seventeen ")] = Symbol(L" 17 ");
	_keys[_num_keys++] = Symbol(L" seventeen ");
	(*_table)[Symbol(L" eighteen ")] = Symbol(L" 18 ");
	_keys[_num_keys++] = Symbol(L" eighteen ");
	(*_table)[Symbol(L" nineteen ")] = Symbol(L" 19 ");
	_keys[_num_keys++] = Symbol(L" nineteen ");
	(*_table)[Symbol(L" twenty ")] = Symbol(L" 20 ");
	_keys[_num_keys++] = Symbol(L" twenty ");
	(*_table)[Symbol(L" thirty ")] = Symbol(L" 30 ");
	_keys[_num_keys++] = Symbol(L" thirty ");
	(*_table)[Symbol(L" forty ")] = Symbol(L" 40 ");
	_keys[_num_keys++] = Symbol(L" forty ");
	(*_table)[Symbol(L" fifty ")] = Symbol(L" 50 ");
	_keys[_num_keys++] = Symbol(L" fifty ");
	(*_table)[Symbol(L" sixty ")] = Symbol(L" 60 ");
	_keys[_num_keys++] = Symbol(L" sixty ");
	(*_table)[Symbol(L" seventy ")] = Symbol(L" 70 ");
	_keys[_num_keys++] = Symbol(L" seventy ");
	(*_table)[Symbol(L" eighty ")] = Symbol(L" 80 ");
	_keys[_num_keys++] = Symbol(L" eighty ");
	(*_table)[Symbol(L" ninety ")] = Symbol(L" 90 ");
	_keys[_num_keys++] = Symbol(L" ninety ");
	(*_table)[Symbol(L" first ")] = Symbol(L" 1 ");
	_keys[_num_keys++] = Symbol(L" first ");
	(*_table)[Symbol(L" second ")] = Symbol(L" 2 ");
	_keys[_num_keys++] = Symbol(L" second ");
	(*_table)[Symbol(L" third ")] = Symbol(L" 3 ");
	_keys[_num_keys++] = Symbol(L" third ");
	(*_table)[Symbol(L" fourth ")] = Symbol(L" 4 ");
	_keys[_num_keys++] = Symbol(L" fourth ");
	(*_table)[Symbol(L" fifth ")] = Symbol(L" 5 ");
	_keys[_num_keys++] = Symbol(L" fifth ");
	(*_table)[Symbol(L" sixth ")] = Symbol(L" 6 ");
	_keys[_num_keys++] = Symbol(L" sixth ");
	(*_table)[Symbol(L" seventh ")] = Symbol(L" 7 ");
	_keys[_num_keys++] = Symbol(L" seventh ");
	(*_table)[Symbol(L" eighth ")] = Symbol(L" 8 ");
	_keys[_num_keys++] = Symbol(L" eighth ");
	(*_table)[Symbol(L" ninth ")] = Symbol(L" 9 ");
	_keys[_num_keys++] = Symbol(L" ninth ");
	(*_table)[Symbol(L" tenth ")] = Symbol(L" 10 ");
	_keys[_num_keys++] = Symbol(L" tenth ");
	(*_table)[Symbol(L" eleventh ")] = Symbol(L" 11 ");
	_keys[_num_keys++] = Symbol(L" eleventh ");
	(*_table)[Symbol(L" twelfth ")] = Symbol(L" 12 ");
	_keys[_num_keys++] = Symbol(L" twelfth ");
	(*_table)[Symbol(L" thirteenth ")] = Symbol(L" 13 ");
	_keys[_num_keys++] = Symbol(L" thirteenth ");
	(*_table)[Symbol(L" fourteenth ")] = Symbol(L" 14 ");
	_keys[_num_keys++] = Symbol(L" fourteenth ");
	(*_table)[Symbol(L" fifteenth ")] = Symbol(L" 15 ");
	_keys[_num_keys++] = Symbol(L" fifteenth ");
	(*_table)[Symbol(L" sixteenth ")] = Symbol(L" 16 ");
	_keys[_num_keys++] = Symbol(L" sixteenth ");
	(*_table)[Symbol(L" seventeenth ")] = Symbol(L" 17 ");
	_keys[_num_keys++] = Symbol(L" seventeenth ");
	(*_table)[Symbol(L" eighteenth ")] = Symbol(L" 18 ");
	_keys[_num_keys++] = Symbol(L" eighteenth ");
	(*_table)[Symbol(L" nineteenth ")] = Symbol(L" 19 ");
	_keys[_num_keys++] = Symbol(L" nineteenth ");
	(*_table)[Symbol(L" twentieth ")] = Symbol(L" 20 ");
	_keys[_num_keys++] = Symbol(L" twentieth ");
	(*_table)[Symbol(L" thirtieth ")] = Symbol(L" 30 ");
	_keys[_num_keys++] = Symbol(L" thirtieth ");
	(*_table)[Symbol(L" of ")] = Symbol(L" ");
	_keys[_num_keys++] = Symbol(L" of ");
	//(*_table)[Symbol(L" the ")] = Symbol(L" ");
	//_keys[_num_keys++] = Symbol(L" the ");
	(*_table)[Symbol(L" in ")] = Symbol(L" ");
	_keys[_num_keys++] = Symbol(L" in ");
	(*_table)[Symbol(L" between ")] = Symbol(L" ");
	_keys[_num_keys++] = Symbol(L" between ");
	(*_table)[Symbol(L" during ")] = Symbol(L" ");
	_keys[_num_keys++] = Symbol(L" during ");
	(*_table)[Symbol(L" on ")] = Symbol(L" ");
	_keys[_num_keys++] = Symbol(L" on ");
	(*_table)[Symbol(L" and ")] = Symbol(L" ");
	_keys[_num_keys++] = Symbol(L" and ");
	(*_table)[Symbol(L" a couple ")] = Symbol(L" 2 ");
	_keys[_num_keys++] = Symbol(L" a couple ");
	(*_table)[Symbol(L" a ")] = Symbol(L" 1 ");
	_keys[_num_keys++] = Symbol(L" a ");
	(*_table)[Symbol(L" an ")] = Symbol(L" 1 ");
	_keys[_num_keys++] = Symbol(L" an ");
	(*_table)[Symbol(L" couple ")] = Symbol(L" 2 ");
	_keys[_num_keys++] = Symbol(L" couple ");
	(*_table)[Symbol(L" hundred ")] = Symbol(L" 100 ");
	_keys[_num_keys++] = Symbol(L" hundred ");
	(*_table)[Symbol(L" thousand ")] = Symbol(L" 1000 ");
	_keys[_num_keys++] = Symbol(L" thousand ");


	// Holidays - add more
	(*_table)[Symbol(L" christmas ")] = Symbol(L" 12 / 25 ");
	_keys[_num_keys++] = Symbol(L" christmas ");
	(*_table)[Symbol(L" halloween ")] = Symbol(L" 10 / 31 ");
	_keys[_num_keys++] = Symbol(L" halloween ");
	
}
