#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QDebug>

class CheapProfiler
{
public:
    CheapProfiler(const QString &title) : m_title(title)
    {
        m_timer.start();
    }

    ~CheapProfiler()
    {
        const qint64 msecs = m_timer.elapsed();
#if QT_VERSION > QT_VERSION_CHECK(5, 4, 0)
        qDebug().nospace().noquote() << m_title << ": " << msecs << " msecs";
#else
        qDebug().nospace() << m_title << ": " << msecs << " msecs";
#endif
    }

private:
    const QString m_title;
    QElapsedTimer m_timer;
};
