#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WWebWidget.h>

#include "database.hpp"
#include "util.hpp"
#include "global.hpp"
#include "app.hpp"

#include <QFile>
#include <QDebug>

using std::make_unique;
using namespace Wt;

static QFile s_logFile;

static const char *msgTypeStr(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "[DEBUG]";
    case QtWarningMsg: return "[WARNING]";
    case QtCriticalMsg: return "[CRITICAL]";
    case QtFatalMsg: return "[FATAL]";
#if QT_VERSION > QT_VERSION_CHECK(5, 5, 0)
    case QtInfoMsg: return "[INFO]";
#endif
    default: return "[NONE]";
    }
}

static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(ctx)

    const QDateTime dt = QDateTime::currentDateTime();
    char dtStrBuf[256];
    snprintf(dtStrBuf, sizeof(dtStrBuf), "%4d-%02d-%02d %02d:%02d:%02d:%03d",
             dt.date().year(), dt.date().month(), dt.date().day(),
             dt.time().hour(), dt.time().minute(), dt.time().second(), dt.time().msec());

    const QString out = QString::fromUtf8(dtStrBuf) + " " + QString(msgTypeStr(type)) + " " + msg + "\n";
    s_logFile.write(out.toUtf8());
    s_logFile.flush();
}

int main(int argc, char **argv)
{
    const QByteArray logPath = qgetenv(ENV_LOG_PATH);
    s_logFile.setFileName(logPath.isEmpty() ? QString("lo.log") : QString::fromUtf8(logPath));
    s_logFile.open(QFile::Append);

    qInstallMessageHandler(messageHandler);

    const QByteArray dbPathBer = qgetenv(ENV_DB_PATH_BERLIN);
    const QByteArray dbPathGer = qgetenv(ENV_DB_PATH_GERMANY);
    if (dbPathBer.isEmpty() || dbPathGer.isEmpty()) {
        qCritical() << "Need to specify" << ENV_DB_PATH_BERLIN << "and" << ENV_DB_PATH_GERMANY << "environment variables";
        qFatal("Aborting");
    }

    qDebug() << "Started, opening databases" << dbPathBer << "and" << dbPathGer;

    FoosDB::Database::create("ger", std::string(dbPathGer.constData()));
    FoosDB::Database::create("ber", std::string(dbPathBer.constData()));

    return WRun(argc, argv, [](const WEnvironment& env) {
      return std::make_unique<EloApp>(env);
    });
}
