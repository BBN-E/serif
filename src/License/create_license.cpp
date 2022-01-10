/*************************************************************************
 * Copyright 2008-2012 by Raytheon BBN Technologies.  All Rights Reserved
 *************************************************************************/

#include "License/nemesis_lic/license.hpp"
#include "License/nemesis_log/logging.hpp"
#include "License/nemesis/utilizing/string_utils.hpp"

#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/algorithm/string/replace.hpp>

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <sstream>
#include <cmath>

#ifndef _WIN32
#include <errno.h>
#endif

//
// Main Function
//
int main(int ac, char** av)
{
	srand((unsigned int)time(0));

  // ask user for expiration date
  std::string usage("usage: create_license [-o <out-dir>] -s <YYYY-MM-DD> -e <YYYY-MM-DD> -c <component-name> -u <customer-name> [-m <mac:address:str> -r <restrictions> -n <num-copies>]");
  std::string start_str;
  std::string end_str;
  std::string component;
  std::string customer;
  std::string mac_addr;
  std::string restrictions;
  std::string out_dir;
  int number;

  try {
    po::options_description desc("Options descrition");
    desc.add_options()
      ("help,h",       "produce help message")
      ("start-date,s", po::value<std::string>(&start_str),        "license start date (YYYY-MM-DD)")
      ("end-date,e",   po::value<std::string>(&end_str),          "license   end date (YYYY-MM-DD)")
      ("component,c",  po::value<std::string>(&component),        "license component")
      ("customer,u",   po::value<std::string>(&customer),         "customer name")
      ("mac-addr,m",   po::value<std::string>(&mac_addr),         "target mac-address")
	  ("restrict,r",   po::value<std::string>(&restrictions),     "")
      ("number,n",     po::value<int>(&number)->default_value(1), "number of licenses")
      ("out-dir,o",    po::value<std::string>(&out_dir)->default_value("."), "output directory")
      ;

    po::positional_options_description p;
    p.add("help", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help") || customer.empty() || start_str.empty() || end_str.empty() || component.empty() || 1 == ac) {
      std::cout << std::endl << usage << std::endl << std::endl << desc  << std::endl;
      return 1;
    }
  }
  catch(std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  }
  catch(...) {
    std::cerr << "Exception of unknown type!\n";
  }

  // convert the expiration date to epoch seconds
  nemesis_lic::cube2_time_t64 start_epoch_seconds = 0;
  nemesis_lic::cube2_time_t64 end_epoch_seconds = 0;
  try {
    boost::gregorian::date start = boost::gregorian::from_simple_string(start_str);
    boost::gregorian::date   end = boost::gregorian::from_simple_string(end_str);

    start_epoch_seconds = nemesis_lic::to_time_t(start);
    end_epoch_seconds   = nemesis_lic::to_time_t(end);

    if( (end-start).days() <= 0 ) {
      NEMESIS_LOG(logFATAL) << "Start date [" << start << "] must be after end date [" << end << "]";
      exit(3);
    }

    NEMESIS_LOG(logINFO) << "License valid from: [" << start << "] to [" << end << "]";
    NEMESIS_LOG(logINFO) << "License valid for: [" << (end - start).days() << "] days";
  }
  catch(...) {
    NEMESIS_LOG(logERROR) << "Bad date entered";
    exit(1);
  }

  // number of digits for zero padding
  int num_digits = std::max((int)std::floor(std::log10((double)number)) + 1, 2);

  for (int j = 1; j <= number; ++j) {
    std::stringstream snumformat;
    snumformat.fill('0');
    snumformat.width(num_digits);
    snumformat << j;

    // write the licence file
    std::stringstream sfname;
    sfname << out_dir << "/";
    sfname << customer;
    if (!mac_addr.empty()) {
      sfname << ".";
      std::string mac_path = boost::replace_all_copy(mac_addr, ":", "_");
      sfname << mac_path;
    }
    if (!restrictions.empty()) {
      sfname << ".restrict.";
      std::string restrictions_path = boost::replace_all_copy(restrictions, ":", "_");
      sfname << restrictions_path;
    }
	sfname << ".start.";
    sfname << start_str;
    sfname << ".end.";
    sfname << end_str;
    sfname << ".";
    sfname << snumformat.str();
    sfname << ".lic";

    std::string fname = sfname.str();

    FILE* fp = fopen(fname.c_str(), "wb");
	if (fp == NULL) {
		NEMESIS_LOG(logERROR) << "Could not open " << fname << ": " << std::strerror(errno);
		exit(1);
	}

    if(mac_addr.empty()) {
      nemesis_lic::write_license(fp, component, start_epoch_seconds, end_epoch_seconds, restrictions);
    }
    else {
      std::vector<std::string>    mac_addr_parts = nemesis::split_string(mac_addr, ":");
      std::vector<unsigned char> mac_addr_parts_uchar(mac_addr_parts.size(), 0);

      assert(mac_addr_parts_uchar.size() == 6);

      for(size_t i = 0; i < mac_addr_parts.size(); ++i) {
        long int mac_addr_part_hex = strtol(mac_addr_parts[i].c_str(), 0, 16);
        assert(mac_addr_part_hex < 256);
        assert(mac_addr_part_hex >=  0);
        mac_addr_parts_uchar[i] = (unsigned char)mac_addr_part_hex;
      }

      nemesis_lic::write_license(fp, component, start_epoch_seconds, end_epoch_seconds, restrictions, &mac_addr_parts_uchar);
    }

    fclose(fp);

    NEMESIS_LOG(logINFO) << "Created license file: " << fname;
  }

  return 0;
}
