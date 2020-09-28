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

RankingWidget::RankingWidget()
    : m_db(FoosDB::Database::instance())
{
    setContentAlignment(AlignmentFlag::Center);

    m_layout = new WVBoxLayout();
    setLayout(std::unique_ptr<WVBoxLayout>(m_layout));

    //
    // Add title
    //
    WText *titleText = addToLayout<WText>(m_layout);
    titleText->setText("<h1>TFVB ELO Ranking</h1>");
    titleText->setTextAlignment(AlignmentFlag::Center);

    //
    // Add search bar
    //
    WContainerWidget *search = addToLayout<WContainerWidget>(m_layout);
    search->setLayout(make_unique<WHBoxLayout>());
    search->setWidth("80%");
    search->setMargin(WLength::Auto, Side::Left | Side::Right);

    WText *searchText = addToLayout<WText>(search->layout(), "Search: ");
    searchText->setMargin("5%", Side::Left | Side::Right);
    searchText->decorationStyle().font().setSize("150%");

    m_searchBar = addToLayout<WLineEdit>(search->layout());
    m_searchBar->setWidth("60%");
    m_searchBar->decorationStyle().font().setSize("130%");
    m_searchBar->textInput().connect(this, &RankingWidget::updateSearch);

    //
    // Add rankings table
    //
    m_table = addToLayout<WTable>(m_layout);
    m_table->setWidth("100%");
    m_table->setMargin("2%", Side::Top);

    m_table->insertRow(0)->setHeight("2em");
    m_table->insertColumn(0)->setWidth("8%");
    m_table->insertColumn(1)->setWidth("40%");
    m_table->insertColumn(2)->setWidth("13%");
    m_table->insertColumn(3)->setWidth("13%");
    m_table->insertColumn(4)->setWidth("13%");
    m_table->insertColumn(5)->setWidth("13%");

    m_table->toggleStyleClass("table-bordered", true);

    m_table->elementAt(0, 0)->addWidget(make_unique<WText>("<b>Rank</b>"));
    m_table->elementAt(0, 1)->addWidget(make_unique<WText>("<b>Name</b>"));

    m_comboButton = m_table->elementAt(0, 2)->addWidget(make_unique<WPushButton>("Combo"));
    m_singleButton = m_table->elementAt(0, 3)->addWidget(make_unique<WPushButton>("Single"));
    m_doubleButton = m_table->elementAt(0, 4)->addWidget(make_unique<WPushButton>("Double"));
    m_gamesButton = m_table->elementAt(0, 5)->addWidget(make_unique<WPushButton>("Games"));

    m_comboButton->clicked().connect([=]() { m_sortPolicy = Combined; update(); });
    m_singleButton->clicked().connect([=]() { m_sortPolicy = Single; update(); });
    m_doubleButton->clicked().connect([=]() { m_sortPolicy = Double; update(); });
    m_gamesButton->clicked().connect([=]() { m_sortPolicy = Games; update(); });

    //
    // Add next/prev buttons
    //
    WContainerWidget *buttons = addToLayout<WContainerWidget>(m_layout);
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
    const FoosDB::EloDomain domain = (m_sortPolicy == Games) ? FoosDB::EloDomain::Combined
                                                             : (FoosDB::EloDomain) m_sortPolicy;
    QVector<const FoosDB::Player*> players = m_db->getPlayersByRanking(domain);

    if (m_sortPolicy == Games) {
        std::sort(players.begin(), players.end(), [](const FoosDB::Player *p1, const FoosDB::Player *p2) {
            return (p1->matchCount == p2->matchCount) ? (p1->eloCombined > p2->eloCombined)
                                                      : (p1->matchCount > p2->matchCount);
        });
    }

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
        const auto createText = []() {
            auto ret = make_unique<WText>();
            ret->setMargin("1em", Side::Left);
            return ret;
        };
        row.eloCombined = m_table->elementAt(n, 2)->addWidget(createText());
        row.eloSingle = m_table->elementAt(n, 3)->addWidget(createText());
        row.eloDouble  = m_table->elementAt(n, 4)->addWidget(createText());
        row.matchCount = m_table->elementAt(n, 5)->addWidget(createText());
        m_rows << row;

        const WColor bg1(235, 235, 235), bg2(225, 225, 225);
        for (int i = 0; i < 6; ++i) {
            m_table->elementAt(n, i)->setContentAlignment(AlignmentFlag::Middle);
            const bool odd = (n % 2 == 1);
            m_table->elementAt(n, i)->decorationStyle().setBackgroundColor(odd ? bg2 : bg1);
        }
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

    m_comboButton->decorationStyle().font().setWeight((m_sortPolicy == Combined) ? FontWeight::Bold : FontWeight::Normal);
    m_doubleButton->decorationStyle().font().setWeight((m_sortPolicy == Double) ? FontWeight::Bold : FontWeight::Normal);
    m_singleButton->decorationStyle().font().setWeight((m_sortPolicy == Single) ? FontWeight::Bold : FontWeight::Normal);
    m_gamesButton->decorationStyle().font().setWeight((m_sortPolicy == Games) ? FontWeight::Bold : FontWeight::Normal);

    m_prevButton->setEnabled(m_page > 0);
    m_nextButton->setEnabled(m_page * m_entriesPerPage < board.size());
}
