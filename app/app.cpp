#include "app.hpp"
#include "global.hpp"
#include "util.hpp"

#include <QDebug>

using namespace Wt;
using std::make_unique;

EloApp::EloApp(const WEnvironment& env)
    : WApplication(env)
{
    setTitle(WWidget::tr("page_title"));
    useStyleSheet("elo-style.css");
    messageResourceBundle().use("elo");

    //
    // Create div hierarchy
    //
    WContainerWidget *rootBg = root()->addWidget(make_unique<WContainerWidget>());
    rootBg->addStyleClass("bg");

    WContainerWidget *contentBg = root()->addWidget(make_unique<WContainerWidget>());
    contentBg->addStyleClass("content_bg");

    WContainerWidget *content = contentBg->addWidget(make_unique<WContainerWidget>());
    content->addStyleClass("content");

    m_contentPane = content->addWidget(make_unique<WStackedWidget>());
    m_contentPane->addStyleClass("content_inner");

    m_menuContainer = contentBg->addWidget(make_unique<WContainerWidget>());
    m_menuContainer->addStyleClass("menu_container");
    m_menuContainer->setZIndex(15);

    //
    // Create drop-down menu
    //
    m_menu = m_menuContainer->addNew<Wt::WMenu>();
    m_menu->addStyleClass("menu");
    m_menu->setWidth(150);

    WMenuItem *menuGermany = m_menu->addItem("Deutschland");
    WMenuItem *menuBerlin = m_menu->addItem("Berlin");
    WMenuItem *menuInfo = m_menu->addItem("Info");

    LinkType linkType = useInternalPaths() ? LinkType::InternalPath : LinkType::Url;
    menuGermany->setLink(WLink(linkType, deployPrefix() + "/ger/"));
    menuBerlin->setLink(WLink(linkType, deployPrefix() + "/ber/"));
    menuInfo->setLink(WLink(LinkType::InternalPath, "/info"));

    //
    // Menu button
    //
    m_menuButton = contentBg->addWidget(make_unique<WPushButton>(WWidget::tr("menu_button")));
    m_menuButton->addStyleClass("menu_button");
    m_menuButton->clicked().connect(this, &EloApp::showMenu);
    m_menuButton->decorationStyle().font().setSize("150%");

    //
    // Create content
    //
    m_currentDb = FoosDB::Database::instance("ger");
    m_rankingWidget = m_contentPane->addWidget(make_unique<RankingWidget>(m_currentDb));
    m_playerWidget = m_contentPane->addWidget(make_unique<PlayerWidget>());

    //
    // Dimmer
    //
    m_bgDimmer = root()->addWidget(make_unique<WPushButton>());
    m_bgDimmer->addStyleClass("dimmer");
    m_bgDimmer->setZIndex(10);
    m_bgDimmer->hide();
    // if we click the dimmed area while the menu is open, hide the menu
    m_bgDimmer->clicked().connect([=]() {
        if (!m_infoPopup->isVisible())
            showRanking();
    });

    //
    // Info Popup
    //
    m_infoPopup = contentBg->addWidget(make_unique<InfoPopup>());
    m_infoPopup->setZIndex(20);
    m_infoPopup->hide();
    m_infoPopup->closeClicked().connect(this, &EloApp::showRanking);

    //
    // Navigate to initial page
    //
    if (useInternalPaths()) {
        internalPathChanged().connect(this, &EloApp::navigate);
        navigate(internalPath());
    }
    else {
        internalPathChanged().connect(this, &EloApp::onInternalPathChanged);

        if (!qEnvironmentVariableIsSet(ENV_DEPLOY_PREFIX)) {
            qCritical() << "Not using internal paths, but no" << ENV_DEPLOY_PREFIX << "is set";
            qFatal("Aborting");
        }

        std::string path = env.deploymentPath();
        if (!removePrefix(path, deployPrefix())) {
            qCritical() << "Invalid deploy path encountered:" << path;
        }

        navigate(path);
    }
}

void EloApp::navigate(std::string path)
{
    // this whole navigation business is one big foul pile of excrement,
    // but this is what lighttpd and this whole web fuckery gives us.
    // we abuse the internalPath mechanism to display the info page without reloading
    // the page, and without leaving any trace in the URL bar,
    // by setting internalPath to "/path" and then immediately switching back to ""
    if (m_infoPopup->isVisible()) {
        return;
    }

    if (removePrefix(path, "/ger")) {
        m_currentDb = FoosDB::Database::instance("ger");
        m_rankingWidget->setDatabase(FoosDB::Database::instance("ger"));
    }
    else if (removePrefix(path, "/ber")) {
        m_currentDb = FoosDB::Database::instance("ber");
        m_rankingWidget->setDatabase(FoosDB::Database::instance("ber"));
    }

    if (path == "/info") {
        showInfo();
        setInternalPath("", true);
        return;
    }

    if (removePrefix(path, "/player/")) {
        const int id = atoi(path.data());
        if (id > 0) {
            showPlayer(id);
            return;
        }
    }
    else {
        showRanking();
    }
}

void EloApp::onInternalPathChanged(const std::string &path)
{
    navigate(path);
}

void EloApp::showRanking()
{
    m_contentPane->setCurrentWidget(m_rankingWidget);
    m_menuButton->show();
    m_menuContainer->hide();
    m_bgDimmer->hide();
    m_infoPopup->hide();
}

void EloApp::showPlayer(int id)
{
    m_contentPane->setCurrentWidget(m_playerWidget);
    m_playerWidget->setPlayerId(m_currentDb, id);
    m_menuButton->hide();
    m_menuContainer->hide();
    m_bgDimmer->hide();
    m_infoPopup->hide();
}

void EloApp::showMenu()
{
    m_contentPane->setCurrentWidget(m_rankingWidget);
    m_menuButton->hide();
    m_menuContainer->show();
    m_bgDimmer->show();
    m_infoPopup->hide();
}

void EloApp::showInfo()
{
    m_contentPane->setCurrentWidget(m_rankingWidget);
    m_menuButton->show();
    m_menuContainer->hide();
    m_bgDimmer->show();
    m_infoPopup->show();
}
