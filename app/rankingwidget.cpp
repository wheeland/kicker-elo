#include "rankingwidget.hpp"

using namespace Wt;
using std::make_unique;

RankingWidget::RankingWidget()
{
    setContentAlignment(AlignmentFlag::Center);

    m_title = addWidget(make_unique<WText>("Hellote"));
    m_table = addWidget(make_unique<WTable>());

    for (int i = 0; i < 10; ++i) {
        m_table->elementAt(i, 0)->addWidget(make_unique<WText>("Hello"));
        m_table->elementAt(i, 1)->addWidget(make_unique<WText>(WString("{1} jo").arg(i)));
        m_table->elementAt(i, 1)->addWidget(make_unique<WText>(WString("{1} ahja").arg(i)));
    }
}
