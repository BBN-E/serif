#ifndef ZONING_H_
#define ZONING_H_


#include "Generic/theories/Zone.h"
#include <boost/optional.hpp>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif


class Document;
class Region;

class SERIF_EXPORTED Zoning : public Theory {
private:
	
	std::vector<Zone*> _roots;
	void createFlatZones(std::vector<Zone*> flatZones, Zone* root) const;
	std::vector<Zone*> createFlatZones() const;
public:
	
	Zoning(const std::vector<Zone*>& roots);
	Zoning(SerifXML::XMLTheoryElement zoningElem);
	Zoning(void);
	~Zoning(void);


	const std::vector<Zone*>& getRoots() {return _roots;}

	int getSize() const{

		return static_cast<int>(_roots.size());
	}

	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver)const;
	void resolvePointers(StateLoader * stateLoader);
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0)const;
	
	const wchar_t* XMLIdentifierPrefix() const {
	  return L"zoning";
	} 

	void toLowerCase();
	
	bool assertZonesCompatibleWithRegions(Region ** regions,  int n_regions) const;

	template<typename OffsetType> 
	boost::optional<Zone*> findZone ( OffsetType start_offset) const;

};

#endif /* ZONING_H_ */
