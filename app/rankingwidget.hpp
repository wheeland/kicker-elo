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
    RankingWidget(FoosDB::Database *db);
    ~RankingWidget();

    void setDatabase(FoosDB::Database *db);

private:
    void prev();
    void next();
    void update();
    void updateSearch();

    Wt::WLink createPlayerLink(int id) const;

    FoosDB::Database *m_db;

    enum SortPolicy {
        Single = (int) FoosDB::EloDomain::Single,
        Double = (int) FoosDB::EloDomain::Double,
        Combined = (int) FoosDB::EloDomain::Combined,
        Games
    };

    SortPolicy m_sortPolicy = Combined;

    Wt::WVBoxLayout *m_layout;
    Wt::WLineEdit *m_searchBar;
    Wt::WTable *m_table;
    Wt::WPushButton *m_comboButton;
    Wt::WPushButton *m_doubleButton;
    Wt::WPushButton *m_singleButton;
    Wt::WPushButton *m_gamesButton;
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
