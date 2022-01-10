#include "Generic/common/leak_detection.h"

#include "LocationDB.h"
#include "Generic/common/SessionLogger.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

LocationDB::LocationDB(const std::string dbLocation) {
	// Ignore unspecified parameter
	if (dbLocation == "")
		return;

	// Open the database
	SessionLogger::dbg("location_db") << "Reading Location database " << dbLocation << "..." << std::endl;
	LocationDB::db = new SqliteDB(dbLocation);
	entityMap = new EntityMapType();
	banList = new std::set<std::wstring>();

	Table_ptr uriData;
	uriData = db->exec(L"SELECT Name FROM IgnoreList");
	BOOST_FOREACH(std::vector<std::wstring> row, *uriData) {
		banList->insert(row[0]);
	}

	LocationDB::subgpe_good=0;
	LocationDB::subgpe_bad=0;
	LocationDB::lookup_ambig=0;
	LocationDB::lookup_unambig=0;
	LocationDB::by_default=0;
	LocationDB::by_country=0;
	LocationDB::by_pop=0;
	LocationDB::total=0;
}

LocationDB::~LocationDB(void) {
	//TODO: clear memory
}

void LocationDB::clear() const {
	entityMap->clear();
}

bool compareLocData (LocationData i,LocationData j) { 
	return (i.Population>j.Population); 
}

void LocationDB::addEntity(int entityId, const std::set<std::wstring> &aliases, const std::set<std::wstring> &relations, int mentionCount, bool relationParent) {
	Locations aliasVector;
	LocationDB::total++;
	SessionLogger::dbg("location_db") << L"\nAlias Set for " << boost::lexical_cast<std::wstring>(entityId) << "\n-----------------\n";
	bool good = false; //for debug sub-gpe counting
	BOOST_FOREACH(std::wstring alias, aliases) {
		SessionLogger::dbg("location_db") << L"Alias:" << alias << std::endl;
		if (banList->find(alias) != banList->end()) { //if an alias is in the ban list then skip this entity.
			SessionLogger::dbg("location_db") << L"Banning " << alias << std::endl;
		} else {
			Table_ptr uriData;
			bool something_added = false;
			if (!relations.empty()) {
				//if we have sub-GPE data from the part-whole side or the whole-part side
				if (relationParent) {
					BOOST_FOREACH(std::wstring relation, relations) {
						uriData = db->exec(
							L"SELECT sub.Alias, sub.URI, sub.StateURI, sub.NationURI, CAST(sub.Population AS TEXT), CAST(sub.DefaultLocation AS TEXT),"
								L"CAST(sub.Latitude AS TEXT),CAST(sub.Longitude AS TEXT) "
							L"FROM AliasLookup AS sub "
							L"INNER JOIN AliasLookup AS parent ON sub.StateURI = parent.URI OR sub.NationURI = parent.URI "
							L"WHERE sub.Alias='"+alias+L"' AND parent.Alias='"+relation+L"' ORDER BY sub.Population DESC"
							);
						if (!uriData->empty()) {
							something_added = true;
						}
						LocationDB::addSqlTable(uriData,aliasVector,mentionCount);
					}
				} else {
					BOOST_FOREACH(std::wstring relation, relations) {

						uriData = db->exec(
							L"SELECT parent.Alias, parent.URI, parent.StateURI, parent.NationURI, CAST(parent.Population AS TEXT), CAST(parent.DefaultLocation AS TEXT),"
								L"CAST(parent.Latitude AS TEXT),CAST(parent.Longitude AS TEXT) "
							L"FROM AliasLookup AS sub "
							L"INNER JOIN AliasLookup AS parent ON sub.StateURI = parent.URI OR sub.NationURI = parent.URI "
							L"WHERE parent.Alias='"+alias+L"' AND sub.Alias='"+relation+L"' ORDER BY parent.Population DESC"
							);
						if (!uriData->empty()) {
							something_added = true;
						}
						LocationDB::addSqlTable(uriData,aliasVector,mentionCount);
					}
				}
			}

			if (relations.empty() || !something_added) { //no sub-GPE relation or sub-GPE relation was not found in gazetteer
				uriData = db->exec(
					L"SELECT Alias, URI, StateURI, NationURI, CAST(Population AS TEXT), CAST(DefaultLocation AS TEXT),"
						L"CAST(Latitude AS TEXT),CAST(Longitude AS TEXT) "
					L"FROM AliasLookup "
					L"WHERE Alias='"+alias+L"' ORDER BY Population DESC"
					);
				LocationDB::addSqlTable(uriData,aliasVector,mentionCount);
			} else {
				good = true;
			}
		}
	}

	if (!relations.empty()) {
		if (good) {
			LocationDB::subgpe_good++;
		} else {
			LocationDB::subgpe_bad++;
		}
	}
	
	//disambiguate the easy cases (default and country) immediately
	(*entityMap)[entityId] = LocationDB::easyDisambiguate(aliasVector);
}

