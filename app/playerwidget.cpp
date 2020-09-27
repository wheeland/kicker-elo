#include "playerwidget.hpp"
#include "util.hpp"

#include <Wt/WVBoxLayout.h>
#include <Wt/WDate.h>
#include <Wt/WTime.h>
#include <Wt/WDateTime.h>
#include <Wt/WStandardItem.h>

using namespace Wt;
using namespace Util;
using std::make_unique;

static std::string date2str(const QDateTime &dt)
{
    const QDate date = dt.date();
    return QString::asprintf("%02d.%02d.%d", date.day(), date.month(), date.year()).toStdString();
}

static std::string diff2str(int diff)
{
    return (diff > 0) ? ("+" + num2str(diff)) : num2str(diff);
}

static std::string player2str(const FoosDB::Player *player)
{
    return player ? (player->firstName + " " + player->lastName).toStdString() : "";
}

PlayerWidget::PlayerWidget(FoosDB::Database *db, int playerId)
    : m_db(db)
{
    setContentAlignment(AlignmentFlag::Center);

    m_title = addWidget(make_unique<WText>(""));
    m_eloCombind = addWidget(make_unique<WText>(""));
    m_eloDouble = addWidget(make_unique<WText>(""));
    m_eloSingle = addWidget(make_unique<WText>(""));

    //
    // setup ELO plot
    //
    m_eloChart = addWidget(make_unique<Wt::Chart::WCartesianChart>());
    m_eloChart->setBackground(WColor(220, 220, 220));
    m_eloChart->setLegendEnabled(true);
    m_eloChart->setType(Chart::ChartType::Scatter);
    m_eloChart->setPlotAreaPadding(40, Side::Left | Side::Top | Side::Bottom);
    m_eloChart->setPlotAreaPadding(120, Side::Right);
    m_eloChart->resize(800, 400);
    m_eloChart->setMargin(WLength::Auto, Side::Left | Side::Right);

    //
    // setup opponents table
    //
    m_opponents = addWidget(make_unique<WTable>());

    //
    // setup matches table
    //
    m_matchesTable = addWidget(make_unique<WTable>());
    m_matchesTable->insertRow(0)->setHeight("2em");
    m_matchesTable->insertColumn(0)->setWidth("15vw");
    m_matchesTable->insertColumn(1)->setWidth("15vw");
    m_matchesTable->insertColumn(2)->setWidth("5vw");
    m_matchesTable->insertColumn(3)->setWidth("15vw");
    m_matchesTable->insertColumn(4)->setWidth("15vw");

    m_matchesTable->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Competition</b>"));
    m_matchesTable->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Team 1</b>"));
    m_matchesTable->elementAt(0, 2)->addWidget(make_unique<WText>("<b>Score</b>"));
    m_matchesTable->elementAt(0, 3)->addWidget(make_unique<WText>("<b>Team 2</b>"));
    m_matchesTable->elementAt(0, 4)->addWidget(make_unique<WText>("<b>ELO</b>"));

    //
    // setup buttons
    //
    m_prevButton = addWidget(make_unique<WPushButton>("Prev"));
    m_nextButton = addWidget(make_unique<WPushButton>("Next"));

    m_prevButton->clicked().connect(this, &PlayerWidget::prev);
    m_nextButton->clicked().connect(this, &PlayerWidget::next);

    setPlayerId(playerId);
}

PlayerWidget::~PlayerWidget()
{
}

void PlayerWidget::setPlayerId(int id)
{
    m_playerId = id;
    m_player = m_db->getPlayer(id);
    m_page = 0;

    if (m_player) {
        m_pvpStats = m_db->getPlayerVsPlayerStats(m_player);
        m_progression = m_db->getPlayerEloProgression(m_player);
        m_singleCount = m_db->getPlayerMatchCount(m_player, FoosDB::EloDomain::Single);
        m_doubleCount = m_db->getPlayerMatchCount(m_player, FoosDB::EloDomain::Double);
    } else {
        m_pvpStats.clear();
        m_progression.clear();
        m_singleCount = 0;
        m_doubleCount = 0;
    }

    //
    // Update Peak ELO statistics
    //
    m_peakSingle = m_peakDouble = m_peakCombined = 0;
    for (const FoosDB::PlayerEloProgression &progression : m_progression) {
        m_peakSingle = qMax(m_peakSingle, progression.eloSingle);
        m_peakDouble = qMax(m_peakDouble, progression.eloDouble);
        m_peakCombined = qMax(m_peakCombined, progression.eloCombined);
    }

    if (m_player) {
        const auto eloText = [](const std::string &name, int curr, int peak) {
            return "<p><b>" + name + ": " + num2str(curr) + " (Peak: " + num2str(peak) + ")</b></p>";
        };
        m_title->setText("<b>" + player2str(m_player) + "</b>");
        m_eloCombind->setText(eloText("Combined", m_player->eloCombined, m_peakCombined));
        m_eloDouble->setText(eloText("Double", m_player->eloDouble, m_peakDouble));
        m_eloSingle->setText(eloText("Single", m_player->eloSingle, m_peakSingle));
    } else {
        m_title->setText("Invalid Player");
        m_eloCombind->setText("");
        m_eloDouble->setText("");
        m_eloSingle->setText("");
    }

    updateChart();
    updateOpponents();
    updateMatchTable();
}

