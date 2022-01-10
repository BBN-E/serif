#include "Zoning.h"
#include "Region.h"

class Region;
Zoning::Zoning(const std::vector<Zone*>& roots)
{
	_roots=roots;

}

Zoning::~Zoning(void)
{
	for(unsigned int i=0;i<_roots.size();i++){
		delete _roots[i];
	}
}

void Zoning::toLowerCase() {

	for(unsigned int i=0;i<_roots.size();i++){
		_roots[i]->toLowerCase();
	}
}


void Zoning::saveXML(SerifXML::XMLTheoryElement zoningElem, const Theory *context) const {

}

void Zoning::updateObjectIDTable() const {
	throw InternalInconsistencyException("Zoning::updateObjectIDTable()",
										"Using unimplemented method.");
}

void Zoning::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("Zoning::saveState()",
										"Using unimplemented method.");

}

void Zoning::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("Zoning::resolvePointers()",
										"Using unimplemented method.");
}


Zoning::Zoning(SerifXML::XMLTheoryElement zoningElem)
{	using namespace SerifXML;
	std::vector<Zone*> roots;
	std::vector<XMLTheoryElement> zoneElems = zoningElem.getChildElementsByTagName(X_Zone);

	for(unsigned int i=0;i<zoneElems.size();i++){
	
		roots.push_back( new Zone(zoneElems[i]));
	}
	_roots=roots;
}


template boost::optional<Zone*> Zoning::findZone( EDTOffset start_offset)const ;
template<typename OffsetType> 
boost::optional<Zone*> Zoning::findZone ( OffsetType start_offset) const {
		
	 	for(unsigned int i=0;i<_roots.size();i++){
			Zone* zone=_roots[i];
			
			boost::optional<Zone*> foundZone = zone->findZone<OffsetType>(start_offset);
			if(foundZone){
				return foundZone;
			}
		}
		return boost::optional<Zone*>();

}


void Zoning::createFlatZones(std::vector<Zone*> flatZones, Zone* root) const {
	flatZones.push_back(root);
	std::vector<Zone*> children=root->getChildren();
	for(unsigned int i=0;i<children.size();i++){
		createFlatZones(flatZones,children[i]);
	}
}
std::vector<Zone*> Zoning::createFlatZones() const{
	std::vector<Zone*> flatZones;
	for(unsigned int i=0;i<_roots.size();i++){
		createFlatZones(flatZones,_roots[i]);
	}
	return flatZones;
	
}

bool Zoning::assertZonesCompatibleWithRegions(Region ** regions,  int n_regions) const{
	std::vector<Zone*> flatZones=createFlatZones();
		for(int i=0;i<n_regions;i++){
			Region* region=regions[i];
			EDTOffset region_start=region->getStartEDTOffset();
			EDTOffset region_end=region->getEndEDTOffset();
			int counter=0;
			std::vector<Zone*> overlappedZones;

			for(unsigned int j=0;j<flatZones.size();j++){
				Zone* zone=flatZones[j];
				EDTOffset zone_start=zone->getStartEDTOffset();
				EDTOffset zone_end=zone->getEndEDTOffset();

				if(region_end<zone_start || zone_end<region_start){
					continue;
				}
				else{ //there is an overlap
					overlappedZones.push_back(zone);
					counter+=1;


				}
			
			}
			if(counter>1){
				
				//last one should be the children of the previous one
				Zone* last=overlappedZones.back();
				overlappedZones.pop_back();
				while(overlappedZones.size()>0){
					bool flag=false;
					Zone* second_last=overlappedZones.back();
					overlappedZones.pop_back();
					std::vector<Zone*> children=second_last->getChildren();
					for(unsigned int i=0;i<children.size();i++){
						if(last==children[i]){
							flag=true;
							break;
						}
					}
					if(flag==false){
						return false;
					}
					else{
						last=second_last;
					}
				}
			}
		
				
		}
		return true;
	
}
