#include "Generic/common/leak_detection.h"

#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"

AttributeValuePairExtractorBase::AttributeValuePairExtractorMap AttributeValuePairExtractorBase::_featureExtractors;

AttributeValuePairExtractorBase::AttributeValuePairExtractorBase(Symbol sourceType, Symbol name) : 
	_sourceType(sourceType), _name(name) 
{
	registerExtractor(sourceType, this);
}

void AttributeValuePairExtractorBase::registerExtractor(Symbol sourceType, AttributeValuePairExtractorBase* extractor) {
	Symbol fullName = getFullName(sourceType, extractor->getName());
	if (_featureExtractors.find(fullName) != _featureExtractors.end()) {
		std::wstringstream message;
		message << "Extractor type " << fullName.to_string() << " has already been registered.";
		throw UnexpectedInputException("AttributeValuePairExtractorBase::registerExtractor()", message);
	}
	_featureExtractors[fullName] = extractor;
}

const AttributeValuePairExtractorBase* AttributeValuePairExtractorBase::getExtractor(Symbol sourceType, Symbol name) {
	return getExtractor(getFullName(sourceType, name));
}

const AttributeValuePairExtractorBase* AttributeValuePairExtractorBase::getExtractor(Symbol fullName) {
	AttributeValuePairExtractorMap::const_iterator it = _featureExtractors.find(fullName);
	if (it == _featureExtractors.end()) {
		std::wstringstream message;
		message << "Attempt to get undefined extractor type: " << fullName.to_string();
		throw InternalInconsistencyException("AttributeValuePairExtractorBase::getExtractor()", message);
	} else {
		return (*it).second;
	}
}

Symbol AttributeValuePairExtractorBase::getFullName() const {
	return getFullName(_sourceType, _name);
}

Symbol AttributeValuePairExtractorBase::getFullName(Symbol sourceType, Symbol name) {
	std::wstring full_name = sourceType.to_string();
	full_name.append(L"-");
	full_name.append(name.to_string());
	return Symbol(full_name);
}
