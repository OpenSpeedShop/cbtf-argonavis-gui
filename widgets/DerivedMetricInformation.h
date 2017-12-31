#ifndef DERIVEDMETRICINFORMATION_H
#define DERIVEDMETRICINFORMATION_H

#include <QObject>
#include <QString>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QJsonObject>
#endif


namespace ArgoNavis { namespace GUI {


class DerivedMetricInformation
{
    Q_GADGET

public:

    explicit DerivedMetricInformation();
    explicit DerivedMetricInformation(const QString& nameDescription, const QString& formula, bool enabled);

    QString nameDescription() const;
    void setNameDescription(const QString &nameDescription);

    QString formula() const;
    void setFormula(const QString &formula);

    bool enabled() const;
    void setEnabled(bool enabled);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    void read(const QJsonObject& json);
    void write(QJsonObject &json) const;
#endif

private:

    QString m_nameDescription;
    QString m_formula;
    bool m_enabled;

};


} // GUI
} // ArgoNavis

#endif // DERIVEDMETRICINFORMATION_H