void PlayerWidget::prev()
{
    m_page = qMax(m_page - 1, 0);
    updateMatchTable();
}

void PlayerWidget::next()
{
    m_page++;
    updateMatchTable();
}

void PlayerWidget::updateChart()
{
    m_eloModel = std::make_shared<Wt::WStandardItemModel>(m_progression.size(), 4);
    m_eloModel->setHeaderData(0, WString("Date"));
    m_eloModel->setHeaderData(1, WString("Combined"));
    m_eloModel->setHeaderData(2, WString("Double"));
    m_eloModel->setHeaderData(3, WString("Single"));

    for (int i = 0; i < m_progression.size(); ++i) {
        const FoosDB::PlayerEloProgression pep = m_progression[i];
        const WDate date(pep.date.date().year(), pep.date.date().month(), pep.date.date().day());
        m_eloModel->setData(i, 0, date);
        m_eloModel->setData(i, 1, (float) pep.eloCombined);
        m_eloModel->setData(i, 2, (float) pep.eloDouble);
        m_eloModel->setData(i, 3, (float) pep.eloSingle);
    }

    m_eloChart->setModel(m_eloModel);
    m_eloChart->setXSeriesColumn(0);
    m_eloChart->axis(Chart::Axis::X).setScale(Chart::AxisScale::Date);
    for (int i = 0; i < 3; ++i) {
        auto s = make_unique<Chart::WDataSeries>(i + 1, Chart::SeriesType::Line);
        m_eloChart->addSeries(std::move(s));
    }
}

void PlayerWidget::updateOpponents()
{
    m_opponents->clear();

    if (!m_player)
        return;

//    const auto getstr = [](const QVector<OtherPlayerStats> &stats, int idx) {
//        if (qAbs(idx) > stats.size())
//            return std::string();
//        const int realIdx = (idx > 0) ? (idx - 1) : (stats.size() + idx);
//        const OtherPlayerStats ops = stats.value(realIdx);
//        return "<p><b>" + player2str(ops.player) + "</b>: " +
//                diff2str(ops.eloDelta) + " <i>(" + num2str(ops.matchCount) + " matches)</i></p>";
//    };

    m_opponents->insertRow(0)->setHeight("2em");
    m_opponents->insertColumn(0)->setWidth("15vw");
    m_opponents->insertColumn(1)->setWidth("15vw");
    m_opponents->insertColumn(2)->setWidth("15vw");
    m_opponents->insertColumn(3)->setWidth("15vw");
    m_opponents->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Evil Wizards</b>"));
    m_opponents->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Poor Souls</b>"));
    m_opponents->elementAt(0, 2)->addWidget(make_unique<WText>("<b>Idiots</b>"));
    m_opponents->elementAt(0, 3)->addWidget(make_unique<WText>("<b>Heroes</b>"));

//    for (int i = 1 ; i <= 3; ++i) {
//        const std::string o1 = getstr(m_combinedStats.m_opponentDelta, i);
//        const std::string o2 = getstr(m_combinedStats.m_opponentDelta, -i);
//        const std::string p1 = getstr(m_combinedStats.m_partnerDelta, i);
//        const std::string p2 = getstr(m_combinedStats.m_partnerDelta, -i);

//        if (o1.empty() && o2.empty() && p1.empty() && p2.empty())
//            break;

//        m_opponents->insertRow(m_opponents->rowCount() - 1)->setHeight("1.8em");;
//        const int n = m_opponents->rowCount() - 1;
//        m_opponents->elementAt(n, 0)->addWidget(make_unique<WText>(o1));
//        m_opponents->elementAt(n, 1)->addWidget(make_unique<WText>(o2));
//        m_opponents->elementAt(n, 2)->addWidget(make_unique<WText>(p1));
//        m_opponents->elementAt(n, 3)->addWidget(make_unique<WText>(p2));
//    }
}

