#include "database.hpp"

#include <QDebug>
#include <QSqlError>
#include <QThread>

static QString genConnName()
{
    static QAtomicInt counter = 0;
    return QString("SqliteConnection%1").arg(counter.fetchAndAddOrdered(1));
}

namespace FoosDB {

Database::Database(const std::string &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", genConnName());
    db.setDatabaseName(QString::fromStdString(dbPath));
    if (!db.open()) {
        qWarning() << "Error opening database:" << db.lastError();
    }

    m_dbs[QThread::currentThread()] = new QSqlDatabase(db);

    readData();
}

Database::~Database()
{
    QWriteLocker lock(&m_dbLock);
    for (auto it = m_dbs.begin(); it != m_dbs.end(); ++it)
        delete it.value();
}

QSqlDatabase *Database::getOrCreateDb()
{
    QThread *thread = QThread::currentThread();

    QReadLocker readLock(&m_dbLock);
    auto it = m_dbs.find(thread);
    if (it == m_dbs.end()) {
        QReadLocker writeLock(&m_dbLock);
        QSqlDatabase newDb = QSqlDatabase::cloneDatabase(*m_dbs.begin().value(), genConnName());
        if (!newDb.open()) {
            qWarning() << "Error opening database:" << newDb.lastError();
        }
        it = m_dbs.insert(thread, new QSqlDatabase(newDb));
    }

    return it.value();
}

void Database::readData()
{
    QSqlDatabase *db = getOrCreateDb();

    QSqlQuery playerQuery(
        "SELECT p.id, p.firstName, p.lastName, e.single, e.double, e.combined "
        "FROM players AS p "
        "INNER JOIN elo_current AS e ON p.id = e.player_id", *db);

    while (playerQuery.next()) {
        const int id = playerQuery.value(0).toInt();
        const QString firstName = playerQuery.value(1).toString();
        const QString lastName = playerQuery.value(2).toString();
        const float es = playerQuery.value(3).toFloat();
        const float ed = playerQuery.value(4).toFloat();
        const float ec = playerQuery.value(5).toFloat();
        m_players[id] = Player{id, firstName, lastName, es, ed, ec, 0};
    }

    QSqlQuery matchCountQuery(
        "SELECT player_id, COUNT(*) "
        "FROM played_matches "
        "GROUP BY player_id", *db);

    while (matchCountQuery.next()) {
        const int id = matchCountQuery.value(0).toInt();
        const int count = matchCountQuery.value(1).toInt();
        m_players[id].matchCount = count;
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
        case EloDomain::Single: return p1->eloSingle > p2->eloSingle;
        case EloDomain::Double: return p1->eloDouble > p2->eloDouble;
        case EloDomain::Combined: return p1->eloCombined > p2->eloCombined;
        }
    });

    if (start > 0)
        ret = ret.mid(start);

    if (count > 0 && count < ret.size())
        ret.resize(count);

    return ret;
}

QVector<PlayerMatch> Database::getPlayerMatches(const Player *player, int start, int count)
{
    const qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    QSqlDatabase *db = getOrCreateDb();
    QVector<PlayerMatch> ret;

    QString queryString =
        "SELECT pm.match_id, "
        "       m.type, m.score1, m.score2, m.p1, m.p2, m.p11, m.p22, "
        "       c.name, c.date, c.type, "
        "       es.rating, es.change, ec.rating, ec.change "
        "FROM played_matches AS pm "
        "INNER JOIN matches AS m ON pm.match_id = m.id "
        "INNER JOIN competitions AS c ON m.competition_id = c.id "
        "INNER JOIN elo_separate AS es ON pm.id = es.played_match_id "
        "INNER JOIN elo_combined AS ec ON pm.id = ec.played_match_id "
        "WHERE pm.player_id = %1 "
        "ORDER BY pm.id ";

    if (start > 0 || count > 0) {
        if (count <= 0)
            count = 1000;
        queryString += QString::asprintf("LIMIT %d OFFSET %d", count, start);
    }

    QSqlQuery query(queryString.arg(player->id), *db);

    const qint64 t1 = QDateTime::currentMSecsSinceEpoch();

    while (query.next()) {
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
        const float es = query.value(11).toInt();
        const float esc = query.value(12).toInt();
        const float ec = query.value(13).toInt();
        const float ecc = query.value(14).toInt();

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

        match.eloSeparate = es;
        match.eloSeparateDiff = esc;
        match.eloCombined = ec;
        match.eloCombinedDiff = ecc;

        ret << match;
    }

    const qint64 t2 = QDateTime::currentMSecsSinceEpoch();
    qWarning() << t2 -t1 << t1- t0;

    return ret;

}

} // namespace FoosDB
