#ifndef CALLTREEGRAPHMANAGER_H
#define CALLTREEGRAPHMANAGER_H

#include <QObject>

#include <boost/graph/adjacency_list.hpp>

#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <iostream>

namespace ArgoNavis { namespace GUI {


class CalltreeGraphManager : public QObject
{
    Q_OBJECT

public:

    explicit CalltreeGraphManager(QObject *parent = 0);

    typedef std::size_t handle_t;

    typedef std::tuple< std::string, std::string > NameValuePair_t;
    typedef std::vector< NameValuePair_t > MetricValues;

    typedef std::logic_error edge_exception;

    handle_t addFunctionNode(const std::string& functionName,
                             const std::string& sourceFilename,
                             uint32_t lineNumber,
                             const std::string& linkedObjectName,
                             const MetricValues metricValues = MetricValues());

    handle_t addCallEdge(const handle_t& head,
                         const handle_t& tail,
                         const std::string& labelOrMetricName = std::string(),
                         const MetricValues& metricValues = MetricValues());

    void write_graphviz(std::ostream& os);

    void generate_call_depths(std::map< std::pair< handle_t, handle_t>, uint32_t >& call_depth_map);

private:

    // Create a struct to hold properties for each vertex
    struct VertexProperties
    {
       // stack frame information
       std::string functionName;
       std::string sourceFilename;
       uint32_t lineNumber;
       std::string linkedObjectName;
       // metric values for this stack frame item
       MetricValues metricValues;
    };

    // Create a struct to hold properties for each edge
    struct EdgeProperties
    {
        // label for edge
        std::string label;
        // metric values for this stack frame item
        MetricValues metricValues;
    };

    // Define the graph using those classes
#if 1
    using CallTree = boost::adjacency_list< boost::listS, boost::vecS, boost::directedS, VertexProperties, boost::property< boost::edge_weight_t, double > >;
#else
    using CallTree = boost::adjacency_list< boost::listS, boost::vecS, boost::directedS, VertexProperties, EdgeProperties >;
#endif

    // Some typedefs for simplicity
    typedef typename boost::graph_traits< CallTree >::vertex_descriptor vertex_t;
    typedef typename boost::graph_traits< CallTree >::edge_descriptor edge_t;

    CallTree m_calltree;

    std::vector< vertex_t > m_vertices;
    std::vector< edge_t > m_edges;

};


} // GUI
} // ArgoNavis

#endif // CALLTREEGRAPHMANAGER_H
