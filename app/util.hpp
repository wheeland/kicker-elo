#pragma once

#include <QString>

namespace Util {

template <class T>
static std::string num2str(T num)
{
    return QString::number(num).toStdString();
}

} // namespace Util

