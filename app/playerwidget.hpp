#pragma once

#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
#include <Wt/WContainerWidget.h>

#include "database.hpp"

class PlayerWidget : public Wt::WContainerWidget
{
public:
    PlayerWidget(FoosDB::Database *db, int playerId);
    ~PlayerWidget();

    void setPlayerId(int id);

private:
    void prev();
    void next();
    void update();

    FoosDB::Database *m_db;
    int m_playerId;
    const FoosDB::Player *m_player = nullptr;
    Wt::WText *m_title;
    Wt::WTable *m_matches;
    Wt::WPushButton *m_prevButton;
    Wt::WPushButton *m_nextButton;

    int m_page = 0;
    int m_entriesPerPage = 20;

    struct Row {
        // column 1
        Wt::WText *date;
        Wt::WText *competition;
        // column 2
        Wt::WAnchor *player1;
        Wt::WAnchor *player11;
        // column 3
        Wt::WText *score;
        // column 4
        Wt::WAnchor *player2;
        Wt::WAnchor *player22;
        // column 5?
        Wt::WText *eloCombined;
        Wt::WText *eloSingle;
        Wt::WText *eloDouble;
    };
    QVector<Row> m_rows;
};
