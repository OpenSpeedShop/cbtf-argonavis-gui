#include "DerivedMetricInformation.h"


namespace ArgoNavis { namespace GUI {

/**
 * @brief DerivedMetricInformation::DerivedMetricInformation
 *
 * Constructs a DerivedMetricInformation instance.
 */
DerivedMetricInformation::DerivedMetricInformation()
{

}

/**
 * @brief DerivedMetricInformation::DerivedMetricInformation
 * @param nameDescription - the name of the derived metric
 * @param formula - the formula for the derived metric
 * @param enabled - whether the derived metric should be enabled
 *
 * Constructs a DerivedMetricInformation instance.
 */
DerivedMetricInformation::DerivedMetricInformation(const QString &nameDescription, const QString &formula, bool enabled)
    : m_nameDescription( nameDescription )
    , m_formula( formula )
    , m_enabled( enabled )
{

}

/**
 * @brief DerivedMetricInformation::nameDescription
 * @return - the name of the derived metric
 *
 * The name/description getter.
 */
QString DerivedMetricInformation::nameDescription() const
{
    return m_nameDescription;
}

/**
 * @brief DerivedMetricInformation::setNameDescription
 * @param nameDescription - the name of the derived metric
 *
 * The name/description setter.
 */
void DerivedMetricInformation::setNameDescription(const QString &nameDescription)
{
    m_nameDescription = nameDescription;
}

/**
 * @brief DerivedMetricInformation::formula
 * @return - the formula for the derived metric
 *
 * The formula getter.
 */
QString DerivedMetricInformation::formula() const
{
    return m_formula;
}

/**
 * @brief DerivedMetricInformation::setFormula
 * @param nameDescription - the formula for the derived metric
 *
 * The formula setter.
 */
void DerivedMetricInformation::setFormula(const QString &formula)
{
    m_formula = formula;
}

/**
 * @brief DerivedMetricInformation::enabled
 * @return - whether the derived metric is enabled
 *
 * The enabled getter.
 */
bool DerivedMetricInformation::enabled() const
{
    return m_enabled;
}

/**
 * @brief DerivedMetricInformation::setEnabled
 * @param enabled - whether the derived metric is enabled
 *
 * The enabled setter.
 */
void DerivedMetricInformation::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

/**
 * @brief DerivedMetricInformation::read
 * @param json - the json object
 *
 * This method reads the XML data in the JSON object to extract the derived metric information and set the class state variables.
 */
void DerivedMetricInformation::read(const QJsonObject &json)
{
    if ( json.contains("name") && json["name"].isString() ) {
        m_nameDescription = json["name"].toString();
    }

    if ( json.contains("formula") && json["formula"].isString() ) {
        m_formula = json["formula"].toString();
    }

    if ( json.contains("enabled") && json["enabled"].isBool() ) {
        m_enabled = json["enabled"].toBool();
    }
}

/**
 * @brief DerivedMetricInformation::write
 * @param json - the json object
 *
 * Sets the JSON object state from the class state variables.
 */
void DerivedMetricInformation::write(QJsonObject &json) const
{
    json["name"] = m_nameDescription;
    json["formula"] = m_formula;
    json["enabled"] = m_enabled;
}


} // GUI
} // ArgoNavis
