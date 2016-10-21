#ifndef VIEWSORTFILTERPROXYMODEL_H
#define VIEWSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


class ViewSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:

    explicit ViewSortFilterProxyModel(QObject* parent = Q_NULLPTR);
    virtual ~ViewSortFilterProxyModel();

    void setFilterRange(double lower, double upper);

protected:

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:

    double m_lower;
    double m_upper;

};


} // GUI
} // ArgoNavis

#endif // VIEWSORTFILTERPROXYMODEL_H
