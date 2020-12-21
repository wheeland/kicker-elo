#include "rankingwidget.hpp"
#include "util.hpp"
#include "global.hpp"

#include <Wt/WHBoxLayout.h>
#include <Wt/WAnchor.h>
#include <Wt/WCssDecorationStyle.h>

using namespace Wt;
using std::make_unique;

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
    CheapProfiler prof("Creating RankingWidget");

    setContentAlignment(AlignmentFlag::Center);

    m_layout = new WVBoxLayout();
    setLayout(std::unique_ptr<WVBoxLayout>(m_layout));

    //
    // Add title
    //
    WText *titleText = addToLayout<WText>(m_layout);
    titleText->setText(tr("ranking_title"));
    titleText->setTextAlignment(AlignmentFlag::Center);

    //
    // Add search bar
    //
    WContainerWidget *search = addToLayout<WContainerWidget>(m_layout);
    search->addStyleClass("player_search");
    search->setLayout(make_unique<WHBoxLayout>());

    WText *searchText = addToLayout<WText>(search->layout(), tr("ranking_search"));
    searchText->addStyleClass("player_search_text");

    m_searchBar = addToLayout<WLineEdit>(search->layout());
    m_searchBar->addStyleClass("player_search_box");
    m_searchBar->textInput().connect(this, &RankingWidget::updateSearch);

    //
    // Add rankings table
    //
    m_table = addToLayout<WTable>(m_layout);
    m_table->addStyleClass("ranking_table");

    m_table->insertRow(0)->setStyleClass("ranking_table_header");
    m_table->insertColumn(0)->setStyleClass("ranking_col_1");
    m_table->insertColumn(1)->setStyleClass("ranking_col_2");
    m_table->insertColumn(2)->setStyleClass("ranking_col_3");
    m_table->insertColumn(3)->setStyleClass("ranking_col_4");
    m_table->insertColumn(4)->setStyleClass("ranking_col_5");
    m_table->insertColumn(5)->setStyleClass("ranking_col_6");

    m_table->elementAt(0, 0)->addWidget(make_unique<WText>(tr("ranking_rank")))->decorationStyle().font().setWeight(FontWeight::Bold);
    m_table->elementAt(0, 1)->addWidget(make_unique<WText>(tr("ranking_name")))->decorationStyle().font().setWeight(FontWeight::Bold);

    m_comboButton = m_table->elementAt(0, 2)->addWidget(make_unique<WPushButton>(tr("combo")));
    m_singleButton = m_table->elementAt(0, 3)->addWidget(make_unique<WPushButton>(tr("single")));
    m_doubleButton = m_table->elementAt(0, 4)->addWidget(make_unique<WPushButton>(tr("double")));
    m_gamesButton = m_table->elementAt(0, 5)->addWidget(make_unique<WPushButton>(tr("games")));

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

void RankingWidget::setDatabase(FoosDB::Database *db)
{
    m_db = db;
    update();
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

WLink RankingWidget::createPlayerLink(int id) const
{
    LinkType linkType = useInternalPaths() ? LinkType::InternalPath : LinkType::Url;
    return Wt::WLink(linkType, deployPrefix() + "/" + m_db->name() + "/player/" + std::to_string(id));
}

void RankingWidget::update()
{
    CheapProfiler prof("Updating RankingWidget");

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
        m_table->insertRow(n);
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

        for (int i = 0; i < 6; ++i) {
            const std::string c = (n % 2 == 1) ? "ranking_table_1" : "ranking_table_2";
            m_table->elementAt(n, i)->addStyleClass(c);
        }
    }

    while (m_table->rowCount() - 1 > count) {
        m_table->removeRow(m_table->rowCount() - 1);
        m_rows.removeLast();
    }

    for (int i = 0; i < count; ++i) {
        const FoosDB::Player *p = board[start + i].first;
        const std::string name = (p->firstName + " " + p->lastName).toStdString();

        m_rows[i].rank->setText(std::to_string(board[start + i].second + 1));
        m_rows[i].player->setLink(createPlayerLink(p->id));
        m_rows[i].player->setText(name);
        m_rows[i].eloCombined->setText(std::to_string((int) p->eloCombined));
        m_rows[i].eloSingle->setText(std::to_string((int) p->eloSingle));
        m_rows[i].eloDouble->setText(std::to_string((int) p->eloDouble));
        m_rows[i].matchCount->setText(std::to_string(p->matchCount));
    }

    m_comboButton->decorationStyle().font().setWeight((m_sortPolicy == Combined) ? FontWeight::Bold : FontWeight::Normal);
    m_doubleButton->decorationStyle().font().setWeight((m_sortPolicy == Double) ? FontWeight::Bold : FontWeight::Normal);
    m_singleButton->decorationStyle().font().setWeight((m_sortPolicy == Single) ? FontWeight::Bold : FontWeight::Normal);
    m_gamesButton->decorationStyle().font().setWeight((m_sortPolicy == Games) ? FontWeight::Bold : FontWeight::Normal);

    m_prevButton->setEnabled(m_page > 0);
    m_nextButton->setEnabled(m_page * m_entriesPerPage < board.size());
}
