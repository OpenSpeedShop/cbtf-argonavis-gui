/*!
   \file SourceView.cpp
   \author Dane Gardner <dane.gardner@gmail.com>

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

#include "SourceView.h"


#include <QPaintEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QPainter>
#include <QToolTip>
#include <QFile>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "SyntaxHighlighter.h"

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


const QString s_timeTitle = QStringLiteral("Time (msec)");
const QString s_functionTitle = QStringLiteral("Function (defining location)");


SourceView::SourceView(QWidget *parent) :
    QPlainTextEdit(parent),
    m_SideBarArea(new SideBarArea(this)),
    m_SyntaxHighlighter(new SyntaxHighlighter(this->document()))
{
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateSideBarAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateSideBarArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    m_font = QFont("Monospace");
    m_font.setStyleHint(QFont::TypeWriter);
    setFont(m_font);

    m_metricsFont = m_font;
    m_metricsFont.setPointSize( m_font.pointSize()-2 );

    setReadOnly( true );

    updateSideBarAreaWidth(0);
    highlightCurrentLine();
}

SourceView::~SourceView()
{
}

void SourceView::setCurrentLineNumber(const int &lineNumber)
{
    const QTextBlock &block = document()->findBlockByNumber(lineNumber-1);
    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 0);
    setTextCursor(cursor);
    centerCursor();
    setFocus();
}

void SourceView::addAnnotation(int lineNumber, QString toolTip, QColor color)
{
    Annotation annotation;
    annotation.toolTip = toolTip;
    annotation.color = color;

    m_Annotations[lineNumber] = annotation;
}

void SourceView::removeAnnotation(int lineNumber)
{
    m_Annotations.remove(lineNumber);
}

void SourceView::handleAddMetricView(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, const QStringList &metrics)
{
    Q_UNUSED( clusteringCriteriaName );

    if ( metrics.contains( s_timeTitle ) && metrics.contains( s_functionTitle ) ) {
        const QString metricViewname = metricName + "-" + viewName;
        const int timeTitleIdx = metrics.indexOf( s_timeTitle );
        const int functionTitleIdx = metrics.indexOf( s_functionTitle );
        m_watchedMetricViews.insert( metricViewname, qMakePair( timeTitleIdx, functionTitleIdx ) );
    }
}

void SourceView::handleAddMetricViewData(const QString &clusteringCriteriaName, const QString &metricName, const QString &viewName, const QVariantList &data, const QStringList &columnHeaders)
{
    Q_UNUSED( clusteringCriteriaName );

    const QString metricViewName = metricName + "-" + viewName;

    if ( ! m_watchedMetricViews.contains( metricViewName ) )
        return;

    QPair<int, int> indexes = m_watchedMetricViews.value( metricViewName );

    const double value = data.at( indexes.first ).toDouble();
    const QString definingLocation = data.at( indexes.second ).toString();

    int lineNumber;
    QString filename;

    extractFilenameAndLine( definingLocation, filename, lineNumber );

    if ( filename.isEmpty() || lineNumber < 1 )
        return;

    QMap< QString, QVector< double > >& metricViewData = m_metrics[ metricViewName ];

    QVector< double >& metrics = metricViewData[ filename ];

    if ( metrics.size() < lineNumber+1 )
        metrics.resize( lineNumber+1 );

    metrics[ lineNumber ] = value;
}

void SourceView::handleClearSourceView()
{
    clear();
}

void SourceView::handleDisplaySourceFileLineNumber(const QString &filename, int lineNumber)
{
    QString filenameToLoad( filename );

    QMap< QString, QString >::iterator iter( m_pathSubstitutions.begin() );
    while ( iter != m_pathSubstitutions.end() ) {
        if ( filename.contains( iter.key() ) ) {
            filenameToLoad.replace( iter.key(), iter.value() );
            break;
        }
        iter++;
    }

    QFile file( filenameToLoad );
    if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        QTextDocument* document = new QTextDocument( file.readAll() );
        document->setDocumentLayout( new QPlainTextDocumentLayout( document ) );
        setDocument( document );
        setCurrentLineNumber( lineNumber );
    }
    else {
        handleClearSourceView();
    }

    m_currentFilename = filename;
}

void SourceView::handleAddPathSubstitution(int index, const QString &oldPath, const QString &newPath)
{
    // check if modifying existing entry and remove it
    if ( index < m_pathSubstitutions.size() ) {
        int count( 0 );
        QMutableMapIterator<QString, QString> iter( m_pathSubstitutions );
        while ( iter.hasNext() ) {
            iter.next();
            if ( count == index ) {
                iter.remove();
                break;
            }
        }
    }

    // add entry
    m_pathSubstitutions.insert( oldPath, newPath );
}

int SourceView::sideBarAreaWidth()
{
    int digits = QString::number(qMax(1, blockCount())).length();
    int digitWidth = fontMetrics().width(QLatin1Char('9')) * digits;

    QFontMetrics fontMetrics( m_metricsFont );

    m_metricValueWidth = fontMetrics.width( QStringLiteral("9999999.9") );

    return digitWidth + m_metricValueWidth + 10;
}

void SourceView::updateSideBarAreaWidth(int newBlockCount)
{
    Q_UNUSED(newBlockCount)
    setViewportMargins(sideBarAreaWidth(), 0, 0, 0);
}

void SourceView::updateSideBarArea(const QRect &rect, int dy)
{
    if(dy) {
        m_SideBarArea->scroll(0, dy);
    } else {
        m_SideBarArea->update(0, rect.y(), m_SideBarArea->width(), rect.height());
    }

    if(rect.contains(viewport()->rect())) {
        updateSideBarAreaWidth(0);
    }
}

void SourceView::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    m_SideBarArea->setGeometry(QRect(cr.left(), cr.top(), sideBarAreaWidth(), cr.height()));
}

void SourceView::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    setExtraSelections(extraSelections);
}

void SourceView::sideBarAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_SideBarArea);

    painter.fillRect(event->rect(), Qt::lightGray);

    if ( document()->isEmpty() )
        return;

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    painter.setFont( m_font );
    const int height = fontMetrics().height();

    QMap< QString, QVector< double > >& metricViewData = m_metrics[ m_currentMetricView ];
    QVector< double >& metrics = metricViewData[ m_currentFilename ];

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const int lineNumber = blockNumber + 1;
            QString number = QString::number(lineNumber);

            bool lineHasMetrics( metrics.size() > lineNumber && metrics[lineNumber] > 0.0 );

            if ( lineHasMetrics ) {
                QString value = QString::number( metrics[lineNumber], 'f', 1 );
                painter.setPen( Qt::darkRed );
                painter.setFont( m_metricsFont );
                painter.drawText( 0, top, m_metricValueWidth, height, Qt::AlignRight|Qt::AlignVCenter, value );
            }

#ifdef HAS_SOURCE_CODE_LINE_HIGHLIGHTS
            QTextBlockFormat format;
            format.setBackground( lineHasMetrics ? QColor("#ff6961") : QColor("#ffffff") );
            QTextCursor cursor( block );
            cursor.mergeBlockFormat( format );
#endif

            painter.setPen( Qt::darkGray );
            painter.setFont( m_font );
            painter.drawText(0, top, m_SideBarArea->width(), fontMetrics().height(), Qt::AlignRight, number);

            if(m_Annotations.contains(lineNumber)) {
                Annotation annotation = m_Annotations.value(blockNumber+1);
                QBrush brush = painter.brush();
                if(brush.color() != annotation.color) {
                    brush.setColor(annotation.color);
                    painter.setBrush(brush);
                }

                int diameter = fontMetrics().height();
                painter.drawEllipse(diameter/2, top-(diameter/2), diameter, diameter);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

bool SourceView::event(QEvent *event)
{
    if(event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

        QPoint position = helpEvent->pos();
        if(position.x() > sideBarAreaWidth()) {
            QToolTip::hideText();
            event->ignore();
            return true;
        }

        QTextCursor cursor = cursorForPosition(position);
        int lineNumber = cursor.blockNumber() + 1;
        if(!m_Annotations.contains(lineNumber)) {
            QToolTip::hideText();
            event->ignore();
            return true;
        }

        Annotation annotation = m_Annotations.value(lineNumber);
        QToolTip::showText(helpEvent->globalPos(), annotation.toolTip);
        return true;
    }

    return QPlainTextEdit::event(event);
}

/**
 * @brief SourceView::extractFilenameAndLine
 * @param text - text containing defining location information
 * @param filename - the filename obtained from the defining location information
 * @param lineNumber - the line number obtained from the defining location information
 *
 * Extracts the filename and line number from the string.  The string is in the format used on
 * the metric views.
 */
void SourceView::extractFilenameAndLine(const QString& text, QString& filename, int& lineNumber)
{
    QString definingLocation;
    lineNumber = -1;
    int startParenIdx = text.lastIndexOf( '(' );
    if ( -1 != startParenIdx ) {
        definingLocation = text.mid( startParenIdx + 1 );
    }
    else {
        definingLocation = text;
    }
    int sepIdx = definingLocation.lastIndexOf( ',' );
    if ( -1 != sepIdx ) {
        filename = definingLocation.left( sepIdx );
        QString lineNumberStr = definingLocation.mid( sepIdx + 1 );
        int endParenIdx = lineNumberStr.lastIndexOf( ')' );
        if ( -1 != endParenIdx ) {
            lineNumberStr.chop( lineNumberStr.length() - endParenIdx );
        }
        lineNumber = lineNumberStr.toInt();
    }
}

/**
 * @brief SourceView::handleRequestMetricView
 * @param metricViewName - the name of the current view active in the Metric Table View
 *
 * Handler to record the current view active in the Metric Table View.
 */
void SourceView::handleMetricViewChanged(const QString& metricViewName)
{
    m_currentMetricView = metricViewName;

    update();
}

} // GUI
} // ArgoNavis
