/*
 * Zone.h
 *
 *  Created on: Apr 14, 2014
 *      Author: fhuang
 */


#ifndef ZONE_H_
#define ZONE_H_


#include "Generic/common/Symbol.h"
#include "Generic/common/limits.h"
#include "Generic/theories/Theory.h"
#include "Generic/state/XMLStrings.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>


#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif


class Document;
class LocatedString;
typedef boost::shared_ptr<LocatedString> LSPtr;
class SERIF_EXPORTED Zone : public Theory {

private:

	Symbol _type; // The type of the Zone
	LocatedString* _content; // The cleaned document text; determines start/end offsets and may skip child Zones; Zone takes ownership of it, since it was presumably created by LocatedString::substring()
	std::vector<Zone*> _children; // Any zones immediately contained by this one; could be empty; 
	std::map<std::wstring, LSPtr> _attributes; // Attributes of the zone in the format <attribute_key, attribute_value>.
	LSPtr _author;
	LSPtr _datetime;
public:

	EDTOffset getStartEDTOffset() const;
	EDTOffset getEndEDTOffset() const;

	Zone(const Symbol& type,const std::map<std::wstring,LSPtr>& attributes, const std::vector<Zone*>& children, LocatedString* content);
	Zone(const Symbol& type,const std::map<std::wstring,LSPtr>& attributes, LSPtr author, LSPtr datetime, const std::vector<Zone*>& children, LocatedString* content);

	virtual ~Zone();

	const Symbol& getType() const { return _type; }

	const std::vector<Zone*>& getChildren() const{ return _children;}

	bool hasAttribute(const std::wstring& key) const;
	
	LSPtr getAttribute(const std::wstring& key) const;

	template<typename OffsetType>
	OffsetType start() const ;
	template<typename OffsetType>
	OffsetType end() const ;

	bool parentOf(const Zone* other, bool direct ) const;

	bool childOf(const Zone* other, bool direct) const {
		return other->parentOf(this, direct);
	}
	
	LSPtr getAuthor() const{ return _author;}
	LSPtr getDatetime() const {return _datetime;}



	//LocatedString* getString() {return  _content;}
	const LocatedString* getString()const {return  _content;}
	

	template<typename OffsetType>
	boost::optional<Zone*> findZone(OffsetType offset);

	
	template<typename OffsetType>
	bool contains(OffsetType offset) const;
	void toLowerCase();
	

	const wchar_t* XMLIdentifierPrefix() const {
	  return L"zone";
	} 

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;

	explicit Zone(SerifXML::XMLTheoryElement elem);


};

#endif /* ZONE_H_ */