void LocationDB::addSqlTable(const Table_ptr uriData, Locations &aliasVector, int mentionCount) const {
	BOOST_FOREACH(std::vector<std::wstring> row, *uriData) {
		// Get the values from the database table
		LocationData ld;
		ld.URI = row[1];
		ld.StateURI = row[2];
		ld.NationURI = row[3];
		ld.Population = boost::lexical_cast<int>(row[4]);
		if (ld.Population == -1) {
			ld.Population = DEFAULT_POP;
		}
		ld.Default = (boost::lexical_cast<int>(row[5])==1);
		ld.Latitude = (boost::lexical_cast<double>(row[6])/DEGREES_TO_RADIANS);
		ld.Longitude = (boost::lexical_cast<double>(row[7])/DEGREES_TO_RADIANS);
		ld.MentionCount = mentionCount;
		if ((ld.Latitude == 0) && (ld.Longitude == 0)) {
			Table_ptr uriData2 = db->exec(L"SELECT Latitude,Longitude FROM AliasLookup WHERE URI='"+ld.NationURI+L"'");
			ld.Latitude = boost::lexical_cast<double>((*uriData2)[0][0]);
			ld.Longitude = boost::lexical_cast<double>((*uriData2)[0][1]);
		}

		//don't add duplicates
		bool add_row = true;
		BOOST_FOREACH(LocationData ld2, aliasVector) {
			if (ld2.URI == ld.URI) {
				add_row = false;
				break;
			}
		}
		if (add_row)
			aliasVector.push_back(ld);
	}
}

//disambiguation by default and country, also add up state and nation counts for later disambiguation
Locations LocationDB::easyDisambiguate(Locations aliasVector) {
	if (aliasVector.size() > 1) {
		LocationDB::lookup_ambig++;
		Locations result;
		for (int i=0;i<(int)aliasVector.size();i++) {
			if (aliasVector[i].Default) {  // if there is a default, that becomes the only result
				result.clear();
				result.push_back(aliasVector[i]);
				LocationDB::by_default++;
				break;
			}
			else if (aliasVector[i].URI == aliasVector[i].NationURI) { //if it's a country, that becomes the only result
				result.clear();
				result.push_back(aliasVector[i]);
				LocationDB::by_country++;
				break;
			}
			else {
				result.push_back(aliasVector[i]);
			}
		}
		//sort by population
		std::sort(result.begin(),result.end(),compareLocData);
		return result;

	} else if (aliasVector.size() == 1) {
		LocationDB::lookup_unambig++;
		SessionLogger::dbg("location_db") <<  aliasVector[0].URI << L"\n"; //for debug
		return aliasVector;
	} else {
		SessionLogger::dbg("location_db") <<  ""; //need new line for consistency
		return aliasVector;
	}
}

