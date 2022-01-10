/**
 * Factory class for ElfRelationArg and its subclasses.
 *
 * @file ElfRelationArgFactory.cpp
 * @author afrankel@bbn.com
 * @date 2010.06.23
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/values/TemporalNormalizer.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "LearnIt/MainUtilities.h"
#include "PredFinder/common/ElfMultiDoc.h"
#include "ElfRelationArgFactory.h"
#include "ElfIndividualFactory.h"
#include "ElfDescriptor.h"
#include "Generic/patterns/TextPattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "boost/make_shared.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/split.hpp"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include "boost/lexical_cast.hpp"

#pragma warning(disable : 4244)

using boost::dynamic_pointer_cast;
/**
 * Static factory method that builds a vector of ElfRelationArg
 * objects from a ReturnPatternFeature found using manual
 * Distillation patterns. Usually, we just call the ElfRelationArg
 * constructor, and there will be only one item
 * in the returned vector, but there are two exceptions in which
 * the vector will contain more items:
 * (a) an NFL player for whom a unique match is found in the DB 
 * will cause an NFL team arg to also be inserted into the vector
 * (b) a pattern containing the "extract-regex" keyword will
 * cause multiple args to be inserted.
 *
 * @param doc_theory The DocTheory containing the matched
 * PatternReturnFeature, used for determining offsets.
 * @param feature The ReturnPatternFeature that contains
 * a mention, text, etc. we can convert to one or more ElfRelationArg objects.
 * @return The constructed vector of ElfRelationArg ptrs.
 *
 * @author afrankel@bbn.com
 * @date 2010.08.03
 **/
std::vector<ElfRelationArg_ptr> ElfRelationArgFactory::from_return_feature(const DocTheory* doc_theory, 
																		   ReturnPatternFeature_ptr feature) {
	// Check for bad feature or document
	if (feature == NULL) {
		throw std::runtime_error("ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): Feature null");
	}
	if (doc_theory == NULL) {
		throw std::runtime_error("ElfRelationArgFactory::from_return_feature("
			"DocTheory*, ReturnPatternFeature_ptr): Document theory null");
	}

	// If we're processing an NFL player or doing regex expansion, we need to call the appropriate method.
	// We assume that we'll never do regex expansion on an NFL player (since this would be unnecessary).
	if (feature->getReturnValue(L"role") == L"eru:NFLPlayer") {
		return nfl_player_from_return_feature(doc_theory, feature);
	} else if (feature->hasReturnValue(L"extract-regex") && feature->getReturnValue(L"extract-regex") == L"true") {
		return from_return_feature_w_regex(doc_theory, feature);
	} else {
		// The relation_args we build from the feature
		std::vector<ElfRelationArg_ptr> relation_args;
		relation_args.push_back(boost::make_shared<ElfRelationArg>(doc_theory, feature));
		// Note that we didn't do any error-checking on the role or type to make sure
		// that each only contained a single value. However, we can do this when we load the pattern.
		return relation_args;
	}
}


/**
 * Private method called by from_return_feature(). 
 * Builds a vector of ElfRelationArg
 * objects from a ReturnPatternFeature found using manual
 * Distillation patterns. An NFL player for whom a unique 
 * match is found in the DB will cause an NFL team arg to 
 * be inserted into the vector along with the player arg.
 *
 * @param doc_theory The DocTheory containing the matched
 * PatternReturnFeature, used for determining offsets.
 * @param feature The ReturnPatternFeature that contains
 * a mention, text, etc. we can convert to one or more ElfRelationArg objects.
 * @return The constructed vector of ElfRelationArg ptrs.
 *
 * @author afrankel@bbn.com
 * @date 2011.04.07
 **/
