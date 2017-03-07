#include "CalltreeGraphManager.h"

#include <boost/graph/graphviz.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>

#include <iostream>
#include <iomanip>


namespace ArgoNavis { namespace GUI {


/**
 * Template class for writing graph vertex information in DOT format
 */
template <typename PropMap>
class node_writer
{
public:
    node_writer(const PropMap& pmap)
        : m_pmap(pmap) {}
    template <class Vertex>
    void operator()(std::ostream& out, const Vertex& v) const
    {
        out << " [label=\"" << m_pmap[v].functionName
            << "\", file=\"" << m_pmap[v].sourceFilename
            << "\", line=\"" << m_pmap[v].lineNumber
            << "\", unit=\"" << m_pmap[v].linkedObjectName;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        using namespace std;
#else
        using namespace boost;
#endif

        for(int i=0; i<m_pmap[v].metricValues.size(); i++) {
            tuple< std::string, std::string > nameValuePair = m_pmap[v].metricValues[i];
            out << "\", " << get<0>(nameValuePair) << "=\"" << get<1>(nameValuePair);
        }

        out << "\"]";
    }
private:
    const PropMap& m_pmap;
};

/**
 * Template class for writing graph edge information in DOT format
 */
template <typename PropMap>
class edge_writer
{
public:
    edge_writer(const PropMap& pmap)
        : m_pmap(pmap) {}
    template <class Edge>
    void operator()(std::ostream& out, const Edge& e) const
    {
#if 1
        out << " [label=\"" << m_pmap[e] << "\"]";
#else
        out << " [label=\"" << m_pmap[e].label;

        for(int i=0; i<m_pmap[e].metricValues.size(); i++) {
            std::tuple< std::string, std::string > nameValuePair = m_pmap[e].metricValues[i];
            out << "\", " << std::get<0>(nameValuePair) << "=\"" << std::get<1>(nameValuePair);
        }
        out << "\"]";
#endif
    }
private:
    const PropMap& m_pmap;
};

/**
 * @brief CalltreeGraphManager::CalltreeGraphManager
 * @param parent - the parent object
 *
 * Constructs a CalltreeGraphManager instance
 */
CalltreeGraphManager::CalltreeGraphManager(QObject *parent)
    : QObject( parent )
{

}

/**
 * @brief CalltreeGraphManager::addFunctionNode
 * @param functionName - the function's name (is the vertex/node label)
 * @param sourceFilename - the associated source-code filename
 * @param lineNumber - the line number in the associated source-code file
 * @param linkedObjectName - the name of the linked object (the program executable or other dependent library name)
 * @param metricValues - the set of metric name/value pairs for the function
 * @return - the internal handle value used for subsequent method calls (such as addCallEdge() method)
 *
 * This method adds a vertex to the calltree and returns the internal handle value.
 */
CalltreeGraphManager::handle_t CalltreeGraphManager::addFunctionNode(
        const std::string &functionName,
        const std::string &sourceFilename,
        uint32_t lineNumber,
        const std::string &linkedObjectName,
        const CalltreeGraphManager::MetricValues metricValues)
{
    // Create vertex
    vertex_t vertex = boost::add_vertex( m_calltree );

    // Set the properties of the vertex
    m_calltree[vertex].functionName = functionName;
    m_calltree[vertex].sourceFilename = sourceFilename;
    m_calltree[vertex].lineNumber = lineNumber;
    m_calltree[vertex].linkedObjectName = linkedObjectName;
    m_calltree[vertex].metricValues = metricValues;

    m_vertices.push_back( vertex );

    return m_vertices.size()-1;
}

/**
 * @brief CalltreeGraphManager::addCallEdge
 * @param head - the calling function node handle (caller)
 * @param tail - the function being called node handle (callee)
 * @param labelOrMetricName - the label for the edge (either refers to 'name' in set of metric name/value pairs or the actual label value)
 * @param metricValues - the set of metric name/value pairs for the edge
 * @return - the internal handle value used for subsequent method calls (TBD)
 *
 * This method adds an edge to the calltree which defines the caller-callee relationship between two previously
 * defined function nodes and upon success returns the internal handle value for the edge.  If the caller or
 * callee node handles are invalid or the operation to add an edge to the calltree fails, an 'edge_exception'
 * will be raised which contains a string value indicating which of the three conditions occurred:
 *     "invalid head handle", "invalid tail handle" or "edge not added"
 */
CalltreeGraphManager::handle_t CalltreeGraphManager::addCallEdge(
        const CalltreeGraphManager::handle_t &head,
        const CalltreeGraphManager::handle_t &tail,
        const std::string &labelOrMetricName,
        const CalltreeGraphManager::MetricValues& metricValues)
{
    // The following variables are tied to std::pair values returned by boost::add_edge
    bool added;   // the second part of the std::pair indicating whether the edge was successfully added to graph
    edge_t edge;  // the first part of the std::pair providing the edge descriptor for the newly created edge

    // Validate "head" and "tail" node handles
    if ( head >= m_vertices.size() )
        throw edge_exception("invalid head handle");
    if ( tail >= m_vertices.size() )
        throw edge_exception("invalid tail handle");

    // Lookup "head" and "tail" node vertex descriptors
    vertex_t head_node = m_vertices[head];
    vertex_t tail_node = m_vertices[tail];

#if 1
    // Create an edge connecting vertex "head" to "tail"
    // NOTE: At this point we do have inclusive times to assign to the edge weight and in fact
    // we wish to have the edge weights to all have the value of one so that the call depths
    // can be computed using the boost::johnson_all_pairs_shortest_paths algorithm.
    boost::tie(edge, added) = boost::add_edge( head_node, tail_node, 1.0, m_calltree );
#else
    // Create an edge conecting vertex "head" to "tail"
    boost::tie(edge, added) = boost::add_edge( head_node, tail_node, m_calltree );
#endif

    if ( ! added )
        throw edge_exception("edge not added");

#if 0
    // Determine actual label value
    std::string label( labelOrMetricName );
    foreach ( const NameValuePair_t& pair, metricValues ) {
        if ( std::get<0>(pair) == labelOrMetricName ) {
            label = std::get<1>(pair);
            break;
        }
    }

    // Set the properties of the edge
    m_calltree[edge].label = label;
    m_calltree[edge].metricValues = metricValues;
#endif

    const handle_t this_handle = m_edges.size();

    m_edges[ edge ] = this_handle;

    return this_handle;
}

/**
 * @brief CalltreeGraphManager::write_graphviz
 * @param os - the output stream for writing - ie std::cout, std::ostringstream
 *
 * This method writes the calltree representation in DOT format.  The bundled vertex and edge attributes are
 * preserved as external attributes.  The 'label' attribute will also be written for each vertex and edge.
 */
void CalltreeGraphManager::write_graphviz(std::ostream& os)
{
    // Get vertex and edge attribute bundles
    boost::property_map< CallTree, boost::vertex_bundle_t >::type
            vertexBundle = boost::get(boost::vertex_bundle, m_calltree);
#if 0
    boost::property_map< CallTree, boost::edge_bundle_t>::type
            edgeBundle = boost::get(boost::edge_bundle, m_calltree);

    // Write Boost graph structure in DOT format to specified output stream
    boost::write_graphviz( os, m_calltree,
                           node_writer< boost::property_map< CallTree, boost::vertex_bundle_t >::type>(vertexBundle),
                           edge_writer< boost::property_map< CallTree, boost::edge_bundle_t >::type>(edgeBundle) );
#else
    boost::property_map< CallTree, boost::edge_weight_t>::type weightMap = boost::get( boost::edge_weight, m_calltree );

    // Write Boost graph structure in DOT format to specified output stream
    boost::write_graphviz( os, m_calltree,
                           node_writer< boost::property_map< CallTree, boost::vertex_bundle_t >::type>(vertexBundle),
                           edge_writer<boost::property_map< CallTree, boost::edge_weight_t>::type>(weightMap) );
#endif
}

/**
 * @brief CalltreeGraphManager::generate_call_depths
 * @param call_depth_map - the map of each pair of functions and the depth of the stackframe from the first function in the pair to the second
 *
 * This method produces a map of each pair of functions in a calltree and the depth of the stackframe from the first function in the pair to the second.
 */
void CalltreeGraphManager::generate_call_depths(std::map< std::pair< handle_t, handle_t>, uint32_t >& call_depth_map)
{
    const int V = m_vertices.size();

    std::vector < double > d( V, (std::numeric_limits < double >::max)() );

    double** D = new double*[ V * sizeof(double) ];
    for (int i=0; i<V; i++)
        D[i] = new double[ V * sizeof(double) ];

    boost::johnson_all_pairs_shortest_paths( m_calltree, D, boost::distance_map(d.data()) );

    for ( int i = 0; i < V; ++i ) {
        for ( int j = 0; j < V; ++j ) {
            if ( D[i][j] != 0 && D[i][j] != std::numeric_limits<double>::max() ) {
                call_depth_map[ std::make_pair( i, j ) ] = D[i][j];
            }
        }
    }

    for ( int i=0; i<V; i++ )
        delete[] D[i];

    delete[] D;
}

/**
 * @brief CalltreeGraphManager::setEdgeWeights
 * @param callPairToWeightMap - the edge weight map
 *
 * Set the weight values in the property map to the value in the callpair-to-weight map.
 */
void CalltreeGraphManager::setEdgeWeights(const EdgeWeightMap& edgeWeightMap)
{
    // get calltree graph edge weight property map
    boost::property_map< CallTree, boost::edge_weight_t>::type weightMap = boost::get( boost::edge_weight, m_calltree );

    // iterate thru all the edges and set the weight values in the property map to the value in the callpair-to-weight map
    boost::graph_traits < CallTree >::edge_iterator e, e_end;
    for ( boost::tie(e, e_end) = boost::edges(m_calltree); e != e_end; ++e ) {
        edge_t desc = *e;
        if ( m_edges.find( desc ) != m_edges.end() ) {
            handle_t handle = m_edges[ desc ];
            if ( edgeWeightMap.find( handle ) != edgeWeightMap.end() ) {
                weightMap[ desc ] = edgeWeightMap.at( handle );
            }
        }
    }
}


} // GUI
} // ArgoNavis
