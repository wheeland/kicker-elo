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

using std::make_unique;
using namespace Wt;

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
    setTitle("ELO!");

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
    FoosDB::Database::create("db.sqlite");

    return WRun(argc, argv, [](const WEnvironment& env) {
      return std::make_unique<EloApp>(env);
    });
}