std::vector<ElfRelationArg_ptr> ElfRelationArgFactory::nfl_player_from_return_feature(
	const DocTheory* doc_theory, ReturnPatternFeature_ptr feature) {


	// The relation_args we build from the feature
	std::vector<ElfRelationArg_ptr> relation_args;

	// Create an arg for the player as normal
	ElfRelationArg_ptr player_arg = boost::make_shared<ElfRelationArg>(doc_theory, feature);
	relation_args.push_back(player_arg);

	// Try to get a player name and clean it up
	ElfIndividual_ptr player_individual = player_arg->get_individual();
	if (player_individual.get() != NULL) {
		ElfString_ptr player_name = player_individual->get_name_or_desc();
		if (player_name.get() != NULL) {
			std::wstring player_name_string = player_name->get_value();
			if (player_name_string != L"") {
				// Collapse whitespace, since the structured database only has spaces in names
				boost::wregex spaces_re(L"\\s+");
				player_name_string = boost::regex_replace(player_name_string, spaces_re, L" ");

				// Try to guess a season based on the document date
				int season = -1;
				std::wstring doc_date = L"";
				std::wstring doc_additional_time = L"";
				ElfMultiDoc::temporal_normalizer->normalizeDocumentDate(doc_theory, doc_date, doc_additional_time);
				if (doc_date != L"" && doc_date != L"XXXX-XX-XX") {
					// Parse out the year and month to determine season
					season = boost::lexical_cast<int>(doc_date.substr(0, 4));
					int month = boost::lexical_cast<int>(doc_date.substr(5, 2));
					if (month <= 3)
						// Up through March is considered previous season
						season--;
				}

				// Retrieve the candidate player/team URIs.
				// If we couldn't find a season, we'll just hope the name is unique 
				// and only played for one team.
				PlayerResultMap player_team_uris = ElfMultiDoc::get_teams_for_player(player_name_string, season);

				// Make sure we got a unique result
				if (player_team_uris.size() == 1 && player_team_uris.begin()->second.size() == 1) {
					// Get the matching URIs
					std::wstring player_uri = player_team_uris.begin()->first;
					std::wstring team_uri = *(player_team_uris.begin()->second.begin());

					// Replace the player individual id with the found URI (gives us some basic coref)
					player_individual->set_bound_uri(player_uri);

					// Extract span information from the player's associated type object
					EDTOffset player_start, player_end;
					ElfType_ptr player_type = player_individual->get_type();
					player_type->get_offsets(player_start, player_end);
					std::wstring player_text = player_type->get_string();

					// Generate a new arg for the matched team, with hard-coded type and role
					ElfType_ptr team_evidence = boost::make_shared<ElfType>(L"nfl:NFLTeam", player_text, player_start, player_end);
					ElfIndividual_ptr team_individual = boost::make_shared<ElfIndividual>(team_uri, ElfString_ptr(), team_evidence);
					team_individual->set_bound_uri(team_uri);
					ElfRelationArg_ptr team_arg = boost::make_shared<ElfRelationArg>(L"eru:NFLTeam", team_individual);
					relation_args.push_back(team_arg);
				} else if (player_team_uris.size() == 0) {
					std::stringstream error;
					// Since it's a static variable, it's only examined the first time this method is executed.
					// We know that by the time we're executing this method, the param file has already been
					// read (by PredicationFinder).
					static bool warn_about_unfound_players = ParamReader::getOptionalTrueFalseParamWithDefaultVal("warn_about_unfound_nfl_players", /*defaultVal=*/ false);

					// If _warn_about_unfound_players == false, the method will still throw an exception, but
					// the string will be empty so it won't clutter up the screen.
					if (warn_about_unfound_players) {
						error << "ElfRelationArgFactory::from_return_feature(DocTheory*, SnippetReturnFeature*): "
							  << "Could not find player '" << UnicodeUtil::toUTF8StdString(player_name_string) 
							  << "' in DB in season " << season;
					}
					throw std::runtime_error(error.str().c_str());
				} else if (player_team_uris.begin()->second.size() != 1) {
					std::stringstream error;
					error << "ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
						  << "Player '" << UnicodeUtil::toUTF8StdString(player_name_string) 
						  << "' not on unique team in season " << season;
					throw std::runtime_error(error.str().c_str());
				}
			}
		}
	}
	return relation_args;
}

