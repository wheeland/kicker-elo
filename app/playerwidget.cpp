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

PlayerWidget::PlayerWidget(FoosDB::Database *db, int playerId)
    : m_db(db)
{
    setContentAlignment(AlignmentFlag::Center);

    m_title = addWidget(make_unique<WText>(""));

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
    // setup table
    //
    m_matches = addWidget(make_unique<WTable>());
    m_matches->insertRow(0)->setHeight("2em");
    m_matches->insertColumn(0)->setWidth("15vw");
    m_matches->insertColumn(1)->setWidth("15vw");
    m_matches->insertColumn(2)->setWidth("5vw");
    m_matches->insertColumn(3)->setWidth("15vw");
    m_matches->insertColumn(4)->setWidth("15vw");

    m_matches->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Competition</b>"));
    m_matches->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Team 1</b>"));
    m_matches->elementAt(0, 2)->addWidget(make_unique<WText>("<b>Score</b>"));
    m_matches->elementAt(0, 3)->addWidget(make_unique<WText>("<b>Team 2</b>"));
    m_matches->elementAt(0, 4)->addWidget(make_unique<WText>("<b>ELO</b>"));

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

    if (m_player)
        m_playerMatches = m_db->getPlayerMatches(m_player);
    else
        m_playerMatches.clear();

    updateChart();
    updateTable();
}

void PlayerWidget::prev()
{
    m_page = qMax(m_page - 1, 0);
    updateTable();
}

void PlayerWidget::next()
{
    m_page++;
    updateTable();
}

void PlayerWidget::updateChart()
{
    if (m_playerMatches.isEmpty())
        return;

    int eloCombined = m_playerMatches.first().eloCombined - m_playerMatches.first().eloCombinedDiff;
    int eloSingle = eloCombined;
    int eloDouble = eloCombined;

    m_eloModel = std::make_shared<Wt::WStandardItemModel>(m_playerMatches.size(), 4);
    m_eloModel->setHeaderData(0, WString("Date"));
    m_eloModel->setHeaderData(1, WString("Combined"));
    m_eloModel->setHeaderData(2, WString("Double"));
    m_eloModel->setHeaderData(3, WString("Single"));

    for (int i = 0; i < m_playerMatches.size(); ++i) {
        const FoosDB::PlayerMatch &pm = m_playerMatches[i];

        const WDate date(pm.date.date().year(), pm.date.date().month(), pm.date.date().day());

        eloCombined = pm.eloCombined;
        if (pm.matchType == FoosDB::MatchType::Single)
            eloSingle = pm.eloSeparate;
        else
            eloDouble = pm.eloSeparate;

        m_eloModel->setData(i, 0, date);
        m_eloModel->setData(i, 1, (float) eloCombined);
        m_eloModel->setData(i, 2, (float) eloDouble);
        m_eloModel->setData(i, 3, (float) eloSingle);
    }

    m_eloChart->setModel(m_eloModel);
    m_eloChart->setXSeriesColumn(0);
    m_eloChart->axis(Chart::Axis::X).setScale(Chart::AxisScale::Date);
    for (int i = 0; i < 3; ++i) {
        auto s = make_unique<Chart::WDataSeries>(i + 1, Chart::SeriesType::Line);
        m_eloChart->addSeries(std::move(s));
    }
}

void PlayerWidget::updateTable()
{
    if (!m_player) {
        while (m_matches->rowCount() > 1)
            m_matches->removeRow(m_matches->rowCount() - 1);
        return;
    }

    m_page = qMin(m_page, m_player->matchCount / m_entriesPerPage);
    const int count = qMin(m_player->matchCount - m_page * m_entriesPerPage, m_entriesPerPage);

    while (m_matches->rowCount() - 1 < count) {
        const int n = m_matches->rowCount();

        Row row;
        m_matches->insertRow(n)->setHeight("1.8em");

        WContainerWidget *competitionWidget = m_matches->elementAt(n, 0)->addWidget(make_unique<WContainerWidget>());
        row.date = competitionWidget->addWidget(make_unique<WText>());
        row.competition = competitionWidget->addWidget(make_unique<WText>());

        WContainerWidget *player1Widget = m_matches->elementAt(n, 1)->addWidget(make_unique<WContainerWidget>());
        row.player1  = player1Widget->addWidget(make_unique<WAnchor>());
        row.player11 = player1Widget->addWidget(make_unique<WAnchor>());

        row.score = m_matches->elementAt(n, 2)->addWidget(make_unique<WText>());

        WContainerWidget *player2Widget = m_matches->elementAt(n, 3)->addWidget(make_unique<WContainerWidget>());
        row.player2  = player2Widget->addWidget(make_unique<WAnchor>());
        row.player22 = player2Widget->addWidget(make_unique<WAnchor>());

        row.eloCombined = m_matches->elementAt(n, 4)->addWidget(make_unique<WText>());

        m_rows << row;
    }

    while (m_matches->rowCount() - 1 > count) {
        m_matches->removeRow(m_matches->rowCount() - 1);
        m_rows.removeLast();
    }

    m_title->setText(("<b>" + m_player->firstName + " " + m_player->lastName + "</b>").toStdString());

    for (int i = 0; i < count; ++i) {
        const FoosDB::PlayerMatch &m = m_playerMatches[m_playerMatches.size() - 1 - (i + m_page * m_entriesPerPage)];

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

        const auto playerStr = [](const FoosDB::Player *p) {
            return p ? ("<p>" + p->firstName + " " + p->lastName + "</p>").toStdString() : "";
        };

        const auto playerLink = [](const FoosDB::Player *p) {
            return p ? Wt::WLink(LinkType::InternalPath, "/player/" + num2str(p->id)) : Wt::WLink();
        };

        m_rows[i].player1 ->setText(playerStr(m_player));
        m_rows[i].player11->setText(playerStr(m.partner));
        m_rows[i].player2 ->setText(playerStr(m.opponent1));
        m_rows[i].player22->setText(playerStr(m.opponent2));

        m_rows[i].player1 ->setLink(playerLink(m_player));
        m_rows[i].player11->setLink(playerLink(m.partner));
        m_rows[i].player2 ->setLink(playerLink(m.opponent1));
        m_rows[i].player22->setLink(playerLink(m.opponent2));

        m_rows[i].eloCombined->setText(ratingStr(m.eloCombined, m.eloCombinedDiff));
    }
}

