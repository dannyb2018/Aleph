#ifndef ALEPH_GEOMETRY_DOWKER_COMPLEX_HH__
#define ALEPH_GEOMETRY_DOWKER_COMPLEX_HH__

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>

#include <vector>

// FIXME: remove after debugging
#include <iostream>

namespace aleph
{

namespace geometry
{

namespace detail
{

// FIXME: the weight type of an edge should be configurable as an
// additional template parameter
using EdgeWeightProperty = boost::property<boost::edge_weight_t, double>;

// FIXME: the data type of the graph should be configurable as an
// additional template parameter.
using Graph = boost::adjacency_list<
  boost::vecS,
  boost::vecS,
  boost::directedS,
  boost::no_property,
  EdgeWeightProperty
>;

using VertexDescriptor = boost::graph_traits<Graph>::vertex_descriptor;

// FIXME: the index type should be configurable as an additional
// template parameter.
using Pair = std::pair<std::size_t, std::size_t>;

} // namespace detail

/**
  Calculates a set of admissible pairs from a matrix of weights and
  a given distance threshold. The matrix of weights does *not* have
  to satisfy symmetry constraints.

  @param W Weighted adjacency matrix
  @param R Maximum weight
*/

template <class Matrix, class T> std::vector<detail::Pair> admissiblePairs( const Matrix& W, T R )
{
  using namespace detail;

  // Convert matrix into a graph ---------------------------------------

  auto n          = W.size();
  using IndexType = decltype(n);

  detail::Graph G( n );

  for( IndexType i = 0; i < n; i++ )
  {
    for( IndexType j = 0; j < n; j++ )
    {
      if( W[i][j] > 0 )
      {
        EdgeWeightProperty weight = W[i][j];

        boost::add_edge( VertexDescriptor(i), VertexDescriptor(j),
                         weight,
                         G );
      }
    }
  }

  double density
    = static_cast<double>( boost::num_edges(G) ) / static_cast<double>( boost::num_vertices(G) * ( boost::num_vertices(G) - 1 ) );

  // This 'pseudo-matrix' contains the completion of the weight function
  // specified by the input matrix.
  std::vector< std::vector<double> > D( boost::num_vertices(G),
                                        std::vector<double>( boost::num_vertices(G) ) );

  if( density >= 0.5 )
    boost::floyd_warshall_all_pairs_shortest_paths( G, D );
  else
    boost::johnson_all_pairs_shortest_paths( G, D );

  std::vector<Pair> pairs;

  // Create admissible pairs -------------------------------------------
  //
  // A pair is admissible if it satisfies a reachability property,
  // meaning that the induced graph distance permits to reach both
  // vertices under the specified distance threshold.

  for( IndexType i = 0; i < n; i++ )
  {
    for( IndexType j = 0; j < n; j++ )
    {
      if( D[i][j] <= R )
        pairs.push_back( std::make_pair(i,j) );
    }
  }

  return pairs;
}

} // namespace geometry

} // namespace aleph

#endif