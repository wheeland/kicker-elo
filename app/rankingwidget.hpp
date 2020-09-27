#pragma once

#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WAnchor.h>
#include <Wt/WPushButton.h>
#include <Wt/WLineEdit.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>

#include "database.hpp"

class RankingWidget : public Wt::WContainerWidget
{
public:
    RankingWidget(FoosDB::Database *db);
    ~RankingWidget();

private:
    void prev();
    void next();
    void update();
    void updateSearch();

    FoosDB::Database *m_db;
    FoosDB::EloDomain m_sortDomain = FoosDB::EloDomain::Combined;

    Wt::WVBoxLayout *m_layout;
    Wt::WLineEdit *m_searchBar;
    Wt::WTable *m_table;
    Wt::WPushButton *m_prevButton;
    Wt::WPushButton *m_nextButton;

    int m_page = 0;
    int m_entriesPerPage = 20;

    struct Row {
        Wt::WText *rank;
        Wt::WAnchor *player;
        Wt::WText *eloCombined;
        Wt::WText *eloSingle;
        Wt::WText *eloDouble;
        Wt::WText *matchCount;
    };
    QVector<Row> m_rows;
};
