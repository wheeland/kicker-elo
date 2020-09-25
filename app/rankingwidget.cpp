#include "rankingwidget.hpp"

#include <Wt/WHBoxLayout.h>
#include <QString>

using namespace Wt;
using std::make_unique;

template <class T>
std::string num2str(T num)
{
    return QString::number(num).toStdString();
}

std::unique_ptr<WText> createText(const std::string &txt, const std::string &name = "text")
{
    std::unique_ptr<WText> ret = make_unique<WText>(txt);
    ret->setObjectName(name);
    return ret;
}

RankingWidget::RankingWidget(FoosDB::Database *db)
    : m_db(db)
{
    setContentAlignment(AlignmentFlag::Center);

    m_title = addWidget(make_unique<WText>("Leaderboard"));
    m_table = addWidget(make_unique<WTable>());

    // setup table headers
    m_table->insertColumn(0)->setWidth("5vw");
    m_table->insertColumn(1)->setWidth("15vw");
    m_table->insertColumn(2)->setWidth("5vw");
    m_table->insertColumn(3)->setWidth("5vw");
    m_table->insertColumn(4)->setWidth("5vw");
    m_table->insertColumn(5)->setWidth("5vw");

    m_table->insertRow(0)->setHeight("2em");

    m_table->elementAt(0, 0)->addWidget(createText("Rank"));
    m_table->elementAt(0, 1)->addWidget(createText("Name"));
    m_table->elementAt(0, 2)->addWidget(createText("Combined"));
    m_table->elementAt(0, 3)->addWidget(createText("Single"));
    m_table->elementAt(0, 4)->addWidget(createText("Double"));
    m_table->elementAt(0, 5)->addWidget(createText("# Games"));

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
        m_table->insertRow(m_table->rowCount())->setHeight("2em");
        m_table->elementAt(m_table->rowCount() - 1, 0)->addWidget(createText(""));
        m_table->elementAt(m_table->rowCount() - 1, 1)->addWidget(createText(""));
        m_table->elementAt(m_table->rowCount() - 1, 2)->addWidget(createText(""));
        m_table->elementAt(m_table->rowCount() - 1, 3)->addWidget(createText(""));
        m_table->elementAt(m_table->rowCount() - 1, 4)->addWidget(createText(""));
        m_table->elementAt(m_table->rowCount() - 1, 5)->addWidget(createText(""));
    }

    while (m_table->rowCount() - 1 > toDisplay) {
        m_table->removeRow(m_table->rowCount() - 1);
    }

    for (int i = 0; i < toDisplay; ++i) {
        const std::string pos = num2str(m_page * m_entriesPerPage + 1 + i);
        const std::string name = (board[i]->firstName + " " + board[i]->lastName).toStdString();
        const std::string es = num2str((int) board[i]->eloSingle);
        const std::string ed = num2str((int) board[i]->eloDouble);
        const std::string ec = num2str((int) board[i]->eloCombined);
        const std::string matchCount = num2str(board[i]->matchCount);

        ((WText*) m_table->elementAt(i+1, 0)->find("text"))->setText(pos);
        ((WText*) m_table->elementAt(i+1, 1)->find("text"))->setText(name);
        ((WText*) m_table->elementAt(i+1, 2)->find("text"))->setText(ec);
        ((WText*) m_table->elementAt(i+1, 3)->find("text"))->setText(es);
        ((WText*) m_table->elementAt(i+1, 4)->find("text"))->setText(ed);
        ((WText*) m_table->elementAt(i+1, 5)->find("text"))->setText(matchCount);
    }
}
