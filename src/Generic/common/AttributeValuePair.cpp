
#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/AttributeValuePair.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"

std::wstring AttributeValuePairBase::getFullName(Symbol extractorName, Symbol key) {
	std::wstring result;
	result += std::wstring(extractorName.to_string());
	result += L":";
	result += std::wstring(key.to_string());
	return result;
}

std::wstring AttributeValuePairBase::getFullName() const {
	return AttributeValuePairBase::getFullName(_extractorName, _key);
}

template <class ValueType>
bool AttributeValuePair<ValueType>::equals(const AttributeValuePair_ptr other) const { 
	if (boost::shared_ptr< AttributeValuePair<ValueType> > p = boost::dynamic_pointer_cast< AttributeValuePair<ValueType> >(other))	
		return equals(p);
	else
		return false;
}

template <class ValueType>
bool AttributeValuePair<ValueType>::equals(const boost::shared_ptr< AttributeValuePair<ValueType> > other) const {
	return (_extractorName == other->getExtractorName() && _key == other->getKey() && _value == other->getValue()); 
}

template<>
bool AttributeValuePair<const SynNode*>::equals(const boost::shared_ptr< AttributeValuePair<const SynNode*> > other) const {
	return (_extractorName == other->getExtractorName() && _key == other->getKey() && _value == other->getValue()); 
}

template<>
bool AttributeValuePair<const Proposition*>::equals(const boost::shared_ptr< AttributeValuePair<const Proposition*> > other) const {
	return (_extractorName == other->getExtractorName() && _key == other->getKey() && _value->getID() == other->getValue()->getID()); 
}

template<>
bool AttributeValuePair<const Mention*>::equals(const boost::shared_ptr< AttributeValuePair<const Mention*> > other) const {
	return (_extractorName == other->getExtractorName() && _key == other->getKey() && _value->getUID() == other->getValue()->getUID()); 
}

template <class ValueType>
bool AttributeValuePair<ValueType>::valueEquals(const AttributeValuePair_ptr other) const { 
	if (boost::shared_ptr< AttributeValuePair<ValueType> > p = boost::dynamic_pointer_cast< AttributeValuePair<ValueType> >(other))	
		return valueEquals(p);
	else
		return false;
}

template <class ValueType>
bool AttributeValuePair<ValueType>::valueEquals(const boost::shared_ptr< AttributeValuePair<ValueType> > other) const {
	return (_value == other->getValue()); 
}

template <class ValueType>
std::wstring AttributeValuePair<ValueType>::toString() const {
		std::wstring result;
		result += getFullName();
		result += L" ";
		result += L"<unprintable-value>";
		return result;
}

template<> 
std::wstring AttributeValuePair<float>::toString() const {
	std::wstringstream result;
	result << L"[" << getFullName() << L" " << _value << L"]";
	return result.str();
}

template<> 
std::wstring AttributeValuePair<int>::toString() const {
	std::wstringstream result;
	result << L"[" << getFullName() << L" " << _value << L"]";
	return result.str();
}

template<> 
std::wstring AttributeValuePair<Symbol>::toString() const {
	std::wstringstream result;
	result << L"[" << getFullName() << L" '" << _value.to_string() << L"']";
	return result.str();
}

template<> 
std::wstring AttributeValuePair<const Proposition*>::toString() const {
	std::wstringstream result;
	result << L"[" << getFullName() << L" " << _value->getID() << L"]";
	return result.str();
}

template<> 
std::wstring AttributeValuePair<const Mention*>::toString() const {
	std::wstringstream result;
	result << L"[" << getFullName() << L" " << _value->getUID() << L"]";
	return result.str();
}

template<> 
std::wstring AttributeValuePair<const SynNode*>::toString() const {
	std::wstringstream result;
	result << L"[" << getFullName() << L" " << _value->toFlatString() << L"]";
	return result.str();
}

template class AttributeValuePair<bool>;
template class AttributeValuePair<float>;
template class AttributeValuePair<int>;
template class AttributeValuePair<Symbol>;
template class AttributeValuePair<const Proposition*>;
template class AttributeValuePair<const Mention*>;
template class AttributeValuePair<const SynNode*>;
