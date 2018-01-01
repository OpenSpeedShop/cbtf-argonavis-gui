/*!
   \file DerivedMetricsSolver.cpp
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2018 Schultz Software Solutions, LLC

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by the
   Free Software Foundation; either version 2.1 of the License, or (at your
   option) any later version.

   This library is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "DerivedMetricsSolver.h"

#include "common/openss-gui-config.h"

#include <QStack>
#include <QStringList>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

namespace ArgoNavis { namespace GUI {


QAtomicPointer< DerivedMetricsSolver > DerivedMetricsSolver::s_instance = nullptr;


/**
 * @brief DerivedMetricsSolver::DerivedMetricsSolver
 * @param parent - the parent widget
 *
 * Constructs an DerivedMetricsSolver instance of the given parent.
 */
DerivedMetricsSolver::DerivedMetricsSolver(QObject *parent)
    : QObject( parent )
    , m_derived_definitions(
        { { "Instructions Per Cycle", { true, false, { "PAPI_TOT_INS", "PAPI_TOT_CYC" }, "PAPI_TOT_INS / PAPI_TOT_CYC" } },
          { "Issued Instructions Per Cycle", { true, false, { "PAPI_TOT_IIS", "PAPI_TOT_CYC" }, "PAPI_TOT_IIS / PAPI_TOT_CYC" } },
          { "FP Instructions Per Cycle", { true, false, { "PAPI_FP_INS", "PAPI_TOT_CYC" }, "PAPI_FP_INS / PAPI_TOT_CYC" } },
          { "Percentage FP Instructions", { true, false, { "PAPI_FP_INS", "PAPI_TOT_INS" }, "PAPI_FP_INS / PAPI_TOT_INS" } },
          { "Graduated Instructions / Issued Instructions", { true, false, { "PAPI_TOT_INS", "PAPI_TOT_IIS" }, "PAPI_FP_INS / PAPI_TOT_IIS" } },
          { "% of Cycles with no instruction issue", { true, false, { "PAPI_STL_ICY", "PAPI_TOT_CYC" }, "100.0 * ( PAPI_STL_ICY / PAPI_TOT_CYC )" } },
          { "% of Cycles Waiting for Memory Access", { true, false, { "PAPI_STL_SCY", "PAPI_TOT_CYC" }, "100.0 * ( PAPI_STL_SCY / PAPI_TOT_CYC )" } },
          { "% of Cycles Stalled on Any Resource", { true, false, { "PAPI_RES_STL", "PAPI_TOT_CYC" }, "100.0 * ( PAPI_RES_STL / PAPI_TOT_CYC )" } },
          { "Data References Per Instruction", { true, false, { "PAPI_L1_DCA", "PAPI_TOT_INS" }, "PAPI_L1_DCA / PAPI_TOT_INS" } },
          { "L1 Cache Line Reuse (data)", { true, false, { "PAPI_LST_INS", "PAPI_L1_DCM" }, "( PAPI_LST_INS - PAPI_L1_DCM ) / PAPI_L1_DCM" } },
          { "L1 Cache Data Hit Rate", { true, false, { "PAPI_L1_DCM", "PAPI_LST_INS" }, "1.0 - ( PAPI_L1_DCM / PAPI_LST_INS )" } },
          { "L1 Data Cache Read Miss Ratio", { true, false, { "PAPI_L1_DCM", "PAPI_L1_DCA" }, "PAPI_L1_DCM / PAPI_L1_DCA" } },
          { "L2 Cache Line Reuse (data)",  { true, false, { "PAPI_L1_DCM", "PAPI_L2_DCM" }, "( PAPI_L1_DCM - PAPI_L2_DCM ) / PAPI_L2_DCM" } },
          { "L2 Cache Data Hit Rate", { true, false, { "PAPI_L2_DCM", "PAPI_L1_DCM" }, "1.0 - ( PAPI_L2_DCM / PAPI_L1_DCM )" } },
          { "L2 Cache Miss Ratio", { true, false, { "PAPI_L2_TCM", "PAPI_L2_TCA" }, "PAPI_L2_TCM / PAPI_L2_TCA" } },
          { "L3 Cache Line Reuse (data)",  { true, false, { "PAPI_L2_DCM", "PAPI_L3_DCM" }, "( PAPI_L2_DCM - PAPI_L3_DCM ) / PAPI_L3_DCM" } },
          { "L3 Cache Data Hit Rate", { true, false, { "PAPI_L3_DCM", "PAPI_L2_DCM"}, "1.0 - ( PAPI_L3_DCM / PAPI_L2_DCM )" } },
          { "L3 Data Cache Miss Ratio", { true, false, { "PAPI_L3_DCM", "PAPI_L3_DCA" }, "PAPI_L3_DCM / PAPI_L3_DCA" } },
          { "L3 Cache Data Read Ratio", { true, false, { "PAPI_L3_DCR", "PAPI_L3_DCA" }, "PAPI_L3_DCR / PAPI_L3_DCA" } },
          { "L3 Cache Instruction Miss Ratio", { true, false, { "PAPI_L3_ICM", "PAPI_L3_ICR" }, "PAPI_L3_ICM / PAPI_L3_ICR" } },
          { "% of Cycles Stalled on Memory Access", { true, false, { "PAPI_MEM_SCY", "PAPI_TOT_CYC" }, "100.0 * ( PAPI_MEM_SCY / PAPI_TOT_CYC )" } },
          { "% of Cycles Stalled on Any Resource", { true, false, { "PAPI_RES_STL", "PAPI_TOT_CYC" }, "100.0 * ( PAPI_RES_STL / PAPI_TOT_CYC )" } },
          { "Ratio L1 Data Cache Miss to Total Cache Access", { true, false, { "PAPI_L1_DCM", "PAPI_L1_TCA" }, "PAPI_L1_DCM / PAPI_L1_TCA" } },
          { "Ratio L2 Data Cache Miss to Total Cache Access", { true, false, { "PAPI_L2_DCM", "PAPI_L2_TCA" }, "PAPI_L2_DCM / PAPI_L2_TCA" } },
          { "Ratio L3 Total Cache Miss to Data Cache Access", { true, false, { "PAPI_L3_TCM", "PAPI_L3_DCA" }, "PAPI_L3_TCM / PAPI_L3_DCA" } },
          { "L3 Total Cache Miss Ratio", { true, false, { "PAPI_L3_TCM", "PAPI_L3_TCA" }, "PAPI_L3_TCM / PAPI_L3_TCA" } },
          { "Ratio Mispredicted to Correctly Predicted Branches", { true, false, { "PAPI_BR_MSP", "PAPI_BR_PRC" }, "PAPI_BR_MSP / PAPI_BR_PRC" } } } )
{

}

