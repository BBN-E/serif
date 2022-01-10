// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORLDKNOWLEDGE_FT_H
#define WORLDKNOWLEDGE_FT_H

#include "Generic/common/Assert.h"
#include "Generic/common/limits.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/std_hash.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"

#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

#if !defined(_WIN32)
#if defined(__APPLE_CC__)
    namespace std {
#else
    namespace __gnu_cxx {
#endif
		template<> struct hash<std::wstring> {
			size_t operator()( const std::wstring & s ) const { return s.size(); };
        };
    }
#endif

/* This features uses lists from Wikipedia to check if 2 known names refer to the
 * same or different entities
 */
class WorldKnowledgeFT : public DTCorefFeatureType {

private:

	class WKMap {
	private:
		SymbolArrayIntegerMap _wkGpe;
		SymbolArraySet _wkGpeSingletons;
		SymbolArrayIntegerMap _wkOrg;
		SymbolArraySet _wkOrgSingletons;
		SymbolArrayIntegerMap _wkPer;
		SymbolArraySet _wkPerSingletons;
	public:
		// All of these methods take ownership of the SymbolArray arugment:
		void addGpe(SymbolArray* sa, int id) {
			_wkGpe[sa] = id;
			if ((*_wkGpe.find(sa)).first != sa)
				delete sa;
		}
		void addGpeSingleton(SymbolArray* sa) {
			_wkGpeSingletons.insert(sa);
			if (*_wkGpeSingletons.find(sa) != sa)
				delete sa;
		}
		int getGpeId(SymbolArray *sa) {
			int* value = _wkGpe.get(sa);
			return value ? *value : 0;
		}
		bool isSingletonGpe(SymbolArray *sa) {
			return _wkGpeSingletons.exists(sa);
		}
		void addOrg(SymbolArray* sa, int id) {
			_wkOrg[sa] = id;
			if ((*_wkOrg.find(sa)).first != sa)
				delete sa;
		}
		void addOrgSingleton(SymbolArray* sa) {
			_wkOrgSingletons.insert(sa);
			if (*_wkOrgSingletons.find(sa) != sa)
				delete sa;
		}
		int getOrgId(SymbolArray *sa) {
			int* value = _wkOrg.get(sa);
			return value ? *value : 0;
		}
		bool isSingletonOrg(SymbolArray *sa) {
			return _wkOrgSingletons.exists(sa);
		}
		void addPer(SymbolArray* sa, int id) {
			_wkPer[sa] = id;
			if ((*_wkPer.find(sa)).first != sa)
				delete sa;
		}
		void addPerSingleton(SymbolArray* sa) {
			_wkPerSingletons.insert(sa);
			if (*_wkPerSingletons.find(sa) != sa)
				delete sa;
		}
		int getPerId(SymbolArray *sa) {
			int* value = _wkPer.get(sa);
			return value ? *value : 0;
		}
		bool isSingletonPer(SymbolArray *sa) {
			return _wkPerSingletons.exists(sa);
		}
		WKMap(): _wkGpe(), _wkOrg(), _wkPer(), _wkGpeSingletons(100), _wkOrgSingletons(100), _wkPerSingletons(100) {}
		~WKMap() {
			for (SymbolArrayIntegerMap::iterator iter=_wkGpe.begin(); iter!=_wkGpe.end(); ++iter)
				delete (*iter).first;
			for (SymbolArraySet::iterator iter=_wkGpeSingletons.begin(); iter!=_wkGpeSingletons.end(); ++iter)
				delete *iter;
			for (SymbolArrayIntegerMap::iterator iter=_wkOrg.begin(); iter!=_wkOrg.end(); ++iter)
				delete (*iter).first;
			for (SymbolArraySet::iterator iter=_wkOrgSingletons.begin(); iter!=_wkOrgSingletons.end(); ++iter)
				delete *iter;
			for (SymbolArrayIntegerMap::iterator iter=_wkPer.begin(); iter!=_wkPer.end(); ++iter)
				delete (*iter).first;
			for (SymbolArraySet::iterator iter=_wkPerSingletons.begin(); iter!=_wkPerSingletons.end(); ++iter)
				delete *iter;
		}
	};

	static const int MAX_MENTION_NAME_SYMS_PLUS = 20;
	static bool _use_gpe_world_knowledge_feature;
	static WKMap _wkMap;
	static Symbol GPE;
	static Symbol ORG;
	static Symbol PER;

	void readOrderedHash(UTF8InputStream& uis, Symbol type){
		UTF8Token tok;
		int lineno = 0;
		Symbol toksyms[MAX_MENTION_NAME_SYMS_PLUS];
		Symbol discardToks[MAX_MENTION_NAME_SYMS_PLUS * 2];
		SymbolArray * lastGpe = 0;
		int lastGpeId = -1;
		bool lastGpeSingle;
		SymbolArray * lastOrg = 0;
		int lastOrgId = -1;
		bool lastOrgSingle;
		SymbolArray * lastPer = 0;
		int lastPerId = -1;
		bool lastPerSingle;
		int singles = 0;
		int multiples = 0;
		int truncatedNames = 0;
		while(!uis.eof()){
			uis >> tok;	// this should be the opening '('
			if(uis.eof()){
				break;
			}
			uis >> tok;	
			int ntoks = 0;
			int ndtoks = 0;
			while(tok.symValue() != Symbol(L")")){
				if (ntoks < MAX_MENTION_NAME_SYMS_PLUS) {
					toksyms[ntoks++] = tok.symValue();
				}else if (ndtoks < MAX_MENTION_NAME_SYMS_PLUS * 2){
					discardToks[ndtoks++] = tok.symValue();
					
				}
				uis >> tok;
			}
			bool reportLongNames = false;
			if (ndtoks > 0){
				truncatedNames++;
				if (reportLongNames){
                    ostringstream ostr;
					ostr << "WorldKnowledgeFT : load world knowledge hash found over-long name; kept [" ;
					for (int nt = 0; nt < ntoks; nt++){
						ostr << toksyms[nt].to_debug_string() << " ";
					}
					ostr << "]\n" << "\t discarding [";
					for (int nt = 0; nt < ndtoks; nt++){
						ostr << discardToks[nt].to_debug_string() << " ";
					}
					if (ndtoks == (MAX_MENTION_NAME_SYMS_PLUS * 2)){
						ostr <<" ...";
					}
					ostr << "]\n";	
					SessionLogger::warn("world_knowledge") << ostr.str();
				}
			}
			uis >> tok;
			int id = _wtoi(tok.chars());
			lineno++;
			SymbolArray * sa = _new SymbolArray(toksyms, ntoks);

			//if((lineno % 50000) == 0){
			//	std::wcout <<" lines: "<< lineno
			//		<<" \t"<< id <<" \t" << type.to_string() << " SA = " ;
			//	for (int nt = 0; nt < ntoks; nt++){
			//		std::wcout << toksyms[nt] << " ";
			//	}
			//	std::wcout <<std::endl;
			//}
			
			if (type == Symbol(L"GPE")){
				if (id == lastGpeId) {
					_wkMap.addGpe(sa, id);
					lastGpeSingle = false;
					multiples++;
				} else {
					if (lastGpeId != -1){
						if (lastGpeSingle){
							_wkMap.addGpeSingleton(lastGpe);
							singles++;
						}else{
							_wkMap.addGpe(lastGpe, lastGpeId);
							multiples++;
						}
					}
					lastGpe = sa;
					lastGpeId = id;
					lastGpeSingle = true;
				}
			} else if (type == Symbol(L"ORG")){
				if (id == lastOrgId) {
					_wkMap.addOrg(sa, id);
					lastOrgSingle = false;
					multiples++;
				} else {
					if (lastOrgId != -1){
						if (lastOrgSingle){
							_wkMap.addOrgSingleton(lastOrg);
							singles++;
						}else{
							_wkMap.addOrg(lastOrg, lastOrgId);
							multiples++;
						}
					}
					lastOrg = sa;
					lastOrgId = id;
					lastOrgSingle = true;
				}
			} else if (type == Symbol(L"PER")) {
				if (id == lastPerId) {
					_wkMap.addPer(sa, id);
					lastPerSingle = false;
					multiples++;
				} else {
					if (lastPerId != -1){
						if (lastPerSingle){
							_wkMap.addPerSingleton(lastPer);
							singles++;
						}else{
							_wkMap.addPer(lastPer, lastPerId);
							multiples++;
						}
					}
					lastPer = sa;
					lastPerId = id;
					lastPerSingle = true;
				}
			} else {
				throw UnrecoverableException("WorldKnowledgeFT::readOrderedHash()", "Unrecognized type");	
			}
		}// end of read loop

		// store possible held Per and/or Org
		if (lastGpeId != -1){
			if (lastGpeSingle){
				_wkMap.addGpeSingleton(lastGpe);
				singles++;
			} else {
				_wkMap.addGpe(lastGpe, lastGpeId);
				multiples++;
			}
		}
		if (lastOrgId != -1){
			if (lastOrgSingle){
				_wkMap.addOrgSingleton(lastOrg);
				singles++;
			} else {
				_wkMap.addOrg(lastOrg, lastOrgId);
				multiples++;
			}
		}
		if (lastPerId != -1){
			if (lastPerSingle){
				_wkMap.addPerSingleton(lastPer);
				singles++;
			} else {
				_wkMap.addPer(lastPer, lastPerId);
				multiples++;
			}
		}
		SessionLogger::dbg("world_knowledge_ft") 
		  << "readOrderedHash loaded " << singles << " singletons and " 
		  << multiples << " linked names of type " << type << " with "
		  << truncatedNames << " truncated names" << std::endl;
		if ((singles == 0) && (multiples==0)) {
			if (uis.fail()) {
				throw UnexpectedInputException("WorldKnowledgeFT::readOrderedHash",
					"Error while reading ordered hash file");
			} else {
				throw UnexpectedInputException("WorldKnowledgeFT::readOrderedHash",
					"Empty ordered hash file -- probably caused by an IO failure");
			}
		}
		//std::wcout << "readOrderedHash loaded " << singles << " singletons and " 
		//	<< multiples << " linked names of type " << type << std::endl;
		if (truncatedNames > 0){
			SessionLogger::dbg("world_knowledge") << "WorldKnowledgeFT: Truncated " << truncatedNames << " over-long names"
				<< "\n";
		}
	}
	void initializeLists(){

		if (!ParamReader::isInitialized()) return;

		static bool initialized = false;
		if (initialized) return;
			
		// Figure out if we are using gpe world knowledge (this should become the default once we verify it works)
		_use_gpe_world_knowledge_feature = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_gpe_world_knowledge_feature", false);

		if (_use_gpe_world_knowledge_feature) {
			std::string gpe_buffer = ParamReader::getRequiredParam("dtnamelink_gpe_wk_file");
			//std::wcout << "preloading sorted GPE wk file " << gpe_buffer << std::endl;
			//std::flush(wcout);
			boost::scoped_ptr<UTF8InputStream> gpe_uis_scoped_ptr(UTF8InputStream::build(gpe_buffer.c_str()));
			UTF8InputStream& gpe_uis(*gpe_uis_scoped_ptr);
			readOrderedHash(gpe_uis, GPE);
		}
		std::string org_buffer = ParamReader::getRequiredParam("dtnamelink_org_wk_file");
		//std::wcout << "preloading sorted ORG wk file " << org_buffer << std::endl;
		//std::flush(wcout);
		boost::scoped_ptr<UTF8InputStream> org_uis_scoped_ptr(UTF8InputStream::build(org_buffer.c_str()));
		UTF8InputStream& org_uis(*org_uis_scoped_ptr);
		readOrderedHash(org_uis, ORG);

		std::string per_buffer = ParamReader::getRequiredParam("dtnamelink_per_wk_file");
		//std::wcout << "preloading sorted PER wk file " << per_buffer << std::endl;
		//std::flush(wcout);
		boost::scoped_ptr<UTF8InputStream> per_uis_scoped_ptr(UTF8InputStream::build(per_buffer.c_str()));
		UTF8InputStream& per_uis(*per_uis_scoped_ptr);
		readOrderedHash(per_uis, PER);

		initialized = true;
	}

	static int lookUpName(SymbolArray *sa, Symbol type) {
		int value = 0;
		if (type == GPE){
			value = _wkMap.isSingletonGpe(sa) ? -1 : _wkMap.getGpeId(sa);
		} else if (type == ORG){
			value = _wkMap.isSingletonOrg(sa) ? -1 : _wkMap.getOrgId(sa);
		} else if (type == PER){
			value = _wkMap.isSingletonPer(sa) ? -1 : _wkMap.getPerId(sa);
		} else {
			throw UnrecoverableException("WorldKnowledgeFT::lookUpName()", "Unrecognized type");	
		}
		//removing 'the' lowers performance 
		//std::wcout<<"("<<str<<")  ...... " <<value<<std::endl;
		return value;
	}

public:
	WorldKnowledgeFT() : DTCorefFeatureType(Symbol(L"world-knowledge")) {
	}

	~WorldKnowledgeFT() {}
	void validateRequiredParameters() { initializeLists();	}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{

		DTCorefObservation *o = static_cast<DTCorefObservation*>(
				state.getObservation(0));
		Entity *entity = o->getEntity();
		const Mention *ment = o->getMention();
		const EntitySet *eset = o->getEntitySet();
		const MentionSymArrayMap *HWMap = o->getHWMentionMapper();
		bool has_match = false;
		bool has_clash = false;
		bool has_one_unk = false;
		bool has_both_unk = false;
		Symbol ment_subtype = Symbol(L"-UNKST-");
		if(ment->getEntitySubtype().isDetermined()){
			ment_subtype = ment->getEntitySubtype().getName();
		}
		Symbol match_subtype = Symbol(L"-UNKST-");;
		Symbol clash_subtype = Symbol(L"-UNKST-");;
		Symbol one_unk_subtype= Symbol(L"-UNKST-");;
		if(ment->getMentionType() != Mention::NAME){
			return 0;
		}
		if (_use_gpe_world_knowledge_feature && ment->getEntityType().matchesGPE() && entity->getType().matchesGPE()) {
			SymbolArray* ment_sa = CorefUtilities::getNormalizedSymbolArray(ment);
			if (ment_sa->getSizeTLength() == 0){
				SessionLogger::warn("world_knowledge") << "WorldKnowledgeFT normalized GPE mention to zero length SymbolArray\n";
				delete ment_sa;
				return 0;
			}
			int mentid = WorldKnowledgeFT::lookUpName(ment_sa, GPE);
			if(mentid == 0){
				has_one_unk = true;
			}
			for(int mentno = 0; mentno < entity->getNMentions(); mentno++){
				const Mention* othment = eset->getMention(entity->getMention(mentno));
				if(othment->getMentionType() == Mention::NAME){
					SymbolArray* othment_sa = CorefUtilities::getNormalizedSymbolArray(othment);
					if((* othment_sa) == (* ment_sa)){
						;
					}
					else{
						int othmentid = lookUpName(othment_sa, GPE);
//						std::wcout<<"Not Exact Match: ("<<othment_str <<") ("<<othmentid<<") ---> ("<<ment_str
//									<<") ("<<mentid<<")"<<std::endl;
						if(mentid != -1 && othmentid == mentid){
							if(othmentid == 0){
								has_both_unk = true;
							} else{
								has_match = true;
								if(othment->getEntitySubtype().isDetermined())
									match_subtype = othment->getEntitySubtype().getName();
							}
						} else if(othmentid == 0){
							has_one_unk = true;
							if(othment->getEntitySubtype().isDetermined())
								one_unk_subtype = othment->getEntitySubtype().getName();
						} else if((mentid == -1) || (othmentid != mentid)){
							has_clash = true;
							if(othment->getEntitySubtype().isDetermined())
								clash_subtype = othment->getEntitySubtype().getName();
						}
					}
					delete othment_sa;
				}
			}
			delete ment_sa;
		} else if (ment->getEntityType().matchesORG() && entity->getType().matchesORG()) {
			SymbolArray* ment_sa = CorefUtilities::getNormalizedSymbolArray(ment);
			if (ment_sa->getSizeTLength() == 0){
				SessionLogger::warn("world_knowledge") << "WorldKnowledgeFT normalized ORG mention to zero length SymbolArray\n";
				delete ment_sa;
				return 0;
			}
			int mentid = WorldKnowledgeFT::lookUpName(ment_sa, ORG);
			if(mentid == 0){
				has_one_unk = true;
			}
			for(int mentno = 0; mentno < entity->getNMentions(); mentno++){
				const Mention* othment = eset->getMention(entity->getMention(mentno));
				if(othment->getMentionType() == Mention::NAME){
					SymbolArray* othment_sa = CorefUtilities::getNormalizedSymbolArray(othment);
					if((* othment_sa) == (* ment_sa)){
						;
					}
					else{
						int othmentid = lookUpName(othment_sa, ORG);
//						std::wcout<<"Not Exact Match: ("<<othment_str <<") ("<<othmentid<<") ---> ("<<ment_str
//									<<") ("<<mentid<<")"<<std::endl;
						if(mentid != -1 && othmentid == mentid){
							if(othmentid == 0){
								has_both_unk = true;
							} else{
								has_match = true;
								if(othment->getEntitySubtype().isDetermined())
									match_subtype = othment->getEntitySubtype().getName();
							}
						} else if(othmentid == 0){
							has_one_unk = true;
							if(othment->getEntitySubtype().isDetermined())
								one_unk_subtype = othment->getEntitySubtype().getName();
						} else if((mentid == -1) || (othmentid != mentid)){
							has_clash = true;
							if(othment->getEntitySubtype().isDetermined())
								clash_subtype = othment->getEntitySubtype().getName();
						}
					}
					delete othment_sa;
				}
			}
			delete ment_sa;
		} else if (ment->getEntityType().matchesPER() && entity->getType().matchesPER()) {
			SymbolArray* ment_sa = CorefUtilities::getNormalizedSymbolArray(ment);
			if (ment_sa->getSizeTLength() == 0){
				SessionLogger::warn("world_knowledge") << "WorldKnowledgeFT normalized PER mention to zero length SymbolArray\n";
				delete ment_sa;
				return 0;
			}
			int mentid = WorldKnowledgeFT::lookUpName(ment_sa, PER);

			if(mentid == 0){
				//don't use for unknown single person names
				if(ment->getHead()->getNTerminals() < 2){
					delete ment_sa;
					return 0;
				}
				has_one_unk = true;
			}
			for(int mentno = 0; mentno < entity->getNMentions(); mentno++){
				const Mention* othment = eset->getMention(entity->getMention(mentno));
				if(othment->getMentionType() == Mention::NAME){
					SymbolArray* othment_sa = CorefUtilities::getNormalizedSymbolArray(othment);
					if((* ment_sa) == (* othment_sa)){
						;
					}
					else{
						int othmentid = lookUpName(othment_sa, PER);
						if((mentid != -1) && (othmentid == mentid)){
							if(othmentid == 0){
								if(othment->getHead()->getNTerminals() > 1){
									has_both_unk = true;
								}
							} else{
								has_match = true;
								if(othment->getEntitySubtype().isDetermined())			
									match_subtype = othment->getEntitySubtype().getName();
							}
						} else if(othmentid == 0){
							if(othment->getHead()->getNTerminals() > 1){
								has_one_unk = true;
								if(othment->getEntitySubtype().isDetermined())
									one_unk_subtype = othment->getEntitySubtype().getName();
							}
						} else if((mentid == -1) || (othmentid != mentid)){
							//	std::wcout<<"CLASH: "<<
							//	othment_str <<"("<<othmentid<<") ---> "<<ment_str
							//	<<"("<<mentid<<")"<<std::endl;
							has_clash = true;
							if(othment->getEntitySubtype().isDetermined())
								clash_subtype = othment->getEntitySubtype().getName();
						}
					}
					delete othment_sa;
				}
			}
			delete ment_sa;
		} else {
			return 0;
		}
		int nfeatures = 0;
		Symbol type;
		if (entity->getType().matchesGPE()){
			type = Symbol(L"GPE");
		} else if (entity->getType().matchesORG()){
			type = Symbol(L"ORG");
		} else if (entity->getType().matchesPER()){
			type = Symbol(L"PER");
		} else {
			throw UnrecoverableException("WorldKnowledgeFT::extractFeatures()", "Unrecognized type");	
		}

		if(has_one_unk){
			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
				Symbol(L"-WK_1UNK-"), type, type);
//			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
//				Symbol(L"-WK_1UNK-"), ment_subtype, one_unk_subtype);
		}  
		if(has_both_unk){
			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
				Symbol(L"-WK_2UNK-"), type, type);
//			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
//				Symbol(L"-WK_2UNK-"), ment_subtype, one_unk_subtype);
		}  		
		
		if(has_match){
			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
				Symbol(L"-HAS_MATCH-"), type, type);
//			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
//				Symbol(L"-HAS_MATCH-"), ment_subtype, match_subtype);
		}  
		if(has_clash){
			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
				Symbol(L"-HAS_CLASH-"), type, type);
//			resultArray[nfeatures++] = _new DTQuadgramFeature(this, state.getTag(),
//				Symbol(L"-HAS_CLASH-"), ment_subtype, clash_subtype);
		}

		return nfeatures;
		
	}
};

#endif


