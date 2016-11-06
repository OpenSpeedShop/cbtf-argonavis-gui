#ifndef VIEWSORTFILTERPROXYMODEL_H
#define VIEWSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "common/openss-gui-config.h"

#include <QString>
#include <QSet>


namespace ArgoNavis { namespace GUI {


class ViewSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:

    explicit ViewSortFilterProxyModel(const QString& type = "*", QObject* parent = Q_NULLPTR);
    virtual ~ViewSortFilterProxyModel();

    void setFilterRange(double lower, double upper);
    void setColumnHeaders(const QStringList& columnHeaders);

protected:

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;

private:

    QString m_type;
    double m_lower;
    double m_upper;

    QSet< int > m_columns;

};


} // GUI
} // ArgoNavis

#endif // VIEWSORTFILTERPROXYMODEL_H
