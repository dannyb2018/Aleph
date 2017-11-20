#include <tests/Base.hh>

#include <aleph/containers/PointCloud.hh>

#include <aleph/geometry/BruteForce.hh>
#include <aleph/geometry/VietorisRipsComplex.hh>

#include <aleph/geometry/distances/Euclidean.hh>

#include <aleph/persistentHomology/Calculation.hh>
#include <aleph/persistentHomology/PhiPersistence.hh>

#include <aleph/topology/BarycentricSubdivision.hh>
#include <aleph/topology/Simplex.hh>
#include <aleph/topology/SimplicialComplex.hh>
#include <aleph/topology/Skeleton.hh>
#include <aleph/topology/Spine.hh>

#include <vector>

#include <cmath>

template <class T> void testS1vS1()
{
  using DataType   = T;
  using PointCloud = aleph::containers::PointCloud<DataType>;

  ALEPH_TEST_BEGIN( "Spine: S^1 v S^1" );

  unsigned n = 50;

  PointCloud pc( 2*n, 2 );

  for( unsigned i = 0; i < n; i++ )
  {
    auto x0 = DataType( std::cos( 2*M_PI / n * i ) );
    auto y0 = DataType( std::sin( 2*M_PI / n * i ) );

    auto x1 = x0 + 2;
    auto y1 = y0;

    pc.set(2*i  , {x0, y0});
    pc.set(2*i+1, {x1, y1});
  }

  using Distance          = aleph::geometry::distances::Euclidean<DataType>;
  using NearestNeighbours = aleph::geometry::BruteForce<PointCloud, Distance>;

  auto K
    = aleph::geometry::buildVietorisRipsComplex(
      NearestNeighbours( pc ),
      DataType( 0.30 ),
      2
  );

  auto D1 = aleph::calculatePersistenceDiagrams( K );

  // Persistent homology -----------------------------------------------
  //
  // This should not be surprising: it is possible to extract the two
  // circles from the data set.

  ALEPH_ASSERT_EQUAL( D1.size(),     2 );
  ALEPH_ASSERT_EQUAL( D1[0].betti(), 1 );
  ALEPH_ASSERT_EQUAL( D1[1].betti(), 2 );

  // Persistent intersection homology ----------------------------------
  //
  // Regardless of the stratification, it is impossible to detect the
  // singularity in dimension 0.

  auto L  = aleph::topology::BarycentricSubdivision()( K, [] ( std::size_t dimension ) { return dimension == 0 ? 0 : 0.5; } );
  auto K0 = aleph::topology::Skeleton()( 0, K );
  auto K2 = aleph::topology::Skeleton()( 2, K );

  auto D2 = aleph::calculateIntersectionHomology( L, {K0,K}, aleph::Perversity( {-1} ) );

  ALEPH_ASSERT_EQUAL( D2.size(),     3 );
  ALEPH_ASSERT_EQUAL( D2[0].betti(), 1 );

  ALEPH_TEST_END();
}

template <class T> void testTriangle()
{
  using DataType   = bool;
  using VertexType = T;

  using Simplex           = aleph::topology::Simplex<DataType, VertexType>;
  using SimplicialComplex = aleph::topology::SimplicialComplex<Simplex>;

  SimplicialComplex K = {
    {0,1,2},
    {0,1}, {0,2}, {1,2},
    {0}, {1}, {2}
  };

  auto L = aleph::topology::spine( K );

  ALEPH_ASSERT_THROW( L.size() < K.size() );
  ALEPH_ASSERT_EQUAL( L.size(), 1 );
}

int main( int, char** )
{
  testS1vS1<float> ();
  testS1vS1<double>();

  testTriangle<short>   ();
  testTriangle<unsigned>();
}