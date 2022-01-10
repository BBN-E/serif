/******************************************************************
* Copyright 2008 by BBN Technologies Corp.     All Rights Reserved
******************************************************************/

#ifndef CUBE2_LIB_NEMESIS_LICENSE_HPP
#define CUBE2_LIB_NEMESIS_LICENSE_HPP

#pragma warning(push)
#pragma warning(disable : 4244)
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#pragma warning(pop)

#include <string>
#include <ctime>
#include <cstdio>
#include <utility> // for std::pair<T1, T2>

#include <climits>
#include <cwchar>
#include <boost/static_assert.hpp>

namespace nemesis_lic
{

typedef boost::uint64_t cube2_time_t64;
BOOST_STATIC_ASSERT(sizeof(cube2_time_t64) * CHAR_BIT == 64);

// ----------------------------------------------------------------------
// Convert time_T to boost date.
// ----------------------------------------------------------------------
inline
boost::gregorian::date from_time_t(cube2_time_t64 t)
{
  // The boost::posix_time::time_duration construction routines that
  // take number of seconds all take a long (32-bits) as input.  Here
  // we expect some durations to be larger than can be fit in a long,
  // so we add a little at a time.
  //
  // We know that internally the library can handle larger
  // "durations", it is just that the construction routines do not
  // allow you to create them.

	cube2_time_t64 num_longs = t / std::numeric_limits<boost::int32_t>::max();
	long remainder = t % std::numeric_limits<boost::int32_t>::max();

  boost::posix_time::time_duration duration(boost::posix_time::seconds(0));

  for (cube2_time_t64 j = 0; j < num_longs; ++j) {
	  duration = duration + boost::posix_time::seconds(std::numeric_limits<boost::int32_t>::max());
  }
  duration = duration + boost::posix_time::seconds(remainder);

  boost::posix_time::ptime retval(boost::gregorian::date(1970,1,1), duration);

  return retval.date();
}

// ----------------------------------------------------------------------
// Convert boost posix_time to time_t.
// ----------------------------------------------------------------------
inline
cube2_time_t64 to_time_t(const boost::posix_time::ptime& t)
{
  if(t == boost::posix_time::neg_infin) {
     return 0;
  }
  else if(t == boost::posix_time::pos_infin) {
	  return (cube2_time_t64)std::numeric_limits<boost::int64_t>::max();
  }

  // The simple (t-start).seconds() call returns a long (32-bits).
  // This is now enough to hold some of our durations.  We know that
  // internally the library is capable of more.  Luckily the library
  // gives you access to a 64-bit "number of ticks".  From there it is
  // easy to convert to seconds.

  boost::posix_time::ptime start(boost::gregorian::date(1970,1,1));
  boost::int64_t ticks_duration = (t - start).ticks();
  cube2_time_t64 retval = ticks_duration / boost::posix_time::time_duration::ticks_per_second();

  return retval;
}

// ----------------------------------------------------------------------
// Convert boost date to time_t.
// ----------------------------------------------------------------------
inline
cube2_time_t64 to_time_t(const boost::gregorian::date& d)
{
  boost::posix_time::ptime t(d);
  return to_time_t(t);
}

// ----------------------------------------------------------------------
// Write a license file.
// ----------------------------------------------------------------------
void write_license(FILE* fp,
                          const std::string& component,
                          cube2_time_t64 start_epoch_seconds,
                          cube2_time_t64 end_epoch_seconds,
						  const std::string& restrictions,
                          const std::vector<unsigned char>* mac_addr = 0);

std::string get_restrictions();

// ----------------------------------------------------------------------
// Verify a license file.
// Returns md5sum of license file.
// ----------------------------------------------------------------------
std::string verify_license(FILE* fp,
                           const std::string& component,
                           const bool print_mode = false);

// ----------------------------------------------------------------------
// Helper method to only print the license.
// ----------------------------------------------------------------------
inline void print_license(FILE* fp) {
  verify_license(fp, "IGNORED", true);
}

#endif // CUBE2_LIB_NEMESIS_LICENSE_HPP

}
