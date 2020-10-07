#include "database.hpp"
#include "rating.hpp"

#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

const float kTournament = 30.f;
const float kLeague = 20.f;

Database::Database(const QString &sqlitePath)
    : m_db(QSqlDatabase::addDatabase("QSQLITE"))
{
    m_db.setDatabaseName(sqlitePath);

    if (!m_db.open()) {
        qWarning() << "Error opening database:" << m_db.lastError();
    }

    createTables();
    createQueries();
    readData();
}

Database::~Database()
{

}

void Database::execQuery(const QString &query)
{
    QSqlQuery sqlQuery(query);
    checkQueryStatus(sqlQuery);
}

void Database::createTables()
{
    execQuery("CREATE TABLE IF NOT EXISTS competitions ( \
        id integer NOT NULL, \
        tfvbId integer NOT NULL, \
        type integer, \
        name text, \
        date integer, \
        primary key (id))"
    );
    execQuery("CREATE TABLE IF NOT EXISTS played_matches ( \
        id integer NOT NULL, \
        player_id integer NOT NULL, \
        match_id integer, \
        primary key (id), \
        constraint fk_played_matches_player foreign key (player_id) references players (id) deferrable initially deferred, \
        constraint fk_played_matches_match foreign key (match_id) references matches (id) deferrable initially deferred)"
    );
    execQuery("CREATE TABLE IF NOT EXISTS matches ( \
        id integer NOT NULL, \
        competition_id integer NOT NULL, \
        position integer, \
        type integer, \
        score1 integer, \
        score2 integer, \
        p1 integer NOT NULL, \
        p2 integer NOT NULL, \
        p11 integer NOT NULL, \
        p22 integer NOT NULL, \
        primary key (id), \
        constraint fk_matches_competition foreign key (competition_id) references competitions (id) deferrable initially deferred)"
    );
    execQuery("CREATE TABLE IF NOT EXISTS players ( \
        id integer NOT NULL, \
        firstName text NOT NULL, \
        lastName text NOT NULL, \
        primary key (id))"
    );
    execQuery("CREATE TABLE IF NOT EXISTS elo_separate ( \
        played_match_id integer NOT NULL, \
        rating smallint NOT NULL, \
        change smallint NOT NULL, \
        primary key (played_match_id))"
    );
    execQuery("CREATE TABLE IF NOT EXISTS elo_combined ( \
        played_match_id integer NOT NULL, \
        rating smallint NOT NULL, \
        change smallint NOT NULL, \
        primary key (played_match_id))"
    );
    execQuery("CREATE TABLE IF NOT EXISTS elo_current ( \
        player_id integer NOT NULL, \
        single smallint NOT NULL, \
        double smallint NOT NULL, \
        combined smallint NOT NULL, \
        primary key (player_id))"
    );

    execQuery("CREATE TABLE IF NOT EXISTS player_vs_player_stats ( \
        player_id integer NOT NULL, \
        other_id integer NOT NULL, \
        single_wins smallint NOT NULL, \
        single_draws smallint NOT NULL, \
        single_losses smallint NOT NULL, \
        double_wins smallint NOT NULL, \
        double_draws smallint NOT NULL, \
        double_losses smallint NOT NULL, \
        partner_wins smallint NOT NULL, \
        partner_draws smallint NOT NULL, \
        partner_losses smallint NOT NULL, \
        combined_delta smallint NOT NULL, \
        double_delta smallint NOT NULL, \
        single_delta smallint NOT NULL, \
        partner_combined_delta smallint NOT NULL, \
        partner_double_delta smallint NOT NULL)");
}

void Database::checkQueryStatus(const QSqlQuery &query) const
{
    if (query.lastError().type() != QSqlError::NoError) {
        qWarning() << "SQL Error:" << query.lastError();
    }
}

