/*************************************************************************
 * Copyright 2010-2012 by Raytheon BBN Technologies.  All Rights Reserved
 *************************************************************************/

#include "License/nemesis_lic/license.hpp"
#include "License/nemesis_log/logging.hpp"
#include "Generic/common/version.h"

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>         // equivalent to unistd.h
#include <winsock2.h>   // equivalent to socket.h
#include <iphlpapi.h>   // covers a lot of the network stuff
#else
#include <unistd.h>     // for close()
#include <sys/socket.h>
#include <sys/ioctl.h>  // for ioctl()
#include <net/if.h>     // for struct ifreq
#include <netdb.h>      // for getprotobyname()
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>
#endif

#include <string.h>     // for memcpy()

#include <sys/types.h>  // for socket()

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <set>
#include <memory>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <openssl/evp.h>

// This code uses named version of the operator
#ifdef _WIN32
#define xor ^
#endif

#define  NEMESIS_LICENSE_MAGIC             "BBN-Serif 0003"
#define  NEMESIS_LICENSE_MAGIC_MAC_ADDRESS "BBN-Serif-mac 0003"

#define  NEMESIS_LICENSE_START_XOR_INDEX   10
#define  NEMESIS_LICENSE_START_TIME_INDEX  30

#define  NEMESIS_LICENSE_END_XOR_INDEX     50
#define  NEMESIS_LICENSE_END_TIME_INDEX    70

#define  NEMESIS_HEADER_SIZE              416
#define  NEMESIS_CONTENT_SIZE             576
#define  NEMESIS_MD5SUM_SIZE               32
//                                    + ------
#define  NEMESIS_LICENSE_FILE_SIZE       1024