std::wstring LocationDB::getEntityUri(int entityId) const {
	Locations temp = (*entityMap)[entityId];
	
	if (temp.size() == 0) {
		//alias does not have URI in system
		SessionLogger::dbg("location_db") << L"COULD NOT DISAMBIGUATE:" << boost::lexical_cast<std::wstring>(entityId); //debug
		return L"";
	} else {
		SessionLogger::dbg("location_db") << L"DISAMBIGUATED:" << boost::lexical_cast<std::wstring>(entityId) << L" - " << temp[0].URI; //debug
		return temp[0].URI;
	}
}

bool LocationDB::isValidIfNationState(const EntitySet* entities, const Entity* entity) const {
	// Find the gazetteer entry we mapped to this entity
	Locations locations = (*entityMap)[entity->getID()];

	if (locations.size() == 0)
		//alias does not have URI in system; we don't need to check further since getEntityURI has already been called
		return false;

	// Get the entity subtype
	static EntitySubtype nation_subtype = EntitySubtype(Symbol(L"GPE.Nation"));
	EntitySubtype entity_subtype = entities->guessEntitySubtype(entity);
	if (entity_subtype != nation_subtype)
		// We leave non-Nations alone
		return true;

	// Check if the best location is a NationState, matching this Country entity
	return (locations[0].URI == locations[0].NationURI);
}

std::set<ElfRelation_ptr> LocationDB::getURIGazetteerRelations(const DocTheory* doc_theory, ElfIndividual_ptr ind, std::wstring domain_prefix) {
	// Default to looking up URIs for th
	int entity_id = ind->get_entity_id();
	std::set<ElfRelation_ptr> generated_relations;

	// Determine if we need to look at a different mention (individual matched a partitive or appositive
	MentionUID mention_uid = ind->get_mention_uid();
	if (mention_uid != MentionUID()) {
		const Mention* mention = doc_theory->getSentenceTheory(mention_uid.sentno())->getMentionSet()->getMention(mention_uid.index());
		const Mention* child = NULL;
		switch (mention->getMentionType()) {
			case Mention::APPO:
			case Mention::PART:
				// Check for a child name mention
				child = mention->getChild();
				if (child != NULL) {
					if (child->getMentionType() == Mention::NAME) {
						const Entity* entity = child->getEntity(doc_theory);
						if (entity != NULL)
							// Override to use the entity from the more specific mention (more likely to have a successful and correct URI lookup)
							entity_id = entity->getID();
					} else {
						// Check if the child has a next mention
						const Mention* next_child = child->getNext();
						if (next_child != NULL) {
							if (next_child->getMentionType() == Mention::NAME) {
								const Entity* entity = next_child->getEntity(doc_theory);
								if (entity != NULL)
									// Override to use the entity from the more specific mention (more likely to have a successful and correct URI lookup)
									entity_id = entity->getID();
							}
						}
					}
				}
				break;
			case Mention::LIST:
				// We can't be certain that we matched the right entity in the list, so don't map
				return generated_relations;
			default:
				// Just look up based on the default entity
				break;
		}
	}
	Locations mapped_locations = (*entityMap)[entity_id];
	
	if (mapped_locations.size() > 0) {
		LocationData target = mapped_locations[0];
		//SessionLogger::dbg("location_db") << target.URI << L" more relations:\n";
		switch (LocationDB::getType(target)) {
			case 1:
				generated_relations.insert(LocationDB::getDirectRelationArg(L"CityTownOrVillage", domain_prefix, target.URI, ind));
				if (target.StateURI != L"") {
					generated_relations.insert(LocationDB::getDirectRelationArg(L"StateOrProvince", domain_prefix, target.StateURI, ind));

					generated_relations.insert(LocationDB::getIndirectRelationArg(L"CityTownOrVillage", L"StateOrProvince", target.URI, target.StateURI, domain_prefix, ind));
					generated_relations.insert(LocationDB::getIndirectRelationArg(L"StateOrProvince", L"NationState", target.StateURI, target.NationURI, domain_prefix, ind));
				}
				generated_relations.insert(LocationDB::getDirectRelationArg(L"NationState", domain_prefix, target.NationURI, ind));
				generated_relations.insert(LocationDB::getIndirectRelationArg(L"CityTownOrVillage", L"NationState", target.URI, target.NationURI, domain_prefix, ind));
				break;
			case 2:
				generated_relations.insert(LocationDB::getDirectRelationArg(L"StateOrProvince", domain_prefix, target.StateURI, ind));
				generated_relations.insert(LocationDB::getIndirectRelationArg(L"StateOrProvince", L"NationState", target.StateURI, target.NationURI, domain_prefix, ind));
				generated_relations.insert(LocationDB::getDirectRelationArg(L"NationState", domain_prefix, target.NationURI, ind));
				break;
			case 3:
				generated_relations.insert(LocationDB::getDirectRelationArg(L"NationState", domain_prefix, target.NationURI, ind));
				break;
		}
	}
	return generated_relations;
}

