#include <Wt/WApplication.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>

#include "rankingwidget.hpp"
#include "playerwidget.hpp"
#include "database.hpp"

class EloApp : public Wt::WApplication
{
public:
    EloApp(const Wt::WEnvironment& env);

private:
    RankingWidget *m_rankingWidget;
    PlayerWidget *m_playerWidget;
    FoosDB::Database m_database;
};

EloApp::EloApp(const Wt::WEnvironment& env)
    : Wt::WApplication(env)
    , m_database("db.sqlite")
{
    setTitle("Hello world");

//    m_rankingWidget = root()->addWidget(std::make_unique<RankingWidget>(&m_database));
    m_playerWidget = root()->addWidget(std::make_unique<PlayerWidget>(&m_database, 1917));
}

int main(int argc, char **argv)
{
    return Wt::WRun(argc, argv, [](const Wt::WEnvironment& env) {
      return std::make_unique<EloApp>(env);
    });
}
