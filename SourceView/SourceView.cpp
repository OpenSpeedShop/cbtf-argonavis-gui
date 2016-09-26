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

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "SyntaxHighlighter.h"


namespace ArgoNavis { namespace GUI {

SourceView::SourceView(QWidget *parent) :
    QPlainTextEdit(parent),
    m_SideBarArea(new SideBarArea(this)),
    m_SyntaxHighlighter(new SyntaxHighlighter(this->document()))
{
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateSideBarAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateSideBarArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);

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

    int diameter = fontMetrics().height();

    return digitWidth + diameter + 3;
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

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::darkGray);

            painter.drawText(0, top, m_SideBarArea->width(), fontMetrics().height(), Qt::AlignRight, number);

            if(m_Annotations.contains(blockNumber + 1)) {
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


} // GUI
} // ArgoNavis
