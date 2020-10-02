#include "database.hpp"

#include <QDebug>
#include <QSqlError>
#include <QThread>

struct Profiler {
    Profiler(const QString &c) : caption(c), t0(QDateTime::currentMSecsSinceEpoch()) {}
    ~Profiler() { qWarning() << caption << QDateTime::currentMSecsSinceEpoch() - t0; }
    QString caption;
    qint64 t0;
};

static QString genConnName()
{
    static QAtomicInt counter = 0;
    return QString("SqliteConnection%1").arg(counter.fetchAndAddOrdered(1));
}

namespace FoosDB {

Player::EloProgression::EloProgression(const QDateTime &date, int s, int d, int c)
{
    const QDate dt = date.date();
    day = dt.day();
    month = dt.month();
    year = dt.year();
    eloSingle = s;
    eloDouble = d;
    eloCombined = c;
}

static Database *s_instance = nullptr;

void Database::create(const std::string &dbPath)
{
    if (!s_instance) {
        s_instance = new Database(dbPath);
    }
}

void Database::destroy()
{
    delete s_instance;
    s_instance = nullptr;
}

Database *Database::instance()
{
    return s_instance;
}

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

    //
    // Read all player data
    //
    QSqlQuery playerQuery(
        "SELECT p.id, p.firstName, p.lastName, e.single, e.double, e.combined "
        "FROM players AS p "
        "INNER JOIN elo_current AS e ON p.id = e.player_id", *db);

    while (playerQuery.next()) {
        const int id = playerQuery.value(0).toInt();
        const QString firstName = playerQuery.value(1).toString();
        const QString lastName = playerQuery.value(2).toString();
        const int es = playerQuery.value(3).toInt();
        const int ed = playerQuery.value(4).toInt();
        const int ec = playerQuery.value(5).toInt();
        m_players[id] = Player{id, firstName, lastName, es, ed, ec, 0};
    }

