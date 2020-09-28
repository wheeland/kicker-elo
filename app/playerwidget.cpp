#include "playerwidget.hpp"
#include "util.hpp"

#include <Wt/WVBoxLayout.h>
#include <Wt/WDate.h>
#include <Wt/WTime.h>
#include <Wt/WDateTime.h>
#include <Wt/WStandardItem.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WCssDecorationStyle.h>

using namespace Wt;
using namespace Util;
using std::make_unique;

template<typename T, typename... Args>
static inline T *addToLayout(WContainerWidget *widget, Args&&... args)
{
    T *ret = new T(std::forward<Args>(args)...);
    widget->layout()->addWidget(std::unique_ptr<T>(ret));
    return ret;
}

static std::string date2str(const QDateTime &dt)
{
    const QDate date = dt.date();
    return QString::asprintf("%02d.%02d.%d", date.day(), date.month(), date.year()).toStdString();
}

static std::string diff2str(int diff)
{
    return (diff >= 0) ? ("+" + num2str(diff)) : num2str(diff);
}

static std::string player2str(const FoosDB::Player *player)
{
    return player ? (player->firstName + " " + player->lastName).toStdString() : "";
}

static WLink player2href(const FoosDB::Player *p)
{
    return p ? WLink(LinkType::InternalPath, "/player/" + num2str(p->id)) : WLink();
};

void sortPvp(QVector<FoosDB::PlayerVsPlayerStats> &list, qint16 FoosDB::PlayerVsPlayerStats::*member)
{
    std::sort(list.begin(), list.end(), [=](const FoosDB::PlayerVsPlayerStats &a, const FoosDB::PlayerVsPlayerStats &b) {
        return a.*member < b.*member;
    });
}

