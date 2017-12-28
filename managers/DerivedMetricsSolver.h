/*!
   \file DerivedMetricsSolver.h
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

#ifndef DERIVEDMETRICSSOLVER_H
#define DERIVEDMETRICSSOLVER_H

#include <QObject>

#include <QMap>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QChar>

#include <map>
#include <vector>
#include <set>
#include <QAtomicPointer>


namespace ArgoNavis { namespace GUI {

class DerivedMetricsSolver : public QObject
{
    Q_OBJECT

public:

    static DerivedMetricsSolver *instance();

    static void destroy();

    QStringList getDerivedMetricList(const std::set<QString>& configured) const;

    double solve(const QString &formula, QMap<QString, qulonglong> hwCounterValues, double walltime) const;

    QVector<QVariantList> getDerivedMetricData() const;

    void setEnabled(const QString& key, bool enabled);

private:

    explicit DerivedMetricsSolver(QObject *parent = nullptr);

    bool isEqualPrecedence(const QString &op1, const QString &op2) const;
    bool isHigherPrecedence(const QString &op1, const QString &op2) const;
    bool isLeftAssociative(const QString &op) const;

    std::vector<QString> convertInfixToRPN(const QString &infix) const;

    double evaluate(double lhs, double rhs, const QChar &op) const;

private:

    static QAtomicPointer< DerivedMetricsSolver > s_instance;

    typedef struct {
        bool enabled;
        std::set<QString> events;
        QString formula;
    } DerivedMetricDefinition;

    mutable std::map< QString, DerivedMetricDefinition > m_derived_definitions;

};

} // GUI
} // ArgoNavis

#endif // DERIVEDMETRICSSOLVER_H
