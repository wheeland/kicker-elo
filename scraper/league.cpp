#include "league.hpp"
#include "scrapeutil.hpp"

#include <QDebug>

using namespace ScrapeUtil;

QVector<LeagueGame> scrapeLeagueSeason(Database *db, GumboOutput *output)
{
    QVector<LeagueGame> ret;
    QVector<int> doneIds;

    for (GumboElement *elem : collectElements(output->root, GUMBO_TAG_A)) {
        QString href = QString::fromUtf8(attributeValue(elem, "href"));

        if (href.contains("begegnung_spielplan")) {
            const int id = href.split("&id=").last().toInt();

            if (doneIds.contains(id))
                continue;

            if (!href.startsWith("https://tfvb.de"))
                href = QString("https://tfvb.de") + href;

            ret << LeagueGame{href, id};
            doneIds << id;
        }
    }

    return ret;
}

void scrapeLeageGame(Database *db, int tfvbId, GumboOutput *output)
{
    #define CHECK(condition, message) if (!(condition)) { qWarning() << "League game" << tfvbId << ":" << message; continue; }

    QString competitionName;
    QDateTime competitionDateTime;

    //
    // check for a header element that contains the match name
    //
    const auto isPotentialHeader = [](GumboElement *elem) {
        return elem->tag == GUMBO_TAG_TH
                && attributeValue(elem, "class") == "sectiontableheader"
                && attributeValue(elem, "align") == "left";
    };
    for (GumboElement *elem : collectElements(output->root, isPotentialHeader)) {
        const QStringList texts = collectTexts(elem);
        // its all garbled ffs
        if (!texts.isEmpty() && texts.last().contains("vs.")) {
            competitionName = texts.last().mid(2).trimmed().replace("vs.", " vs. ");
        }
    }

    //
    // check for match date
    //
    const auto isPotentialDate = [](GumboElement *elem) {
        return elem->tag == GUMBO_TAG_TABLE && attributeValue(elem, "class") == "contentpaneopen";
    };
    for (GumboElement *elem : collectElements(output->root, isPotentialDate)) {
        const QStringList texts = collectTexts(elem);
        if (texts.size() != 1)
            continue;
        const QStringList parts = texts.first().trimmed().split(", ");
        if (parts.size() < 3)
            continue;

        const QStringList dateTimeParts = parts[1].split(" ");
        if (dateTimeParts.size() != 2)
            continue;

        const QStringList dateParts = dateTimeParts[0].split(".");
        const QStringList timeParts = dateTimeParts[1].split(":");
        if (dateParts.size() != 3 || timeParts.size() != 2)
            continue;

        const int day = dateParts[0].toInt();
        const int mon = dateParts[1].toInt();
        const int year = dateParts[2].toInt();
        const int hour = timeParts[0].toInt();
        const int min = timeParts[1].toInt();

        competitionDateTime = QDateTime(QDate(year, mon, day), QTime(hour, min, 0));
    }
    if (competitionDateTime.isNull()) {
        qWarning() << "Invalid match date";
        return;
    }

    const int competitionId = db->addCompetition(tfvbId, false, competitionName, competitionDateTime);

    //
    // collect root-level match elements
    //
    const auto isMatchElem = [](GumboElement *elem) {
        return (elem->tag == GUMBO_TAG_TR) && attributeValue(elem, "class").startsWith("sectiontableentry");
    };
    const QVector<GumboElement*> matchNodes = collectElements(output->root, isMatchElem);

    //
    // for each match, extract players + score
    //
    for (int pos = 0; pos < matchNodes.size(); ++pos) {
        GumboElement *matchNode = matchNodes[pos];

        const QVector<GumboElement*> tds = collectElements(matchNode, GUMBO_TAG_TD);
        const QVector<GumboElement*> playerLinks = collectElements(matchNode, GUMBO_TAG_A);

        CHECK(tds.size() == 6 || tds.size() == 4, "Wrong number of tds in match element");
        CHECK(playerLinks.size() == 2 || playerLinks.size() == 4, "Invalid player link count");

        const QString scoreStr = getFirstText(tds[tds.size() / 2]);
        const QStringList scores = scoreStr.split(":");
        CHECK(scores.size() == 2, "Invalid score string");
        bool score1ok, score2ok;
        const int score1 = scores[0].toInt(&score1ok);
        const int score2 = scores[1].toInt(&score2ok);
        CHECK(score1ok && score2ok, "Invalid score string");

        bool playersOk = true;
        QVector<int> playerIds;
        QStringList playerFirstNames, playerLastNames;

        for (GumboElement *playerLink : playerLinks) {
            const int id = QString::fromUtf8(attributeValue(playerLink, "href")).split("&id=").last().toInt();
            playerIds << id;
            if (id <= 0)
                playersOk = false;

            const QString playerName = getFirstText(playerLink);
            const QStringList firstLast = playerName.split(", ");
            if (firstLast.size() == 2) {
                playerLastNames << firstLast.first();
                playerFirstNames << firstLast.last();
            } else {
                playersOk = false;
            }
        }

        CHECK(playersOk, "Player information invalid");

        //
        // add the match to the database
        //
        for (int i = 0; i < playerLinks.size(); ++i) {
            db->addPlayer(playerIds[i], playerFirstNames[i], playerLastNames[i]);
        }

        if (playerLinks.size() == 2)
            db->addMatch(competitionId, pos, score1, score2, playerIds[0], playerIds[1]);
        else if (playerLinks.size() == 4)
            db->addMatch(competitionId, pos, score1, score2, playerIds[0], playerIds[1], playerIds[2], playerIds[3]);
    }

    #undef CHECK
}