PlayerWidget::PlayerWidget(int playerId)
    : m_db(FoosDB::Database::instance())
{
    setContentAlignment(AlignmentFlag::Center);

    m_layout = new WVBoxLayout();
    setLayout(std::unique_ptr<WVBoxLayout>(m_layout));

    m_title = addToLayout<WText>(this);
    m_title->setTextAlignment(AlignmentFlag::Center);

    //
    // Setup three ELO headers
    //
    WContainerWidget *eloNumbers = addToLayout<WContainerWidget>(this);
    eloNumbers->setPositionScheme(PositionScheme::Relative);
    eloNumbers->setWidth("100%");
    eloNumbers->setHeight("8em");
    eloNumbers->setMargin(WLength::Auto, Side::Left | Side::Right);
    eloNumbers->setContentAlignment(AlignmentFlag::Center);

    const auto addText = [&](WContainerWidget *container, const std::string &str, FontSize sz) {
        WText *txt = addToLayout<WText>(container, str);
        txt->decorationStyle().font().setSize(sz);
        return txt;
    };

    const auto addEloButton = [&](WContainerWidget *container) {
        WPushButton *btn = addToLayout<WPushButton>(container, "Select");
        btn->setWidth("50%");
        btn->setMargin("5%", Side::Top);
        btn->setMargin(WLength::Auto, Side::Left | Side::Right);
        return btn;
    };

    WContainerWidget *groupCombined = eloNumbers->addWidget(make_unique<WContainerWidget>());
    groupCombined->setPositionScheme(PositionScheme::Absolute);
    groupCombined->setWidth("25%");
    groupCombined->setOffsets("10%", Side::Left);
    groupCombined->setLayout(make_unique<WVBoxLayout>());
    addText(groupCombined, "Combined", FontSize::Large);
    m_eloCombined = addText(groupCombined, "", FontSize::XLarge);
    m_eloCombinedPeak = addText(groupCombined, "", FontSize::Large);
    m_eloCombinedButton = addEloButton(groupCombined);

    WContainerWidget *groupDouble = eloNumbers->addWidget(make_unique<WContainerWidget>());
    groupDouble->setPositionScheme(PositionScheme::Absolute);
    groupDouble->setWidth("25%");
    groupDouble->setOffsets("37.5%", Side::Left);
    groupDouble->setLayout(make_unique<WVBoxLayout>());
    addText(groupDouble, "Double", FontSize::Large);
    m_eloDouble = addText(groupDouble, "", FontSize::XLarge);
    m_eloDoublePeak = addText(groupDouble, "", FontSize::Large);
    m_eloDoubleButton = addEloButton(groupDouble);

    WContainerWidget *groupSingle = eloNumbers->addWidget(make_unique<WContainerWidget>());
    groupSingle->setPositionScheme(PositionScheme::Absolute);
    groupSingle->setWidth("25%");
    groupSingle->setOffsets("65%", Side::Left);
    groupSingle->setLayout(make_unique<WVBoxLayout>());
    addText(groupSingle, "Single", FontSize::Large);
    m_eloSingle = addText(groupSingle, "", FontSize::XLarge);
    m_eloSinglePeak = addText(groupSingle, "", FontSize::Large);
    m_eloSingleButton = addEloButton(groupSingle);

    m_eloCombinedButton->clicked().connect([=]() { setDomain(FoosDB::EloDomain::Combined); });
    m_eloDoubleButton->clicked().connect([=]() { setDomain(FoosDB::EloDomain::Double); });
    m_eloSingleButton->clicked().connect([=]() { setDomain(FoosDB::EloDomain::Single); });

    //
    // setup ELO plot
    //
    m_chartStack = addToLayout<WStackedWidget>(this);
    m_chartStack->resize(760, 400);
    m_chartStack->setMargin(WLength::Auto, Side::Left | Side::Right);
    const auto addChart = [&]() {
        Chart::WCartesianChart *chart = m_chartStack->addWidget(make_unique<Chart::WCartesianChart>());
        chart->setBackground(WColor(220, 220, 220));
        chart->setType(Chart::ChartType::Scatter);
        chart->resize(760, 400);
        return chart;
    };
    m_eloChartCombined = addChart();
    m_eloChartDouble = addChart();
    m_eloChartSingle = addChart();

    //
    // setup opponents table
    //
    m_opponents = addToLayout<WTable>(this);

    //
    // setup matches table
    //
    m_matchesTable = addToLayout<WTable>(this);
//    m_matchesTable->insertRow(0)->setHeight("2em");
    m_matchesTable->insertColumn(0)->setWidth("22%");
    m_matchesTable->insertColumn(1)->setWidth("28%");
    m_matchesTable->insertColumn(2)->setWidth("10%");
    m_matchesTable->insertColumn(3)->setWidth("28%");

//    m_matchesTable->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Competition</b>"));
//    m_matchesTable->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Team 1</b>"));
//    m_matchesTable->elementAt(0, 2)->addWidget(make_unique<WText>("<b>Score</b>"));
//    m_matchesTable->elementAt(0, 3)->addWidget(make_unique<WText>("<b>Team 2</b>"));
//    m_matchesTable->elementAt(0, 4)->addWidget(make_unique<WText>("<b>ELO</b>"));

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

    m_peakSingle = m_peakDouble = m_peakCombined = 0;
    const int es = m_player ? m_player->eloSingle : 0;
    const int ed = m_player ? m_player->eloDouble : 0;
    const int ec = m_player ? m_player->eloCombined : 0;

    if (m_player) {
        m_pvpStats = m_db->getPlayerVsPlayerStats(m_player);
        m_singleCount = m_db->getPlayerMatchCount(m_player, FoosDB::EloDomain::Single);
        m_doubleCount = m_db->getPlayerMatchCount(m_player, FoosDB::EloDomain::Double);
        m_progression = m_db->getPlayerProgression(m_player);

        for (const FoosDB::Player::EloProgression &progression : m_progression) {
            m_peakSingle = qMax(m_peakSingle, (int) progression.eloSingle);
            m_peakDouble = qMax(m_peakDouble, (int) progression.eloDouble);
            m_peakCombined = qMax(m_peakCombined, (int) progression.eloCombined);
        }
    } else {
        m_pvpStats.clear();
        m_singleCount = 0;
        m_doubleCount = 0;
    }

    //
    // Update ELO/title texts
    //
    m_title->setText("<h1>" + player2str(m_player) + "</h1>");
    m_eloCombined->setText("<b>" + num2str(ec) + "</b>");
    m_eloDouble->setText("<b>" + num2str(ed) + "</b>");
    m_eloSingle->setText("<b>" + num2str(es) + "</b>");
    m_eloCombinedPeak->setText("(Peak: <b>" + num2str(m_peakCombined) + "</b>)");
    m_eloDoublePeak->setText("(Peak: <b>" + num2str(m_peakDouble) + "</b>)");
    m_eloSinglePeak->setText("(Peak: <b>" + num2str(m_peakSingle) + "</b>)");

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

void PlayerWidget::setDomain(FoosDB::EloDomain domain)
{
    m_displayedDomain = domain;

    selectChart();
    updateMatchTable();

    m_eloCombinedButton->setEnabled(m_displayedDomain != FoosDB::EloDomain::Combined);
    m_eloDoubleButton->setEnabled(m_displayedDomain != FoosDB::EloDomain::Double);
    m_eloSingleButton->setEnabled(m_displayedDomain != FoosDB::EloDomain::Single);
}

void PlayerWidget::updateChart()
{
    m_eloModel = std::make_shared<Wt::WStandardItemModel>(m_progression.size(), 4);
    m_eloModel->setHeaderData(0, WString("Date"));
    m_eloModel->setHeaderData(1, WString("Combined"));
    m_eloModel->setHeaderData(2, WString("Double"));
    m_eloModel->setHeaderData(3, WString("Single"));

    for (int i = 0; i < m_progression.size(); ++i) {
        const FoosDB::Player::EloProgression pep = m_progression[i];
        const WDate date(pep.year, pep.month, pep.day);
        m_eloModel->setData(i, 0, date);
        m_eloModel->setData(i, 1, (float) pep.eloCombined);
        m_eloModel->setData(i, 2, (float) pep.eloDouble);
        m_eloModel->setData(i, 3, (float) pep.eloSingle);
    }

    m_eloChartCombined->setModel(m_eloModel);
    m_eloChartCombined->setXSeriesColumn(0);
    m_eloChartCombined->axis(Chart::Axis::X).setScale(Chart::AxisScale::Date);
    m_eloChartCombined->addSeries(make_unique<Chart::WDataSeries>(1, Chart::SeriesType::Line));

    m_eloChartDouble->setModel(m_eloModel);
    m_eloChartDouble->setXSeriesColumn(0);
    m_eloChartDouble->axis(Chart::Axis::X).setScale(Chart::AxisScale::Date);
    m_eloChartDouble->addSeries(make_unique<Chart::WDataSeries>(2, Chart::SeriesType::Line));

    m_eloChartSingle->setModel(m_eloModel);
    m_eloChartSingle->setXSeriesColumn(0);
    m_eloChartSingle->axis(Chart::Axis::X).setScale(Chart::AxisScale::Date);
    m_eloChartSingle->addSeries(make_unique<Chart::WDataSeries>(3, Chart::SeriesType::Line));

    selectChart();
}

void PlayerWidget::selectChart()
{
    if (m_displayedDomain == FoosDB::EloDomain::Combined)
        m_chartStack->setCurrentWidget(m_eloChartCombined);
    else if (m_displayedDomain == FoosDB::EloDomain::Double)
        m_chartStack->setCurrentWidget(m_eloChartDouble);
    else
        m_chartStack->setCurrentWidget(m_eloChartSingle);
}

void PlayerWidget::updateOpponents()
{
    m_opponents->clear();

    if (!m_player)
        return;

//    const auto getstr = [](const QVector<FoosDB::PlayerVsPlayerStats> &stats, int idx) {
//        if (qAbs(idx) > stats.size())
//            return std::string();
//        const int realIdx = (idx > 0) ? (idx - 1) : (stats.size() + idx);
//        const FoosDB::PlayerVsPlayerStats ops = stats.value(realIdx);
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

    QVector<FoosDB::PlayerVsPlayerStats> opponents = m_pvpStats;
    QVector<FoosDB::PlayerVsPlayerStats> partners = m_pvpStats;

    sortPvp(opponents, &FoosDB::PlayerVsPlayerStats::combinedDiff);
    sortPvp(partners, &FoosDB::PlayerVsPlayerStats::partnerDoubleDiff);

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
        while (m_matchesTable->rowCount() > 0)
            m_matchesTable->removeRow(m_matchesTable->rowCount() - 1);
        m_rows.clear();
        return;
    }

    const bool isCombined = (m_displayedDomain == FoosDB::EloDomain::Combined);
    const int totalMatchCount =
            (m_displayedDomain == FoosDB::EloDomain::Single) ? m_singleCount :
            (m_displayedDomain == FoosDB::EloDomain::Double) ? m_doubleCount :
                                                               (m_singleCount + m_doubleCount);

    m_page = qMin(m_page, totalMatchCount / m_entriesPerPage);
    const int start = m_page * m_entriesPerPage;
    int count = qMin(totalMatchCount - start, m_entriesPerPage);

    const QVector<FoosDB::PlayerMatch> matches = m_db->getPlayerMatches(m_player, m_displayedDomain, start, count);
    count = qMin(count, matches.size());

    while (m_matchesTable->rowCount() < count) {
        const int n = m_matchesTable->rowCount();

        Row row;
        m_matchesTable->insertRow(n)->setHeight("1.4em");

        const auto addVContainer = [&](int c) {
            WContainerWidget *container = m_matchesTable->elementAt(n, c)->addWidget(make_unique<WContainerWidget>());
            container->setLayout(make_unique<WVBoxLayout>());
            container->setContentAlignment(AlignmentFlag::Center);
            return container;
        };

        WContainerWidget *competition = addVContainer(0);
        row.date = addToLayout<WText>(competition);
        row.date->decorationStyle().font().setSize(FontSize::Small);
        row.date->decorationStyle().font().setStyle(FontStyle::Italic);
        row.competition = addToLayout<WText>(competition);
        row.competition->decorationStyle().font().setSize(FontSize::XSmall);

        WContainerWidget *player1Widget = addVContainer(1);
        WContainerWidget *p1 = addToLayout<WContainerWidget>(player1Widget);
        row.player1 = p1->addWidget(make_unique<WAnchor>());
        row.player1Elo = p1->addWidget(make_unique<WText>());;
        WContainerWidget *p11 = addToLayout<WContainerWidget>(player1Widget);
        row.player11 = p11->addWidget(make_unique<WAnchor>());
        row.player11Elo = p11->addWidget(make_unique<WText>());

        WContainerWidget *result = addVContainer(2);
        row.score = addToLayout<WText>(result);
        row.score->decorationStyle().font().setWeight(FontWeight::Bold);
        row.eloChange = addToLayout<WText>(result);

        WContainerWidget *player2Widget = addVContainer(3);
        WContainerWidget *p2 = addToLayout<WContainerWidget>(player2Widget);
        row.player2 = p2->addWidget(make_unique<WAnchor>());
        row.player2Elo = p2->addWidget(make_unique<WText>());
        WContainerWidget *p22 = addToLayout<WContainerWidget>(player2Widget);
        row.player22 = p22->addWidget(make_unique<WAnchor>());
        row.player22Elo = p22->addWidget(make_unique<WText>());

        m_matchesTable->elementAt(n, 1)->setContentAlignment(AlignmentFlag::Middle);
        m_matchesTable->elementAt(n, 3)->setContentAlignment(AlignmentFlag::Middle);

        const WColor bg(230, 230, 230);
        if (n % 2) {
            m_matchesTable->elementAt(n, 0)->decorationStyle().setBackgroundColor(bg);
            m_matchesTable->elementAt(n, 1)->decorationStyle().setBackgroundColor(bg);
            m_matchesTable->elementAt(n, 2)->decorationStyle().setBackgroundColor(bg);
            m_matchesTable->elementAt(n, 3)->decorationStyle().setBackgroundColor(bg);
        }

        m_rows << row;
    }

    while (m_matchesTable->rowCount() > count) {
        m_matchesTable->removeRow(m_matchesTable->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < count; ++i) {
        const FoosDB::PlayerMatch &m = matches[i];

        m_rows[i].date->setText(date2str(m.date));
        m_rows[i].competition->setText(m.competitionName.toStdString());
        m_rows[i].competition->setTextFormat(TextFormat::UnsafeXHTML);

        if (m.myScore > 1 || m.opponentScore > 1)
            m_rows[i].score->setText(num2str(m.myScore) + ":" + num2str(m.opponentScore));
        else
            m_rows[i].score->setText((m.myScore > 0) ? "Win" : "Loss");
        const int diff = isCombined ? m.eloCombinedDiff : m.eloSeparateDiff;
        m_rows[i].eloChange->setText(diff2str(diff) + " ELO");
        m_rows[i].eloChange->decorationStyle().setForegroundColor((diff >= 0) ? WColor(0, 140, 0) : WColor(200, 0, 0));

        const auto setPlayerDetails = [&](WAnchor *a, WText *t, const FoosDB::PlayerMatch::Participant &p) {
            a->setText(player2str(p.player));
            a->setLink(player2href(p.player));
            t->setText(p.player ? " (" + num2str(isCombined ? p.eloCombined : p.eloSeparate) + ")"
                                : "");
        };

        setPlayerDetails(m_rows[i].player1, m_rows[i].player1Elo, m.myself);
        setPlayerDetails(m_rows[i].player11, m_rows[i].player11Elo, m.partner);
        setPlayerDetails(m_rows[i].player2, m_rows[i].player2Elo, m.opponent1);
        setPlayerDetails(m_rows[i].player22, m_rows[i].player22Elo, m.opponent2);
    }
}