void PlayerWidget::updateMatchTable()
{
    if (!m_player) {
        while (m_matchesTable->rowCount() > 1)
            m_matchesTable->removeRow(m_matchesTable->rowCount() - 1);
        return;
    }

    const int totalMatchCount =
            (m_displayedDomain == FoosDB::EloDomain::Single) ? m_singleCount :
            (m_displayedDomain == FoosDB::EloDomain::Double) ? m_doubleCount :
                                                               (m_singleCount + m_doubleCount);

    m_page = qMin(m_page, totalMatchCount / m_entriesPerPage);
    const int start = m_page * m_entriesPerPage;
    int count = qMin(totalMatchCount - start, m_entriesPerPage);

    const QVector<FoosDB::PlayerMatch> matches = m_db->getPlayerMatches(m_player, m_displayedDomain, start, count);
    count = qMin(count, matches.size());

    while (m_matchesTable->rowCount() - 1 < count) {
        const int n = m_matchesTable->rowCount();

        Row row;
        m_matchesTable->insertRow(n)->setHeight("1.8em");

        WContainerWidget *competitionWidget = m_matchesTable->elementAt(n, 0)->addWidget(make_unique<WContainerWidget>());
        row.date = competitionWidget->addWidget(make_unique<WText>());
        row.competition = competitionWidget->addWidget(make_unique<WText>());

        WContainerWidget *player1Widget = m_matchesTable->elementAt(n, 1)->addWidget(make_unique<WContainerWidget>());
        row.player1  = player1Widget->addWidget(make_unique<WAnchor>());
        row.player11 = player1Widget->addWidget(make_unique<WAnchor>());

        row.score = m_matchesTable->elementAt(n, 2)->addWidget(make_unique<WText>());

        WContainerWidget *player2Widget = m_matchesTable->elementAt(n, 3)->addWidget(make_unique<WContainerWidget>());
        row.player2  = player2Widget->addWidget(make_unique<WAnchor>());
        row.player22 = player2Widget->addWidget(make_unique<WAnchor>());

        row.eloCombined = m_matchesTable->elementAt(n, 4)->addWidget(make_unique<WText>());

        m_rows << row;
    }

    while (m_matchesTable->rowCount() - 1 > count) {
        m_matchesTable->removeRow(m_matchesTable->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < count; ++i) {
        const FoosDB::PlayerMatch &m = matches[i];

        m_rows[i].date->setText("<p><small><i>" + date2str(m.date) + "</i></small></p>");
        m_rows[i].competition->setText("<p><small>" + m.competitionName.toStdString() + "</small></p>");
        m_rows[i].competition->setTextFormat(TextFormat::UnsafeXHTML);

        if (m.myScore > 1 || m.opponentScore > 1)
            m_rows[i].score->setText(num2str(m.myScore) + ":" + num2str(m.opponentScore));
        else
            m_rows[i].score->setText((m.myScore > 0) ? "Win" : "Loss");

        const auto ratingStr = [](int elo, int diff) {
            return num2str(elo) + " (" + num2str(diff) + ")";
        };

        const auto playerLink = [](const FoosDB::Player *p) {
            return p ? Wt::WLink(LinkType::InternalPath, "/player/" + num2str(p->id)) : Wt::WLink();
        };

        m_rows[i].player1 ->setText("<p>" + player2str(m_player) + " " + num2str(m.myself.eloCombined) + "</p>");
        m_rows[i].player11->setText("<p>" + player2str(m.partner.player) + " " + num2str(m.partner.eloCombined) + "</p>");
        m_rows[i].player2 ->setText("<p>" + player2str(m.opponent1.player) + " " + num2str(m.opponent1.eloCombined) + "</p>");
        m_rows[i].player22->setText("<p>" + player2str(m.opponent2.player) + " " + num2str(m.opponent2.eloCombined) + "</p>");

        m_rows[i].player1 ->setLink(playerLink(m_player));
        m_rows[i].player11->setLink(playerLink(m.partner.player));
        m_rows[i].player2 ->setLink(playerLink(m.opponent1.player));
        m_rows[i].player22->setLink(playerLink(m.opponent2.player));

        m_rows[i].eloCombined->setText(ratingStr(m.myself.eloCombined + m.eloCombinedDiff, m.eloCombinedDiff));
    }
}