void Database::createQueries()
{
    if (!m_insertPlayerQuery.prepare("INSERT INTO players (id, firstName, lastName) VALUES (?, ?, ?)")) {
        qWarning() << "Failed to prepare query";
    }
    if (!m_insertCompetitionQuery.prepare("INSERT INTO competitions (id, tfvbId, type, name, date) VALUES (?, ?, ?, ?, ?)")) {
        qWarning() << "Failed to prepare query";
    }
    if (!m_insertMatchQuery.prepare("INSERT INTO matches (id, competition_id, position, type, score1, score2, p1, p2, p11, p22) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
        qWarning() << "Failed to prepare query";
    }
}

void Database::readData()
{
    QSqlQuery playerQuery("SELECT id, firstName, lastName FROM players");
    checkQueryStatus(playerQuery);
    while (playerQuery.next()) {
        const int id = playerQuery.value(0).toInt();
        const QString firstName = playerQuery.value(1).toString();
        const QString lastName = playerQuery.value(2).toString();
        m_players[id] = Player{id, firstName, lastName};
    }

    QSqlQuery competitionQuery("SELECT id, tfvbId, type, name, date FROM competitions");
    checkQueryStatus(competitionQuery);
    while (competitionQuery.next()) {
        const int id = competitionQuery.value(0).toInt();
        const int tfvbId = competitionQuery.value(1).toInt();
        const int type = competitionQuery.value(2).toInt();
        const QString name = competitionQuery.value(3).toString();
        const QDateTime dt = QDateTime::fromSecsSinceEpoch(competitionQuery.value(4).toLongLong());
        m_competitions[id] = Competition{id, tfvbId, (CompetitionType) type, name, dt};
    }

    QSqlQuery matchQuery("SELECT id, competition_id, position, type, score1, score2, p1, p2, p11, p22 FROM matches");
    checkQueryStatus(matchQuery);
    while (matchQuery.next()) {
        const int id = matchQuery.value(0).toInt();
        const int competition = matchQuery.value(1).toInt();
        const int position = matchQuery.value(2).toInt();
        const int type = matchQuery.value(3).toInt();
        const int score1 = matchQuery.value(4).toInt();
        const int score2 = matchQuery.value(5).toInt();
        const int p1 = matchQuery.value(6).toInt();
        const int p2 = matchQuery.value(7).toInt();
        const int p11 = matchQuery.value(8).toInt();
        const int p22 = matchQuery.value(9).toInt();
        m_matches[id] = Match{id, competition, position, (MatchType) type, score1, score2, p1, p2, p11, p22};
        m_nextMatchId = qMax(m_nextMatchId, id + 1);
    }
}

int Database::addCompetition(int tfvbId, CompetitionType type, const QString &name, QDateTime dt)
{
    int lastId = 0;

    for (auto it = m_competitions.begin(); it != m_competitions.end(); ++it) {
        if (it->tfvbId == tfvbId && it->type == type)
            return it->id;
        lastId = qMax(lastId, it->id);
    }

    m_insertCompetitionQuery.bindValue(0, lastId + 1);
    m_insertCompetitionQuery.bindValue(1, tfvbId);
    m_insertCompetitionQuery.bindValue(2, (int) type);
    m_insertCompetitionQuery.bindValue(3, name);
    m_insertCompetitionQuery.bindValue(4, dt.toSecsSinceEpoch());
    m_insertCompetitionQuery.exec();
    checkQueryStatus(m_insertCompetitionQuery);

    m_competitions[lastId + 1] = Competition{lastId + 1, tfvbId, type, name, dt};

    return lastId + 1;
}

int Database::competitionGameCount(int tfvbId, CompetitionType type)
{
    int id = -1;
    for (auto it = m_competitions.begin(); it != m_competitions.end(); ++it) {
        if (it->tfvbId == tfvbId && it->type == type) {
            id = it->id;
            break;
        }
    }

    if (id < 0)
        return 0;

    int count = 0;
    for (auto it = m_matches.cbegin(), end = m_matches.cend(); it != end; ++it) {
        if (it->competition == id)
            ++count;
    }

    return count;
}

void Database::addPlayer(int id, const QString &firstName, const QString &lastName)
{
    if (m_players.contains(id))
        return;

    m_insertPlayerQuery.bindValue(0, id);
    m_insertPlayerQuery.bindValue(1, firstName);
    m_insertPlayerQuery.bindValue(2, lastName);
    m_insertPlayerQuery.exec();
    checkQueryStatus(m_insertPlayerQuery);

    m_players[id] = Player{id, firstName, lastName};
}

void Database::addMatch(int competition, int position, int score1, int score2, int p1, int p2)
{
    const int id = m_nextMatchId++;

    m_insertMatchQuery.bindValue(0, id);
    m_insertMatchQuery.bindValue(1, competition);
    m_insertMatchQuery.bindValue(2, position);
    m_insertMatchQuery.bindValue(3, (int) MatchType::Single);
    m_insertMatchQuery.bindValue(4, score1);
    m_insertMatchQuery.bindValue(5, score2);
    m_insertMatchQuery.bindValue(6, p1);
    m_insertMatchQuery.bindValue(7, p2);
    m_insertMatchQuery.bindValue(8, 0);
    m_insertMatchQuery.bindValue(9, 0);
    m_insertMatchQuery.exec();
    checkQueryStatus(m_insertMatchQuery);

    m_matches[id] = Match{id, competition, position, MatchType::Single, score1, score2, p1, p2, 0, 0};
}

void Database::addMatch(int competition, int position, int score1, int score2, int p1a, int p1b, int p2a, int p2b)
{
    const int id = m_nextMatchId++;

    m_insertMatchQuery.bindValue(0, id);
    m_insertMatchQuery.bindValue(1, competition);
    m_insertMatchQuery.bindValue(2, position);
    m_insertMatchQuery.bindValue(3, (int) MatchType::Double);
    m_insertMatchQuery.bindValue(4, score1);
    m_insertMatchQuery.bindValue(5, score2);
    m_insertMatchQuery.bindValue(6, p1a);
    m_insertMatchQuery.bindValue(7, p2a);
    m_insertMatchQuery.bindValue(8, p1b);
    m_insertMatchQuery.bindValue(9, p2b);
    m_insertMatchQuery.exec();
    checkQueryStatus(m_insertMatchQuery);

    m_matches[id] = Match{id, competition, position, MatchType::Double, score1, score2, p1a, p2a, p1b, p2b};
}

struct RatingChange
{
    int id;
    float rating;
};

void Database::recompute()
{
    //
    // build a list of all matches, sorted by time/pos
    //
    QVector<Match> sortedMatches;
    for (auto it = m_matches.cbegin(); it != m_matches.cend(); ++it)
        sortedMatches << it.value();
    std::sort(sortedMatches.begin(), sortedMatches.end(), [&](const Match &m1, const Match &m2) {
        if (m1.competition == m2.competition) {
            return m1.position < m2.position;
        }
        const QDateTime t1 = m_competitions[m1.competition].dateTime;
        const QDateTime t2 = m_competitions[m2.competition].dateTime;
        if (t1 == t2) {
            return m1.competition < m2.competition;
        }
        return t1 < t2;
    });

    //
    // Re-build played_matches and ELO tables within these QVariantLists
    //
    QVariantList pm_ids;
    QVariantList pm_players;
    QVariantList pm_matches;

    const auto addPlayedMatch = [&](int p, int m) {
        const int id = pm_ids.size() + 1;
        pm_ids << id;
        pm_players << p;
        pm_matches << m;
        return id;
    };

    //
    // Keep track of player ELOs
    //
    QHash<int, EloRating> playersSingle;
    QHash<int, EloRating> playersDouble;
    QHash<int, EloRating> playersCombined;
    using PlayerElo = QPair<int, EloRating>;
    const auto getPlayerElo = [](const QHash<int, EloRating> &players, int id) {
        return qMakePair(id, players[id]);
    };

    struct RatingDomain {
        QVariantList pmIds;
        QVariantList ratings;
        QVariantList changes;
        void add(int pmid, float rating, float change) {
            pmIds << pmid;
            ratings << (qint16) qRound(rating);
            changes << (qint16) qRound(change);
        }
    };
    RatingDomain eloSeparate;
    RatingDomain eloCombined;

    //
    // keep track of player-vs-player statistics
    //
    struct PlayerVsPlayer
    {
        struct Stats {
            qint16 wins = 0, losses = 0, draws = 0;
            void checkin(float result) {
                if (result == 1.0f) wins++;
                else if (result == 0.5f) draws++;
                else if (result == 0.0f) losses++;
                else qFatal("this shall not be");
            }
        };
        Stats singleStats, doubleStats, partnerStats;
        float singleDiff = 0.f, doubleDiff = 0.f, combinedDiff = 0.f;
        float partnerCombinedDiff = 0.f, partnerDoubleDiff = 0.f;
    };
    QHash<int, QHash<int, PlayerVsPlayer>> playerVsPlayer;

    const auto rateSingle = [&](int pid, int pmid, bool separate, float res, float k, const PlayerElo &other) {
        QHash<int, EloRating> &players = separate ? playersSingle : playersCombined;
        RatingDomain &domain = separate ? eloSeparate : eloCombined;

        const float oldRating = players[pid].abs();
        players[pid].adjust(k, res, other.second);
        const float newRating = players[pid].abs();
        domain.add(pmid, oldRating, newRating - oldRating);

        if (separate) {
            playerVsPlayer[pid][other.first].singleDiff += newRating - oldRating;
            playerVsPlayer[pid][other.first].singleStats.checkin(res);
        }
        else
            playerVsPlayer[pid][other.first].combinedDiff += newRating - oldRating;
    };

    const auto rateDouble = [&](int pid, int pmid, bool separate, float res, float k, const PlayerElo &partner, const PlayerElo &o1, const PlayerElo &o2) {
        QHash<int, EloRating> &players = separate ? playersDouble : playersCombined;
        RatingDomain &domain = separate ? eloSeparate : eloCombined;

        const float oldRating = players[pid].abs();
        players[pid].adjust(k, partner.second, res, o1.second, o2.second);
        const float newRating = players[pid].abs();
        domain.add(pmid, oldRating, newRating - oldRating);

        if (separate) {
            playerVsPlayer[pid][partner.first].partnerDoubleDiff += newRating - oldRating;
            playerVsPlayer[pid][o1.first].doubleDiff += newRating - oldRating;
            playerVsPlayer[pid][o2.first].doubleDiff += newRating - oldRating;

            playerVsPlayer[pid][partner.first].partnerStats.checkin(res);

            playerVsPlayer[pid][o1.first].doubleStats.checkin(res);
            playerVsPlayer[pid][o2.first].doubleStats.checkin(res);
        } else {
            playerVsPlayer[pid][partner.first].partnerCombinedDiff += newRating - oldRating;
            playerVsPlayer[pid][o1.first].combinedDiff += newRating - oldRating;
            playerVsPlayer[pid][o2.first].combinedDiff += newRating - oldRating;
        }
    };

    qWarning() << "Recomputing" << sortedMatches.size() << "matches";

    for (const Match &match : sortedMatches) {
        const float result = (match.score1 > match.score2) ? 0.0f :
                             (match.score1 < match.score2) ? 1.0f : 0.5f;
        const bool isTournament = (m_competitions[match.competition].type == CompetitionType::Tournament);
        const float k = isTournament ? kTournament : kLeague;

        if (match.type == MatchType::Single) {
            const int pm1id = addPlayedMatch(match.p1, match.id);
            const int pm2id = addPlayedMatch(match.p2, match.id);

            const PlayerElo s1 = getPlayerElo(playersSingle, match.p1);
            const PlayerElo s2 = getPlayerElo(playersSingle, match.p2);
            rateSingle(match.p1, pm1id, true, 1.0f - result, k, s2);
            rateSingle(match.p2, pm2id, true, result, k, s1);

            const PlayerElo c1 = getPlayerElo(playersCombined, match.p1);
            const PlayerElo c2 = getPlayerElo(playersCombined, match.p2);
            rateSingle(match.p1, pm1id, false, 1.0f - result, k, c2);
            rateSingle(match.p2, pm2id, false, result, k, c1);
        }
        else if (match.type == MatchType::Double) {
            const int pm1id  = addPlayedMatch(match.p1,  match.id);
            const int pm11id = addPlayedMatch(match.p11, match.id);
            const int pm2id  = addPlayedMatch(match.p2,  match.id);
            const int pm22id = addPlayedMatch(match.p22, match.id);

            const PlayerElo d1  = getPlayerElo(playersDouble, match.p1);
            const PlayerElo d11 = getPlayerElo(playersDouble, match.p11);
            const PlayerElo d2  = getPlayerElo(playersDouble, match.p2);
            const PlayerElo d22 = getPlayerElo(playersDouble, match.p22);
            rateDouble(match.p1,  pm1id,  true, 1.0f - result, k, d11, d2, d22);
            rateDouble(match.p11, pm11id, true, 1.0f - result, k, d1,  d2, d22);
            rateDouble(match.p2,  pm2id,  true, result, k, d22, d1, d11);
            rateDouble(match.p22, pm22id, true, result, k, d2,  d1, d11);

            const PlayerElo c1  = getPlayerElo(playersCombined, match.p1);
            const PlayerElo c11 = getPlayerElo(playersCombined, match.p11);
            const PlayerElo c2  = getPlayerElo(playersCombined, match.p2);
            const PlayerElo c22 = getPlayerElo(playersCombined, match.p22);
            rateDouble(match.p1,  pm1id,  false, 1.0f - result, k, c11, c2, c22);
            rateDouble(match.p11, pm11id, false, 1.0f - result, k, c1,  c2, c22);
            rateDouble(match.p2,  pm2id,  false, result, k, c22, c1, c11);
            rateDouble(match.p22, pm22id, false, result, k, c2,  c1, c11);
        }
    }

    qWarning() << "Build PVP stats";

    //
    // Store current ELOs in separate table
    //
    QVariantList playerIds;
    QVariantList playerSingleElos;
    QVariantList playerDoubleElos;
    QVariantList playerCombinedElos;
    for (auto it = m_players.cbegin(); it != m_players.cend(); ++it) {
        playerIds << it->id;
        playerSingleElos << (qint16) qRound(playersSingle[it->id].abs());
        playerDoubleElos << (qint16) qRound(playersDouble[it->id].abs());
        playerCombinedElos << (qint16) qRound(playersCombined[it->id].abs());
    }

    //
    // Store player stats in separate table
    //
    QVariantList pvpIds, pvpOtherIds;
    QVariantList pvpSingleWins, pvpSingleDraws, pvpSingleLosses;
    QVariantList pvpDoubleWins, pvpDoubleDraws, pvpDoubleLosses;
    QVariantList pvpPartnerWins, pvpPartnerDraws, pvpPartnerLosses;
    QVariantList pvpCombinedDelta, pvpDoubleDelta, pvpSingleDelta;
    QVariantList pvpPartnerCombinedDelta, pvpPartnerDoubleDelta;
    for (auto it1 = playerVsPlayer.cbegin(); it1 != playerVsPlayer.cend(); ++it1) {
        const int playerId = it1.key();
        for (auto it2 = it1.value().cbegin(); it2 != it1.value().cend(); ++it2) {
            const int otherId = it2.key();
            pvpIds << playerId;
            pvpOtherIds << otherId;
            pvpSingleWins << it2->singleStats.wins;
            pvpSingleDraws << it2->singleStats.draws;
            pvpSingleLosses << it2->singleStats.losses;
            pvpDoubleWins << it2->doubleStats.wins;
            pvpDoubleDraws << it2->doubleStats.draws;
            pvpDoubleLosses << it2->doubleStats.losses;
            pvpPartnerWins << it2->partnerStats.wins;
            pvpPartnerDraws << it2->partnerStats.draws;
            pvpPartnerLosses << it2->partnerStats.losses;
            pvpCombinedDelta << (qint16) it2->combinedDiff;
            pvpDoubleDelta << (qint16) it2->doubleDiff;
            pvpSingleDelta << (qint16) it2->singleDiff;
            pvpPartnerCombinedDelta << (qint16) it2->partnerCombinedDiff;
            pvpPartnerDoubleDelta << (qint16) it2->partnerDoubleDiff;
        }
    }

    //
    // Write new played_matches and ELO tables
    //
    execQuery("DELETE FROM played_matches");
    execQuery("DELETE FROM elo_separate");
    execQuery("DELETE FROM elo_combined");
    execQuery("DELETE FROM elo_current");
    execQuery("DELETE FROM player_vs_player_stats");

    QSqlQuery query;

    m_db.transaction();
    query.prepare("INSERT INTO played_matches (id, player_id, match_id) VALUES (?, ?, ?)");
    query.addBindValue(pm_ids);
    query.addBindValue(pm_players);
    query.addBindValue(pm_matches);
    query.execBatch();
    checkQueryStatus(query);
    m_db.commit();

    m_db.transaction();
    query.prepare("INSERT INTO elo_separate (played_match_id, rating, change) VALUES (?, ?, ?)");
    query.addBindValue(eloSeparate.pmIds);
    query.addBindValue(eloSeparate.ratings);
    query.addBindValue(eloSeparate.changes);
    query.execBatch();
    checkQueryStatus(query);
    m_db.commit();

    m_db.transaction();
    query.prepare("INSERT INTO elo_combined (played_match_id, rating, change) VALUES (?, ?, ?)");
    query.addBindValue(eloCombined.pmIds);
    query.addBindValue(eloCombined.ratings);
    query.addBindValue(eloCombined.changes);
    query.execBatch();
    checkQueryStatus(query);
    m_db.commit();

    m_db.transaction();
    query.prepare("INSERT INTO elo_current (player_id, single, double, combined) VALUES (?, ?, ?, ?)");
    query.addBindValue(playerIds);
    query.addBindValue(playerSingleElos);
    query.addBindValue(playerDoubleElos);
    query.addBindValue(playerCombinedElos);
    query.execBatch();
    checkQueryStatus(query);
    m_db.commit();

    m_db.transaction();
    query.prepare(
        "INSERT INTO player_vs_player_stats ( "
        "   player_id, other_id, "
        "   single_wins, single_draws, single_losses, "
        "   double_wins, double_draws, double_losses, "
        "   partner_wins, partner_draws, partner_losses, "
        "   combined_delta, double_delta, single_delta, "
        "   partner_combined_delta, partner_double_delta "
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(pvpIds);
    query.addBindValue(pvpOtherIds);
    query.addBindValue(pvpSingleWins);
    query.addBindValue(pvpSingleDraws);
    query.addBindValue(pvpSingleLosses);
    query.addBindValue(pvpDoubleWins);
    query.addBindValue(pvpDoubleDraws);
    query.addBindValue(pvpDoubleLosses);
    query.addBindValue(pvpPartnerWins);
    query.addBindValue(pvpPartnerDraws);
    query.addBindValue(pvpPartnerLosses);
    query.addBindValue(pvpCombinedDelta);
    query.addBindValue(pvpDoubleDelta);
    query.addBindValue(pvpSingleDelta);
    query.addBindValue(pvpPartnerCombinedDelta);
    query.addBindValue(pvpPartnerDoubleDelta);
    query.execBatch();
    checkQueryStatus(query);
    m_db.commit();
}
