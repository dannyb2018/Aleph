#ifndef ALEPH_TOPOLOGY_SPINE_HH__
#define ALEPH_TOPOLOGY_SPINE_HH__

#include <aleph/topology/Intersections.hh>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>

namespace aleph
{

namespace topology
{

// Contains the 'dumbest' implementation for calculating the spine, i.e.
// without any optimizations or skips. This will be used as the baseline
// for comparisons, but also to check the correctness of the approach.
namespace dumb
{

/**
  Checks whether a simplex in a simplicial complex is principal, i.e.
  whether it is not a proper face of any other simplex in K.
*/

template <class SimplicialComplex, class Simplex> bool isPrincipal( const Simplex& s, const SimplicialComplex& K )
{
  // Individual vertices cannot be considered to be principal because
  // they do not have a free face.
  if( s.dimension() == 0 )
    return false;

  bool principal = true;
  auto itPair    = K.range( s.dimension() + 1 );

  for( auto it = itPair.first; it != itPair.second; ++it )
  {
    auto&& t = *it;

    // This check assumes that the simplicial complex is valid, so it
    // suffices to search faces in one dimension _below_ s. Note that
    // the check only has to evaluate the *size* of the intersection,
    // as this is sufficient to determine whether a simplex is a face
    // of another simplex.
    if( sizeOfIntersection(s,t) == s.size() )
      principal = false;
  }

  return principal;
}

/**
  Checks whether a simplex in a simplicial complex is admissible, i.e.
  the simplex is *principal* and has at least one free face.
*/

template <class SimplicialComplex, class Simplex> Simplex isAdmissible( const Simplex& s, const SimplicialComplex& K )
{
  if( !isPrincipal(s,K) )
    return Simplex();

  // Check whether a free face exists ----------------------------------
  //
  // This involves iterating over all simplices that have the *same*
  // dimension as s, because we are interested in checking whether a
  // simplex shares a face of s.

  std::vector<Simplex> faces( s.begin_boundary(), s.end_boundary() );
  std::vector<bool> admissible( faces.size(), true );

  std::size_t i = 0;
  auto itPair   = K.range( s.dimension() ); // valid range for searches, viz. *all*
                                            // faces in "one dimension up"

  for( auto&& face : faces )
  {
    for( auto it = itPair.first; it != itPair.second; ++it )
    {
      auto&& t = *it;

      // We do not have to check for intersections with the original
      // simplex from which we started---we already know that we are
      // a face.
      if( t != s )
      {
        if( sizeOfIntersection(face,t) == face.size() )
        {
          admissible[i] = false;
          break;
        }
      }
    }

    ++i;
  }

  // Return the free face if possible; as usual, an empty return value
  // indicates that we did not find such a face. Note that the call to
  // the `find()` function prefers the lexicographically smallest one,
  // which ensures consistency.
  auto pos = std::find( admissible.begin(), admissible.end(), true );
  if( pos == admissible.end() )
    return Simplex();
  else
    return faces.at( std::distance( admissible.begin(), pos ) );
}

/**
  Calculates all principal faces of a given simplicial complex and
  returns them.
*/

template <class SimplicialComplex> std::unordered_map<typename SimplicialComplex::value_type, typename SimplicialComplex::value_type> principalFaces( const SimplicialComplex& K )
{
  using Simplex = typename SimplicialComplex::value_type;
  auto L        = K;

  std::unordered_map<Simplex, Simplex> admissible;

  // Step 1: determine free faces --------------------------------------
  //
  // This first checks which simplices have at least one free face,
  // meaning that they may be potentially admissible.

  for( auto it = L.begin(); it != L.end(); ++it )
  {
    if( it->dimension() == 0 )
      continue;

    // The range of the complex M is sufficient because we have
    // already encountered all lower-dimensional simplices that
    // precede the current one given by `it`.
    //
    // This complex will be used for testing free faces.
    SimplicialComplex M( L.begin(), it );

    // FIXME:
    //
    // In case of equal data values, the assignment from above does
    // *not* work and will result in incorrect candidates.
    M = L;

    bool hasFreeFace = false;
    Simplex freeFace = Simplex();

    for( auto itFace = it->begin_boundary(); itFace != it->end_boundary(); ++itFace )
    {
      bool isFace = false;
      for( auto&& simplex : M )
      {
        if( itFace->dimension() + 1 == simplex.dimension() && simplex != *it )
        {
          // The current face must *not* be a face of another simplex in
          // the simplicial complex.
          if( intersect( *itFace, simplex ) == *itFace )
          {
            isFace = true;
            break;
          }
        }
      }

      hasFreeFace = !isFace;
      if( hasFreeFace )
      {
        freeFace = *itFace;
        break;
      }
    }

    if( hasFreeFace )
      admissible.insert( std::make_pair( *it, freeFace ) );
  }

  // Step 2: determine principality ------------------------------------
  //
  // All simplices that are faces of higher-dimensional simplices are
  // now removed from the map of admissible simplices.

  for( auto&& s : L )
  {
    for( auto itFace = s.begin_boundary(); itFace != s.end_boundary(); ++itFace )
      admissible.erase( *itFace );
  }

  return admissible;
}

/**
  Performs an iterated elementary simplicial collapse until *all* of the
  admissible simplices have been collapsed. This leads to the *spine* of
  the simplicial complex.

  Notice that this is the *dumbest* possible implementation, as no state
  will be stored, and the search for new principal faces starts fresh in
  every iteration.

  This implementation is useful to check improved algorithms.

  @see S. Matveev, "Algorithmic Topology and Classification of 3-Manifolds"
*/

template <class SimplicialComplex> SimplicialComplex spine( const SimplicialComplex& K )
{
  auto L          = K;
  auto admissible = principalFaces( L );

  while( !admissible.empty() )
  {
    auto s = admissible.begin()->first;
    auto t = admissible.begin()->second;

    L.remove_without_validation( s );
    L.remove_without_validation( t );

    admissible = principalFaces( L );
  }

  return L;
}

} // namespace dumb

// ---------------------------------------------------------------------
// From this point on, only 'smart' implementations of the spine
// calculation will be given. Various optimizations will be used
// in order to improve the run-time.
// ---------------------------------------------------------------------

/**
  Stores coface relationships in a simplicial complex. Given a simplex
  \f$\sigma\f$, the map contains all of its cofaces. Note that the map
  will be updated upon every elementary collapse.
*/

template <class Simplex> using CofaceMap = std::unordered_map<Simplex, std::unordered_set<Simplex> >;

template <class SimplicialComplex> CofaceMap<typename SimplicialComplex::ValueType> buildCofaceMap( const SimplicialComplex& K )
{
  using Simplex   = typename SimplicialComplex::ValueType;
  using CofaceMap = CofaceMap<Simplex>;

  CofaceMap cofaces;
  for( auto&& s : K )
  {
    // Adding an *empty* list of cofaces (so far) for this simplex
    // simplifies the rest of the code because there is no need to
    // check for the existence of a simplex.
    if( cofaces.find(s) == cofaces.end() )
      cofaces[s] = {};

    for( auto itFace = s.begin_boundary(); itFace != s.end_boundary(); ++itFace )
      cofaces[ *itFace ].insert( s );
  }

  return cofaces;
}

/**
  Checks whether a given simplex is *principal* with respect to its
  coface relations. A principal simplex is not the proper face of a
  simplex in the complex. Hence, it has no cofaces.
*/

template <class Simplex> bool isPrincipal( const CofaceMap<Simplex>& cofaces, const Simplex& s )
{
  return cofaces.at( s ).empty();
}

/**
  Given a *principal* simplex, i.e. a simplex that is not a proper face
  of another simplex in the complex, returns the first free face of the
  simplex, i.e. a face that only has the given simplex as a coface.

  If no such face is found, the empty simplex is returned.
*/

template <class Simplex> Simplex getFreeFace( const CofaceMap<Simplex>& cofaces, const Simplex& s )
{
  if( !isPrincipal( cofaces, s ) )
    return Simplex();

  // Check whether a free face exists ----------------------------------

  for( auto itFace = s.begin_boundary(); itFace != s.end_boundary(); ++itFace )
  {
    auto&& allCofaces = cofaces.at( *itFace );
    if( allCofaces.size() == 1 && allCofaces.find( s ) != allCofaces.end() )
      return *itFace;
  }

  return Simplex();
}

/**
  Gets *all* principal simplices along with their free faces and stores them in
  a map. The map contains the principal simplex as its key, and the *free face*
  as its value.
*/

template <class SimplicialComplex> std::unordered_map<typename SimplicialComplex::value_type, typename SimplicialComplex::value_type> getPrincipalFaces( const CofaceMap<typename SimplicialComplex::ValueType>& cofaces, const SimplicialComplex& K )
{
  using Simplex = typename SimplicialComplex::value_type;
  auto L        = K;

  std::unordered_map<Simplex, Simplex> admissible;

  for( auto&& s : K )
  {
    auto freeFace = getFreeFace( cofaces, s );
    if( freeFace )
      admissible.insert( std::make_pair( s, freeFace ) );
  }

  return admissible;
}

/**
  Performs an iterated elementary simplicial collapse until *all* of the
  admissible simplices have been collapsed. This leads to the *spine* of
  the simplicial complex.

  @see S. Matveev, "Algorithmic Topology and Classification of 3-Manifolds"
*/

template <class SimplicialComplex> SimplicialComplex spine( const SimplicialComplex& K )
{
  auto L          = K;
  auto cofaces    = buildCofaceMap( L );
  auto admissible = getPrincipalFaces( cofaces, L );

  while( !admissible.empty() )
  {
    auto s = admissible.begin()->first;
    auto t = admissible.begin()->second;

    L.remove_without_validation( s );
    L.remove_without_validation( t );

    admissible.erase( s );

    // Predicate for removing s and t, the principal simplex with its
    // free face, from the coface data structure. This is required in
    // order to keep the coface relation up-to-date.
    using Simplex        = typename SimplicialComplex::ValueType;
    auto removeSimplices = [&cofaces,&s,&t] ( const Simplex& sigma )
    {
      cofaces.at(sigma).erase(s);
      cofaces.at(sigma).erase(t);
    };

    std::for_each( s.begin_boundary(), s.end_boundary(), removeSimplices );
    std::for_each( t.begin_boundary(), t.end_boundary(), removeSimplices );

    // Both s and t do not have to be stored any more because they
    // should not be queried again.
    cofaces.erase(s);
    cofaces.erase(t);

    // New simplices ---------------------------------------------------
    //
    // Add new admissible simplices that may potentially have been
    // spawned by the removal of s.

    // 1. Add all faces of the principal simplex, as they may
    //    potentially become admissible again.
    std::vector<Simplex> faces( s.begin_boundary(), s.end_boundary() );

    std::for_each( faces.begin(), faces.end(),
      [&t, &admissible, &cofaces] ( const Simplex& s )
      {
        if( t != s )
        {
          auto face = getFreeFace( cofaces, s );
          if( face )
            admissible.insert( std::make_pair( s, face ) );
        }
      }
    );

    // 2. Add all faces of the free face, as they may now themselves
    //    become admissible.
    faces.assign( t.begin_boundary(), t.end_boundary() );

    std::for_each( faces.begin(), faces.end(),
      [&admissible, &cofaces] ( const Simplex& s )
      {
        auto face = getFreeFace( cofaces, s );
        if( face )
          admissible.insert( std::make_pair( s, face ) );
      }
    );

    // The heuristic above is incapable of detecting *all* principal
    // faces of the complex because this may involve searching *all*
    // co-faces. Instead, it is easier to fill up the admissible set
    // here.
    if( admissible.empty() )
      admissible = getPrincipalFaces( cofaces, L );
  }

  return L;
}

} // namespace topology

} // namespace aleph

#endif
