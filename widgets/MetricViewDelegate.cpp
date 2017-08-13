/*!
   \file MetricViewDelegate.cpp
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

#include "MetricViewDelegate.h"


namespace ArgoNavis { namespace GUI {


/**
 * @brief MetricViewDelegate::MetricViewDelegate
 * @param parent - the parent widget
 *
 * Constructs an MetricViewDelegate instance of the given parent.
 */
MetricViewDelegate::MetricViewDelegate(QObject *parent)
    : QStyledItemDelegate( parent )
{

}

/**
 * @brief MetricViewDelegate::displayText
 * @param value - a value provided by the model
 * @param locale - the locale instance to use
 * @return - return the properly formatted display string
 *
 * Reimplements QStyledItemDelegate::displayText() method to reformat values in the model of type 'double' to show 6 digits of precision.
 */
QString MetricViewDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    // if the value is of user-type 'double' then reformat the value to have six digits of precision (digits to the right of the period)
    if ( value.userType() == QVariant::Double ) {
        return locale.toString( value.toDouble(), 'f', 6 );
    }
    // all other QVariant user-types are formatted with the default implementation of displayText()
    else {
        return QStyledItemDelegate::displayText( value, locale );
    }
}

/**
 * @brief MetricViewDelegate::paint
 * @param painter - the painter instance
 * @param option - the style option for the item specified by index
 * @param index - the index of the item in the model the delegate should draw
 *
 * Reimplements QStyledItemDelegate::paint to modify the 'displayAlignment' attribute of the QStyleOptionViewItem option to
 * have values of user-type 'double' to have right-alignment in the column.
 */
void MetricViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // get the item's value
    const QVariant value = index.model()->data( index, Qt::EditRole );

    // get a mutable copy of the style option
    QStyleOptionViewItem extended_option = option;

    // if the value is of user-type 'double' or 'unsigned long lomg' then change the 'displayAlignment' attribute from left-alignment to right-alignment
    if ( value.userType() == QVariant::Double || value.userType() == QVariant::ULongLong ) {
        ( extended_option.displayAlignment ^= Qt::AlignLeft ) |= Qt::AlignRight;
    }

    // invoke the base class method using the potentially modified style option
    QStyledItemDelegate::paint( painter, extended_option, index );
}


} // GUI
} // ArgoNavis
