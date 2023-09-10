#include "util.hpp"

CheapProfiler::CheapProfiler(const QString &title) : m_title(title)
{
#ifdef ENABLE_CHEAP_PROFILER
    m_timer.start();
#endif
}

CheapProfiler::~CheapProfiler()
{
#ifdef ENABLE_CHEAP_PROFILER
    const qint64 msecs = m_timer.elapsed();
#if QT_VERSION > QT_VERSION_CHECK(5, 4, 0)
    qDebug().nospace().noquote() << m_title << ": " << msecs << " msecs";
#else
    qDebug().nospace() << m_title << ": " << msecs << " msecs";
#endif
#endif // ENABLE_CHEAP_PROFILER
}

QDebug &operator<<(QDebug &dbg, const std::string &s)
{
    dbg << s.c_str();
    return dbg;
}

bool removePrefix(std::string &str, const std::string &prefix)
{
    if (str.find(prefix) == 0) {
        str.erase(0, prefix.size());
        return true;
    }
    return false;
}
