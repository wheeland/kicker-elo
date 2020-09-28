#pragma once

#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WAnchor.h>
#include <Wt/WPushButton.h>
#include <Wt/WLineEdit.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>

#include "database.hpp"

#include <QVector>

class RankingWidget : public Wt::WContainerWidget
{
public:
    RankingWidget();
    ~RankingWidget();

private:
    void prev();
    void next();
    void update();
    void updateSearch();

    FoosDB::Database *m_db;

    enum SortPolicy {
        Single = (int) FoosDB::EloDomain::Single,
        Double = (int) FoosDB::EloDomain::Double,
        Combined = (int) FoosDB::EloDomain::Combined,
        Games
    };

    SortPolicy m_sortPolicy = Single;

    Wt::WVBoxLayout *m_layout;
    Wt::WLineEdit *m_searchBar;
    Wt::WTable *m_table;
    Wt::WPushButton *m_prevButton;
    Wt::WPushButton *m_nextButton;

    int m_page = 0;
    int m_entriesPerPage = 50;

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
