/*!
   \file SyntaxHighlighter.cpp
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

#include "SyntaxHighlighter.h"

#include <QTextDocument>

namespace ArgoNavis { namespace GUI {

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent)
{
    init();
}

void SyntaxHighlighter::init()
{
    QStringList keywords;
    keywords << "asm" << "break" << "case" << "catch" << "class" << "const_cast" << "continue" << "default" << "delete"
             << "do" << "dynamic_cast" << "else" << "enum" << "explicit" << "export" << "extern" << "false" << "friend"
             << "for" << "goto" << "if" << "inline" << "namespace" << "new" << "operator" << "private" << "protected"
             << "public" << "reinterpret_cast" << "return" << "sizeof" << "static_cast" << "struct" << "switch"
             << "template" << "this" << "throw" << "true" << "try" << "typedef" << "typeid" << "type_info"
             << "typename" << "union" << "using" << "virtual" << "while" << "and" << "and_eq" << "bad_cast"
             << "bad_typeid" << "bitand" << "bitor" << "compl" << "not" << "not_eq" << "or" << "or_eq" << "xor"
             << "xor_eq";

    QString keywordPattern;
    keywordPattern.append("\\b(");
    keywordPattern.append(keywords.join("|"));
    keywordPattern.append(")\\b");
    m_Keywords.setPattern(keywordPattern);

    QStringList dataTypes;
    dataTypes << "auto" << "bool" << "char" << "const" << "double" << "float" << "int" << "long" << "mutable"
              << "register" << "short" << "signed" << "static" << "unsigned" << "void" << "volatile" << "uchar"
              << "uint" << "int8_t" << "int16_t" << "int32_t" << "int64_t" << "uint8_t" << "uint16_t" << "uint32_t"
              << "uint64_t" << "wchar_t";

    QString dataTypePattern;
    dataTypePattern.append("\\b(");
    dataTypePattern.append(dataTypes.join("|"));
    dataTypePattern.append(")\\b");
    m_DataTypes.setPattern(dataTypePattern);

}


void SyntaxHighlighter::highlightBlock(const QString &text)
{
    int index = text.indexOf(m_Keywords);
    while (index >= 0) {
        int length = m_Keywords.matchedLength();
        setFormat(index, length, Qt::darkYellow);
        index = text.indexOf(m_Keywords, index + length);
    }

    index = text.indexOf(m_DataTypes);
    while (index >= 0) {
        int length = m_DataTypes.matchedLength();
        setFormat(index, length, Qt::darkMagenta);
        index = text.indexOf(m_DataTypes, index + length);
    }

    static QRegExp mpiMethod("\\bMPI_\\w+\\b");
    index = text.indexOf(mpiMethod);
    while (index >= 0) {
        int length = mpiMethod.matchedLength();
        setFormat(index, length, Qt::red);
        index = text.indexOf(mpiMethod, index + length);
    }

    static QRegExp preprocessor("#\\s*\\w+\\b");
    index = text.indexOf(preprocessor);
    while (index >= 0) {
        int length = preprocessor.matchedLength();
        setFormat(index, length, Qt::darkBlue);
        index = text.indexOf(preprocessor, index + length);
    }

    static QRegExp quote("(\".*\")|('.')|(<\\s*[\\w\\.]+\\s*>)");
    index = text.indexOf(quote);
    while (index >= 0) {
        int length = quote.matchedLength();
        setFormat(index, length, Qt::darkGreen);
        index = text.indexOf(quote, index + length);
    }

    static QRegExp oneLineComment("(//.*$)");
    index = text.indexOf(oneLineComment);
    while (index >= 0) {
        int length = oneLineComment.matchedLength();
        setFormat(index, length, Qt::darkGreen);
        index = text.indexOf(oneLineComment, index + length);
    }

    /* Multi-line comments */
    static QRegExp startExpression("/\\*");
    static QRegExp endExpression("\\*/");

    setCurrentBlockState(0);

    int startIndex = 0;
    if(previousBlockState() != 1) {
        startIndex = text.indexOf(startExpression);
    }

    while(startIndex >= 0) {
       int endIndex = text.indexOf(endExpression, startIndex);
       int commentLength;
       if(endIndex == -1) {
           setCurrentBlockState(1);
           commentLength = text.length() - startIndex;
       } else {
           commentLength = endIndex - startIndex + endExpression.matchedLength();
       }
       setFormat(startIndex, commentLength, Qt::darkGreen);
       startIndex = text.indexOf(startExpression, startIndex + commentLength);
    }
}


} // GUI
} // ArgoNavis
