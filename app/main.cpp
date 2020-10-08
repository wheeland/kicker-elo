#include <Wt/WApplication.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>

#include "rankingwidget.hpp"
#include "playerwidget.hpp"
#include "database.hpp"

#include <QFile>

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

class EloApp : public WApplication
{
public:
    EloApp(const WEnvironment& env);

    void onInternalPathChanged(const std::string &path);

private:
    void showRanking();
    void showPlayer(int id);

    WStackedWidget *m_stack;
    WAnchor *m_backToRanking;
    RankingWidget *m_rankingWidget;
    PlayerWidget *m_playerWidget;
};

EloApp::EloApp(const WEnvironment& env)
    : WApplication(env)
{
    setTitle("TFVB Elo Rankings");

    WContainerWidget *rootBg = root()->addWidget(make_unique<WContainerWidget>());
    rootBg->addStyleClass("bg");

    WContainerWidget *contentBg = root()->addWidget(make_unique<WContainerWidget>());
    contentBg->addStyleClass("content_bg");

    WContainerWidget *content = contentBg->addWidget(make_unique<WContainerWidget>());
    content->addStyleClass("content");

    m_backToRanking = content->addWidget(make_unique<WAnchor>());
    m_backToRanking->setText("&lt;&lt;");
    m_backToRanking->setPositionScheme(PositionScheme::Absolute);
    m_backToRanking->setOffsets("-100px", Side::Left);
    m_backToRanking->setOffsets("100px", Side::Top);

    m_stack = content->addWidget(make_unique<WStackedWidget>());
    m_rankingWidget = m_stack->addWidget(make_unique<RankingWidget>());
    m_playerWidget = m_stack->addWidget(make_unique<PlayerWidget>(1917));

    internalPathChanged().connect(this, &EloApp::onInternalPathChanged);

    useStyleSheet("style.css");
    messageResourceBundle().use("elo");

    onInternalPathChanged(internalPath());
}

void EloApp::onInternalPathChanged(const std::string &path)
{
    if (path.find("/player/") == 0) {
        const int id = atoi(path.data() + 8);
        if (id > 0) {
            showPlayer(id);
        }
    }
    else {
        showRanking();
    }
}

void EloApp::showRanking()
{
    m_stack->setCurrentWidget(m_rankingWidget);
}

void EloApp::showPlayer(int id)
{
    m_stack->setCurrentWidget(m_playerWidget);
    m_playerWidget->setPlayerId(id);
}

int main(int argc, char **argv)
{
    const QByteArray logPath = qgetenv("LOGPATH");
    s_logFile.setFileName(logPath.isEmpty() ? QString("/var/elo/elo.log") : QString::fromUtf8(logPath));
    s_logFile.open(QFile::Append);

    qInstallMessageHandler(messageHandler);

    const QByteArray dbPath = qgetenv("SQLITEDB");
    if (dbPath.isEmpty()) {
        qFatal("Need to specify SQLITEDB environment variable");
    }

    FoosDB::Database::create(std::string(dbPath.constData()));

    return WRun(argc, argv, [](const WEnvironment& env) {
      return std::make_unique<EloApp>(env);
    });
}
