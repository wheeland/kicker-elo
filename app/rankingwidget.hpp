#pragma once

#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
#include <Wt/WContainerWidget.h>

#include "database.hpp"

class RankingWidget : public Wt::WContainerWidget
{
public:
    RankingWidget(FoosDB::Database *db);
    ~RankingWidget();

    Wt::Signal<int>& playerClicked() { return m_playerClicked; }

private:
    void prev();
    void next();
    void update();

    FoosDB::Database *m_db;
    FoosDB::EloDomain m_sortDomain = FoosDB::EloDomain::Combined;
    Wt::WText *m_title;
    Wt::WTable *m_table;
    Wt::WPushButton *m_prevButton;
    Wt::WPushButton *m_nextButton;

    int m_page = 0;
    int m_entriesPerPage = 20;

    Wt::Signal<int> m_playerClicked;
};
