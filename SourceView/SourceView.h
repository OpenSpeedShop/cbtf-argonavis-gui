/*!
   \file SourceView.h
   \author Dane Gardner <dane.gardner@gmail.com>
   \remarks Greg Schultz <gregory.schultz@embarqmail.com> made modifications to support new GUI source view needs.

   \section LICENSE
   This file is part of the Parallel Tools GUI Framework (PTGF)
   Copyright (C) 2010-2013 Argo Navis Technologies, LLC

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

#ifndef PLUGINS_SOURCEVIEW_SOURCEVIEW_H
#define PLUGINS_SOURCEVIEW_SOURCEVIEW_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QSize>
#include <QColor>
#include <QThread>

#include "SourceViewMetricsCache.h"

class QPaintEvent;
class QResizeEvent;
class QEvent;

namespace ArgoNavis { namespace GUI {

class SyntaxHighlighter;

class SourceView : public QPlainTextEdit
{
    Q_OBJECT

public:

    explicit SourceView(QWidget *parent = 0);
    ~SourceView();

    void setCurrentLineNumber(const int &lineNumber);

    void addAnnotation(int lineNumber, QString toolTip = QString(), QColor color = QColor("orange"));
    void removeAnnotation(int lineNumber);

signals:

    void addMetricView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName, const QStringList& metrics);
    void addAssociatedMetricView(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName, const QString& attachedMetricViewName, const QStringList& metrics);
    void addMetricViewData(const QString& clusteringCriteriaName, const QString& metricName, const QString& viewName, const QVariantList& data, const QStringList& columnHeaders = QStringList());

public slots:

    void handleClearSourceView();
    void handleDisplaySourceFileLineNumber(const QString& filename, int lineNumber);
    void handleAddPathSubstitution(int index, const QString& oldPath, const QString& newPath);
    void handleMetricViewChanged(const QString& metricViewName);

protected:

    void resizeEvent(QResizeEvent *event);
    bool event(QEvent *event);

private:

    void sideBarAreaPaintEvent(QPaintEvent *event);
    int sideBarAreaWidth();
    void refreshStatements();

private slots:

    void updateSideBarAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateSideBarArea(const QRect &, int);

private:

    QWidget *m_SideBarArea;
    SyntaxHighlighter *m_SyntaxHighlighter;

    struct Annotation { QColor color; QString toolTip; };
    QMap<int, Annotation> m_Annotations;

    QString m_currentFilename;
    QString m_currentMetricView;

    int m_metricValueWidth;

    QFont m_font;
    QFont m_metricsFont;

    QMap<QString, QString> m_pathSubstitutions;

    SourceViewMetricsCache m_metricsCache;
    QThread m_thread;

    friend class SideBarArea;
};

class SideBarArea : public QWidget
{
public:

    explicit SideBarArea(SourceView *sourceView) : QWidget(sourceView) { m_SourceView = sourceView; }

    QSize sizeHint() const { return QSize(m_SourceView->sideBarAreaWidth(), 0); }

protected:

    void paintEvent(QPaintEvent *event) { m_SourceView->sideBarAreaPaintEvent(event); }

private:

    SourceView *m_SourceView;

};

} // namespace SourceView
} // namespace Plugins

#endif // PLUGINS_SOURCEVIEW_SOURCEVIEW_H
