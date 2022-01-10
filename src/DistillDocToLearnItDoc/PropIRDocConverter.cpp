#include "common/leak_detection.h"
#include "PropIRDocConverter.h"
#include "PatternIRDocConverter.h"

#include <boost/foreach.hpp>
#include "theories/DocTheory.h"
#include "theories/ValueMentionSet.h"
#include "theories/SynNode.h"
#include "theories/ValueMention.h"
#include "Generic/PropTree/PropForestFactory.h"

using namespace std;

const wstring PropIRDocConverter::PROP_MENTION_ANN = L"m";
const wstring PropIRDocConverter::PROP_NAME_MENTION_ANN = L"n";
const wstring PropIRDocConverter::PROP_DESC_MENTION_ANN = L"d";
const wstring PropIRDocConverter::PROP_MEL_NAME = L"en";
const wstring PropIRDocConverter::PROP_MEL_DESC = L"ed";

void PropIRDocConverter::processSentence(const DocTheory* dt,
										 int sn, wostream& out)
{
	const SentenceTheory* st=dt->getSentenceTheory(sn);

	printPropsForSentence((*getForestWithCache(dt))[sn], st, 
		dt->getEntitySet(), st->getValueMentionSet(), out);
}

DocPropForest_ptr& PropIRDocConverter::getForestWithCache(
	const DocTheory* dt) 
{
	if (dt->getDocument()->getName()==_cached_forest_docid) {
		return _cached_forest;
	} else {
		_cached_forest_docid=std::wstring(dt->getDocument()->getName().to_string());
		_cached_forest=PropForestFactory::getDocumentForest(dt);
		return _cached_forest;
	}
}

void PropIRDocConverter::printPropsForSentence(
					const PropNode::PropNodes_ptr& sentenceRoots, 
					const SentenceTheory* st, const EntitySet* es,
					const ValueMentionSet* vms, wostream& out)
{
	BOOST_FOREACH(const PropNode::ptr_t& node, *sentenceRoots) {
		printPropTree(*node, st, es, vms, out);
	}
}

wstring normalize(const wstring& s) {
	static wstring ALLGONE=L"ALLGONE";
	wstring ret=s;

	// remove all whitespace and non-alpha numeric characters
	for (size_t i=0; i<ret.length(); ++i) {
		if (!::isalnum(ret[i])) {
			ret[i]=' ';
		}
	}
	ret.erase(remove_if(ret.begin(), ret.end(), ::isspace), ret.end());
	
	// we might have deleted everything...
	if (ret.empty()) {
		ret=ALLGONE;
	}

	return ret;
}

void PropIRDocConverter::printTriple(const wstring& a, const wstring& b, 
									 const wstring& c, wostream& out) 
{
	out << normalize(a) << L"x" << normalize(b) << L"x" << normalize(c) << L" ";
}

void PropIRDocConverter::printQuadruple(const wstring& a, const wstring& b, 
									 const wstring& c, const wstring& d, wostream& out) 
{
	out << normalize(a) << L"x" << normalize(b) << L"x" << normalize(c) << L"x" << normalize(d) << L" ";
}

bool hasPredicate(const PropNode::ChildRole& kidRole) {
	return 0!=kidRole.get<0>()->getRepresentativePredicate();
}

// gets children, descending into conjunctions
PropIRDocConverter::ChildRoleList 
PropIRDocConverter::getChildRolesThroughConjunctions(const PropNode& node) 
{
	ChildRoleList kidVector=node.getChildRoles();
		
	bool done=false;
	while (!done) {
		const ChildRoleList::iterator compoundsIt=
			partition(kidVector.begin(), kidVector.end(), hasPredicate);
		// keep going until we have no more conjunctions in the kid list
		if (kidVector.end()==compoundsIt) {
			done=true;
		} else {
			// replace all conjunctions in the kid list with their children
			ChildRoleList compoundKids(compoundsIt, kidVector.end());
			kidVector.erase(compoundsIt, kidVector.end());
			BOOST_FOREACH(PropNode::ChildRole& compoundKidRole, compoundKids) 
			{
				ChildRoleList compoundKidKids=
					compoundKidRole.get<0>()->getChildRoles();
				
				// children of compounds will have a useless role like
				// "member". We want to propagate the real role the 
				// conjunction as a whole plays onto each of the children
				const Symbol& role=compoundKidRole.get<1>();
				BOOST_FOREACH(PropNode::ChildRole& grandkid, compoundKidKids) 
				{
					grandkid.get<1>()=role;
				}

				kidVector.insert(kidVector.end(), compoundKidKids.begin(),
					compoundKidKids.end());
			}
		}
	}	

	return kidVector;
}

