#include "rankingwidget.hpp"

#include <Wt/WHBoxLayout.h>
#include <Wt/WAnchor.h>
#include <Wt/WCssDecorationStyle.h>
#include <QString>

using namespace Wt;
using std::make_unique;

template <class T>
static std::string num2str(T num)
{
    return QString::number(num).toStdString();
}

static std::unique_ptr<WText> createText(const std::string &txt, const std::string &objectName = std::string())
{
    std::unique_ptr<WText> ret = make_unique<WText>(txt);
    ret->setObjectName(objectName);
    return ret;
}

static Wt::WLink createPlayerLink(int id)
{
    return Wt::WLink(LinkType::InternalPath, "/player/" + num2str(id));
}

static std::unique_ptr<WAnchor> createPlayerAnchor(int id, const std::string &name, const std::string &objectName = std::string())
{
    std::unique_ptr<WAnchor> ret = make_unique<WAnchor>(createPlayerLink(id), name);
    ret->setObjectName(objectName);
    return ret;
}

RankingWidget::RankingWidget(FoosDB::Database *db)
    : m_db(db)
{
    setContentAlignment(AlignmentFlag::Center);

    m_title = addWidget(make_unique<WText>("Leaderboard"));
    m_table = addWidget(make_unique<WTable>());
    m_table->setAttributeValue("border", "2");

    // setup table headers
    m_table->insertRow(0)->setHeight("2em");
    m_table->insertColumn(0)->setWidth("5vw");
    m_table->insertColumn(1)->setWidth("15vw");
    m_table->insertColumn(2)->setWidth("5vw");
    m_table->insertColumn(3)->setWidth("5vw");
    m_table->insertColumn(4)->setWidth("5vw");
    m_table->insertColumn(5)->setWidth("5vw");

    m_table->elementAt(0, 0)->addWidget(createText("<b>Rank</b>"));
    m_table->elementAt(0, 1)->addWidget(createText("<b>Name</b>"));
    m_table->elementAt(0, 2)->addWidget(createText("<b>Combined</b>"));
    m_table->elementAt(0, 3)->addWidget(createText("<b>Single</b>"));
    m_table->elementAt(0, 4)->addWidget(createText("<b>Double</b>"));
    m_table->elementAt(0, 5)->addWidget(createText("<b># Games</b>"));

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
        m_table->insertRow(m_table->rowCount())->setHeight("1.8em");

        m_table->elementAt(m_table->rowCount() - 1, 0)->addWidget(createText("", "rankText"));
        m_table->elementAt(m_table->rowCount() - 1, 1)->addWidget(createPlayerAnchor(0, "", "playerAnchor"));
        m_table->elementAt(m_table->rowCount() - 1, 2)->addWidget(createText("", "ecText"));
        m_table->elementAt(m_table->rowCount() - 1, 3)->addWidget(createText("", "esText"));
        m_table->elementAt(m_table->rowCount() - 1, 4)->addWidget(createText("", "edText"));
        m_table->elementAt(m_table->rowCount() - 1, 5)->addWidget(createText("", "mcText"));
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

        ((WText*) m_table->elementAt(i+1, 0)->find("rankText"))->setText(pos);
        ((WAnchor*) m_table->elementAt(i+1, 1)->find("playerAnchor"))->setLink(createPlayerLink(board[i]->id));
        ((WAnchor*) m_table->elementAt(i+1, 1)->find("playerAnchor"))->setText(name);
        ((WText*) m_table->elementAt(i+1, 2)->find("ecText"))->setText(ec);
        ((WText*) m_table->elementAt(i+1, 3)->find("esText"))->setText(es);
        ((WText*) m_table->elementAt(i+1, 4)->find("edText"))->setText(ed);
        ((WText*) m_table->elementAt(i+1, 5)->find("mcText"))->setText(matchCount);
    }
}
