#include "playerwidget.hpp"
#include "util.hpp"
#include "global.hpp"

#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WDate.h>
#include <Wt/WTime.h>
#include <Wt/WDateTime.h>
#include <Wt/WStandardItem.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WCssDecorationStyle.h>

using namespace Wt;
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
    char buf[128];
    snprintf(buf, sizeof(buf), "%02d.%02d.%d", date.day(), date.month(), date.year());
    return std::string(buf);
}

static std::string diff2str(int diff)
{
    return (diff >= 0) ? ("+" + std::to_string(diff)) : std::to_string(diff);
}

static std::string player2str(const FoosDB::Player *player)
{
    return player ? (player->firstName + " " + player->lastName).toStdString() : "";
}

static WLink player2href(const FoosDB::Player *p)
{
    LinkType linkType = useInternalPaths() ? LinkType::InternalPath : LinkType::Url;
    return p ? WLink(linkType, deployPrefix() + "/player/" + std::to_string(p->id)) : WLink();
};

PlayerWidget::PlayerWidget()
{
    CheapProfiler prof("Creating PlayerWidget");

    setContentAlignment(AlignmentFlag::Center);

    m_layout = new WVBoxLayout();
    setLayout(std::unique_ptr<WVBoxLayout>(m_layout));

    WContainerWidget *headerGroup = addToLayout<WContainerWidget>(this);
    headerGroup->setWidth(WLength(100, WLength::Unit::Percentage));

    WPushButton *backButton = headerGroup->addWidget(make_unique<WPushButton>("<<"));
    backButton->setFloatSide(Side::Left);
    backButton->setVerticalAlignment(AlignmentFlag::Middle);
    backButton->decorationStyle().font().setSize("150%");
    backButton->setEnabled(true);
    LinkType linkType = useInternalPaths() ? LinkType::InternalPath : LinkType::Url;
    backButton->setLink(WLink(linkType, deployPrefix() + "/"));

    m_title = headerGroup->addWidget(make_unique<WText>());
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

    const auto addText = [&](WContainerWidget *container, const WString &str, FontSize sz) {
        WText *txt = addToLayout<WText>(container, str);
        txt->decorationStyle().font().setSize(sz);
        return txt;
    };

    const auto addEloButton = [&](WContainerWidget *container) {
        WPushButton *btn = addToLayout<WPushButton>(container, tr("player_select"));
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
    addText(groupCombined, tr("combo"), FontSize::Large);
    m_eloCombined = addText(groupCombined, "", FontSize::XLarge);
    m_eloCombinedPeak = addText(groupCombined, "", FontSize::Large);
    m_eloCombinedButton = addEloButton(groupCombined);

    WContainerWidget *groupDouble = eloNumbers->addWidget(make_unique<WContainerWidget>());
    groupDouble->setPositionScheme(PositionScheme::Absolute);
    groupDouble->setWidth("25%");
    groupDouble->setOffsets("37.5%", Side::Left);
    groupDouble->setLayout(make_unique<WVBoxLayout>());
    addText(groupDouble, tr("double"), FontSize::Large);
    m_eloDouble = addText(groupDouble, "", FontSize::XLarge);
    m_eloDoublePeak = addText(groupDouble, "", FontSize::Large);
    m_eloDoubleButton = addEloButton(groupDouble);

    WContainerWidget *groupSingle = eloNumbers->addWidget(make_unique<WContainerWidget>());
    groupSingle->setPositionScheme(PositionScheme::Absolute);
    groupSingle->setWidth("25%");
    groupSingle->setOffsets("65%", Side::Left);
    groupSingle->setLayout(make_unique<WVBoxLayout>());
    addText(groupSingle, tr("single"), FontSize::Large);
    m_eloSingle = addText(groupSingle, "", FontSize::XLarge);
    m_eloSinglePeak = addText(groupSingle, "", FontSize::Large);
    m_eloSingleButton = addEloButton(groupSingle);

    m_eloCombinedButton->clicked().connect([=]() { setDomain(FoosDB::EloDomain::Combined); });
    m_eloDoubleButton->clicked().connect([=]() { setDomain(FoosDB::EloDomain::Double); });
    m_eloSingleButton->clicked().connect([=]() { setDomain(FoosDB::EloDomain::Single); });

    m_eloCombinedButton->setEnabled(m_displayedDomain != FoosDB::EloDomain::Combined);
    m_eloDoubleButton->setEnabled(m_displayedDomain != FoosDB::EloDomain::Double);
    m_eloSingleButton->setEnabled(m_displayedDomain != FoosDB::EloDomain::Single);

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
    WContainerWidget *pvp = addToLayout<WContainerWidget>(this);
    pvp->setLayout(make_unique<WHBoxLayout>());

    const auto addTable = [&](const WString &title, WContainerWidget *group) {
        pvp->layout()->addWidget(std::unique_ptr<WContainerWidget>(group));
        group->setContentAlignment(AlignmentFlag::Center);

        WText *tableTitle = group->addWidget(make_unique<WText>());
        tableTitle->setText(title);
        tableTitle->setTextAlignment(AlignmentFlag::Center);

        WTable *table = group->addWidget(make_unique<WTable>());
        table->insertColumn(0)->setStyleClass("player_opponents_col");
        return table;
    };
    m_partnersWinGroup = new WContainerWidget();
    m_partnersLoseGroup = new WContainerWidget();
    m_opponentsWin  = addTable(tr("player_opponents_loved"), new WContainerWidget());
    m_opponentsLose = addTable(tr("player_opponents_feared"), new WContainerWidget());
    m_partnersWin   = addTable(tr("player_partners_loved"), m_partnersWinGroup);
    m_partnersLose  = addTable(tr("player_partners_feared"), m_partnersLoseGroup);

    //
    // setup matches table
    //
    WText *matchesTitle = addToLayout<WText>(this);
    matchesTitle->setText(tr("player_matches"));
    matchesTitle->setTextAlignment(AlignmentFlag::Center);

    m_matchesTable = addToLayout<WTable>(this);
    m_matchesTable->addStyleClass("player_match_table");
    m_matchesTable->insertColumn(0)->setStyleClass("player_match_col_1");
    m_matchesTable->insertColumn(1)->setStyleClass("player_match_col_2");
    m_matchesTable->insertColumn(2)->setStyleClass("player_match_col_3");
    m_matchesTable->insertColumn(3)->setStyleClass("player_match_col_4");

    //
    // setup buttons
    //
    WContainerWidget *buttons = addToLayout<WContainerWidget>(this);
    buttons->setWidth("30%");
    buttons->setMargin(WLength::Auto, Side::Left | Side::Right);
    buttons->setMargin("2%", Side::Top);

    m_prevButton = buttons->addWidget(make_unique<WPushButton>("<<"));
    m_prevButton->setFloatSide(Side::Left);
    m_prevButton->decorationStyle().font().setSize("150%");
    m_prevButton->setEnabled(m_page > 0);

    m_nextButton = buttons->addWidget(make_unique<WPushButton>(">>"));
    m_nextButton->setFloatSide(Side::Right);
    m_nextButton->decorationStyle().font().setSize("150%");
    m_nextButton->setEnabled(true);

    m_prevButton->clicked().connect(this, &PlayerWidget::prev);
    m_nextButton->clicked().connect(this, &PlayerWidget::next);
}

PlayerWidget::~PlayerWidget()
{
}

void PlayerWidget::setPlayerId(FoosDB::Database *db, int id)
{
    CheapProfiler prof("PlayerWidget::SetPlayerId()");

    m_db = db;
    m_playerId = id;
    m_player = db->getPlayer(id);
    m_page = 0;

    m_peakSingle = m_peakDouble = m_peakCombined = 0;
    const int es = m_player ? m_player->eloSingle : 0;
    const int ed = m_player ? m_player->eloDouble : 0;
    const int ec = m_player ? m_player->eloCombined : 0;

    if (m_player) {
        m_pvpStats = db->getPlayerVsPlayerStats(m_player);
        m_singleCount = db->getPlayerMatchCount(m_player, FoosDB::EloDomain::Single);
        m_doubleCount = db->getPlayerMatchCount(m_player, FoosDB::EloDomain::Double);
        m_progression = db->getPlayerProgression(m_player);

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
    m_eloCombined->setText("<b>" + std::to_string(ec) + "</b>");
    m_eloDouble->setText("<b>" + std::to_string(ed) + "</b>");
    m_eloSingle->setText("<b>" + std::to_string(es) + "</b>");
    m_eloCombinedPeak->setText(tr("player_peak").arg(std::to_string(m_peakCombined)));
    m_eloDoublePeak->setText(tr("player_peak").arg(std::to_string(m_peakDouble)));
    m_eloSinglePeak->setText(tr("player_peak").arg(std::to_string(m_peakSingle)));

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
    updateOpponents();
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
    m_opponentsWin->clear();
    m_opponentsLose->clear();
    m_partnersWin->clear();
    m_partnersLose->clear();

    if (!m_player)
        return;

    using ResultFunction = FoosDB::PlayerVsPlayerStats::Results (FoosDB::PlayerVsPlayerStats::*)() const;

    const auto sortPvp = [](QVector<FoosDB::PlayerVsPlayerStats> &list, ResultFunction result) {
        std::sort(list.begin(), list.end(), [=](const FoosDB::PlayerVsPlayerStats &a, const FoosDB::PlayerVsPlayerStats &b) {
            return (a.*result)().delta > (b.*result)().delta;
        });
    };

    const bool isSingle = (m_displayedDomain == FoosDB::EloDomain::Single);
    QVector<FoosDB::PlayerVsPlayerStats> opponents = m_pvpStats;
    QVector<FoosDB::PlayerVsPlayerStats> partners = m_pvpStats;
    ResultFunction opponentResults;
    ResultFunction partnerResults;

    if (m_displayedDomain == FoosDB::EloDomain::Single) {
        opponentResults = &FoosDB::PlayerVsPlayerStats::singleResults;
        partnerResults = &FoosDB::PlayerVsPlayerStats::singleResults;
    }
    else if (m_displayedDomain == FoosDB::EloDomain::Double) {
        opponentResults = &FoosDB::PlayerVsPlayerStats::doubleResults;
        partnerResults = &FoosDB::PlayerVsPlayerStats::partnerDoubleResults;
    }
    else {
        opponentResults = &FoosDB::PlayerVsPlayerStats::combinedResults;
        partnerResults = &FoosDB::PlayerVsPlayerStats::partnerCombinedResults;
    }

    sortPvp(opponents, opponentResults);
    if (!isSingle)
        sortPvp(partners, partnerResults);

    m_partnersWinGroup->setHidden(isSingle);
    m_partnersLoseGroup->setHidden(isSingle);

    for (int i = 0 ; i < 4; ++i) {
        const std::string cssClass = (i % 2 == 0) ? "player_pvp_1" : "player_pvp_2";

        const auto addToRow = [&](
                WTable *table1,
                WTable *table2,
                const QVector<FoosDB::PlayerVsPlayerStats> &stats,
                int idx,
                ResultFunction results) {
            if (idx >= stats.size())
                return;

            const FoosDB::PlayerVsPlayerStats pvp1 = stats.value(idx);
            const FoosDB::PlayerVsPlayerStats pvp2 = stats.value(stats.size() - idx - 1);

            const FoosDB::PlayerVsPlayerStats::Results res1 = (pvp1.*results)();
            const FoosDB::PlayerVsPlayerStats::Results res2 = (pvp2.*results)();

            const auto resultStr = [&](const FoosDB::PlayerVsPlayerStats::Results &res) {
                return "  (" + std::to_string(res.wins) + " : " + std::to_string(res.draws) + " : " + std::to_string(res.losses) + ")";
            };

            if (res1.delta > 0) {
                table1->insertRow(table1->rowCount())->setHeight("1.4em");;
                const int n1 = table1->rowCount() - 1;
                WContainerWidget *container = table1->elementAt(n1, 0)->addWidget(make_unique<WContainerWidget>());
                container->setStyleClass(cssClass);
                container->setLayout(make_unique<WVBoxLayout>());
                container->setContentAlignment(AlignmentFlag::Center);
                WContainerWidget *result = addToLayout<WContainerWidget>(container);
                result->addWidget(make_unique<WText>(diff2str(res1.delta)))->setStyleClass("player_elo_plus");
                result->addWidget(make_unique<WText>(resultStr(res1)))->setStyleClass("player_pvp_stats");
                addToLayout<WAnchor>(container, player2href(pvp1.player), player2str(pvp1.player));
            }

            if (res2.delta < 0) {
                table2->insertRow(table2->rowCount())->setHeight("1.4em");;
                const int n2 = table2->rowCount() - 1;
                WContainerWidget *container = table2->elementAt(n2, 1)->addWidget(make_unique<WContainerWidget>());
                container->setStyleClass(cssClass);
                container->setLayout(make_unique<WVBoxLayout>());
                container->setContentAlignment(AlignmentFlag::Center);
                WContainerWidget *result = addToLayout<WContainerWidget>(container);
                result->addWidget(make_unique<WText>(diff2str(res2.delta)))->setStyleClass("player_elo_minus");
                result->addWidget(make_unique<WText>(resultStr(res2)))->setStyleClass("player_pvp_stats");
                addToLayout<WAnchor>(container, player2href(pvp2.player), player2str(pvp2.player));
            }
        };

        addToRow(m_opponentsWin, m_opponentsLose, opponents, i, opponentResults);

        if (!isSingle)
            addToRow(m_partnersWin, m_partnersLose, partners, i, partnerResults);
    }
}

void PlayerWidget::updateMatchTable()
{
    CheapProfiler prof("PlayerWidget::UpdateMatchTable()");

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

    m_page = qMin(m_page, totalMatchCount / m_matchesPerPage);
    const int start = m_page * m_matchesPerPage;
    int count = qMin(totalMatchCount - start, m_matchesPerPage);

    const QVector<FoosDB::PlayerMatch> matches = m_db->getPlayerMatches(m_player, m_displayedDomain, start, count);
    count = qMin(count, matches.size());

    while (m_matchesTable->rowCount() < count) {
        const int n = m_matchesTable->rowCount();

        Row row;

        const auto addVContainer = [&](int c) {
            WContainerWidget *container = m_matchesTable->elementAt(n, c)->addWidget(make_unique<WContainerWidget>());
            container->setLayout(make_unique<WVBoxLayout>());
            container->setContentAlignment(AlignmentFlag::Center);
            return container;
        };

        WContainerWidget *competition = m_matchesTable->elementAt(n, 0)->addWidget(make_unique<WContainerWidget>());
        competition->setPadding("0px");

        row.date = competition->addWidget(make_unique<WText>());
        row.date->setStyleClass("player_match_date");
        row.competition = competition->addWidget(make_unique<WText>());
        row.competition->setStyleClass("player_match_competition");

        WContainerWidget *player1Widget = addVContainer(1);
        WContainerWidget *p1 = addToLayout<WContainerWidget>(player1Widget);
        row.player1 = p1->addWidget(make_unique<WAnchor>());
        row.player1Elo = p1->addWidget(make_unique<WText>());;
        WContainerWidget *p11 = addToLayout<WContainerWidget>(player1Widget);
        row.player11 = p11->addWidget(make_unique<WAnchor>());
        row.player11Elo = p11->addWidget(make_unique<WText>());

        WContainerWidget *result = addVContainer(2);
        row.score = addToLayout<WText>(result);
//        row.score->decorationStyle().font().setWeight(FontWeight::Bold);
        row.eloChange = addToLayout<WText>(result);

        WContainerWidget *player2Widget = addVContainer(3);
        WContainerWidget *p2 = addToLayout<WContainerWidget>(player2Widget);
        row.player2 = p2->addWidget(make_unique<WAnchor>());
        row.player2Elo = p2->addWidget(make_unique<WText>());
        WContainerWidget *p22 = addToLayout<WContainerWidget>(player2Widget);
        row.player22 = p22->addWidget(make_unique<WAnchor>());
        row.player22Elo = p22->addWidget(make_unique<WText>());

        const std::string c = (n % 2 == 1) ? "player_match_table_1" : "player_match_table_2";
        for (int i = 0; i < 4; ++i)
            m_matchesTable->elementAt(n, i)->addStyleClass(c);

        m_rows << row;
    }

    while (m_matchesTable->rowCount() > count) {
        m_matchesTable->removeRow(m_matchesTable->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < count; ++i) {
        const FoosDB::PlayerMatch &m = matches[i];

        m_rows[i].date->setText(date2str(m.date) + "<br/>");
        m_rows[i].competition->setText(m.competitionName.toStdString());
        m_rows[i].competition->setTextFormat(TextFormat::UnsafeXHTML);

        if (m.myScore + m.opponentScore > 1)
            m_rows[i].score->setText(std::to_string(m.myScore) + ":" + std::to_string(m.opponentScore));
        else
            m_rows[i].score->setText((m.myScore > 0) ? tr("win") : tr("loss"));
        const int diff = isCombined ? m.eloCombinedDiff : m.eloSeparateDiff;
        m_rows[i].eloChange->setText(diff2str(diff));
        m_rows[i].eloChange->setStyleClass((diff >= 0) ? "player_elo_plus" : "player_elo_minus");

        const auto setPlayerDetails = [&](WAnchor *a, WText *t, const FoosDB::PlayerMatch::Participant &p) {
            a->setText(player2str(p.player));
            a->setLink(player2href(p.player));
            t->setText(p.player ? " (" + std::to_string(isCombined ? p.eloCombined : p.eloSeparate) + ")"
                                : "");
        };

        setPlayerDetails(m_rows[i].player1, m_rows[i].player1Elo, m.myself);
        setPlayerDetails(m_rows[i].player11, m_rows[i].player11Elo, m.partner);
        setPlayerDetails(m_rows[i].player2, m_rows[i].player2Elo, m.opponent1);
        setPlayerDetails(m_rows[i].player22, m_rows[i].player22Elo, m.opponent2);
    }

    m_prevButton->setEnabled(m_page > 0);
    m_nextButton->setEnabled((m_page + 1) * m_matchesPerPage < totalMatchCount);
}