/**
 * @brief DerivedMetricsSolver::instance
 * @return - return a pointer to the singleton instance
 *
 * This method provides a pointer to the singleton instance.
 */
DerivedMetricsSolver *DerivedMetricsSolver::instance()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    DerivedMetricsSolver* inst = s_instance.loadAcquire();
#else
    DerivedMetricsSolver* inst = s_instance;
#endif

    if ( ! inst ) {
        inst = new DerivedMetricsSolver();
        if ( ! s_instance.testAndSetRelease( 0, inst ) ) {
            delete inst;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            inst = s_instance.loadAcquire();
#else
            inst = s_instance;
#endif
        }
    }

    return inst;
}

/**
 * @brief DerivedMetricsSolver::destroy
 *
 * Static method to destroy the singleton instance.
 */
void DerivedMetricsSolver::destroy()
{
    delete s_instance.fetchAndStoreRelease( Q_NULLPTR );
}

/**
 * @brief DerivedMetricsSolver::getMatchingDerivedMetricList
 * @param configured - the set of configured PAPI events
 * @return - the list of derived metric names that can be computed with the data for the configured PAPI events
 *
 * The function determines the list of derived metrics that can be computed using data for the configured PAPI events.
 */
QStringList DerivedMetricsSolver::getDerivedMetricList(const std::set<QString> &configured) const
{
    QStringList derivedMetricList;

    for( auto iter = m_derived_definitions.begin(); iter != m_derived_definitions.end(); ++iter ) {
        if ( ! iter->second.enabled )  // skip disabled metrics
            continue;

        std::set<QString> derived( iter->second.events );  // get the next derived definition to match with configured set

        std::set<QString> intersection;

        std::set_intersection( configured.begin(), configured.end(),
                               derived.begin(), derived.end(),
                               std::inserter( intersection, intersection.begin() ) );

        if ( derived == intersection ) {
            derivedMetricList << iter->first;
        }
    }

    return derivedMetricList;
}

/**
 * @brief DerivedMetricsSolver::isEqualPrecedence
 * @param op1 - the first operator
 * @param op2 - the second operator
 * @return - returns true if precedence of 'op1' is equal to precedence of 'op2'
 *
 * This method determines whether the precedence of 'op1' is the same as that of 'op2'.
 * The only supported operators are: '+', '-'. '*', '/'.
 * The operators of equal precedence are '+' and '-' and '*' and '/'.
 */
