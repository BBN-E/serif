#include "Generic/common/leak_detection.h"
#include "ProgramOptionsUtils.h"
#include <iostream>
#include <boost/program_options.hpp>

using std::cout;
using std::endl;

void write_help_msg(const boost::program_options::options_description & desc) {
	cout << desc << "\n";
	cout << "An argument whose value is \"\" will be ignored.\n"
		 << "Multiple instances can be given where indicated (e.g., \"-p param1:true -p param2:false\")." << endl;
	exit(-1);
}

// Validates a command-line argument that must be defined at least once.
void validate_mandatory_cmd_line_arg(const boost::program_options::options_description & desc,
								  const boost::program_options::variables_map & var_map, const std::string & param_name) {
	if (var_map.count(param_name) < 1) {
		std::cerr << "Mandatory command-line argument '" << param_name << "' was not specified." << std::endl; 
		write_help_msg(desc);
	}
}

// Validates a command-line argument that must be defined no more than once.
void validate_unique_cmd_line_arg(const boost::program_options::options_description & desc,
								  const boost::program_options::variables_map & var_map, const std::string & param_name) {
	if (var_map.count(param_name) > 1) {
		std::cerr << "Command-line argument '" << param_name << "' can only be specified at most once." << std::endl; 
		write_help_msg(desc);
	}
}

// Validates a command-line argument that must be defined once and only once.
void validate_mandatory_unique_cmd_line_arg(const boost::program_options::options_description & desc,
								  const boost::program_options::variables_map & var_map, const std::string & param_name) {
	validate_mandatory_cmd_line_arg(desc, var_map, param_name);
	validate_unique_cmd_line_arg(desc, var_map, param_name);
}

// Validates two command-line arguments that cannot be specified together
void validate_exclusive_cmd_line_args(const boost::program_options::options_description & desc,
								  	  const boost::program_options::variables_map & var_map,
									  const std::string & param_name_1, const std::string & param_name_2) {
	if (var_map.count(param_name_1) >= 1 && var_map.count(param_name_2) >= 1) {
		std::cerr << "Command-line arguments '" << param_name_1 << "' and '" << param_name_2 << " are mutually exclusive." << std::endl; 
		write_help_msg(desc);
	}
}