void PropIRDocConverter::printPropTree(const PropNode& node,
									   const SentenceTheory* st,
									   const EntitySet* es,
									   const ValueMentionSet* vms,
									   wostream& out)
{
	if (node.getRepresentativePredicate()) {
		const wstring head=node.getRepresentativePredicate()->pred().to_string();
		ChildRoleList kidVector=getChildRolesThroughConjunctions(node);

		BOOST_FOREACH(const PropNode::ChildRole& childRole, kidVector) {
			const PropNode& kidNode=*childRole.get<0>();
			// kid guaranteed to have representative predicate by 
			// construction of kidVector
			const wstring kidHead=kidNode.getRepresentativePredicate()->pred().to_string();
			const wstring roleString=childRole.get<1>().to_string();

			// how ought value arguments work into this?
			printTriple(head, roleString, kidHead, out);
			printPropAnnotations(head, roleString, kidNode, es, vms, out);
		}	
	}

	BOOST_FOREACH(const PropNode::ChildRole& childRole, node) {
		printPropTree(*childRole.get<0>(), st, es, vms, out);
	}
}

void PropIRDocConverter::printPropAnnotations(const wstring& head, 
		const wstring& roleString, const PropNode& kidNode,
		const EntitySet* es, const ValueMentionSet* vms, wostream& out) 
{
	const Mention* m=kidNode.getMention();

	if (m) {
		// print mention information
		wstring mention_type=PROP_MENTION_ANN;
		if (m->mentionType==Mention::NAME) {
			mention_type=PROP_NAME_MENTION_ANN;
		} else {
			mention_type=PROP_DESC_MENTION_ANN;
		}
		printTriple(head, roleString, mention_type, out);

		const Entity* entity=es->getEntityByMentionWithoutType(m->getUID());
		if (entity) {
			// print min-entity-level information
			pair<bool, bool> hasNameDesc=
				PatternIRDocConverter::entityHasNameDescMention(entity, es);
			if (hasNameDesc.first) {
				printTriple(head, roleString, PROP_MEL_NAME, out);
			} else if (hasNameDesc.second) {
				printTriple(head, roleString, PROP_MEL_DESC, out);
			}
		
			// entity type annotation (e.g. ORG, PER, etc.)
			const wstring typeString=entity->getType().getName().to_string();
			const wstring subtypeString=m->getEntitySubtype().getName().to_string();
			if (!typeString.empty()) {
				if (!subtypeString.empty() && (subtypeString != L"UNDET")) {
					printQuadruple(head, roleString, typeString, subtypeString, out);
				} else {
					printTriple(head, roleString, typeString, out);
				}
			}
		}
	}

	// since ValueMentions don't appear as arguments in PropTrees we fall
	// back to marking a node as part of a ValueMention if it is a subspan
	const SynNode* syn=(m && m->getNode())?m->getNode():kidNode.getSynNode();
	if (syn) {
		vector<wstring> valueAnnotations;
		for (int i=0; i<vms->getNValueMentions(); ++i) {
			const ValueMention* vm=vms->getValueMention(i);
			if (vm->getStartToken()<=syn->getStartToken() &&
				vm->getEndToken()>=syn->getEndToken()) 
			{
				valueAnnotations.push_back(
					vm->isTimexValue()?
						vm->getSubtype().to_string()
						:vm->getType().to_string());
			}
		}

		BOOST_FOREACH(const wstring& valueAnn, valueAnnotations) {
			printTriple(head, roleString, valueAnn, out);
		}
	}
}

