#ifndef ALEPH_STRING_HH__
#define ALEPH_STRING_HH__

#include <algorithm>
#include <regex>
#include <sstream>
#include <vector>

#include <cctype>

namespace aleph
{

namespace utilities
{

template <class T> T ltrim( T sequence )
{
  sequence.erase( sequence.begin(),
                  std::find_if_not( sequence.begin(), sequence.end(),
                                    [] ( const typename T::value_type& c )
                                    {
                                      return std::isspace( c );
                                    } ) );

  return sequence;
}

template <class T> T rtrim( T sequence )
{
  sequence.erase( std::find_if_not( sequence.rbegin(), sequence.rend(),
                                    [] ( const typename T::value_type& c )
                                    {
                                      return std::isspace( c );
                                    } ).base(),
                  sequence.end() );

  return sequence;
}

template <class T> T trim( T sequence )
{
  return ltrim( rtrim( sequence ) );
}

template <class T> std::vector<T> split( const T& sequence,
                                         const T& regex = "[[:space:]]+" )
{
  std::regex re( regex );
  std::sregex_token_iterator begin( sequence.begin(), sequence.end(), re, -1 );
  std::sregex_token_iterator end;

  return { begin, end };
}

template <class T1, class T2> T2 convert( const T1& sequence )
{
  T2 result = T2();

  std::istringstream converter( sequence );
  converter >> result;

  return result;
}

}

}

#endif
