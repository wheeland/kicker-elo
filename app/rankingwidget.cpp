#include "rankingwidget.hpp"
#include "util.hpp"

#include <Wt/WHBoxLayout.h>
#include <Wt/WAnchor.h>
#include <Wt/WCssDecorationStyle.h>

using namespace Wt;
using namespace Util;
using std::make_unique;

static Wt::WLink createPlayerLink(int id)
{
    return Wt::WLink(LinkType::InternalPath, "/player/" + num2str(id));
}

RankingWidget::RankingWidget(FoosDB::Database *db)
    : m_db(db)
{
    setContentAlignment(AlignmentFlag::Center);

    m_title = addWidget(make_unique<WText>("Leaderboard"));
    m_table = addWidget(make_unique<WTable>());

    // setup table headers
    m_table->insertRow(0)->setHeight("2em");
    m_table->insertColumn(0)->setWidth("5vw");
    m_table->insertColumn(1)->setWidth("15vw");
    m_table->insertColumn(2)->setWidth("5vw");
    m_table->insertColumn(3)->setWidth("5vw");
    m_table->insertColumn(4)->setWidth("5vw");
    m_table->insertColumn(5)->setWidth("5vw");

    m_table->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Rank</b>"));
    m_table->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Name</b>"));
    m_table->elementAt(0, 2)->addWidget(make_unique<WText>("<b>Combined</b>"));
    m_table->elementAt(0, 3)->addWidget(make_unique<WText>("<b>Single</b>"));
    m_table->elementAt(0, 4)->addWidget(make_unique<WText>("<b>Double</b>"));
    m_table->elementAt(0, 5)->addWidget(make_unique<WText>("<b># Games</b>"));

    m_prevButton = addWidget(make_unique<WPushButton>("Prev"));
    m_nextButton = addWidget(make_unique<WPushButton>("Next"));

    m_prevButton->clicked().connect(this, &RankingWidget::prev);
    m_nextButton->clicked().connect(this, &RankingWidget::next);

    update();
}

RankingWidget::~RankingWidget()
{
}

void RankingWidget::prev()
{
    m_page = qMax(m_page - 1, 0);
    update();
}

void RankingWidget::next()
{
    m_page = qMin(m_page + 1, m_db->getPlayerCount() / m_entriesPerPage);
    update();
}

void RankingWidget::update()
{
    const QVector<const FoosDB::Player*> board =
            m_db->getPlayersByRanking(m_sortDomain, m_page * m_entriesPerPage, m_entriesPerPage + 1);

    const int toDisplay = qMin(board.size(), m_entriesPerPage);

    while (m_table->rowCount() - 1 < toDisplay) {
        const int n = m_table->rowCount();

        Row row;
        m_table->insertRow(n)->setHeight("1.8em");
        row.rank = m_table->elementAt(n, 0)->addWidget(make_unique<WText>());
        row.player = m_table->elementAt(n, 1)->addWidget(make_unique<WAnchor>(createPlayerLink(0)));
        row.eloCombined = m_table->elementAt(n, 2)->addWidget(make_unique<WText>());
        row.eloSingle = m_table->elementAt(n, 3)->addWidget(make_unique<WText>());
        row.eloDouble  = m_table->elementAt(n, 4)->addWidget(make_unique<WText>());
        row.matchCount = m_table->elementAt(n, 5)->addWidget(make_unique<WText>());
        m_rows << row;
    }

    while (m_table->rowCount() - 1 > toDisplay) {
        m_table->removeRow(m_table->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < toDisplay; ++i) {
        const std::string name = (board[i]->firstName + " " + board[i]->lastName).toStdString();

        m_rows[i].rank->setText(num2str(m_page * m_entriesPerPage + 1 + i));
        m_rows[i].player->setLink(createPlayerLink(board[i]->id));
        m_rows[i].player->setText(name);
        m_rows[i].eloCombined->setText(num2str((int) board[i]->eloCombined));
        m_rows[i].eloSingle->setText(num2str((int) board[i]->eloSingle));
        m_rows[i].eloDouble->setText(num2str((int) board[i]->eloDouble));
        m_rows[i].matchCount->setText(num2str(board[i]->matchCount));
    }
}
