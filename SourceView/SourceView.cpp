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
#include <QAction>
#include <QMenu>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "SyntaxHighlighter.h"
#include "SourceViewMetricsCache.h"

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


SourceView::SourceView(QWidget *parent)
    : QPlainTextEdit( parent )
    , m_SideBarArea( new SideBarArea( this ) )
    , m_SyntaxHighlighter( new SyntaxHighlighter( this->document() ) )

{
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateSideBarAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateSideBarArea(QRect,int)));

    m_font = QFont("Monospace");
    m_font.setStyleHint(QFont::TypeWriter);
    setFont(m_font);

    m_metricsFont = m_font;
    m_metricsFont.setPointSize( m_font.pointSize()-2 );

    setReadOnly( true );
    setTextInteractionFlags( Qt::NoTextInteraction );

    updateSideBarAreaWidth(0);

    // connect signals to the metric cache manager
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( this, &SourceView::addMetricView, &m_metricsCache, &SourceViewMetricsCache::handleAddMetricView );
    connect( this, &SourceView::addMetricViewData, &m_metricsCache, &SourceViewMetricsCache::handleAddMetricViewData );
#else
    connect( this, SIGNAL(addMetricView(QString,QString,QString,QStringList)),
             &m_metricsCache, SLOT(handleAddMetricView(QString,QString,QString,QStringList)) );
    connect( this, SIGNAL(addMetricViewData(QString,QString,QString,QVariantList,QStringList)),
             &m_metricsCache, SLOT(handleAddMetricViewData(QString,QString,QString,QVariantList,QStringList)) );
#endif

    connect( &m_metricsCache, SIGNAL(signalSelectedMetricChanged(QString,QString)), this, SLOT(update()) );

    m_metricsCache.moveToThread( &m_thread );
    m_thread.start();
}

SourceView::~SourceView()
{
    m_thread.quit();
    m_thread.wait();

    qDeleteAll( m_actions );
    m_actions.clear();
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

void SourceView::handleClearSourceView()
{
    clear();

    m_Annotations.clear();

    m_metricsCache.clear();

    qDeleteAll( m_actions );
    m_actions.clear();
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

    m_metricValueWidth = fontMetrics.width( QStringLiteral("999999999999.9") );

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

void SourceView::sideBarAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_SideBarArea);

    painter.fillRect(event->rect(), Qt::lightGray);

    if ( document()->isEmpty() )
        return;

    const int currentLineNumber = textCursor().block().blockNumber() + 1;

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    painter.setFont( m_font );
    const int height = fontMetrics().height();

    QVector< double > metrics = m_metricsCache.getMetricsCache( m_currentMetricView, m_currentFilename );

    QString selectedMetricName;
    QVariant::Type selectedMetricType;

    m_metricsCache.getSelectedMetricDetails( m_currentMetricView, selectedMetricName, selectedMetricType );

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const int lineNumber = blockNumber + 1;
            QString number = QString::number(lineNumber);

            bool lineHasMetrics( metrics.size() > lineNumber && metrics[lineNumber] > 0.0 );

            if ( lineHasMetrics ) {
                QString value;
                if ( QVariant::Double == selectedMetricType ) {
                    value = QString::number( metrics[lineNumber], 'f', 1 );
                }
                else {
                    qulonglong tempValue = metrics[lineNumber];
                    value = QString::number( tempValue );
                }
                painter.setPen( Qt::darkRed );
                painter.setFont( m_metricsFont );
                painter.drawText( 0, top, m_metricValueWidth, height, Qt::AlignRight|Qt::AlignVCenter, value );
            }

#ifdef HAS_SOURCE_CODE_LINE_HIGHLIGHTS
            QTextBlockFormat format;
            QColor backgroundColor = QColor("#ffffff");
            if ( lineHasMetrics ) {
                double percentage = metrics[lineNumber] / metrics[0];
                if ( percentage > 0.9 )
                    backgroundColor = QColor("#ff3c33");
                else if ( percentage > 0.75 )
                    backgroundColor = QColor("#ff6969");
                else if ( percentage > 0.5 )
                    backgroundColor = QColor("#ffb347");
                else if ( percentage > 0.25 )
                    backgroundColor = QColor("#fee270");
                else if ( percentage > 0.1 )
                    backgroundColor = QColor("#faffcd");
                else if ( percentage > 0.0 )
                    backgroundColor = QColor("#afdbaf");
            }
            format.setBackground( backgroundColor);
            QTextCursor cursor( block );
            cursor.mergeBlockFormat( format );
#endif

            painter.setPen( lineNumber == currentLineNumber ? Qt::white : Qt::darkGray );
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

#ifndef QT_NO_CONTEXTMENU
/**
 * @brief SourceView::contextMenuEvent
 * @param event - the context-menu event details
 *
 * This is the handler to receive context-menu events for the widget.
 */
void SourceView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu( this );

    const QStringList metricNameChoices = m_metricsCache.getMetricChoices( m_currentMetricView );

    // user has only one or no metric choices so don't create context-menu
    //if ( metricNameChoices.size() < 2 )
    //    return;

    // get details regarding the currently selected metric - name and value type

    QString selectedMetricName;
    QVariant::Type selectedMetricType;

    m_metricsCache.getSelectedMetricDetails( m_currentMetricView, selectedMetricName, selectedMetricType );

    // add the name of each selectable metric to the context menu with the currently selected metric being checked
    foreach ( const QString& choice, metricNameChoices ) {
        QAction* action;
        if ( ! m_actions.contains( choice ) ) {
            action = new QAction( choice, this );
            action->setCheckable( true );
            action->setProperty( "metricViewName", m_currentMetricView );
            m_actions[ choice ] = action;
        }
        else {
            action = m_actions[ choice ];
            disconnect( action, SIGNAL(changed()), &m_metricsCache, SLOT(handleSelectedMetricChanged()) );
        }

        action->setChecked( choice == selectedMetricName );
        connect( action, SIGNAL(changed()), &m_metricsCache, SLOT(handleSelectedMetricChanged()) );

        menu.addAction( action );
    }

    menu.exec( event->globalPos() );
}
#endif // QT_NO_CONTEXTMENU

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
