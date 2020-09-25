#include "database.hpp"

#include <QDebug>
#include <QSqlError>

namespace FoosDB {

Database::Database(const std::string &dbPath)
    : m_db(QSqlDatabase::addDatabase("QSQLITE"))
{
    m_db.setDatabaseName(QString::fromStdString(dbPath));

    if (!m_db.open()) {
        qWarning() << "Error opening database:" << m_db.lastError();
    }

    readData();

    for (auto v : getPlayersByRanking(EloDomain::Double)) {
        qWarning() << v->eloDouble << v->firstName << v->lastName;
    }

    QVector<PlayerMatch> matches = getRecentMatches(getPlayer(1917));
    for (PlayerMatch m : matches) {
        const auto pstr = [](const Player *p) { return p ? p->firstName + " " + p->lastName : QString(); };

        if (m.matchType == MatchType::Single) {
            qWarning() << m.date << (int) m.competitionType
                       << m.myScore << m.opponentScore
                       << m.eloDiffSeparate << m.eloDiffCombined
                       << "vs" << pstr(m.opponent1) << m.competitionName;
        } else {
            qWarning() << m.date << (int) m.competitionType
                       << m.myScore << m.opponentScore
                       << m.eloDiffSeparate << m.eloDiffCombined
                       << "with" << pstr(m.partner) << "vs" << pstr(m.opponent1) << "+" << pstr(m.opponent2) << m.competitionName;
        }
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
    QSqlQuery playerQuery(
        "SELECT p.id, p.firstName, p.lastName, e.single, e.double, e.combined "
        "FROM players AS p "
        "INNER JOIN elo_current AS e ON p.id = e.player_id");

    while (playerQuery.next()) {
        const int id = playerQuery.value(0).toInt();
        const QString firstName = playerQuery.value(1).toString();
        const QString lastName = playerQuery.value(2).toString();
        const float es = playerQuery.value(3).toFloat();
        const float ed = playerQuery.value(4).toFloat();
        const float ec = playerQuery.value(5).toFloat();
        m_players[id] = Player{id, firstName, lastName, es, ed, ec};
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

QVector<const Player*> Database::getPlayersByRanking(EloDomain domain, int start, int count) const
{
    QVector<const Player*> ret;

    for (auto it = m_players.begin(); it != m_players.end(); ++it)
        ret << &it.value();

    std::sort(ret.begin(), ret.end(), [=](const Player *p1, const Player *p2) {
        switch (domain) {
        case EloDomain::Single: return p1->eloSingle < p2->eloSingle;
        case EloDomain::Double: return p1->eloDouble < p2->eloDouble;
        case EloDomain::Combined: return p1->eloCombined < p2->eloCombined;
        }
    });

    if (start > 0)
        ret = ret.mid(start);

    if (count > 0 && count < ret.size())
        ret.resize(count);

    return ret;
}

QVector<PlayerMatch> Database::getRecentMatches(const Player *player, int start, int count)
{
    QVector<PlayerMatch> ret;

    QString queryString =
        "SELECT pm.match_id, "
        "       m.type, m.score1, m.score2, m.p1, m.p2, m.p11, m.p22, "
        "       c.name, c.date, c.type, "
        "       es.rating, ec.rating "
        "FROM played_matches AS pm "
        "INNER JOIN matches AS m ON pm.match_id = m.id "
        "INNER JOIN competitions AS c ON m.competition_id = c.id "
        "INNER JOIN elo_separate AS es ON pm.id = es.played_match_id "
        "INNER JOIN elo_combined AS ec ON pm.id = ec.played_match_id "
        "WHERE pm.player_id = %1 ORDER BY pm.id";

    QSqlQuery query(queryString.arg(player->id));

    QSqlQuery matchQuery;
    matchQuery.prepare("SELECT * FROM matches WHERE id = ?");

    while (query.next()) {
        const int matchId = query.value(0).toInt();
        const MatchType matchType = (MatchType) query.value(1).toInt();
        int score1 = query.value(2).toInt();
        int score2 = query.value(3).toInt();
        int p1  = query.value(4).toInt();
        int p2  = query.value(5).toInt();
        int p11 = query.value(6).toInt();
        int p22 = query.value(7).toInt();
        const QString competiton = query.value(8).toString();
        const QDateTime date = QDateTime::fromSecsSinceEpoch(query.value(9).toLongLong());
        const CompetitionType compType = (CompetitionType) query.value(10).toInt();
        const float es = query.value(11).toFloat();
        const float ec = query.value(12).toFloat();

        if (p2 == player->id || p22 == player->id) {
            qSwap(p1, p2);
            qSwap(p11, p22);
            qSwap(score1, score2);
        }
        if (p11 == player->id)
            qSwap(p1, p11);

        PlayerMatch match;
        match.date = date;
        match.competitionName = competiton;
        match.competitionType = compType;
        match.matchType = matchType;

        match.partner = (matchType == MatchType::Double) ? &m_players[p11] : nullptr;
        match.opponent1 = &m_players[p2];
        match.opponent2 = (matchType == MatchType::Double) ? &m_players[p22] : nullptr;

        match.myScore = score1;
        match.opponentScore = score2;

        match.eloDiffSeparate = es;
        match.eloDiffCombined = ec;

        ret << match;
    }

    return ret;

}

} // namespace FoosDB