int LocationDB::getType(LocationData target) {
	if (target.URI == target.NationURI) {
		return 3;
	} else if (target.URI == target.StateURI) {
		return 2;
	} else {
		return 1;
	}
}

ElfRelation_ptr LocationDB::getDirectRelationArg(std::wstring typeName, std::wstring domain_prefix, std::wstring URI, ElfIndividual_ptr ind) {
	EDTOffset start;
	EDTOffset end;
	ind->get_spanning_offsets(start,end);
	//SessionLogger::dbg("location_db") << L"Found direct relation Has" << typeName << L" " << URI << "\n";
	ElfRelation_ptr r = boost::make_shared<ElfRelation>(L"eru:Has"+typeName,
							LocationDB::getRelArgPair(
								new ElfRelationArg(L"eru:subjectOfHas"+typeName,ind),
								new ElfRelationArg(L"eru:objectOfHas"+typeName,domain_prefix+typeName,domain_prefix+URI,L"")),
							ind->get_type()->get_string(),start,end,1,1);
	r->set_source(L"eru:gazetteer");
	return r;
}

ElfRelation_ptr LocationDB::getIndirectRelationArg(std::wstring typeName1, std::wstring typeName2, std::wstring URI1, std::wstring URI2, 
												  std::wstring domain_prefix, ElfIndividual_ptr ind) {
	EDTOffset start;
	EDTOffset end;
	ind->get_spanning_offsets(start,end);
	//SessionLogger::dbg("location_db") << L"Found relation Has" << typeName2 << L" " << URI1 << L" to " << URI2 << L"\n";
	ElfRelation_ptr r = boost::make_shared<ElfRelation>(L"eru:Has"+typeName2,
							LocationDB::getRelArgPair(
								new ElfRelationArg(L"eru:subjectOfHas"+typeName2,domain_prefix+typeName1,domain_prefix+URI1,L""),
								new ElfRelationArg(L"eru:objectOfHas"+typeName2,domain_prefix+typeName2,domain_prefix+URI2,L"")),
							ind->get_type()->get_string(),start,end,1,1);
	r->set_source(L"eru:gazetteer");
	return r;
}

std::vector<ElfRelationArg_ptr> LocationDB::getRelArgPair(ElfRelationArg* arg1,ElfRelationArg* arg2) {
	std::vector<ElfRelationArg_ptr> ret;
	ret.push_back(ElfRelationArg_ptr(arg1));
	ret.push_back(ElfRelationArg_ptr(arg2));
	return ret;
}

