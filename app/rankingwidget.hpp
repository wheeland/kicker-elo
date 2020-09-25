#pragma once

#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WContainerWidget.h>

class RankingWidget : public Wt::WContainerWidget
{
public:
    RankingWidget();

private:
    Wt::WText *m_title;
    Wt::WTable *m_table;
};