/**
 * Private method called by from_return_feature(). 
 * Builds a vector of ElfRelationArg
 * objects from a ReturnPatternFeature found using manual
 * Distillation patterns. Creates arg for each parenthesized
 * regex group.
 *
 * @param doc_theory The DocTheory containing the matched
 * PatternReturnFeature, used for determining offsets.
 * @param feature The ReturnPatternFeature that contains
 * a mention, text, etc. we can convert to one or more ElfRelationArg objects.
 * @return The constructed vector of ElfRelationArg ptrs.
 *
 * @author afrankel@bbn.com
 * @date 2011.04.07
 **/
std::vector<ElfRelationArg_ptr> ElfRelationArgFactory::from_return_feature_w_regex(
	const DocTheory* doc_theory, ReturnPatternFeature_ptr feature) 
{
	std::vector<ElfRelationArg_ptr> relation_args;
	// Get a descriptor string and offsets from the sentence
	SentenceTheory* sent_theory = doc_theory->getSentenceTheory(feature->getSentenceNumber());
	if (!sent_theory) {
		return relation_args;
	}
	TokenSequence* token_sequence = sent_theory->getTokenSequence();
	if (!token_sequence) {
		return relation_args;
	}
	std::wstring regex_string;
	if (TokenSpanReturnPFeature_ptr tsf = dynamic_pointer_cast<TokenSpanReturnPFeature>(feature)) {
		if (TextPattern_ptr text_pattern = dynamic_pointer_cast<TextPattern>(tsf->getPattern())) {
			regex_string = text_pattern->getText();
		} else {
			throw std::runtime_error(
				"ElfRelationArgFactory::from_return_feature("
				"DocTheory*, ReturnPatternFeature_ptr): Pattern type is not TEXT");
		}
	} else {
		throw std::runtime_error(
			"ElfRelationArgFactory::from_return_feature("
			"DocTheory*, ReturnPatternFeature_ptr): Return type is not TOKEN_SPAN");
	}
	boost::wregex regex_obj;
	try {
		regex_obj.assign(regex_string /*, flag_type f = boost::regex_constants::normal*/);
	}
	catch(exception & e) {
		ostringstream ostr;
		ostr << "ElfRelationArgFactory::from_return_feature("
			"DocTheory*, ReturnPatternFeature_ptr): "
			 << "Error in constructing regex from expression <" << regex_string << ">: " << e.what();
		throw std::runtime_error(ostr.str().c_str());
	}

	// Equal to the number of parenthesized expressions plus one 
	// (because regex_obj[0] contains the full regex).
	size_t mark_count(regex_obj.mark_count());
	if (mark_count < 2) {
		ostringstream ostr;
		ostr << "ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
			 << "Regex <" << regex_string 
			 << "> contains no parenthesized subexpressions although 'extract-regex' was specified.";
		throw std::runtime_error(ostr.str().c_str());
	}

	// Get the offsets for the return feature
	EDTOffset start = token_sequence->getToken(feature->getStartToken())->getStartEDTOffset();
	EDTOffset end = token_sequence->getToken(feature->getEndToken())->getEndEDTOffset();

	// Get the text for the token span
	LocatedString* token_span = MainUtilities::substringFromEdtOffsets(
		doc_theory->getDocument()->getOriginalText(), start, end);
	std::wstring feature_text = std::wstring(token_span->toString());
	delete token_span;

	boost::wsmatch match_obj;
	// Note that regex_match, unlike regex_search, consumes all input.
	if (!boost::regex_match(feature_text, match_obj, regex_obj)) {
		ostringstream ostr;
		ostr << "ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
			 << "Regex <" << regex_string << "> did not match feature text:\n" << feature_text;
		throw std::runtime_error(ostr.str().c_str());
	}

	// Get role strings from the return feature
	std::vector<std::wstring> roles;
	if (feature->hasReturnValue(L"role")) {
		std::wstring line(feature->getReturnValue(L"role"));
		boost::algorithm::trim(line);
		size_t len(line.length());
		boost::algorithm::split(roles, line, boost::algorithm::is_from_range(L' ', L' '));
	}
	else {
		ostringstream ostr;
		if (feature->getPatternReturn()) {
			feature->getPatternReturn()->dump(ostr, /*indent=*/0);
		}
		std::string main_str;
		main_str = "ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): ";
		main_str += "No role return specified in manual pattern.\n";
		main_str += ostr.str();
		throw std::runtime_error(main_str.c_str());
	}
	// Pull type names from the return feature; we'll create the type objects later
	std::vector<std::wstring> type_strs;
	if (feature->hasReturnValue(L"type")) {
		std::wstring line(feature->getReturnValue(L"type"));
		// boost::algorithm::trim(line); uncomment only if we find that extra whitespace causes problems
		boost::algorithm::split(type_strs, line, boost::algorithm::is_from_range(L' ', L' '));
	}
	else
		throw std::runtime_error(
			"ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
			"No type return specified in manual pattern");

	size_t role_count(roles.size());
	if (role_count != mark_count - 1) {
		ostringstream ostr;
		ostr << "ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
			 << "number of roles (" << role_count << ") "
			 << "!= number of parenthesized subexpressions (" << mark_count - 1 << ")";
		throw std::runtime_error(ostr.str().c_str());
	}
	if (role_count != type_strs.size()) {
		ostringstream ostr;
		ostr << "ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
			 << "number of roles (" << role_count << ") != number of types (" << type_strs.size() << ")";
		throw std::runtime_error(ostr.str().c_str());
	}
	if (!feature->hasReturnValue(L"value")) {
		throw std::runtime_error(
			"ElfRelationArgFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
			"No value return for manual pattern");
	}

	for (size_t i = 0; i < role_count; ++i) {
		EDTOffset start_pos(start.value() + match_obj.position(i+1));
		EDTOffset end_pos(start_pos.value() + match_obj.length(i+1) - 1);
		ElfType_ptr type(boost::make_shared<ElfType>(type_strs[i], match_obj[i+1], start_pos, end_pos));
		const Mention* m = 0;
		int start_token = -1;
		int end_token = -1;
		//Coerce the argument to be a mention if possible; this will 
		//standardize URI assignment based on the entity set.
		for(int t = 0; t < token_sequence->getNTokens(); t++){
			if(token_sequence->getToken(t)->getStartEDTOffset() == start_pos){
				start_token = t;
			}
			if(token_sequence->getToken(t)->getEndEDTOffset() == end_pos){
				end_token = t;
			}
		}
		if(start_token == -1 || end_token == -1){
			//This is frequently true for scores, so don't suppress warning
			//SessionLogger::warn("LEARNIT")<<"ElfRelationArgFactory:from_return_feature(): "
			// "text does not match tokens: "
			// <<text<<"     --->"<<roles[i]<<std::endl;
			//SessionLogger::info("LEARNIT")<<start_pos<<", "<<end_pos<<std::endl;
			//token_sequence->dump(SessionLogger::info("LEARNIT"));
			//SessionLogger::info("LEARNIT")<<std::endl;						
		}
		else{
			m = MainUtilities::getMentionFromTokenOffsets(sent_theory, start_token, end_token);
		}

		// Use the entity if we have one, unless we know this is an integer match (and Serif incorrectly made it an Entity)
		if(m != 0 && ParamReader::getRequiredTrueFalseParam("coerce_strings_to_mentions") && !boost::starts_with(type_strs[i], L"xsd:")){
			ElfIndividual_ptr individual = boost::make_shared<ElfIndividual>(doc_theory, type_strs[i], m);
			relation_args.push_back(boost::make_shared<ElfRelationArg>(roles[i], individual));
		} else {
			relation_args.push_back(boost::make_shared<ElfRelationArg>(roles[i], type_strs[i], match_obj[i+1], match_obj[i+1], start_pos, end_pos));
		}
	}
	return relation_args;
}