void LocationDB::disambiguate(const EntitySet* eSet) {
	
	for (EntityMapType::const_iterator it = entityMap->begin(); it != entityMap->end(); ++it) {
		if (it->second.size() > 1) {
			/*//For subtype disambiguation
			const Entity* entity = eSet->getEntity(it->first);
			EntitySubtype SERIFsubtype = eSet->guessEntitySubtype(entity);
			bool isSerifNation = SERIFsubtype.getName() ==  Symbol(L"Nation");
			bool isSerifProvince = SERIFsubtype.getName() ==  Symbol(L"State-or-Province");
			bool isSerifCity = SERIFsubtype.getName() ==  Symbol(L"Population-Center");
			*/


			SessionLogger::dbg("location_db") << L"--Scores--------------------" ;
			double maxScore = -1;
			int maxInd = 0;
			double maxTypeMatchScore = -1;
			int maxTypeMatchInd = 0;

			for (int i=0;i<(int)it->second.size();i++) {
				double score = LocationDB::getScore(it->second[i]);
				SessionLogger::dbg("location_db") << it->second[i].URI << L"\t" << boost::lexical_cast<std::wstring>(score);
				if (score > maxScore) {
					maxScore = score;
					maxInd = i;
				}
				/* //Disambiguate by subtype
				if(isSerifNation && it->second[i].URI == it->second[i].NationURI){
					if (score > maxTypeMatchScore) {
						maxTypeMatchScore = score;
						maxTypeMatchInd = i;
					}
				}
				else if(isSerifProvince && it->second[i].URI == it->second[i].StateURI){
					if (score > maxTypeMatchScore) {
						maxTypeMatchScore = score;
						maxTypeMatchInd = i;
					}
				}
				else if(isSerifCity && it->second[i].URI != it->second[i].StateURI  && it->second[i].URI == it->second[i].NationURI){
					if (score > maxTypeMatchScore) {
						maxTypeMatchScore = score;
						maxTypeMatchInd = i;
					}
				}
				*/
			}
			LocationDB::by_pop++;
			Locations filt;
			if(maxTypeMatchScore > 0){//not true unless you turn subtype matching on
				filt.push_back(it->second[maxTypeMatchInd]);
			}
			else{
				filt.push_back(it->second[maxInd]);
			}
			(*entityMap)[it->first] = filt;
			SessionLogger::dbg("location_db") << " "; //newline for formatting
		} else if (it->second.size() == 1) {
			SessionLogger::dbg("location_db") << L"Unambiguous: " << it->second[0].URI;
		}
	}
}

double LocationDB::getScore(const LocationData location) const {
	//get the average distance while sorting to get the top n closest locations
	std::vector<double> distances;
	double avgDistance = 0;
	int count = 0;
	for (EntityMapType::const_iterator it = entityMap->begin(); it != entityMap->end(); ++it) {
		if (it->second.size() == 1) {
			double dist = LocationDB::locationDistance(location,it->second[0]);
			if (location.NationURI == it->second[0].NationURI) {
				dist -= NATION_BOOST;
			}
			if (location.StateURI == it->second[0].StateURI && location.StateURI != L"") {
				dist -= STATE_BOOST;
			}
			for (int i=0;i<it->second[0].MentionCount;i++) { //weight distance by entity's mention count
				distances.push_back(dist);
				count++;
				avgDistance += dist;
			}
		}
	}
	avgDistance = avgDistance / (double)count;
	std::sort(distances.begin(),distances.end());

	//get the top n closest locations average distance.
	double topNDistance = 0;
	int topNCount = 0;
	for (int i=0;i<NEAREST_N && i<(int)distances.size();i++) {
		topNDistance += distances[i];
		topNCount++;
	}
	topNDistance = topNDistance / (double)topNCount;

	//combine everything together
	double dist = (TOPN_W*topNDistance)+((1-TOPN_W)*avgDistance);
	//return (DIST_W * dist) + (POP_W * (double)location.Population);
	
	//sigmoid function to properly balance it all
	double sigm_pop = 1/(1+pow(CONST_E,
			-POPULATION_FITTING*((double)location.Population-POPULATION_CENTER)));
	double sigm_dist = 1/(1+pow(CONST_E,
			-DISTANCE_FITTING*(dist-DISTANCE_CENTER)));
	return POPULATION_TO_DISTANCE_RATIO*sigm_pop+sigm_dist;
}

double LocationDB::locationDistance(const LocationData loc1, const LocationData loc2) const {
	return 3963.0 * acos(sin(loc1.Latitude) *  sin(loc2.Latitude) + 
		cos(loc1.Latitude) * cos(loc2.Latitude) * cos(loc2.Longitude - loc1.Longitude));
}
