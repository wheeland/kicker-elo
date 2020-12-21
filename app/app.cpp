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

    WContainerWidget *rootBg = root()->addWidget(make_unique<WContainerWidget>());
    rootBg->addStyleClass("bg");

    WContainerWidget *contentBg = root()->addWidget(make_unique<WContainerWidget>());
    contentBg->addStyleClass("content_bg");

    WContainerWidget *content = contentBg->addWidget(make_unique<WContainerWidget>());
    content->addStyleClass("content");

    m_stack = content->addWidget(make_unique<WStackedWidget>());
    m_stack->addStyleClass("content_inner");
    m_rankingWidget = m_stack->addWidget(make_unique<RankingWidget>());
    m_playerWidget = m_stack->addWidget(make_unique<PlayerWidget>(1917));

    WPushButton *infoButton = content->addWidget(make_unique<WPushButton>("Information"));
    infoButton->decorationStyle().font().setSize("150%");

    m_bgDimmer = root()->addWidget(make_unique<WContainerWidget>());
    m_bgDimmer->addStyleClass("bg_overlay");
    m_bgDimmer->setZIndex(50);
    m_bgDimmer->hide();

    m_infoPopup = contentBg->addWidget(make_unique<InfoPopup>());
    m_infoPopup->hide();

    m_infoPopup->closeClicked().connect([=]() {
        m_bgDimmer->hide();
        m_infoPopup->hide();
    });

    useStyleSheet("elo-style.css");
    messageResourceBundle().use("elo");

    if (useInternalPaths()) {
        internalPathChanged().connect(this, &EloApp::navigate);
        navigate(internalPath());
    }
    else {
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

void EloApp::navigate(const std::string &path)
{
    if (path.find("/player/") == 0) {
        const int id = atoi(path.data() + 8);
        if (id > 0) {
            showPlayer(id);
            return;
        }
    }
    else {
        showRanking();
    }
}

void EloApp::showRanking()
{
    m_stack->setCurrentWidget(m_rankingWidget);
}

void EloApp::showPlayer(int id)
{
    m_stack->setCurrentWidget(m_playerWidget);
    m_playerWidget->setPlayerId(id);
}
