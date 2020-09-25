#include "database.hpp"

#include <QDebug>
#include <QSqlError>

static QString eloDbName(FoosDB::EloDomain domain)
{
    switch (domain) {
    case FoosDB::EloDomain::Single: return "elo_single";
    case FoosDB::EloDomain::Double: return "elo_double";
    case FoosDB::EloDomain::Combined: return "elo_combined";
    }
}

namespace FoosDB {

Database::Database(const std::string &dbPath)
    : m_db(QSqlDatabase::addDatabase("QSQLITE"))
{
    m_db.setDatabaseName(QString::fromStdString(dbPath));

    if (!m_db.open()) {
        qWarning() << "Error opening database:" << m_db.lastError();
    }

    readData();

    QVector<PlayerMatch> matches = getRecentMatches(getPlayer(1917));
    for (PlayerMatch m : matches) {
        const auto pstr = [](const Player *p) { return p ? p->firstName + " " + p->lastName : QString(); };
        qWarning() << m.id << m.myScore << m.opponentScore << pstr(m.partner) << "VS" << pstr(m.opponent1) << pstr(m.opponent2);
    }
}

Database::~Database()
{
}

void Database::execQuery(const QString &query)
{
    QSqlQuery sqlQuery(query);
    if (sqlQuery.lastError().type() != QSqlError::NoError) {
        qWarning() << "SQL Error:" << sqlQuery.lastError();
    }
}

void Database::readData()
{
    QSqlQuery playerQuery("SELECT id, firstName, lastName FROM players");

    while (playerQuery.next()) {
        const int id = playerQuery.value(0).toInt();
        const QString firstName = playerQuery.value(1).toString();
        const QString lastName = playerQuery.value(2).toString();
        m_players[id] = Player{id, firstName, lastName};
    }
}

const Player *Database::getPlayer(int id) const
{
    const auto it = m_players.find(id);
    return (it != m_players.end()) ? &it.value() : nullptr;
}

QVector<const Player*> Database::searchPlayer(const QString &pattern) const
{
    QVector<const Player*> ret;

    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        if (it->firstName.contains(pattern, Qt::CaseInsensitive) || it->lastName.contains(pattern, Qt::CaseInsensitive))
            ret.push_back(&it.value());
    }

    return ret;
}

QVector<QPair<const Player*, float>> Database::getPlayersByRanking(EloDomain domain, int start, int count)
{
    QVector<QPair<const Player*, float>> ret;

    QString queryString = QString(
        "SELECT played_matches.player_id, %1.rating "
        "FROM played_matches "
        "INNER JOIN %1 ON played_matches.id = %1.played_match_id "
        "WHERE played_matches.match_id = 0 "
        "ORDER BY %1.rating DESC "
    ).arg(eloDbName(domain));

    if (start > 0 || count > 0) {
        if (count <= 0) {
            count = m_players.size();
        }
        queryString += QString("LIMIT %1 OFFSET %2").arg(count).arg(start + 1);
    }

    QSqlQuery query(queryString);
    while (query.next()) {
        const int id = query.value(0).toInt();
        const float rating = query.value(1).toFloat();

        const auto it = m_players.find(id);
        if (it != m_players.cend()) {
            ret << qMakePair(&it.value(), rating);
        }
    }

    return ret;
}

QVector<PlayerMatch> Database::getRecentMatches(const Player *player, int start, int count)
{
    QVector<PlayerMatch> ret;

    QString queryString =
        "SELECT played_matches.match_id "
        "FROM played_matches "
        "    INNER JOIN matches ON played_matches.match_id = matches.id "
        "WHERE played_matches.player_id = %1 ORDER BY played_matches.id";

    QSqlQuery query(queryString.arg(player->id));

    QSqlQuery matchQuery;
    matchQuery.prepare("SELECT * FROM matches WHERE id = ?");

    while (query.next()) {
        const int matchId = query.value(0).toInt();
        matchQuery.bindValue(0, matchId);
        matchQuery.exec();
        if (!matchQuery.next()) qFatal("nay");

        const int competitionId = matchQuery.value(1).toInt();
        const MatchType matchType = (MatchType) matchQuery.value(3).toInt();
        int score1 = matchQuery.value(4).toInt();
        int score2 = matchQuery.value(5).toInt();
        int p1  = matchQuery.value(6).toInt();
        int p2  = matchQuery.value(7).toInt();
        int p11 = matchQuery.value(8).toInt();
        int p22 = matchQuery.value(9).toInt();

        if (p2 == player->id || p22 == player->id) {
            qSwap(p1, p2);
            qSwap(p11, p22);
            qSwap(score1, score2);
        }
        if (p11 == player->id)
            qSwap(p1, p11);

        PlayerMatch match;
        match.id = matchId;
        match.myScore = score1;
        match.opponentScore = score2;
        match.matchType = matchType;
        match.partner = (matchType == MatchType::Double) ? &m_players[p11] : nullptr;
        match.opponent1 = &m_players[p2];
        match.opponent2 = (matchType == MatchType::Double) ? &m_players[p22] : nullptr;
        ret << match;
    }

    return ret;

}

} // namespace FoosDB
