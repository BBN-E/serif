// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


/* Reads in a file in the following format:

# FREQ  HEAD, COMPLEX HEAD, FULL HEAD, INSTANCE

  12416	leader , leader , Palestinian leader , Yasser Arafat 
   5772	leader , leader , Bosnian Serb leader , Radovan Karadzic 
   3839	spokesman , spokesman , White House spokesman , Mike McCurry 
   3660	editor , Foreign editor , Foreign editor , Rick Christie 
   3654	editor , News editor , News editor , Art Dalglish 
   3228	spokesman , spokesman , State Department spokesman , Nicholas Burns 
   3089	spokesman , spokesman , White House spokesman , Marlin Fitzwater 
   2528	leader , leader , PLO leader , Yasser Arafat 
   2157	lady , lady , first lady , Hillary Rodham Clinton 

Containing world knowledge descriptor/head word links.
A file contaning mostly person names found by mostly-automatic annotation 
is here: \\traid01\speed\serif\data\edt\wk-instances.txt

*/

#include "common/leak_detection.h"

#include "edt/NameToDescriptorMap.h"
#include "common/UTF8Token.h"
#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "WordClustering/WordClusterClass.h"
#include <boost/scoped_ptr.hpp>

NameToDescriptorMap::StringToStringMap NameToDescriptorMap::_headWordMap;
NameToDescriptorMap::StringToStringMap NameToDescriptorMap::_lastNameHeadWordMap;
bool NameToDescriptorMap::_initialized;

void NameToDescriptorMap::initialize()
{
	if (_initialized == true) return;

	char filename[501];
	if (!ParamReader::getParam("world_knowledge_name_descriptor_links", filename, 500)) {
		_initialized = false;
		return;
	}

	_initialized = true;

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8Token tok;
	Symbol delim = Symbol(L","); 
	wstring line = L"";

	while (!in.eof()) {
		in.getLine(line);
		size_t comma1 = line.find(L" , ");
		size_t comma2 = line.find(L" , ", comma1 + 3);
		size_t comma3 = line.find(L" , ", comma2 + 3);

		if (comma3 == wstring::npos) continue;

		wstring head_word = line.substr(8, comma1 - 8);
		head_word = safeToLower(head_word);

		wstring core_np = line.substr(comma1 + 3, comma2 - comma1 - 3);
		core_np = safeToLower(core_np);

		wstring full_np = line.substr(comma2 + 3, comma3 - comma2 - 3);
		full_np = safeToLower(full_np);

		wstring name = line.substr(comma3 + 3, line.length() - comma3 - 4);
		name = safeToLower(name);

		name = removeTitles(name);

		// single word names need not be recorded, too unreliable
		if (name.find(L" ") == wstring::npos) continue;

		addToMap(_headWordMap, name, head_word);

		wstring last_name = findLastName(name, true);
		if (last_name.length() == 0) continue;

		addToMap(_lastNameHeadWordMap, last_name, head_word);
	}

	in.close();
}

void NameToDescriptorMap::destroy()
{
	if (!_initialized)
		return;

	StringToStringMap::iterator iter;
	for (iter = _headWordMap.begin();
		 iter != _headWordMap.end();
		 ++iter)
	{
		// destroy all elements of linked list
		WstringElement *elem = (*iter).second; // first element of list
		do {
			WstringElement *next = elem->next;
			delete elem;
			elem = next;
		} while (elem != 0);
	}

	for (iter = _lastNameHeadWordMap.begin();
		 iter != _lastNameHeadWordMap.end();
		 ++iter)
	{
		// destroy all elements of linked list
		WstringElement *elem = (*iter).second; // first element of list
		do {
			WstringElement *next = elem->next;
			delete elem;
			elem = next;
		} while (elem != 0);
	}
}


void NameToDescriptorMap::addToMap(StringToStringMap &dataMap, wstring &key, wstring &value)
{
	WstringElement **currentValue_p = dataMap.get(key);
	if (currentValue_p == 0) {
		WstringElement *newElem = _new WstringElement();
		newElem->next = 0;
		newElem->str = value; 
		dataMap[key] = newElem;
		//wprintf(L"Created new entry - Name: %s, Desc: %s\n", key.c_str(), value.c_str());
		return;
	}
	if (!lookup(dataMap, key, value)) {
		WstringElement *elem = dataMap[key];
		int i = 0;
		while (elem->next != 0) {
			i++;
			elem = elem->next;
		}

		WstringElement *newElem = _new WstringElement();
		newElem->next = 0;
		newElem->str = value;
		elem->next = newElem;
		//wprintf(L"Added entry - Name: %s, Desc: %s Spot: %d\n", key.c_str(), value.c_str(), i);
	}
}



bool NameToDescriptorMap::lookup(StringToStringMap &dataMap, wstring &key, wstring &value) 
{
	WstringElement **currentValue_p = dataMap.get(key);
	if (currentValue_p == 0) return false;

	WstringElement *elem = dataMap[key];
	do {
		if (!elem->str.compare(value)) 
			return true;
		elem = elem->next;
	} while (elem != 0);

	return false;
}

