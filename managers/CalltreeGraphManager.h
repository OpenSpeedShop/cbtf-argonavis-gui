#ifndef CALLTREEGRAPHMANAGER_H
#define CALLTREEGRAPHMANAGER_H

#include <QObject>

#ifndef Q_MOC_RUN
#include <boost/graph/adjacency_list.hpp>
#endif

#include <string>
#include <vector>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <tuple>
#else
#ifndef Q_MOC_RUN
#include "boost/tuple/tuple.hpp"
#endif
#endif
#include <stdexcept>
#include <iostream>

namespace ArgoNavis { namespace GUI {


class CalltreeGraphManager : public QObject
{
    Q_OBJECT

public:

    explicit CalltreeGraphManager(QObject *parent = 0);

    typedef std::size_t handle_t;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    typedef std::tuple< std::string, std::string > NameValuePair_t;
#else
    typedef boost::tuple< std::string, std::string > NameValuePair_t;
#endif
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
    typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS, VertexProperties, boost::property< boost::edge_weight_t, double > > CallTree;
#else
    typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS, VertexProperties, EdgeProperties > CallTree;
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
