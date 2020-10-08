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
        qDebug().nospace().noquote() << m_title << ": " << msecs << " msecs";
    }

private:
    const QString m_title;
    QElapsedTimer m_timer;
};
