/*
 * Zone.cpp
 *
 *  Created on: Apr 14, 2014
 *      Author: fhuang
 */
 

#include "Zone.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/common/Offset.h"

#include <boost/algorithm/string.hpp>
#include "Generic/state/XMLStrings.h"
#include <boost/optional.hpp>
#include "Generic/common/LocatedString.h"

	Zone::Zone(const Symbol& type,const std::map<std::wstring,LSPtr>& attributes,const std::vector<Zone*>& children, LocatedString* content)
		: _type(type), _content(content), _attributes(attributes), _children(children){}

	
	Zone::Zone(const Symbol& type,const std::map<std::wstring,LSPtr>& attributes,LSPtr author, 
				LSPtr datetime,const std::vector<Zone*>& children, LocatedString* content)
				: _type(type), _author(author), _datetime(datetime),_content(content), _attributes(attributes), _children(children){}




	Zone::~Zone() {
		delete _content;
		for(unsigned int i=0;i<_children.size();i++){
			delete _children[i];
		}
	}

	void Zone::toLowerCase() {
		if (_content != 0){
			_content->toLowerCase();
		}
		for(unsigned int i=0;i<getChildren().size();i++){
			getChildren()[i]->toLowerCase();
		}
	}

	EDTOffset Zone::getStartEDTOffset() const { return _content->start<EDTOffset>(); }
	EDTOffset Zone::getEndEDTOffset() const { return _content->end<EDTOffset>(); }


	template<typename OffsetType>
	OffsetType Zone::start() const { return _content->start<OffsetType>(); }
	template<typename OffsetType>
	OffsetType Zone::end() const { return _content->end<OffsetType>(); }


	template boost::optional<Zone*> Zone::findZone<EDTOffset>(EDTOffset offset);
	template<typename OffsetType>
	boost::optional<Zone*> Zone::findZone(OffsetType offset)  {
		// Check if this offset is in this zone
		if (!contains(offset)){
			return boost::optional<Zone*>();
		}
		for(unsigned int i=0;i<_children.size();i++) {
		   Zone* child=_children[i];
			boost::optional<Zone*> zone = child->findZone<OffsetType>(offset);
			if (zone) {
				// For now, always favor a more specific zone's attribute
				return zone;
			}
		}

		// No matching child zones found, return the current zone
	   return boost::optional<Zone*>(this);
	}


	template<typename OffsetType>
	bool Zone::contains(OffsetType offset) const {

		return start<OffsetType>() <= offset && end<OffsetType>() >= offset;
	} 


	bool Zone::hasAttribute(const std::wstring& key) const {
			
		return _attributes.find(key)!=_attributes.end();
	}

	LSPtr Zone::getAttribute(const std::wstring& key) const {
		if(hasAttribute(key)){
			return _attributes.find(key)->second;
		} else {
			return LSPtr();
		}
	}

	bool Zone::parentOf(const Zone* other, bool direct ) const {
		for(std::vector<Zone*>::const_iterator child = _children.begin(); child != _children.end(); ++child) {
	 
			if (*child == other) {
				return true;
			}
			if (!direct && (*child)->parentOf(other,direct)) {
				return true;
			}
		}
		return false;
	}


	void Zone::saveXML(SerifXML::XMLTheoryElement zoneElem, const Theory *context) const {
		using namespace SerifXML;
		// RMG: always use braces
		if (context != 0){
			throw InternalInconsistencyException("Zone::saveXML", "Expected context to be NULL");
		}
		if (!getType().is_null()) {
			zoneElem.setAttribute(X_type, getType());

		}
		
		for (std::map<std::wstring, LSPtr>::const_iterator iter = _attributes.begin(); iter != _attributes.end(); ++iter) {
			SerifXML::XMLTheoryElement zoneAttribute=	zoneElem.addChild(X_LocatedZoneAttribute);
			
			zoneAttribute.setAttribute(X_name,iter->first);
			iter->second.get()->saveXML(zoneAttribute);
		}
		if(_author){
			SerifXML::XMLTheoryElement authorAttribute=	zoneElem.addChild(X_author);
			_author.get()->saveXML(authorAttribute);
		}
		if(_datetime){
			SerifXML::XMLTheoryElement datetimeAttribute=	zoneElem.addChild(X_datetime);
			_datetime.get()->saveXML(datetimeAttribute);
		}


		for(unsigned int i=0;i<_children.size();i++){
			zoneElem.saveChildTheory(X_Zone,_children[i]);
		}

			getString()->saveXML(zoneElem);

	}

	void Zone::updateObjectIDTable() const {
		throw InternalInconsistencyException("Zone::updateObjectIDTable()",
											"Using unimplemented method.");
	}

	void Zone::saveState(StateSaver *stateSaver) const {
		throw InternalInconsistencyException("Zone::saveState()",
											"Using unimplemented method.");
	}

	void Zone::resolvePointers(StateLoader * stateLoader) {
		throw InternalInconsistencyException("Zone::resolvePointers()",
											"Using unimplemented method.");
	}

	Zone::Zone(SerifXML::XMLTheoryElement zoneElem)
	{	using namespace SerifXML;
		
		std::vector<XMLTheoryElement> attributes=zoneElem.getChildElementsByTagName(X_LocatedZoneAttribute);
		for(unsigned int i=0;i<attributes.size();i++){
			Symbol attributeName=attributes[i].getAttribute<Symbol>(X_name, Symbol());
			LSPtr attributeValue(new LocatedString(attributes[i]));
			_attributes.insert(std::make_pair(std::wstring(attributeName.to_string()), attributeValue));
		}

		std::vector<XMLTheoryElement> authorAttributes=zoneElem.getChildElementsByTagName(X_author);
		if(!authorAttributes.empty()){
			if(authorAttributes.size()!=1){
				throw UnexpectedInputException("Zone::Zone()","Each zone can only have one author");
			}
			_author=LSPtr(new LocatedString(authorAttributes[0]));
		}
		std::vector<XMLTheoryElement> datetimeAttributes=zoneElem.getChildElementsByTagName(X_datetime);
		if(!datetimeAttributes.empty()){
			if(datetimeAttributes.size()!=1){
				throw UnexpectedInputException("Zone::Zone()","Each zone can only have one datetime");
			}
			_datetime= LSPtr(new LocatedString(datetimeAttributes[0]));
		}
		
		
		zoneElem.loadId(this);
		_type = zoneElem.getAttribute<Symbol>(X_type, Symbol());
		std::vector<XMLTheoryElement> children=zoneElem.getChildElementsByTagName(X_Zone);
		for(unsigned int i=0;i<children.size();i++ ){
			Zone* childZone=new Zone(children[i]);
			_children.push_back(childZone);
		}
		_content = new LocatedString(zoneElem);

	}

