#pragma once

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>

#include "rankingwidget.hpp"
#include "playerwidget.hpp"
#include "infopopup.hpp"

class EloApp : public Wt::WApplication
{
public:
    EloApp(const Wt::WEnvironment& env);

    void navigate(const std::string &path);

private:
    void showRanking();
    void showPlayer(int id);

    Wt::WStackedWidget *m_stack;
    RankingWidget *m_rankingWidget;
    PlayerWidget *m_playerWidget;

    Wt::WContainerWidget *m_bgDimmer;
    InfoPopup *m_infoPopup;
};