namespace nemesis_lic
{

typedef boost::shared_ptr<std::vector<unsigned char> > mac_t;
/////////////////////////////////////////////////////////////////////////
// Convert mac-addr to printable string
/////////////////////////////////////////////////////////////////////////
static std::string mac_addr_to_string(const std::vector<unsigned char>& mac_addr)
{
  char mac_addr_str[18];

  sprintf(mac_addr_str,
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac_addr[0],
          mac_addr[1],
          mac_addr[2],
          mac_addr[3],
          mac_addr[4],
          mac_addr[5]
          );
  return std::string(mac_addr_str);
}

typedef std::set<mac_t> macSet_t;
/////////////////////////////////////////////////////////////////////////
// Get the mac addresses for the localhost
/////////////////////////////////////////////////////////////////////////
static const macSet_t& get_local_mac_addrs()
{
  static macSet_t mac_addrs;
  static bool init = false;

  if(init) {
    return mac_addrs;
  }

#ifdef _WIN32
  IP_ADAPTER_ADDRESSES *ipa, *item;
  DWORD len = 0;
  DWORD err = GetAdaptersAddresses(0, 0, 0, 0, &len);
  if (err == ERROR_BUFFER_OVERFLOW && len > 0) {
	ipa = new IP_ADAPTER_ADDRESSES[len];
	err = GetAdaptersAddresses(0, 0, 0, ipa, &len);
	if (err != NO_ERROR) {
	  std::string error_msg = "Serif License: cannot get Interface Information";
      NEMESIS_LOG(logERROR) << error_msg;
      throw std::runtime_error(error_msg);
	}
    for (item = ipa; NULL != item; item = item->Next) {
	  if (item->PhysicalAddressLength) {
        mac_t mac(new std::vector<unsigned char>());
        std::copy(item->PhysicalAddress, item->PhysicalAddress+item->PhysicalAddressLength, std::back_inserter(*mac));
        mac_addrs.insert(mac);
	  }
    }
	if (ipa != NULL) {
	  delete ipa;
	}
  }
#else
  struct ifaddrs *ifa, *item;
  unsigned char zero[6] = {0};

  int err = getifaddrs(&ifa);
  if (0 > err) {
    std::string error_msg = "Serif License: cannot get Interface Information";
    NEMESIS_LOG(logERROR) << error_msg;
    throw std::runtime_error(error_msg);
  }

  for (item = ifa; NULL != item; item = item->ifa_next) {
    if (NULL == item->ifa_addr) {
      continue;
    }
    if (AF_PACKET != item->ifa_addr->sa_family) {
      continue;
    }
    struct sockaddr_ll *sa = (struct sockaddr_ll *)item->ifa_addr;
    if (0 == memcmp(sa->sll_addr, zero, 6)) {
      continue;
    }
    mac_t mac(new std::vector<unsigned char>());
    std::copy(sa->sll_addr, sa->sll_addr+sa->sll_halen, std::back_inserter(*mac));
    mac_addrs.insert(mac);
  }
  if (NULL != ifa) {
    freeifaddrs(ifa);
    ifa = NULL;
  }
#endif

  init = true;
  return mac_addrs;
}

/////////////////////////////////////////////////////////////////////////
// Convert md5sum to printable string
/////////////////////////////////////////////////////////////////////////
static std::string md5_to_string (const unsigned char *digest, int len) {
  std::stringstream sbuf;
  for(int i = 0; i < len; i++){
    if(digest[i] < 16) { // notice HEX base
      sbuf << "0"; // so a '9' comes out as '09', 'a' as '0a', etc.
    }
    sbuf << std::hex << (int)digest[i];
  }
  return sbuf.str();
}

/////////////////////////////////////////////////////////////////////////
// Calculate md5sum from a string of bytes.
/////////////////////////////////////////////////////////////////////////
static std::string calculate_md5(const void *content, size_t len){
  EVP_MD_CTX mdctx;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;

  EVP_DigestInit(&mdctx, EVP_md5());
  EVP_DigestUpdate(&mdctx, content, (size_t) len);
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
  EVP_MD_CTX_cleanup(&mdctx);

  return md5_to_string(md_value, md_len);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Write the license expiration date from a
// BBN Byblos License
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void write_license(FILE* fp,
                   const std::string& component,
                   cube2_time_t64 start_epoch_seconds,
                   cube2_time_t64 end_epoch_seconds,
				   const std::string& restrictions,
                   const std::vector<unsigned char>* mac_addr)
{
  assert(component.length() <= 64);
  assert(restrictions.length() < 128);
  if(0 != mac_addr) {
    assert(mac_addr->size() == 6);
  }
  static std::string license_magic_str(NEMESIS_LICENSE_MAGIC);
  static std::string cube2_version(SerifVersion::getVersionString());

  std::vector<unsigned char> lic_header(NEMESIS_HEADER_SIZE, 0);
  std::vector<unsigned char>::iterator lic_header_it = lic_header.begin();

  unsigned char xor_header = 0;
  while(0 == (xor_header = (unsigned char)(rand() % 256))) {
    // This value will be using for scrambling via xor,
    // therefore it should not be zero.
    ;
  }

  // mac-address constrained license file
  if(mac_addr != 0) {
    license_magic_str = NEMESIS_LICENSE_MAGIC_MAC_ADDRESS;
  }

  // -------------------------------------------------------
  // Make the header;
  // -------------------------------------------------------
  *lic_header_it++ = xor_header; // scrambling key

  *lic_header_it++ = (unsigned char)license_magic_str.length();
  lic_header_it = std::copy(license_magic_str.begin(), license_magic_str.end(), lic_header_it);

  *lic_header_it++ = (unsigned char)cube2_version.length();
  lic_header_it = std::copy(cube2_version.begin(), cube2_version.end(), lic_header_it);

  *lic_header_it++ = (unsigned char)component.length();
  lic_header_it = std::copy(component.begin(), component.end(), lic_header_it);

  *lic_header_it++ = (unsigned char)restrictions.length();
  lic_header_it = std::copy(restrictions.begin(), restrictions.end(), lic_header_it);

  if(mac_addr != 0) {
    *lic_header_it++ = (unsigned char)mac_addr->size();
    lic_header_it = std::copy(mac_addr->begin(), mac_addr->end(), lic_header_it);
  }

  // -------------------------------------------------------
  // Pad header with random numbers
  // -------------------------------------------------------
  for(; lic_header_it != lic_header.end(); ++lic_header_it) {
    *lic_header_it = (unsigned char)(rand() % 256);
  }

  // -------------------------------------------------------
  // Scramble the header (except the first byte which
  // is the scrambling key).
  // -------------------------------------------------------
  for(std::vector<unsigned char>::iterator it = lic_header.begin()+1; it != lic_header_it; ++it){
    //std::cout << (int) xor_header << ' ' << *it << ' ';
    *it = *it xor xor_header;
    //std::cout << *it << std::endl;
  }

  // -------------------------------------------------------
  // Make the license content
  // -------------------------------------------------------
  std::vector<cube2_time_t64> lic_content(NEMESIS_CONTENT_SIZE/sizeof(cube2_time_t64), 0);

  // write the license
  cube2_time_t64 start_xor_val = 0;
  cube2_time_t64   end_xor_val = 0;

  for(size_t i = 0; i < NEMESIS_CONTENT_SIZE/sizeof(cube2_time_t64); ++i) {

    if(NEMESIS_LICENSE_START_TIME_INDEX == i) {
      // write the expiration start time
      assert(start_epoch_seconds != 0);
      assert(start_xor_val != 0);

      NEMESIS_LOG(logDEBUG) << "Unscrambled epoch seconds: " << start_epoch_seconds;
      start_epoch_seconds = start_epoch_seconds xor start_xor_val;
      NEMESIS_LOG(logDEBUG) << "Scrambled   epoch seconds: " << start_epoch_seconds;

      lic_content[i] = start_epoch_seconds;
    }
    else if(NEMESIS_LICENSE_END_TIME_INDEX == i) {
      // write the expiration end time
      assert(end_epoch_seconds != 0);
      assert(end_xor_val != 0);

      NEMESIS_LOG(logDEBUG) << "Unscrambled epoch seconds: " << end_epoch_seconds;
      end_epoch_seconds = end_epoch_seconds xor end_xor_val;
      NEMESIS_LOG(logDEBUG) << "Scrambled   epoch seconds: " << end_epoch_seconds;

      lic_content[i] = end_epoch_seconds;
    }
    else {
      // write (64-bit) some junk
	  boost::uint32_t high = rand();
	  boost::uint32_t low  = rand();

      lic_content[i] = ((cube2_time_t64)high << 32) + low;

      if(NEMESIS_LICENSE_START_XOR_INDEX == i) {
        start_xor_val = lic_content[i];
      }
      else if(NEMESIS_LICENSE_END_XOR_INDEX == i) {
        end_xor_val = lic_content[i];
      }
    }
  }

  // -------------------------------------------------------
  // Write the license to a file.
  // -------------------------------------------------------
  unsigned char lic[NEMESIS_HEADER_SIZE + NEMESIS_CONTENT_SIZE];

  NEMESIS_LOG(logDEBUG) << "copying license header...";
  memcpy(lic,
         &(lic_header[0]),
         NEMESIS_HEADER_SIZE);

  NEMESIS_LOG(logDEBUG) << "copying license content...";
  memcpy(lic+NEMESIS_HEADER_SIZE,
         &(lic_content[0]),
         NEMESIS_CONTENT_SIZE);

  NEMESIS_LOG(logDEBUG) << "writing license header and content...";
  fwrite(lic,
         sizeof(unsigned char),
         NEMESIS_HEADER_SIZE + NEMESIS_CONTENT_SIZE,
         fp);

  // -------------------------------------------------------
  // Write the md5sum of the whole lic. (32 bytes).
  // -------------------------------------------------------
  NEMESIS_LOG(logDEBUG) << "calculating md5sum";
  std::string md5sum_str = calculate_md5(lic, NEMESIS_HEADER_SIZE + NEMESIS_CONTENT_SIZE);
  assert(md5sum_str.length() == NEMESIS_MD5SUM_SIZE);
  NEMESIS_LOG(logDEBUG) << "internal md5sum: " << md5sum_str;

  for(std::string::iterator it = md5sum_str.begin(); it != md5sum_str.end(); ++it) {
    //std::cout << (int) xor_header << ' ' << *it << ' ';
    *it = *it xor xor_header;
    //std::cout << *it << std::endl;
  }

  fwrite(md5sum_str.c_str(), sizeof(unsigned char), NEMESIS_MD5SUM_SIZE, fp);
}

typedef boost::tuple<boost::gregorian::date, // from date
                     boost::gregorian::date  // to date
                     > license_period_t;

/////////////////////////////////////////////////////////////////////////
// Read the license expiration date from a
// BBN Byblos License
/////////////////////////////////////////////////////////////////////////
static license_period_t license_period(cube2_time_t64* license_buf)
{
  assert(license_buf != 0);

  cube2_time_t64 start_xor_val = license_buf[NEMESIS_LICENSE_START_XOR_INDEX];
  cube2_time_t64 end_xor_val   = license_buf[NEMESIS_LICENSE_END_XOR_INDEX];

  cube2_time_t64 start_epoch_seconds = license_buf[NEMESIS_LICENSE_START_TIME_INDEX] xor start_xor_val;
  cube2_time_t64 end_epoch_seconds   = license_buf[NEMESIS_LICENSE_END_TIME_INDEX]   xor end_xor_val;

  NEMESIS_LOG(logDEBUG) << "Scrambled   start epoch seconds: " << license_buf[NEMESIS_LICENSE_START_TIME_INDEX];
  NEMESIS_LOG(logDEBUG) << "Unscrambled start epoch seconds: " << start_epoch_seconds;

  NEMESIS_LOG(logDEBUG) << "Scrambled   end epoch seconds: " << license_buf[NEMESIS_LICENSE_END_TIME_INDEX];
  NEMESIS_LOG(logDEBUG) << "Unscrambled end epoch seconds: " << end_epoch_seconds;

  return license_period_t(from_time_t(start_epoch_seconds), from_time_t(end_epoch_seconds));
}

static std::string restrictions;

std::string get_restrictions() { return restrictions; }

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Verify that a BBN Byblos license is valid
// throws on license error
// returns (internal) md5sum string of license file.
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
std::string verify_license(FILE* fp,
                           const std::string& component_str,
                           const bool print_mode)
{
  assert(fp != 0);

  unsigned char  lic[NEMESIS_HEADER_SIZE + NEMESIS_CONTENT_SIZE];
  cube2_time_t64 lic_content[NEMESIS_CONTENT_SIZE/sizeof(cube2_time_t64)];
  char           lic_md5sum[NEMESIS_MD5SUM_SIZE + 1];
  char junk;

  fread(lic, sizeof(unsigned char), NEMESIS_HEADER_SIZE, fp);
  fread(lic_content, sizeof(cube2_time_t64), NEMESIS_CONTENT_SIZE/sizeof(cube2_time_t64), fp);
  fread(lic_md5sum, sizeof(char), NEMESIS_MD5SUM_SIZE, fp);

  fread(&junk, sizeof(char), 1, fp);

  // ---------------------------------------------
  // First verify (internal) md5sum and file size
  // ---------------------------------------------
  memcpy(lic+NEMESIS_HEADER_SIZE, &(lic_content[0]), NEMESIS_CONTENT_SIZE);

  if(!feof(fp)) {
    std::string error_msg = "License file corrupted (bigger than 1024 bytes).";
    NEMESIS_LOG(logERROR) << error_msg;
    throw std::runtime_error(error_msg);
  }
  std::string comp_md5sum = calculate_md5(lic, NEMESIS_HEADER_SIZE + NEMESIS_CONTENT_SIZE);

  unsigned char* lic_ptr = lic;
  char xor_header = *lic_ptr++;

  for(size_t i = 0; i < NEMESIS_MD5SUM_SIZE; ++i) {
    //std::cout << (int) xor_header << ' ' << lic_md5sum[i] << ' ';
    lic_md5sum[i] = lic_md5sum[i] xor xor_header;
    //std::cout << lic_md5sum[i] << std::endl;
  }
  lic_md5sum[NEMESIS_MD5SUM_SIZE] = '\0';

  // NEMESIS_LOG(logDEBUG) << "read md5sum: " << lic_md5sum;
  // NEMESIS_LOG(logDEBUG) << "comp md5sum: " << comp_md5sum;

  if(0 != comp_md5sum.compare(lic_md5sum)) {
    std::string error_msg = "License file corrupted (md5sum does not match).";
    NEMESIS_LOG(logERROR) << error_msg;
    throw std::runtime_error(error_msg);
  }

  // ---------------------------------------------
  // Second verify header and component name
  // ---------------------------------------------
  for(size_t i = 1; i < NEMESIS_HEADER_SIZE; ++i) {
    //std::cout << (int) xor_header << ' ' << lic[i] << ' ';
    lic[i] = lic[i] xor xor_header;
    //std::cout << lic[i] << std::endl;
  }

  // -------------------------------
  // read/verify the magic name
  // -------------------------------
  size_t magic_len = *lic_ptr++;
  std::string magic(magic_len, 0);
  std::copy(lic_ptr, lic_ptr+magic_len, magic.begin());
  lic_ptr += magic_len;
  NEMESIS_LOG(logDEBUG) << "magic: " << magic;

  if(0 != magic.compare(NEMESIS_LICENSE_MAGIC) &&
     0 != magic.compare(NEMESIS_LICENSE_MAGIC_MAC_ADDRESS)) {
    std::string error_msg("Not a valid Serif license file");
    NEMESIS_LOG(logERROR) << error_msg;
    throw std::runtime_error(error_msg);
  }

  // -------------------------------
  // read the cube2 release
  // -------------------------------
  size_t cube2_version_len = *lic_ptr++;
  std::string cube2_version(cube2_version_len, 0);
  std::copy(lic_ptr, lic_ptr+cube2_version_len, cube2_version.begin());
  lic_ptr += cube2_version_len;
  NEMESIS_LOG(logDEBUG) << "serif_version: " << cube2_version;

  // -------------------------------
  // read/verify the component name
  // -------------------------------
  size_t component_len = *lic_ptr++;
  std::string component(component_len, 0);
  std::copy(lic_ptr, lic_ptr+component_len, component.begin());
  lic_ptr += component_len;
  NEMESIS_LOG(logDEBUG) << "component: " << component;

  if(!print_mode && 0 != component.compare(component_str)) {
    std::string error_msg("License component: "+component+" does not match requested component: "+component_str);
    NEMESIS_LOG(logERROR) << error_msg;
    throw std::runtime_error(error_msg);
  }

  // -------------------------------
  // read the restrictions (if any)
  // -------------------------------
  size_t restrictions_len = *lic_ptr++;
  restrictions = std::string(restrictions_len, 0);
  std::copy(lic_ptr, lic_ptr+restrictions_len, restrictions.begin());
  lic_ptr += restrictions_len;
  NEMESIS_LOG(logDEBUG) << "restrictions: " << restrictions;

  // -------------------------------
  // read/verify the mac-address
  // (if needed).
  // -------------------------------
  std::string lic_mac_addr_str;
  if(magic == NEMESIS_LICENSE_MAGIC_MAC_ADDRESS) {
    size_t mac_addr_len = *lic_ptr++;
    std::vector<unsigned char> mac_addr;
    std::copy(lic_ptr, lic_ptr+mac_addr_len, std::back_inserter(mac_addr));
    lic_ptr += component_len;

    lic_mac_addr_str = mac_addr_to_string(mac_addr);

    const macSet_t & mac_addrs = get_local_mac_addrs();
    bool anyMatch = false;
    for (macSet_t::const_iterator mac = mac_addrs.begin() ; mac != mac_addrs.end() ; ++mac) {
      if (mac_addr == (**mac)) { 
        anyMatch = true;
        break;
      }
    }
    if(!print_mode && ! anyMatch) {
      std::string error_msg("No Localhost MAC address matches the license MAC address ("+
                            lic_mac_addr_str +
                            ")");
      NEMESIS_LOG(logERROR) << error_msg;
      throw std::runtime_error(error_msg);
    }
  }

  // ---------------------------------------------
  // Third verify the license period
  // ---------------------------------------------
  license_period_t lic_period  = license_period(lic_content);

  boost::gregorian::date start = lic_period.get<0>();
  boost::gregorian::date   end = lic_period.get<1>();
  boost::gregorian::date today = boost::gregorian::day_clock::local_day();

  if((today - start).days() < 0) {
    std::string error_msg("License period has not yet started: today [" + boost::gregorian::to_simple_string(today) + "]" +
                          "is before the start of the license period [" + boost::gregorian::to_simple_string(start) + "]");
    NEMESIS_LOG(logERROR) << error_msg;
    throw std::runtime_error(error_msg);
  }

  if(!print_mode && (end - today).days() <= 0) {
    std::string error_msg("License period ended: ["  + boost::gregorian::to_simple_string(end) + "]");
    NEMESIS_LOG(logERROR) << error_msg;
    throw std::runtime_error(error_msg);
  }

  // ---------------------------------------------
  // Print license info
  // ---------------------------------------------
  if(print_mode) {
    NEMESIS_LOG(logINFO) << "File is a BBN Serif License";
    NEMESIS_LOG(logINFO) << "License Component name: " << component;
    NEMESIS_LOG(logINFO) << "License Serif release: "  << cube2_version;
    NEMESIS_LOG(logINFO) << "License valid from: [" << start << "] to [" << end << "]";
    NEMESIS_LOG(logINFO) << "Expires in: [" << (end - today).days() << "] days";
	if (restrictions.length()) {
	  NEMESIS_LOG(logINFO) << "Additional restrictions:";
	  NEMESIS_LOG(logINFO) << restrictions;
	}
    if(magic == NEMESIS_LICENSE_MAGIC_MAC_ADDRESS) {
      NEMESIS_LOG(logINFO) << "MAC-Address: " << lic_mac_addr_str;
    }
    //NEMESIS_LOG(logINFO) << "MD5SUM: " << lic_md5sum;
  }

  return lic_md5sum;
}

} // namespace nemesis
