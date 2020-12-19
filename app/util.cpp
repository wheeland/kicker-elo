#include "util.hpp"

CheapProfiler::CheapProfiler(const QString &title) : m_title(title)
{
    m_timer.start();
}

CheapProfiler::~CheapProfiler()
{
    const qint64 msecs = m_timer.elapsed();
#if QT_VERSION > QT_VERSION_CHECK(5, 4, 0)
    qDebug().nospace().noquote() << m_title << ": " << msecs << " msecs";
#else
    qDebug().nospace() << m_title << ": " << msecs << " msecs";
#endif
}

QDebug &operator<<(QDebug &dbg, const std::string &s)
{
    dbg << s.c_str();
    return dbg;
}
