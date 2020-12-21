#pragma once

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WMenu.h>

#include "rankingwidget.hpp"
#include "playerwidget.hpp"
#include "infopopup.hpp"
#include "database.hpp"

class EloApp : public Wt::WApplication
{
public:
    EloApp(const Wt::WEnvironment& env);

private:
    void navigate(std::string path);
    void onInternalPathChanged(const std::string &path);

    void showRanking();
    void showPlayer(int id);
    void showMenu();
    void showInfo();

    FoosDB::Database *m_currentDb;

    Wt::WStackedWidget *m_contentPane;
    RankingWidget *m_rankingWidget;
    PlayerWidget *m_playerWidget;

    Wt::WPushButton *m_menuButton;
    Wt::WContainerWidget *m_menuContainer;
    Wt::WMenu *m_menu;

    Wt::WContainerWidget *m_bgDimmer;
    InfoPopup *m_infoPopup;
};
