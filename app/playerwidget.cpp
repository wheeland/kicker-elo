#include "playerwidget.hpp"
#include "util.hpp"

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
    setPlayerId(playerId);

    setContentAlignment(AlignmentFlag::Center);

    m_title = addWidget(make_unique<WText>(""));

    m_matches = addWidget(make_unique<WTable>());

    // setup table headers
    m_matches->insertRow(0)->setHeight("2em");
    m_matches->insertColumn(0)->setWidth("8vw");
    m_matches->insertColumn(1)->setWidth("15vw");
    m_matches->insertColumn(2)->setWidth("15vw");
    m_matches->insertColumn(3)->setWidth("5vw");
    m_matches->insertColumn(4)->setWidth("15vw");
    m_matches->insertColumn(5)->setWidth("5vw");
    m_matches->insertColumn(6)->setWidth("5vw");
    m_matches->insertColumn(7)->setWidth("5vw");

    m_matches->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Date</b>"));
    m_matches->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Competition</b>"));
    m_matches->elementAt(0, 2)->addWidget(make_unique<WText>("<b>Team 1</b>"));
    m_matches->elementAt(0, 3)->addWidget(make_unique<WText>("<b>Score</b>"));
    m_matches->elementAt(0, 4)->addWidget(make_unique<WText>("<b>Team 2</b>"));
    m_matches->elementAt(0, 5)->addWidget(make_unique<WText>("<b>Combined</b>"));
    m_matches->elementAt(0, 6)->addWidget(make_unique<WText>("<b>Single</b>"));
    m_matches->elementAt(0, 7)->addWidget(make_unique<WText>("<b>Double</b>"));

    m_prevButton = addWidget(make_unique<WPushButton>("Prev"));
    m_nextButton = addWidget(make_unique<WPushButton>("Next"));

    m_prevButton->clicked().connect(this, &PlayerWidget::prev);
    m_nextButton->clicked().connect(this, &PlayerWidget::next);

    update();
}

PlayerWidget::~PlayerWidget()
{

}

void PlayerWidget::setPlayerId(int id)
{
    m_playerId = id;
    m_player = m_db->getPlayer(id);
}

void PlayerWidget::prev()
{
    m_page = qMax(m_page - 1, 0);
    update();
}

void PlayerWidget::next()
{
    const int count = m_player ? m_player->matchCount : 0;
    m_page = qMin(m_page + 1, count / m_entriesPerPage);
    update();
}

void PlayerWidget::update()
{
    const QVector<FoosDB::PlayerMatch> matches =
            m_db->getRecentMatches(m_player, m_page * m_entriesPerPage, m_entriesPerPage + 1);

    const int toDisplay = qMin(matches.size(), m_entriesPerPage);

    while (m_matches->rowCount() - 1 < toDisplay) {
        const int n = m_matches->rowCount();

        Row row;
        m_matches->insertRow(n)->setHeight("1.8em");
        row.date = m_matches->elementAt(n, 0)->addWidget(make_unique<WText>());
        row.competition = m_matches->elementAt(n, 1)->addWidget(make_unique<WText>());
        row.score = m_matches->elementAt(n, 3)->addWidget(make_unique<WText>());
        row.eloCombined = m_matches->elementAt(n, 5)->addWidget(make_unique<WText>());
        row.eloSingle = m_matches->elementAt(n, 6)->addWidget(make_unique<WText>());
        row.eloDouble  = m_matches->elementAt(n, 7)->addWidget(make_unique<WText>());
        m_rows << row;
    }

    while (m_matches->rowCount() - 1 > toDisplay) {
        m_matches->removeRow(m_matches->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < toDisplay; ++i) {
        const FoosDB::PlayerMatch &m = matches[i];

        m_rows[i].date->setText(date2str(m.date));
        m_rows[i].competition->setText(m.competitionName.toStdString());
        m_rows[i].score->setText(num2str(m.myScore) + ":" + num2str(m.opponentScore));

        const auto ratingStr = [](int elo, int diff) {
            return num2str(elo) + " (" + num2str(diff) + ")";
        };

        if (m.matchType == FoosDB::MatchType::Single) {
            m_rows[i].eloSingle->setText(ratingStr(m.eloSeparate, m.eloSeparateDiff));
            m_rows[i].eloDouble->setText("");
        } else {
            m_rows[i].eloDouble->setText(ratingStr(m.eloSeparate, m.eloSeparateDiff));
            m_rows[i].eloSingle->setText("");
        }
        m_rows[i].eloCombined->setText(ratingStr(m.eloCombined, m.eloCombinedDiff));
    }
}

