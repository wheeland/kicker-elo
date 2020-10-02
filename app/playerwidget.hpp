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
    PlayerWidget(int playerId);
    ~PlayerWidget();

    void setPlayerId(int id);

private:
    void updateChart();
    void selectChart();
    void updateOpponents();
    void updateMatchTable();

    void prev();
    void next();

    void setDomain(FoosDB::EloDomain domain);

    FoosDB::Database *m_db;

    int m_playerId = 0;
    const FoosDB::Player *m_player = nullptr;
    QVector<FoosDB::PlayerVsPlayerStats> m_pvpStats;
    QVector<FoosDB::Player::EloProgression> m_progression;

    int m_singleCount = 0;
    int m_doubleCount = 0;
    int m_peakSingle = 0;
    int m_peakDouble = 0;
    int m_peakCombined = 0;

    Wt::WVBoxLayout *m_layout;
    Wt::WText *m_title;

    Wt::WText *m_eloCombined;
    Wt::WText *m_eloCombinedPeak;
    Wt::WPushButton *m_eloCombinedButton;

    Wt::WText *m_eloDouble;
    Wt::WText *m_eloDoublePeak;
    Wt::WPushButton *m_eloDoubleButton;

    Wt::WText *m_eloSingle;
    Wt::WText *m_eloSinglePeak;
    Wt::WPushButton *m_eloSingleButton;

    Wt::WTable *m_opponents;
    Wt::WTable *m_matchesTable;

    Wt::WPushButton *m_prevButton;
    Wt::WPushButton *m_nextButton;

    int m_page = 0;
    int m_entriesPerPage = 20;
    FoosDB::EloDomain m_displayedDomain = FoosDB::EloDomain::Combined;

    struct Row {
        // column 1
        Wt::WText *date;
        Wt::WText *competition;
        Wt::WText *compInfo;
        // column 2
        Wt::WAnchor *player1;
        Wt::WText *player1Elo;
        Wt::WAnchor *player11;
        Wt::WText *player11Elo;
        // column 3
        Wt::WText *score;
        Wt::WText *eloChange;
        // column 4
        Wt::WAnchor *player2;
        Wt::WText *player2Elo;
        Wt::WAnchor *player22;
        Wt::WText *player22Elo;
    };
    QVector<Row> m_rows;

    std::shared_ptr<Wt::WStandardItemModel> m_eloModel;
    Wt::WStackedWidget *m_chartStack;
    Wt::Chart::WCartesianChart *m_eloChartCombined;
    Wt::Chart::WCartesianChart *m_eloChartSingle;
    Wt::Chart::WCartesianChart *m_eloChartDouble;
};
