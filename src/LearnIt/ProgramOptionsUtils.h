#include <string>

namespace boost {
	namespace program_options{ 
		class options_description;
		class variables_map;
	} 
}

void write_help_msg(const boost::program_options::options_description & desc);

// Validates a command-line argument that must be defined at least once.
void validate_mandatory_cmd_line_arg(const boost::program_options::options_description & desc,
	const boost::program_options::variables_map & var_map, const std::string & param_name);

// Validates a command-line argument that must be defined no more than once.
void validate_unique_cmd_line_arg(const boost::program_options::options_description & desc,
  const boost::program_options::variables_map & var_map, const std::string & param_name);

// Validates a command-line argument that must be defined once and only once.
void validate_mandatory_unique_cmd_line_arg(const boost::program_options::options_description & desc,
	const boost::program_options::variables_map & var_map, const std::string & param_name);

// Validates two command-line arguments that cannot be specified together
void validate_exclusive_cmd_line_args(const boost::program_options::options_description & desc,
	const boost::program_options::variables_map & var_map,
	const std::string & param_name_1, const std::string & param_name_2);
