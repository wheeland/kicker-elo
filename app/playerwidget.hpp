#pragma once

#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/Chart/WCartesianChart.h>

#include "database.hpp"

#include <QVector>

class PlayerWidget : public Wt::WContainerWidget
{
public:
    PlayerWidget(FoosDB::Database *db, int playerId);
    ~PlayerWidget();

    void setPlayerId(int id);

private:
    void prev();
    void next();
    void updateChart();
    void updateOpponents();
    void updateMatchTable();

    FoosDB::Database *m_db;

    int m_playerId = 0;
    const FoosDB::Player *m_player = nullptr;
    QVector<FoosDB::PlayerVsPlayerStats> m_pvpStats;
    QVector<FoosDB::PlayerEloProgression> m_progression;

    int m_singleCount = 0;
    int m_doubleCount = 0;
    int m_peakSingle = 0;
    int m_peakDouble = 0;
    int m_peakCombined = 0;

    Wt::WText *m_title;
    Wt::WText *m_eloCombind;
    Wt::WText *m_eloDouble;
    Wt::WText *m_eloSingle;
    Wt::WTable *m_opponents;
    Wt::WTable *m_matchesTable;
    Wt::WPushButton *m_prevButton;
    Wt::WPushButton *m_nextButton;

    int m_page = 0;
    int m_entriesPerPage = 20;
    FoosDB::EloDomain m_displayedDomain = FoosDB::EloDomain::Single;

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

    std::shared_ptr<Wt::WStandardItemModel> m_eloModel;
    Wt::Chart::WCartesianChart *m_eloChart;
};
