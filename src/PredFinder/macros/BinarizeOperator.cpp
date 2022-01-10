/**
 * Class that converts an N-ary relation into
 * an individual that is the subject of a relation
 * for each of its args, named by role. Intended for
 * use with N-ary ontologies like the agent ontology
 * for the MR-KBP task.
 *
 * @file BinarizeOperator.cpp
 * @author nward@bbn.com
 * @date 2011.08.16
 **/

#include "Generic/common/leak_detection.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "BinarizeOperator.h"
#include "RelationParameter.h"
#include "ReadingMacroSet.h"
#include "boost/make_shared.hpp"
#include "boost/foreach.hpp"

/**
 * Reads the binarize operator in some macro
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 *
 * @author nward@bbn.com
 * @date 2011.08.16
 **/
BinarizeOperator::BinarizeOperator(const Sexp* sexp) : ReadingMacroOperator(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 2 || sexp->getNumChildren() > 3) {
		error << "Ill-formed make-triple operator: need 2-3 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("BinarizeOperator::BinarizeOperator(Sexp*)", error.str().c_str());
	}

	// Make sure this is a binarize expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroOperator::BINARIZE_SYM) {
		error << "Ill-formed binarize operator: need binarize expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("BinarizeOperator::BinarizeOperator(Sexp*)", error.str().c_str());
	}

	// Check if we need to start reading subexpressions after a shortcut expression
	int child = 1;
	if (_shortcut_name != Symbol())
		child++;

	// Get the required input predicate name
	if (sexp->getNthChild(child)->isAtom()) {
		_predicate = boost::make_shared<PredicateParameter>(std::wstring(sexp->getNthChild(child)->getValue().to_string()));
	} else {
		error << "Ill-formed binarize operator: need input predicate name; got ";
		error << " " << sexp->getNthChild(child)->to_debug_string();
		throw UnexpectedInputException("BinarizeOperator::BinarizeOperator(Sexp*)", error.str().c_str());
	}

	// Create the output individual type, replacing internal namespace prefixes
	std::wstring output_individual_type = _predicate->get_name();
	output_individual_type = ReadingMacroSet::domain_prefix + L":" + output_individual_type.substr(output_individual_type.find_first_of(L":") + 1);
	_individual = boost::make_shared<IndividualParameter>(output_individual_type);
}

/**
 * Implements ReadingMacroOperator::apply.
 *
 * Splits any matching N-ary ElfRelations in this
 * document, replacing the match with the
 * resulting relations, and adding the newly
 * generated individual. If there are no matches,
 * the result is a no-op and the document is
 * unchanged.
 *
 * @param document The ELF document, which will be
 * transformed in-place.
 *
 * @author nward@bbn.com
 * @date 2011.08.16
 **/
