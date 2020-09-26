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

class EloApp : public Wt::WApplication
{
public:
    EloApp(const Wt::WEnvironment& env);

    void onInternalPathChanged(const std::string &path);

private:
    Wt::WStackedWidget *m_stack;
    RankingWidget *m_rankingWidget;
    PlayerWidget *m_playerWidget;
    FoosDB::Database m_database;
};

EloApp::EloApp(const Wt::WEnvironment& env)
    : Wt::WApplication(env)
    , m_database("db.sqlite")
{
    setTitle("Hello world");

    m_stack = root()->addWidget(make_unique<Wt::WStackedWidget>());
    m_rankingWidget = m_stack->addWidget(make_unique<RankingWidget>(&m_database));
    m_playerWidget = m_stack->addWidget(make_unique<PlayerWidget>(&m_database, 1917));

    internalPathChanged().connect(this, &EloApp::onInternalPathChanged);
}

void EloApp::onInternalPathChanged(const std::string &path)
{
    if (path.find("/player/") == 0) {
        const int id = atoi(path.data() + 8);
        if (id > 0) {
            m_stack->setCurrentWidget(m_playerWidget);
            m_playerWidget->setPlayerId(id);
        }
    }
    else {
        m_stack->setCurrentWidget(m_rankingWidget);
    }
}

int main(int argc, char **argv)
{
    return Wt::WRun(argc, argv, [](const Wt::WEnvironment& env) {
      return std::make_unique<EloApp>(env);
    });
}
