// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_FEATURE_TYPE
#define D_T_FEATURE_TYPE

#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"

class DTState;
class DTFeature;

/** DTFeatureType is an abstract class whose subclasses each represent
  * types of DTFeatures, and are able to extract that feature type, write
  * a representation of such a feature to a stream, and populate such a
  * feature from that representation.
  *
  * Each DTFeatureType subclass has a name, which is referenced in
  * training parameters and model files.
  *
  * One instance of each DTFeatureType subclass is created upon
  * initialization and stored in a hashtable, keyed on its name. This
  * instance is also referred to by DTFeatures, so they know which feature
  * type they correspond to.
  */

class DTFeatureType {
public:
	/** This class holds the types of information used by the feature type to extract features.
	  * This will help improve the speed of PDecoder.
	  * 
	  * Values
	  * ------
	  * InfoSource::OBSERVATION   this feature type extracts the information from the observation.
	  * InfoSource::PREV_TAG      this feature type extracts the information from the previous tag.
	  */
	class InfoSource {
	public:
		InfoSource(const unsigned short i) { SetBySingleValue(i); }
		operator unsigned short() const {
			return (_use_observation? OBSERVATION: 0) |	(_use_prev_tag? PREV_TAG: 0);
		};

		static const unsigned short OBSERVATION = 0x0001;
		static const unsigned short PREV_TAG    = 0x0002;
		
		bool useObservation() const { return _use_observation; }
		bool usePrevTag() const { return _use_prev_tag; }

	private:
		void SetBySingleValue(const unsigned short i) {
			_use_observation = ((i & OBSERVATION) == OBSERVATION);
			_use_prev_tag = ((i & PREV_TAG) == PREV_TAG);
		};

		bool _use_observation;
		bool _use_prev_tag;
	};

protected:
	InfoSource _infoSource;

private:
	static Symbol getFullName(Symbol model, Symbol name) {
		wchar_t buffer[1000];
		wcscpy(buffer, model.to_string());
		wcscat(buffer, L"-");
		wcscat(buffer, name.to_string());
		return Symbol(buffer);
	}
public:
	static const int MAX_FEATURES_PER_EXTRACTION = 100;

	DTFeatureType(Symbol model, Symbol name, InfoSource infoSource = InfoSource::OBSERVATION | InfoSource::PREV_TAG)
		: _name(name), _model(model), _infoSource(infoSource) {
		registerFeatureType(model, this);
	}
	virtual ~DTFeatureType() {}

	Symbol getName() const { return _name; }
	Symbol getModel() const { return _model;}

	/** Create empty DTFeature of whatever subclass corresponds to
	  * this subclass of DTFeatureType. This is used for reading features
	  * in from model files.
	  *
	  * @return A new DTFeature, allocated on the heap(!) */
	virtual DTFeature *makeEmptyFeature() const = 0;

	/** Extract features from tagger state.
	  * This method must populate resultArray with 0 to
	  * MAX_FEATURES_PER_EXTRACTION features, and return that number.
	  *
	  * Note that the new DTFeatures are allocated on the heap. */
	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const = 0;

	static void registerFeatureType(Symbol model, DTFeatureType *featureType) {
		// XXX: Should make sure it's not already there
		Symbol fullname = getFullName(model, featureType->getName());
		if(_featureTypes[fullname] !=0){
			char message[500];
			strcpy(message, "DTFeatureType:registerFeatureType: feature has already been defined ");
			strncat(message, fullname.to_debug_string(), 50);
			throw UnexpectedInputException("DTFeatureType:registerFeatureType()", message);
		}
		_featureTypes[getFullName(model, featureType->getName())] = featureType;
	}
	static DTFeatureType *getFeatureType(Symbol model, Symbol name) {
		DTFeatureType **result = _featureTypes.get(getFullName(model,name));
		if (result == 0)
			return 0;
		else{
			return *result;
		}
	}

	/** this method is used to validate that the feature has it's
	 * required information. It is called only on the actually used features.
	 * features that do not require special parameters can use the empty
	 * implementation. Others can implement and throw an exception.
	*/
	virtual void validateRequiredParameters(){};

	const InfoSource* getInfoSource() const { return &_infoSource; }

private:
	Symbol _name;
	Symbol _model;

	// FeatureTypeMap is basically just a hash-map from Symbol to DTFeatureType*, except
	// that we add a custom destructor that automatically releases the memory for all
	// registered feature types when the static _featureTypes mapping is destroyed.
	// Note: hash_map does not have a virtual destructor.  This should be fine as long as
	// FeatureTypeMap (and its derived class, which is currently none) is never deleted
	// via a pointer to its parent class.
	class FeatureTypeMap: public serif::hash_map<Symbol, DTFeatureType*, Symbol::Hash, Symbol::Eq> {
	public:
		FeatureTypeMap() {
		}
		~FeatureTypeMap() {
			for (iterator i=begin(); i != end(); ++i) {
				delete (*i).second;
			}
		}
	};
	static FeatureTypeMap _featureTypes;

};

#endif
