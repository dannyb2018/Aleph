#include <aleph/geometry/RipsExpander.hh>
#include <aleph/geometry/RipsExpanderTopDown.hh>

#include <tests/Base.hh>

#include <aleph/topology/Simplex.hh>
#include <aleph/topology/SimplicialComplex.hh>

#include <aleph/topology/filtrations/Data.hh>

#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

#include <cmath>

using namespace aleph::geometry;
using namespace aleph::topology;
using namespace aleph;

/** Checks whether co-faces are preceded by their faces in a filtration */
template <class InputIterator> bool isConsistentFiltration( InputIterator begin, InputIterator end )
{
  using Simplex = typename std::iterator_traits<InputIterator>::value_type;

  // Contains all simplices that have already been 'seen' during the
  // traversal. If an unseen face is encountered, we have identified
  // an error.
  std::set<Simplex> seen;

  for( auto it = begin; it != end; ++it )
  {
    seen.insert( *it );

    for( auto itFace = it->begin_boundary(); itFace != it->end_boundary(); ++itFace )
      if( seen.find( *itFace ) == seen.end() )
        return false;
  }

  return true;
}

template <class Data, class Vertex> void triangle()
{
  ALEPH_TEST_BEGIN( "Triangle" );

  using Simplex           = Simplex<Data, Vertex>;
  using SimplicialComplex = SimplicialComplex<Simplex>;

  std::vector<Simplex> simplices
    = { {0}, {1}, {2}, {0,1}, {0,2}, {1,2} };

  SimplicialComplex K( simplices.begin(), simplices.end() );
  RipsExpander<SimplicialComplex> ripsExpander;

  auto vr1 = ripsExpander( K, 2 );
  auto vr2 = ripsExpander( K, 3 );

  ALEPH_ASSERT_THROW( vr1.empty() == false );
  ALEPH_ASSERT_THROW( vr2.empty() == false );
  ALEPH_ASSERT_THROW( vr1.size()  == vr2.size() );
  ALEPH_ASSERT_THROW( vr1.size()  == 7 );

  ALEPH_TEST_END();
}

template <class Data, class Vertex> void nonContiguousTriangle()
{
  ALEPH_TEST_BEGIN( "Triangle (non-contiguous indices)" );

  using Simplex           = Simplex<Data, Vertex>;
  using SimplicialComplex = SimplicialComplex<Simplex>;

  std::vector<Simplex> simplices
    = { {1}, {2}, {4}, {1,2}, {1,4}, {2,4} };

  SimplicialComplex K( simplices.begin(), simplices.end() );
  RipsExpander<SimplicialComplex> ripsExpander;

  auto vr1 = ripsExpander( K, 2 );
  auto vr2 = ripsExpander( K, 3 );

  ALEPH_ASSERT_THROW( vr1.empty() == false );
  ALEPH_ASSERT_THROW( vr2.empty() == false );
  ALEPH_ASSERT_THROW( vr1.size()  == vr2.size() );
  ALEPH_ASSERT_THROW( vr1.size()  == 7 );

  std::vector<Data> data = {
    1, 2, 3
  };

  auto vr3 = ripsExpander.assignMaximumData( vr1, data.begin(), data.end() );

  std::vector<Data> expectedData = {
    1, // [1]
    2, // [2]
    2, // [2,1]
    3, // [4]
    3, // [4,2]
    3, // [4,2,1]
    3, // [4,1]
  };

  std::vector<Data> actualData;
  actualData.reserve( vr3.size() );
  for( auto&& s : vr3 )
    actualData.push_back( s.data() );

  ALEPH_ASSERT_THROW( expectedData == actualData );
  ALEPH_TEST_END();
}