    //
    // Read all ratings for all players for all games ever played
    //
//    const QString ratingsQueryString(
//        "SELECT pm.player_id, m.type, c.date, ec.rating, es.rating "
//        "FROM played_matches AS pm "
//        "INNER JOIN matches AS m "
//        "   ON pm.match_id = m.id "
//        "INNER JOIN competitions AS c "
//        "   ON m.competition_id = c.id "
//        "INNER JOIN elo_combined AS ec "
//        "   ON pm.id = ec.played_match_id "
//        "INNER JOIN elo_separate AS es "
//        "   ON pm.id = es.played_match_id"
//    );
//    QSqlQuery ratingsQuery(ratingsQueryString, *db);

//    while (ratingsQuery.next()) {
//        const int playerId = ratingsQuery.value(0).toInt();
//        const MatchType matchType = (MatchType) ratingsQuery.value(1).toInt();
//        const QDateTime date = QDateTime::fromSecsSinceEpoch(ratingsQuery.value(2).toLongLong());
//        const int eloCombined = ratingsQuery.value(3).toInt();
//        const int eloSeparate = ratingsQuery.value(4).toInt();

//        auto it = m_players.find(playerId);
//        if (it != m_players.end()) {
//            if (it->progression.isEmpty()) {
//                it->progression << Player::EloProgression(date, eloSeparate, eloSeparate, eloCombined);
//            }
//            else {
//                int s = it->progression.last().eloSingle;
//                int d = it->progression.last().eloDouble;
//                if (matchType == MatchType::Single)
//                    s = eloSeparate;
//                else
//                    d = eloSeparate;
//                it->progression << Player::EloProgression(date, s, d, eloCombined);
//            }
//        }
//    }

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

int Database::getPlayerMatchCount(const Player *player, EloDomain domain)
{
    QSqlDatabase *db = getOrCreateDb();
    const QString queryString(
        "SELECT m.type, COUNT(m.type) "
        "FROM played_matches AS pm "
        "INNER JOIN matches AS m "
        "   ON pm.match_id = m.id "
        "WHERE pm.player_id = %1 "
        "GROUP BY m.type"
    );

    int counts[2] = {0, 0};

    QSqlQuery query(queryString.arg(player->id), *db);
    while (query.next()) {
        const int domainIdx = query.value(0).toInt();
        const int count = query.value(1).toInt();
        if (domainIdx >= 1 && domainIdx <= 2)
            counts[domainIdx - 1] = count;
    }

    return (domain == EloDomain::Single) ? counts[0] :
           (domain == EloDomain::Double) ? counts[1] :
           (domain == EloDomain::Combined) ? counts[0] + counts[1] : 0;
}

QVector<PlayerVsPlayerStats> Database::getPlayerVsPlayerStats(const Player *player)
{
    Profiler prof("pvp");

    QSqlDatabase *db = getOrCreateDb();
    QVector<PlayerVsPlayerStats> ret;

    const QString queryString(
        "SELECT other_id, "
        "single_wins, single_draws, single_losses, "
        "double_wins, double_draws, double_losses, "
        "partner_wins, partner_draws, partner_losses, "
        "combined_delta, double_delta, single_delta, "
        "partner_combined_delta, partner_double_delta "
        "FROM player_vs_player_stats AS pvp "
        "WHERE pvp.player_id = %1"
    );

    QSqlQuery query(queryString.arg(player->id), *db);
    while (query.next()) {
        const int otherId = query.value(0).toInt();
        const qint16 singleWins = query.value(1).toInt();
        const qint16 singleDraws = query.value(2).toInt();
        const qint16 singleLosses = query.value(3).toInt();
        const qint16 doubleWins = query.value(4).toInt();
        const qint16 doubleDraws = query.value(5).toInt();
        const qint16 doubleLosses = query.value(6).toInt();
        const qint16 partnerWins = query.value(7).toInt();
        const qint16 partnerDraws = query.value(8).toInt();
        const qint16 partnerLosses = query.value(9).toInt();
        const qint16 combinedDelta = query.value(10).toInt();
        const qint16 doubleDelta = query.value(11).toInt();
        const qint16 singleDelta = query.value(12).toInt();
        const qint16 partnerCombinedDelta = query.value(13).toInt();
        const qint16 partnerDoubleDelta = query.value(14).toInt();
        ret << PlayerVsPlayerStats{
            getPlayer(otherId),
            singleWins, singleDraws, singleLosses,
            doubleWins, doubleDraws, doubleLosses,
            partnerWins, partnerDraws, partnerLosses,
            combinedDelta, doubleDelta, singleDelta,
            partnerCombinedDelta, partnerDoubleDelta
        };
    }

    return ret;
}

QVector<Player::EloProgression> Database::getPlayerProgression(const Player *player)
{
    Profiler prof("pvp");
    QVector<Player::EloProgression> ret;
    QSqlDatabase *db = getOrCreateDb();

    if (player) {
        const QString ratingsQueryString(
            "SELECT m.type, c.date, ec.rating, es.rating "
            "FROM played_matches AS pm "
            "INNER JOIN matches AS m "
            "   ON pm.match_id = m.id "
            "INNER JOIN competitions AS c "
            "   ON m.competition_id = c.id "
            "INNER JOIN elo_combined AS ec "
            "   ON pm.id = ec.played_match_id "
            "INNER JOIN elo_separate AS es "
            "   ON pm.id = es.played_match_id "
            "WHERE pm.player_id = %1"
        );
        QSqlQuery ratingsQuery(ratingsQueryString.arg(player->id), *db);

        while (ratingsQuery.next()) {
            const MatchType matchType = (MatchType) ratingsQuery.value(0).toInt();
            const QDateTime date = QDateTime::fromSecsSinceEpoch(ratingsQuery.value(1).toLongLong());
            const int eloCombined = ratingsQuery.value(2).toInt();
            const int eloSeparate = ratingsQuery.value(3).toInt();


            if (ret.isEmpty()) {
                ret << Player::EloProgression(date, eloSeparate, eloSeparate, eloCombined);
            }
            else {
                int s = ret.last().eloSingle;
                int d = ret.last().eloDouble;
                if (matchType == MatchType::Single)
                    s = eloSeparate;
                else
                    d = eloSeparate;
                ret << Player::EloProgression(date, s, d, eloCombined);
            }
        }
    }

    return ret;
}

QVector<PlayerMatch> Database::getPlayerMatches(const Player *player, EloDomain domain, int start, int count)
{
    Profiler prof("matches");

    QSqlDatabase *db = getOrCreateDb();
    QVector<PlayerMatch> ret;

    using PlayedMatch = QPair<int, int>;
    using CombinedSeparateElo = QPair<int, int>;
    QHash<PlayedMatch, CombinedSeparateElo> matchElos;

    const QString whereString = QString("WHERE pm.player_id = %1 ").arg(player->id) +
            ((domain == EloDomain::Single) ? "AND m.type = 1 " :
             (domain == EloDomain::Double) ? "AND m.type = 2 " : "");

    if (count < 0)
        count = 10000;

    const QString orderString = "ORDER BY pm.id DESC ";
    const QString limitString =
            (start > 0 || count > 0)
            ? QString("LIMIT %1 OFFSET %2 ").arg(count).arg(start)
            : QString();


    //
    // Read all ELO start rankings for all participants in all matches that the player has played
    //
    const QString eloQueryString(
        "SELECT pm2.match_id, pm2.player_id, ec.rating, es.rating "
        "FROM played_matches AS pm2 "
        "INNER JOIN elo_combined AS ec "
        "   ON pm2.id = ec.played_match_id "
        "INNER JOIN elo_separate AS es "
        "   ON pm2.id = es.played_match_id "
        "WHERE pm2.match_id IN ( "
        "   SELECT pm.match_id "
        "   FROM played_matches AS pm "
        "   INNER JOIN matches AS m ON pm.match_id = m.id "
        + whereString + orderString + limitString +
        ") "
    );

    QSqlQuery eloQuery(eloQueryString, *db);
    while (eloQuery.next()) {
        const int matchId = eloQuery.value(0).toInt();
        const int playerId = eloQuery.value(1).toInt();
        const int ec = eloQuery.value(2).toInt();
        const int es = eloQuery.value(3).toInt();
        matchElos[qMakePair(matchId, playerId)] = qMakePair(ec, es);
    }

    //
    // Now read all match details
    //

    QString queryString =
        "SELECT pm.match_id, "
        "       m.type, m.score1, m.score2, m.p1, m.p2, m.p11, m.p22, "
        "       c.name, c.date, c.type, "
        "       es.change, ec.change "
        "FROM played_matches AS pm "
        "INNER JOIN matches AS m ON pm.match_id = m.id "
        "INNER JOIN competitions AS c ON m.competition_id = c.id "
        "INNER JOIN elo_separate AS es ON pm.id = es.played_match_id "
        "INNER JOIN elo_combined AS ec ON pm.id = ec.played_match_id "
        + whereString + orderString + limitString;

    QSqlQuery query(queryString, *db);

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
        const int esc = query.value(11).toInt();
        const int ecc = query.value(12).toInt();

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

        match.myself.player = player;
        match.myself.eloCombined = matchElos[qMakePair(matchId, player->id)].first;
        match.myself.eloSeparate = matchElos[qMakePair(matchId, player->id)].second;

        match.opponent1.player = &m_players[p2];
        match.opponent1.eloCombined = matchElos[qMakePair(matchId, p2)].first;
        match.opponent1.eloSeparate = matchElos[qMakePair(matchId, p2)].second;

        if (matchType == MatchType::Double) {
            match.partner.player = &m_players[p11];
            match.partner.eloCombined = matchElos[qMakePair(matchId, p11)].first;
            match.partner.eloSeparate = matchElos[qMakePair(matchId, p11)].second;

            match.opponent2.player = &m_players[p22];
            match.opponent2.eloCombined = matchElos[qMakePair(matchId, p22)].first;
            match.opponent2.eloSeparate = matchElos[qMakePair(matchId, p22)].second;
        }

        match.myScore = score1;
        match.opponentScore = score2;

        match.eloSeparateDiff = esc;
        match.eloCombinedDiff = ecc;

        ret << match;
    }

    return ret;
}

} // namespace FoosDB