bool DerivedMetricsSolver::isEqualPrecedence(const QString& op1, const QString& op2) const
{
    if ( ( op1 == "+" && op2 == "-" ) ||
         ( op1 == "-" && op2 == "+" ) ||
         ( op1 == "*" && op2 == "/" ) ||
         ( op1 == "/" && op2 == "*" ) ) {
        return true;
    }

    return false;
}

/**
 * @brief DerivedMetricsSolver::isHigherPrecedence
 * @param op1 - the first operator
 * @param op2 - the second operator
 * @return - returns true of precedence or 'op1' is greater than 'op2'
 *
 * This method determines whether the precedence of 'op1' is greater than 'op2'.
 * The only supported operators are: '+', '-'. '*', '/'.
 */
bool DerivedMetricsSolver::isHigherPrecedence(const QString& op1, const QString& op2) const
{
    if ( isEqualPrecedence( op1, op2 ) )
        return false;

    if ( op1 == "+" || op1 == "-" )
        return false;
    else
        return true;
}

/**
 * @brief DerivedMetricsSolver::isLeftAssociative
 * @param op - the operator
 * @return - whether the operator is left associative
 *
 * Since the only supported operators are '+', '-'. '*', '/', this method always returns true.
 */
bool DerivedMetricsSolver::isLeftAssociative(const QString& op) const
{
    return ( op == "+" || op == "-" || op == "*" || op == "/" );
}

/**
 * @brief DerivedMetricsSolver::convertInfixToRPN
 * @param infix - the equal in infix order
 * @return - the equivalent equation in postfix (reverse polish notation) order
 *
 * This method converts an equation from infix to postfix (reverse polish notation) order.
 * Edsger Dijkstra's "shunting-yard" algorithm is used - ref. "https://en.wikipedia.org/wiki/Shunting-yard_algorithm".
 */
std::vector<QString> DerivedMetricsSolver::convertInfixToRPN(const QString& infix) const
{
    const QStringList tokenList = infix.split( ' ', QString::SkipEmptyParts );

    QStack<QString> outputQueue;
    QStack<QString> opStack;

    bool ok;

    foreach ( const QString& token, tokenList ) {
        token.toDouble( &ok );
        if ( ok ) {
            outputQueue.push( token );
        }
        else {
            if ( token == "(" ) {
                opStack.push( token );
            }
            else if ( token == ")" ) {
                while ( ! opStack.empty() ) {
                    const QString top( opStack.pop() );
                    if ( top == "(" ) break;
                    outputQueue.push( top );
                }
            }
            else {
                // assume token is an operator
                while ( ! opStack.empty() && opStack.top() != "(" && ( isHigherPrecedence( opStack.top(), token ) || ( isEqualPrecedence( opStack.top(), token ) && isLeftAssociative( token ) ) ) ) {
                        outputQueue.push( opStack.pop() );
                }
                opStack.push( token );
            }
        }
    }
    while ( ! opStack.empty() ) {
        outputQueue.push( opStack.pop() );
    }
    return outputQueue.toStdVector();
}

/**
 * @brief DerivedMetricsSolver::evaluate
 * @param lhs - the left-hand side operand
 * @param rhs - the right-hand side operand
 * @param op - the operator
 * @return - the result of evaluating 'lhs' <op> 'rhs'
 *
 * This function returns the value obtained by evaluating the expression 'lhs' <op> 'rhs'.
 */
double DerivedMetricsSolver::evaluate(double lhs, double rhs, const QChar& op) const
{
    switch( op.toLatin1() ) {
    case '+': return lhs + rhs;
    case '-': return lhs - rhs;
    case '*': return lhs * rhs;
    case '/': return (rhs != 0.0 ) ? lhs / rhs : 0.0;
    default: return 0.0;
    }
}

/**
 * @brief DerivedMetricsSolver::solveDerivedMetricFormula
 * @param key - the derived metric name
 * @param hwCounterValues - map container containing HW counter values (key is the PAPI event name)
 * @return - returns the result of solving the formula using the HW counter values supplied
 *
 * This function solves the formula using the supplied HW counter values.  First the PAPI event names in the formula
 * are replaced by the actual values by using the PAPI event name as a key into the supplied HW counter values to
 * lookup the value.  If real values have been substituted for ALL the PAPI event names in the formula, then convert
 * the formula with real values to postfix (reverse polish notation).  The postfix vector is processed sequentially
 * to determine the final result.
 */
