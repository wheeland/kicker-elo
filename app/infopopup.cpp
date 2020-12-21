#include "infopopup.hpp"

#include <Wt/WLabel.h>
#include <Wt/WText.h>
#include <Wt/WPushButton.h>

using std::make_unique;
using namespace Wt;

InfoPopup::InfoPopup()
{
    WContainerWidget *content = addWidget(make_unique<WContainerWidget>());
    content->addStyleClass("info_popup_content");

    WContainerWidget *textContainer = content->addWidget(make_unique<WContainerWidget>());
    textContainer->addWidget(make_unique<WText>(tr("info_popup_text")));

    WContainerWidget *okContainer = content->addWidget(make_unique<WContainerWidget>());
    okContainer->addStyleClass("info_popup_footer");

    WPushButton *ok = okContainer->addWidget(make_unique<Wt::WPushButton>(tr("info_popup_button")));
    ok->setDefault(true);
    ok->addStyleClass("info_popup_button");

    ok->clicked().connect([this]() {
        m_closeClicked.emit();
    });

    addStyleClass("info_popup");
    setZIndex(100);
}

InfoPopup::~InfoPopup()
{

}