wstring NameToDescriptorMap::removeTitles(wstring name)
{
	if (!name.substr(0, 3).compare(L"mr ") || 
		!name.substr(0, 3).compare(L"ms ") ||
		!name.substr(0, 3).compare(L"dr "))
	{
		name = name.substr(3);
	}

	if (!name.substr(0, 5).compare(L"miss ") ||
		!name.substr(0, 5).compare(L"prof "))
	{
		name = name.substr(5);
	}

	if (!name.substr(0, 4).compare(L"mrs "))
	{
		name = name.substr(4);
	}

	if (!name.substr(0, 10).compare(L"professor ") ||
		!name.substr(0, 10).compare(L"president "))
	{
		name = name.substr(10);
	}

	if (!name.substr(0, 7).compare(L"doctor "))
	{
		name = name.substr(7);
	}

	return name;

}

wstring NameToDescriptorMap::safeToLower(wstring &str)
{
	wstring lowerStr = L"";
	const wchar_t *ar = str.c_str();
	wchar_t buffer[2];

	for (size_t i = 0; i < str.length(); i++) {
		if (iswupper(ar[i])) {
			buffer[0] = tolower(ar[i]);
			buffer[1] = L'\0';
			lowerStr.append(buffer);
		}
		else {
			buffer[0] = ar[i];
			buffer[1] = L'\0';
			lowerStr.append(buffer);
		}
	}

	return lowerStr;
}

wstring NameToDescriptorMap::findLastName(wstring name, bool strict) 
{	
	wstring last_name(name);

	last_name = removeFromEnd(last_name, L" i");
	last_name = removeFromEnd(last_name, L" ii");
	last_name = removeFromEnd(last_name, L" iii");
	last_name = removeFromEnd(last_name, L" iv");
	last_name = removeFromEnd(last_name, L" v");
	last_name = removeFromEnd(last_name, L" vi");
	last_name = removeFromEnd(last_name, L" vii");
	last_name = removeFromEnd(last_name, L" viii");

	last_name = removeFromEnd(last_name, L" jr");
	last_name = removeFromEnd(last_name, L" sr");

	size_t pos = last_name.find_last_of(L" ");

	if (strict)
	{
		if (pos != wstring.npos && pos < last_name.length() - 1)
		{
			last_name = last_name.substr(pos + 1);
			return last_name;
		}
	}
	else if (!strict)
	{
		int real_pos;
		if (pos == wstring.npos)
			real_pos = 0;
		else 
			real_pos = (int)pos + 1;

		last_name = last_name.substr(real_pos);
		return last_name;
	}

	return wstring(L"");
}

wstring NameToDescriptorMap::removeFromEnd(wstring str, const wchar_t *suffix) 
{
	
	size_t suffix_length = wcslen(suffix);
	
	int diff = (int)str.length() - (int)suffix_length;
	if (diff < 0) diff = 0;

	wstring substring = str.substr(diff);
	if (!substring.compare(suffix) && diff > 0)
		return str.substr(0, str.length() - suffix_length);
	return str;
}

bool NameToDescriptorMap::lookupNameHeadWordLink(wstring &name, wstring &head_word) 
{
	if (!NameToDescriptorMap::_initialized) 
		throw UnexpectedInputException(
			"NameToDescriptorMap::lookupNameHeadWordLink",
			"World Knowledge feature used but world-knowledge-name-descriptor-links param not set");
	
	return lookup(_headWordMap, name, head_word);
}

bool NameToDescriptorMap::lookupLastNameHeadWordLink(wstring &name, wstring &head_word) 
{
	if (!NameToDescriptorMap::_initialized) 
		throw UnexpectedInputException(
			"NameToDescriptorMap::lookupLastNameHeadWordLink",
			"World Knowledge feature used but world-knowledge-name-descriptor-links param not set");

	wstring last_name = findLastName(name, false);
	if (last_name.length() == 0) return false;

	return lookup(_lastNameHeadWordMap, last_name, head_word);
}

int NameToDescriptorMap::getLastNameClusterAgreement(wstring &name, wstring &head_word) 
{
	if (!NameToDescriptorMap::_initialized) 
		throw UnexpectedInputException(
			"NameToDescriptorMap::getLastNameClusterAgreement",
			"World Knowledge feature used but world-knowledge-name-descriptor-links param not set");

	wstring last_name = findLastName(name, false);
	if (last_name.length() == 0) return 0;

	WstringElement **currentValue_p = _lastNameHeadWordMap.get(last_name);
	if (currentValue_p == 0) return 0;

	WordClusterClass headWordClass(Symbol(head_word.c_str()), true);
	
	WstringElement *elem = _lastNameHeadWordMap[last_name];
	int max_agreed = 0;

	do {
		wstring item = elem->str;
		
		WordClusterClass itemWordClass(Symbol(item.c_str()), true);

		int agreed = 0;
		if (itemWordClass.c20() == headWordClass.c20())
			agreed = 20;
		else if (itemWordClass.c16() == headWordClass.c16())
			agreed = 16;
		else if (itemWordClass.c12() == headWordClass.c12())
			agreed = 12;
		else if (itemWordClass.c8() == headWordClass.c8())
			agreed = 8;

		if (agreed > max_agreed) 
			max_agreed = agreed;
		elem = elem->next;
	} while (elem != 0);

	return max_agreed;
}