double DerivedMetricsSolver::solve(const QString& key, QMap< QString, qulonglong > hwCounterValues) const
{
    if ( m_derived_definitions.find(key) == m_derived_definitions.end() )
        return 0.0;

    if ( ! m_derived_definitions[key].enabled )
        return 0.0;

    QString equation( m_derived_definitions[key].formula );

    // substitute actual HW Counter values for PAPI event names in formula
    for ( QMap< QString, qulonglong >::iterator iter = hwCounterValues.begin(); iter != hwCounterValues.end(); ++iter ) {
        equation.replace( iter.key(), QString::number( iter.value() ) );
    }

    // check to make sure equation has all constant values and no more PAPI event names
    if ( equation.contains( QStringLiteral("PAPI") ) )
        return std::numeric_limits< qulonglong >::min();

    // convert equation from infix to postfix (reverse polish notation)
    std::vector<QString> equationRPN = convertInfixToRPN( equation );

    QStack<double> s;

    for ( std::size_t i=0; i<equationRPN.size(); ++i ) {
        const QString& top = equationRPN.at( i );
        if ( top == "+" || top == "-" || top == "*" || top == "/" ) {
            double op2 = s.pop();
            double op1 = s.pop();
            s.push ( evaluate( op1, op2, top[0] ) );
        }
        else {
            s.push( top.toDouble() );
        }
    }

    const double result = ( s.size() == 1 ) ? s.top() : 0.0;

    return result;
}

/**
 * @brief DerivedMetricsSolver::getDerivedMetricData
 * @return - return a variant list containing the basic information regarding the derived metric
 *
 * This method returns a vector of variant lists for each derived metric containing the following information:
 *     - the name /decription
 *     - the formula
 *     - whether currently enabled
 */
QVector<QVariantList> DerivedMetricsSolver::getDerivedMetricData() const
{
    QVector<QVariantList> result;

    for( auto iter = m_derived_definitions.begin(); iter != m_derived_definitions.end(); ++iter ) {
        QVariantList list;

        list << iter->first << iter->second.formula << iter->second.enabled;

        result << list;
    }

    return result;
}

/**
 * @brief DerivedMetricsSolver::setEnabled
 * @param name - the derived metric name
 * @param enabled - whether derived metric is enabled/disabled
 *
 * This method sets the enabled state for the specified derived metric.
 */
void DerivedMetricsSolver::setEnabled(const QString &name, bool enabled)
{
    if ( m_derived_definitions.find(name) != m_derived_definitions.end() ) {
        m_derived_definitions[name].enabled = enabled;
    }
}

/**
 * @brief DerivedMetricsSolver::insert
 * @param name - the derived metric name
 * @param formula - the formula
 * @param enabled - whether derived metric is enabled/disabled
 *
 * This method adds the derived metric if it isn't already in the database.
 */
bool DerivedMetricsSolver::insert(const QString &name, const QString &formula, bool enabled)
{
    if ( m_derived_definitions.find(name) != m_derived_definitions.end() )
        return false;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    const QStringList list = formula.split( QRegularExpression("[^A-Za-z0-9_]"), QString::SkipEmptyParts );
#else
    const QStringList list = formula.split( QRegExp("[^A-Za-z0-9_]"), QString::SkipEmptyParts );
#endif

    if ( list.isEmpty() )
        return false;

    DerivedMetricDefinition derivedMetric;

    derivedMetric.enabled = enabled;
    derivedMetric.formula = formula;
    derivedMetric.isUser = true;

    foreach (const QString& token, list) {
        if ( token.startsWith( "PAPI_" ) ) {
            derivedMetric.events.insert( token );
        }
    }

    if ( derivedMetric.events.empty() )
        return false;

    m_derived_definitions[ name ] = derivedMetric;

    return true;
}

/**
 * @brief DerivedMetricsSolver::get
 * @param index - index of user-defined derived metric to get information about
 * @param name - return the name of the user-defined derived metric
 * @param formula - return the formula for the user-defined derived metric
 * @param enabled - return whether the user-defined derived metric is enabled
 * @return - if the requested user-defined metric has been found and information returned
 *
 * This method returns data on the requested user-defined metric.
 */
bool DerivedMetricsSolver::getUserDefined(std::size_t index, QString &name, QString &formula, bool& enabled)
{
    if ( index < m_derived_definitions.size() ) {
        std::size_t count(0);
        for( auto iter = m_derived_definitions.begin(); iter != m_derived_definitions.end(); ++iter ) {
            if ( iter->second.isUser && ( count++ == index ) ) {
                name = iter->first;
                formula = iter->second.formula;
                enabled = iter->second.enabled;
                return true;
            }
        }
    }

    return false;
}

} // GUI
} // ArgoNavis
