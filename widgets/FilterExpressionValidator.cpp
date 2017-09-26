/*!
   \file FilterExpressionValidator.cpp
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2017 Argo Navis Technologies, LLC

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

#include "FilterExpressionValidator.h"

#include <QRegExp>


namespace ArgoNavis { namespace GUI {


/**
 * @brief FilterExpressionValidator::FilterExpressionValidator
 * @param parent - the parent widget
 *
 * Constructs an FilterExpressionValidator instance of the given parent.
 */
FilterExpressionValidator::FilterExpressionValidator(QObject* parent)
    : QValidator( parent )
{

}

/**
 * @brief FilterExpressionValidator::validate
 * @param input - the input string to be validated
 * @param pos - the cursor position within the input string
 * @return returns 'Acceptable' when the input string is a valid regular expression; otherwise 'Intermediate'
 *
 * This method implements the pure virtual function QValidator::validate().  It checks whether the
 * input string forms a valid regular expression.  If it does, then the function returns 'Acceptable';
 * otherwise returns 'Intermediate'.
 */
QValidator::State FilterExpressionValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED( pos );

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    static QStringList INTERMEDIATE_ERROR_STRINGS{ "unexpected end", "no error occurred", "bad repetition syntax" };
#else
    static QStringList INTERMEDIATE_ERROR_STRINGS = QStringList() << "unexpected end" << "no error occurred" << "bad repetition syntax";
#endif

    QRegExp regularExp( input );

    QValidator::State state;

    if ( regularExp.isValid() )
        state = Acceptable;
    else if ( INTERMEDIATE_ERROR_STRINGS.contains( regularExp.errorString() ) )
        state = Intermediate;
    else
        state = Invalid;

    return state;
}


} // GUI
} // ArgoNavis
