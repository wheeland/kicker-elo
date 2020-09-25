#include <Wt/WApplication.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>

#include "rankingwidget.hpp"
#include "database.hpp"

class EloApp : public Wt::WApplication
{
public:
    EloApp(const Wt::WEnvironment& env);

private:
    RankingWidget *m_rankingWidget;
};

EloApp::EloApp(const Wt::WEnvironment& env)
    : Wt::WApplication(env)
{
    setTitle("Hello world");

    m_rankingWidget = root()->addWidget(std::make_unique<RankingWidget>());
}

int main(int argc, char **argv)
{
    FoosDB::Database m_database("db2.sqlite");

    return Wt::WRun(argc, argv, [](const Wt::WEnvironment& env) {
      return std::make_unique<EloApp>(env);
    });
}
