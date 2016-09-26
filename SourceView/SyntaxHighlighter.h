/*!
   \file SyntaxHighlighter.h
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

#ifndef PLUGINS_SOURCEVIEW_SYNTAXHIGHLIGHTER_H
#define PLUGINS_SOURCEVIEW_SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class QTextDocument;

namespace ArgoNavis { namespace GUI {

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:

    explicit SyntaxHighlighter(QTextDocument *parent);

    void init();

    void highlightBlock(const QString &text);

private:

    QRegExp m_Keywords;
    QRegExp m_DataTypes;

    enum States {
        State_NormalState = -1,
        State_InsideComment,
        State_InsideDoubleQuote,
        State_InsideSingleQuote,
        State_InsideAngleBracketQuote
    };

};

} // GUI
} // ArgoNavis

#endif // PLUGINS_SOURCEVIEW_SYNTAXHIGHLIGHTER_H
