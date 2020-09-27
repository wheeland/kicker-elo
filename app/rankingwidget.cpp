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

template<typename T, typename... Args>
static inline T *addToLayout(WLayout *layout, Args&&... args)
{
    T *ret = new T(std::forward<Args>(args)...);
    layout->addWidget(std::unique_ptr<T>(ret));
    return ret;
}

RankingWidget::RankingWidget(FoosDB::Database *db)
    : m_db(db)
{
    setContentAlignment(AlignmentFlag::Center);

    m_layout = new WVBoxLayout();
    setLayout(std::unique_ptr<WVBoxLayout>(m_layout));

    WContainerWidget *search = addToLayout<WContainerWidget>(m_layout);
    WText *searchText = search->addWidget(make_unique<WText>("Search: "));
    searchText->setMargin(10);
    m_searchBar = search->addWidget(make_unique<WLineEdit>(""));
    m_searchBar->setWidth("30%");
    m_searchBar->textInput().connect(this, &RankingWidget::updateSearch);

    m_table = addToLayout<WTable>(m_layout);

    // setup table headers
    m_table->insertRow(0)->setHeight("2em");
    m_table->insertColumn(0)->setWidth("10%");
    m_table->insertColumn(1)->setWidth("35%");
    m_table->insertColumn(2)->setWidth("15%");
    m_table->insertColumn(3)->setWidth("15%");
    m_table->insertColumn(4)->setWidth("15%");
    m_table->insertColumn(5)->setWidth("10%");

    m_table->toggleStyleClass("table-bordered", true);

    m_table->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Rank</b>"));
    m_table->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Name</b>"));
    m_table->elementAt(0, 2)->addWidget(make_unique<WText>("<b>Combined</b>"));
    m_table->elementAt(0, 3)->addWidget(make_unique<WText>("<b>Single</b>"));
    m_table->elementAt(0, 4)->addWidget(make_unique<WText>("<b>Double</b>"));
    m_table->elementAt(0, 5)->addWidget(make_unique<WText>("<b># Games</b>"));

    WContainerWidget *buttons = addToLayout<WContainerWidget>(m_layout);

    m_prevButton = buttons->addWidget(make_unique<WPushButton>("Prev"));
    m_nextButton = buttons->addWidget(make_unique<WPushButton>("Next"));

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
    m_page++;
    update();
}

void RankingWidget::updateSearch()
{
    update();
}

void RankingWidget::update()
{
    const QVector<const FoosDB::Player*> players = m_db->getPlayersByRanking(m_sortDomain);
    QVector<QPair<const FoosDB::Player*, int>> board;
    for (int i = 0; i < players.size(); ++i)
        board << qMakePair(players[i], i);

    // if we are searching for a player, remove all players that don't match
    if (!m_searchBar->text().empty()) {
        const QString pattern = QString::fromUtf8(m_searchBar->text().toUTF8().data()).trimmed();
        for (auto it = board.begin(); it != board.end(); /*empty*/) {
            if (!it->first->firstName.contains(pattern, Qt::CaseInsensitive)
                    && !it->first->lastName.contains(pattern, Qt::CaseInsensitive))
                it = board.erase(it);
            else
                ++it;
        }
    }

    while (m_page > 0 && m_page * m_entriesPerPage >= board.size())
        --m_page;

    const int start = m_page * m_entriesPerPage;
    const int count = qMin(m_entriesPerPage, board.size() - start);

    while (m_table->rowCount() - 1 < count) {
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

    while (m_table->rowCount() - 1 > count) {
        m_table->removeRow(m_table->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < count; ++i) {
        const FoosDB::Player *p = board[start + i].first;
        const std::string name = (p->firstName + " " + p->lastName).toStdString();

        m_rows[i].rank->setText(num2str(board[start + i].second + 1));
        m_rows[i].player->setLink(createPlayerLink(p->id));
        m_rows[i].player->setText(name);
        m_rows[i].eloCombined->setText(num2str((int) p->eloCombined));
        m_rows[i].eloSingle->setText(num2str((int) p->eloSingle));
        m_rows[i].eloDouble->setText(num2str((int) p->eloDouble));
        m_rows[i].matchCount->setText(num2str(p->matchCount));
    }
}