void  BinarizeOperator::apply(ElfDocument_ptr document) const {
	// The subject of all split relations is the generated individual
	RoleParameter_ptr subject_role = boost::make_shared<RoleParameter>(L"individual", L"rdf:subject");

	// We need to generate a unique URI for each binarized relation
	boost::hash<std::wstring> string_hasher;

	// Loop through the document's relations, checking for matching relations
	std::set<ElfRelation_ptr> relations = document->get_relations();
	std::set<ElfRelation_ptr> relations_to_remove;
	BOOST_FOREACH(ElfRelation_ptr relation, relations) {
		// Make sure this is a matching relation
		if (matches(relation)) {
			// Find an optionally specified URI for the individual we're splitting out of this relation
			std::wstring split_individual_uri = L"";
			BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
				if (arg->get_role() == L"prov:URI" && arg->get_individual().get() != NULL)
					split_individual_uri = arg->get_individual()->get_best_uri();
			}

			// If a URI wasn't specified, we need to generate one
			if (split_individual_uri.empty()) {
				// Accumulate a unique hash value based on the relation name and args
				size_t relation_hash = string_hasher(relation->get_name());
				BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
					// Use unique non-URI non-temporal args
					if (arg->get_role() != L"prov:URI" && !boost::starts_with(arg->get_role(), L"t:")) {
						// Combine the argument's role and individual URI/value
						std::wstring arg_hash_input = arg->get_role();
						ElfIndividual_ptr individual = arg->get_individual();
						if (individual.get() != NULL) {
							if (individual->has_value()) {
								arg_hash_input += individual->get_value();
								ElfType_ptr type = arg->get_type();
								if (type.get() != NULL) {
									arg_hash_input += type->get_value();
								}
							} else {
								arg_hash_input += individual->get_best_uri(document->get_id());
							}
						}
						relation_hash ^= string_hasher(arg_hash_input);
					}
				}

				// Generate the URI using the relation type and the hash we just accumulated
				std::wstringstream id;
				id << document->replace_uri_prefix(_individual->get_type()) << "-" << relation_hash;
				split_individual_uri = id.str();
			}

			// Generate an output relation parameter for each argument in this matching relation
			std::set<ElfRelation_ptr> split_relations;
			BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
				// Ignore URI args
				if (arg->get_role() == L"prov:URI")
					continue;

				// Create the binary relation subject and object roles with this arg as the object
				std::set<RoleParameter_ptr> output_roles;
				output_roles.insert(subject_role);
				output_roles.insert(boost::make_shared<RoleParameter>(arg->get_role(), L"rdf:object"));

				// Create the output relation name based on this role
				std::wstring output_relation_name = arg->get_role();
				if (boost::starts_with(output_relation_name, L"eru:"))
					output_relation_name = ReadingMacroSet::domain_prefix + L":" + output_relation_name.substr(output_relation_name.find_first_of(L":") + 1);
				PredicateParameter_ptr output_predicate = boost::make_shared<PredicateParameter>(output_relation_name);

				// Create the binary relation connecting this arg to the generated individual
				RelationParameter_ptr output_relation = boost::make_shared<RelationParameter>(output_predicate, output_roles);
				ElfRelation_ptr split_relation = output_relation->to_elf_relation(document, relation, _individual);
				split_relation->add_source(_source_id);

				// Check if this relation had an explicit URI specified
				if (split_individual_uri != L"") {
					BOOST_FOREACH(ElfRelationArg_ptr split_arg, split_relation->get_args()) {
						if (split_arg->get_role() == L"rdf:subject" && split_arg->get_individual().get() != NULL)
							split_arg->get_individual()->set_generated_uri(split_individual_uri);
					}
				}

				// Check if we just processed a temporal arg
				if (!boost::starts_with(output_relation_name, L"t:")) {
					// Check if we're processing an Event instead of a Fluent
					if (boost::ends_with(relation->get_name(), L"Event")) {
						// Accumulate a unique hash value based on the split arg relation we just generated
						size_t relation_hash = string_hasher(split_relation->get_name());
						BOOST_FOREACH(ElfRelationArg_ptr split_arg, split_relation->get_args()) {
							// Use unique non-URI non-temporal args
							if (split_arg->get_role() != L"prov:URI" && !boost::starts_with(split_arg->get_role(), L"t:")) {
								// Combine the argument's role and individual URI/value
								std::wstring arg_hash_input = split_arg->get_role();
								ElfIndividual_ptr individual = split_arg->get_individual();
								if (individual.get() != NULL) {
									if (individual->has_value()) {
										arg_hash_input += individual->get_value();
										ElfType_ptr type = arg->get_type();
										if (type.get() != NULL) {
											arg_hash_input += type->get_value();
										}
									} else {
										arg_hash_input += individual->get_best_uri(document->get_id());
									}
								}
								relation_hash ^= string_hasher(arg_hash_input);
							}
						}

						// Determine the name of reified Event argument relation individual and its relations
						std::wstring event_arg_role = arg->get_role().substr(arg->get_role().find_first_of(L":") + 1);
						std::wstring event_output_suffix = boost::to_upper_copy(event_arg_role.substr(0, 1)) + event_arg_role.substr(1);
						IndividualParameter_ptr event_argument_output_individual = boost::make_shared<IndividualParameter>(ReadingMacroSet::domain_prefix + L":" + event_output_suffix);

						// Generate the URI using the relation type and the hash we just accumulated so that it's global
						std::wstringstream id;
						id << document->replace_uri_prefix(event_argument_output_individual->get_type()) << "-" << relation_hash;
						std::wstring event_argument_individual_uri = id.str();

						// Tie the generated Event individual to the reified subject relation
						PredicateParameter_ptr event_subject_output_predicate = boost::make_shared<PredicateParameter>(ReadingMacroSet::domain_prefix + L":subjectOf" + event_output_suffix);
						std::set<RoleParameter_ptr> event_subject_output_roles;
						event_subject_output_roles.insert(subject_role);
						event_subject_output_roles.insert(boost::make_shared<RoleParameter>(L"rdf:subject", L"rdf:object"));
						RelationParameter_ptr event_subject_relation = boost::make_shared<RelationParameter>(event_subject_output_predicate, event_subject_output_roles);

						// Tie the original argument to the reified object relation
						PredicateParameter_ptr event_object_output_predicate = boost::make_shared<PredicateParameter>(ReadingMacroSet::domain_prefix + L":objectOf" + event_output_suffix);
						std::set<RoleParameter_ptr> event_object_output_roles;
						event_object_output_roles.insert(subject_role);
						event_object_output_roles.insert(boost::make_shared<RoleParameter>(L"rdf:object", L"rdf:object"));
						RelationParameter_ptr event_object_relation = boost::make_shared<RelationParameter>(event_object_output_predicate, event_object_output_roles);

						// Create the pair of reified event relations
						ElfRelation_ptr split_event_subject_relation = event_subject_relation->to_elf_relation(document, split_relation, event_argument_output_individual);
						ElfRelation_ptr split_event_object_relation = event_object_relation->to_elf_relation(document, split_relation, event_argument_output_individual);

						// Replace the generated URI for the reified event individua;
						BOOST_FOREACH(ElfRelationArg_ptr split_event_arg, split_event_subject_relation->get_args()) {
							if (split_event_arg->get_role() == L"rdf:subject" && split_event_arg->get_individual().get() != NULL)
								split_event_arg->get_individual()->set_generated_uri(event_argument_individual_uri);
						}
						BOOST_FOREACH(ElfRelationArg_ptr split_event_arg, split_event_object_relation->get_args()) {
							if (split_event_arg->get_role() == L"rdf:subject" && split_event_arg->get_individual().get() != NULL)
								split_event_arg->get_individual()->set_generated_uri(event_argument_individual_uri);
						}

						// Make sure that both relations are valid
						if (split_event_subject_relation->get_args().size() >= 2 && split_event_object_relation->get_args().size() >= 2) {
							split_relations.insert(split_event_subject_relation);
							split_relations.insert(split_event_object_relation);
						}
					} else {
						// Normal arg, just add the reified arg as a relation
						if (split_relation->get_args().size() >= 2)
							split_relations.insert(split_relation);
					}
				} else {
					// Create the temporal output roles and relation (in the ontology, these start with lowercase versions of the relation individual)
					std::wstring temporal_output_relation_name = L"t:" + boost::to_lower_copy(output_relation_name.substr(output_relation_name.find_first_of(L":") + 1, 1)) + output_relation_name.substr(output_relation_name.find_first_of(L":") + 2);
					PredicateParameter_ptr temporal_output_predicate = boost::make_shared<PredicateParameter>(temporal_output_relation_name);
					std::set<RoleParameter_ptr> temporal_output_roles;
					temporal_output_roles.insert(boost::make_shared<RoleParameter>(L"rdf:subject", L"rdf:subject"));
					temporal_output_roles.insert(boost::make_shared<RoleParameter>(L"individual", L"rdf:object"));
					RelationParameter_ptr temporal_relation = boost::make_shared<RelationParameter>(temporal_output_predicate, temporal_output_roles);

					// Determine the temporal individuals and relations generated based on the type of the temporal arg
					IndividualParameter_ptr temporal_individual;
					PredicateParameter_ptr temporal_spec_string_predicate;
					if (boost::ends_with(output_relation_name, L"At")) {
						// HoldsAt, OccursAt produce TimePoints
						temporal_individual = boost::make_shared<IndividualParameter>(L"t:TimePoint");
						temporal_spec_string_predicate = boost::make_shared<PredicateParameter>(L"t:hasTimePointSpecString");
					} else {
						// Clipped*, *Within, *Throughout, *Over produce TimeIntervals
						temporal_individual = boost::make_shared<IndividualParameter>(L"t:TimeInterval");
						temporal_spec_string_predicate = boost::make_shared<PredicateParameter>(L"t:hasTimeIntervalSpecString");
					}
					
					// Create the temporal spec string output roles and relation
					std::set<RoleParameter_ptr> temporal_spec_string_output_roles;
					temporal_spec_string_output_roles.insert(subject_role);
					temporal_spec_string_output_roles.insert(boost::make_shared<RoleParameter>(L"rdf:object", L"rdf:object"));
					RelationParameter_ptr temporal_spec_string_relation = boost::make_shared<RelationParameter>(temporal_spec_string_predicate, temporal_spec_string_output_roles);

					// Create the pair of new temporal relations
					ElfRelation_ptr split_temporal_relation = temporal_relation->to_elf_relation(document, split_relation, temporal_individual);
					ElfRelation_ptr split_temporal_spec_string_relation = temporal_spec_string_relation->to_elf_relation(document, split_relation, temporal_individual);

					// Make sure that both relations are valid
					if (split_temporal_relation->get_args().size() >= 2 && split_temporal_spec_string_relation->get_args().size() >= 2) {
						split_relations.insert(split_temporal_relation);
						split_relations.insert(split_temporal_spec_string_relation);
					}
				}
			}
			document->add_relations(split_relations);

			// Mark the original relation that we split for removal
			//   Don't want to invalidate our iterators
			relations_to_remove.insert(relation);
		}
	}

	// Loop through the relations we split and remove them from the document
	document->remove_relations(relations_to_remove);
}
