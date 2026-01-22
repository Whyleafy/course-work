#ifndef DECIMALUTILS_H
#define DECIMALUTILS_H

#include <QString>
#include <QVariant>

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <exception>
#include <iomanip>
#include <sstream>

using Decimal = boost::multiprecision::cpp_dec_float_50;

inline Decimal decimalFromString(const QString &input)
{
    QString normalized = input.trimmed();
    if (normalized.isEmpty()) {
        return Decimal(0);
    }
    normalized.replace(',', '.');
    try {
        return Decimal(normalized.toStdString());
    } catch (const std::exception&) {
        return Decimal(0);
    }
}

inline Decimal decimalFromVariant(const QVariant &value)
{
    return decimalFromString(value.toString());
}

inline QString decimalToString(const Decimal &value, int decimals = 2)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value;
    return QString::fromStdString(stream.str());
}

#endif // DECIMALUTILS_H
