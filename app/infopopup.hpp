#pragma once

#include <Wt/WContainerWidget.h>

class InfoPopup : public Wt::WContainerWidget
{
public:
    InfoPopup();
    ~InfoPopup();

    Wt::Signal<>& closeClicked() { return m_closeClicked; }

private:
    Wt::Signal<> m_closeClicked;
};
