/*************************************************************************
 * Copyright 2008-2012 by Raytheon BBN Technologies.  All Rights Reserved
 *************************************************************************/

#include "License/nemesis_lic/license.hpp"
#include "License/nemesis_log/logging.hpp"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>

//
// Main Function
//
int main(int argc, char** argv)
{
  int exit_val = 1;

  if(argc != 3) {
    std::cerr << "usage: verify_license <lic-file> <component>" << std::endl;
    exit(1);
  }

  // read the license info licence file
  FILE* fp = fopen(argv[1], "rb");
  bool print_mode = false;
  std::string component(argv[2]);

  try {
    nemesis_lic::verify_license(fp, component,  print_mode);
    exit_val = 0;
  }
  catch(std::exception& e) {
    NEMESIS_LOG(logFATAL) << e.what();
  }

  fclose(fp);

  exit(exit_val);
}

