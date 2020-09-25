#include "tournament.hpp"
#include "scrapeutil.hpp"

using namespace ScrapeUtil;

QStringList scrapeTournamentOverview(GumboOutput *output)
{
    QStringList ret;

    for (GumboElement *elem : collectElements(output->root, GUMBO_TAG_A)) {
        QString href = QString::fromUtf8(attributeValue(elem, "href"));

        if (href.contains("task=turnierdisziplinen&turnierid=")) {
            if (!href.startsWith("https://tfvb.de"))
                href = QString("https://tfvb.de") + href;

            ret << href;
        }
    }

    return ret;
}

QVector<Tournament> scrapeTournamentPage(GumboOutput *output)
{
    QVector<Tournament> ret;

    for (GumboElement *elem : collectElements(output->root, GUMBO_TAG_A)) {
        QString href = QString::fromUtf8(attributeValue(elem, "href"));

        if (href.contains("task=turnierdisziplin&id=")) {
            const int id = href.split("&id=").last().toInt();

            if (!href.startsWith("https://tfvb.de"))
                href = QString("https://tfvb.de") + href;

            ret << Tournament{href, id};
        }
    }

    return ret;
}

void scrapeTournament(Database *db, int tfvbId, GumboOutput *output)
{
    #define REQUIRE(condition, message) if (!(condition)) { qWarning() << "Tournament" << tfvbId << ":" << message; return; }
    #define CHECK(condition, message) if (!(condition)) { qWarning() << "Tournament" << tfvbId << ":" << message; continue; }

    //
    // check for the header element that contains the tournament details
    //
    GumboElement *rightSidebar = getFirstElement(output->root, [](GumboElement *elem) {
        return elem->tag == GUMBO_TAG_DIV && attributeValue(elem, "id") == "right_sidebar";
    });
    REQUIRE(rightSidebar, "Didn't find root div elem");

    GumboElement *nameElem = getFirstElement(rightSidebar, [](GumboElement *elem) {
        return elem->tag == GUMBO_TAG_TABLE && attributeValue(elem, "class") == "contentpaneopen";
    });
    REQUIRE(nameElem, "Didn't find name elem");
    const QStringList headerTexts = collectTexts(nameElem);
    REQUIRE(headerTexts.size() >= 2, "name elem doesn't have enough data");

    //
    // Get Tournament date + name
    //
    const QStringList parts = headerTexts.first().trimmed().split(", ");
    REQUIRE(parts.size() > 2, "Date text invalid");
    const QStringList dateTimeParts = parts[parts.size() - 2].split(" ");
    REQUIRE(dateTimeParts.size() >= 2, "Date text invalid");
    const QStringList dateParts = dateTimeParts[0].split(".");
    const QStringList timeParts = dateTimeParts[1].split(":");
    REQUIRE(dateParts.size() == 3 && timeParts.size() == 2, "Date text invalid");

    const int day = dateParts[0].toInt();
    const int mon = dateParts[1].toInt();
    const int year = dateParts[2].toInt();
    const int hour = timeParts[0].toInt();
    const int min = timeParts[1].toInt();

    const QDateTime competitionDateTime = QDateTime(QDate(year, mon, day), QTime(hour, min, 0));
    const QString competitionName = headerTexts[1];

    //
    // Parse tournament participants
    //
    QHash<QString, int> playerNameToId;
    const auto linksToPlayerPage = [](GumboElement *elem) {
        return elem->tag == GUMBO_TAG_A && attributeValue(elem, "href").contains("task=spieler_details");
    };
    for (GumboElement *playerPageLink : collectElements(rightSidebar, linksToPlayerPage)) {
        QString href(attributeValue(playerPageLink, "href"));
        const int id = href.split("&id=").last().toInt();
        const QString name = getFirstText(playerPageLink);
        playerNameToId[name] = id;
    }

    const int competitionId = db->addCompetition(tfvbId, CompetitionType::Tournament, competitionName, competitionDateTime);

    //
    // Parse match results
    //
    const auto isPotentialMatchResult = [](GumboElement *elem) {
        return elem->tag == GUMBO_TAG_TR && attributeValue(elem, "class").startsWith("sectiontableentry");
    };
    const QVector<GumboElement*> potentialMatches = collectElements(rightSidebar, isPotentialMatchResult);
    int pos = 0;
    for (int i = potentialMatches.size() - 1; i >= 0; --i) {
        GumboElement *elem = potentialMatches[i];
        const QVector<GumboElement*> tbodies = collectElements(elem, GUMBO_TAG_TBODY);
        if (tbodies.size() != 2)
            continue;

        QStringList names = collectTexts(tbodies[0]);
        QStringList names2 = collectTexts(tbodies[1]);

        if (names.size() != names2.size())
            continue;

        bool allFound = true;
        QVector<int> ids;
        names << names2;
        for (QString &name : names) {
            name = name.trimmed();
            ids << playerNameToId.value(name);
            if (ids.last() <= 0)
                allFound = false;
        }

        if (!allFound)
            continue;

        if (ids.size() == 2)
            db->addMatch(competitionId, pos++, 1, 0, ids[0], ids[1]);
        else if (ids.size() == 4)
            db->addMatch(competitionId, pos++, 1, 0, ids[0], ids[1], ids[2], ids[3]);
    }

    REQUIRE(pos > 0, "Didn't add any games from tournament");
}
