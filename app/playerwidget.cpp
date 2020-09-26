#include "playerwidget.hpp"
#include "util.hpp"

#include <Wt/WVBoxLayout.h>

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
            m_db->getRecentMatches(m_player, m_page * m_entriesPerPage, m_entriesPerPage);

    while (m_matches->rowCount() - 1 < matches.size()) {
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

    while (m_matches->rowCount() - 1 > matches.size()) {
        m_matches->removeRow(m_matches->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < matches.size(); ++i) {
        const FoosDB::PlayerMatch &m = matches[i];

        m_rows[i].date->setText("<p>" + date2str(m.date) + "</p>");
        m_rows[i].competition->setText("<p>" + m.competitionName.toStdString() + "</p>");

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

