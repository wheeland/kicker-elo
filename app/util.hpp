#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QDebug>

class CheapProfiler
{
public:
    CheapProfiler(const QString &title);
    ~CheapProfiler();

private:
    const QString m_title;
    QElapsedTimer m_timer;
};

QDebug& operator<<(QDebug &dbg, const std::string &s);
