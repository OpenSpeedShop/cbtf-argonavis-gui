#ifndef DEVICESSORTFILTERPROXYMODEL_H
#define DEVICESSORTFILTERPROXYMODEL_H

#include <QObject>

class DevicesSortFilterProxyModel : public QObject
{
    Q_OBJECT
public:
    explicit DevicesSortFilterProxyModel(QObject *parent = 0);

signals:

public slots:
};

#endif // DEVICESSORTFILTERPROXYMODEL_H