template <class Data, class Vertex> void quad()
{
  ALEPH_TEST_BEGIN( "Quad" );

  using Simplex           = Simplex<Data, Vertex>;
  using SimplicialComplex = SimplicialComplex<Simplex>;

  std::vector<Simplex> simplices
    = { {0},
        {1},
        {2},
        {3},
        Simplex( {0,1}, Data(1) ),
        Simplex( {0,2}, Data( std::sqrt(2.0) ) ),
        Simplex( {1,2}, Data(1) ),
        Simplex( {2,3}, Data(1) ),
        Simplex( {0,3}, Data(1) ),
        Simplex( {1,3}, Data( std::sqrt(2.0) ) ) };

  SimplicialComplex K( simplices.begin(), simplices.end() );
  RipsExpander<SimplicialComplex> ripsExpander;

  auto vr1 = ripsExpander( K, 1 );
  auto vr2 = ripsExpander( K, 2 );
  auto vr3 = ripsExpander( K, 3 );

  vr1 = ripsExpander.assignMaximumWeight( vr1 );
  vr2 = ripsExpander.assignMaximumWeight( vr2 );
  vr3 = ripsExpander.assignMaximumWeight( vr3 );

  vr1.sort( filtrations::Data<Simplex>() );
  vr2.sort( filtrations::Data<Simplex>() );
  vr3.sort( filtrations::Data<Simplex>() );

  ALEPH_ASSERT_THROW( vr1.empty() == false );
  ALEPH_ASSERT_THROW( vr2.empty() == false );
  ALEPH_ASSERT_THROW( vr3.empty() == false );

  ALEPH_ASSERT_THROW( vr1.size() == simplices.size()     );
  ALEPH_ASSERT_THROW( vr2.size() == vr1.size()       + 4 ); // +4 triangles
  ALEPH_ASSERT_THROW( vr3.size() == vr2.size()       + 1 ); // +1 tetrahedron

  ALEPH_ASSERT_THROW( isConsistentFiltration( vr1.begin(), vr1.end() ) );
  ALEPH_ASSERT_THROW( isConsistentFiltration( vr2.begin(), vr2.end() ) );
  ALEPH_ASSERT_THROW( isConsistentFiltration( vr3.begin(), vr3.end() ) );

  ALEPH_TEST_END();
}

template <class Data, class Vertex> void expanderComparison()
{
  ALEPH_TEST_BEGIN( "Rips expander comparison" );

  using Simplex           = Simplex<Data, Vertex>;
  using SimplicialComplex = SimplicialComplex<Simplex>;

  std::vector<Simplex> simplices
    = { {0},
        {1},
        {2},
        {3},
        Simplex( {0,1}, Data(1) ),
        Simplex( {0,2}, Data( std::sqrt(2.0) ) ),
        Simplex( {1,2}, Data(1) ),
        Simplex( {2,3}, Data(1) ),
        Simplex( {0,3}, Data(1) ),
        Simplex( {1,3}, Data( std::sqrt(2.0) ) ) };

  SimplicialComplex K( simplices.begin(), simplices.end() );

  RipsExpander<SimplicialComplex> re;
  RipsExpanderTopDown<SimplicialComplex> retd;

  auto K1 = re( K, 3 );
  auto K2 = retd( K, 3 );

  ALEPH_ASSERT_EQUAL( K1.size(), K2.size() );

  K1 = re.assignMaximumWeight( K1 );
  K2 = retd.assignMaximumWeight( K2, K );

  K1.sort( aleph::topology::filtrations::Data<Simplex>() );
  K2.sort( aleph::topology::filtrations::Data<Simplex>() );

  ALEPH_ASSERT_THROW( K1 == K2 );

  auto K3 = retd( K, 3, 2 );
  K3 = retd.assignMaximumWeight( K3, K );

  K3.sort( aleph::topology::filtrations::Data<Simplex>() );

  ALEPH_ASSERT_THROW( K3.size() < K2.size() );

  {
    auto numSimplices = std::count_if( K2.begin(), K2.end(), [] ( const Simplex& s ) { return s.dimension() >= 2 && s.dimension() <= 3; } );

    ALEPH_ASSERT_EQUAL( decltype(K3.size())( numSimplices ), K3.size() );
  }

  auto K4 = retd( K, 3, 2 );
  K4 = retd.assignMaximumWeight( K4, K );
  K4.sort( aleph::topology::filtrations::Data<Simplex>() );

  ALEPH_ASSERT_EQUAL( K3.size(), K4.size() );

  {
    auto it1 = K3.begin();
    auto it2 = K4.begin();

    for( ; it1 != K3.end() && it2 != K4.end(); ++it1, ++it2 )
    {
      ALEPH_ASSERT_THROW( *it1 == *it2 );
      ALEPH_ASSERT_EQUAL( it1->data(), it2->data() );
    }
  }

  ALEPH_TEST_END();
}

int main()
{
  triangle<double, unsigned>();
  triangle<double, short   >();
  triangle<float,  unsigned>();
  triangle<float,  short   >();

  nonContiguousTriangle<double, unsigned>();
  nonContiguousTriangle<double, short   >();
  nonContiguousTriangle<float,  unsigned>();
  nonContiguousTriangle<float,  short   >();

  quad<double, unsigned>();
  quad<double, short   >();
  quad<float,  unsigned>();
  quad<float,  short   >();

  expanderComparison<double, unsigned>();
  expanderComparison<double, short   >();
  expanderComparison<float,  unsigned>();
  expanderComparison<float,  short   >();
}
