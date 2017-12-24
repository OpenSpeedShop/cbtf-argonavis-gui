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

#include <QStack>

namespace ArgoNavis { namespace GUI {

/**
 * @brief DerivedMetricsSolver::DerivedMetricsSolver
 * @param parent - the parent widget
 *
 * Constructs an DerivedMetricsSolver instance of the given parent.
 */
DerivedMetricsSolver::DerivedMetricsSolver(QObject *parent)
    : QObject( parent )
{

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
 * This method converts an equation from infix to postfix (reverse polish notation) orde.
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
 * @param formula - the derived metric formula
 * @param hwCounterValues - map container containing HW counter values (key is the PAPI event name)
 * @return - returns the result of solving the formula using the HW counter values supplied
 *
 * This function solves the formula using the supplied HW counter values.  First the PAPI event names in the formula
 * are replaced by the actual values by using the PAPI event name as a key into the supplied HW counter values to
 * lookup the value.  If real values have been substituted for ALL the PAPI event names in the formula, then convert
 * the formula with real values to postfix (reverse polish notation).  The postfix vector is processed sequentially
 * to determine the final result.
 */
double DerivedMetricsSolver::solve(const QString& formula, QMap< QString, qulonglong > hwCounterValues) const
{
    QString equation( formula );

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

    for ( int i=0; i<equationRPN.size(); ++i ) {
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

} // GUI
} // ArgoNavis
