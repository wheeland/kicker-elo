#include "database.hpp"
#include "rating.hpp"

#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

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
      rating real NOT NULL, \
      primary key (played_match_id))"
    );
    execQuery("CREATE TABLE IF NOT EXISTS elo_combined ( \
      played_match_id integer NOT NULL, \
      rating real NOT NULL, \
      primary key (played_match_id))"
    );
    execQuery("CREATE TABLE IF NOT EXISTS elo_current ( \
      player_id integer NOT NULL, \
      single real NOT NULL, \
      double real NOT NULL, \
      combined real NOT NULL, \
      primary key (player_id))"
    );
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
    qSort(sortedMatches.begin(), sortedMatches.end(), [&](const Match &m1, const Match &m2) {
        if (m1.competition == m2.competition)
            return m1.position < m2.position;
        const QDateTime t1 = m_competitions[m1.competition].dateTime;
        const QDateTime t2 = m_competitions[m2.competition].dateTime;
        return t1 < t2;
    });

    //
    // Re-build played_matches and ELO tables within these QVariantLists
    //
    QVariantList pm_ids;
    QVariantList pm_players;
    QVariantList pm_matches;

    QHash<int, EloRating> playersSingle;
    QHash<int, EloRating> playersDouble;
    QHash<int, EloRating> playersCombined;

    struct RatingDomain {
        QVariantList pmIds;
        QVariantList ratings;
        void add(int pmid, float rating) {
            pmIds << pmid;
            ratings << rating;
        }
    };
    RatingDomain eloSeparate;
    RatingDomain eloCombined;

    const auto addPlayedMatch = [&](int p, int m) {
        const int id = pm_ids.size() + 1;
        pm_ids << id;
        pm_players << p;
        pm_matches << m;
        return id;
    };

    const auto rateSingle = [&](int pid, int pmid, QHash<int, EloRating> &players, RatingDomain &domain, float res, float k, const EloRating &other) {
        players[pid].adjust(k, res, other);
        domain.add(pmid, players[pid].abs());
    };

    const auto rateDouble = [&](int pid, int pmid, QHash<int, EloRating> &players, RatingDomain &domain, float res, float k, const EloRating &partner, const EloRating &o1, const EloRating &o2) {
        players[pid].adjust(k, partner, res, o1, o2);
        domain.add(pmid, players[pid].abs());
    };

    for (const Match &match : sortedMatches) {
        const float result = (match.score1 > match.score2) ? 0.0f :
                             (match.score1 < match.score2) ? 1.0f : 0.5f;
        const bool isTournament = (m_competitions[match.competition].type == CompetitionType::Tournament);
        const float k = isTournament ? 48.f : 24.f;

        if (match.type == MatchType::Single) {
            const int pm1id = addPlayedMatch(match.p1, match.id);
            const int pm2id = addPlayedMatch(match.p2, match.id);

            const EloRating s1 = playersSingle[match.p1];
            const EloRating s2 = playersSingle[match.p2];
            rateSingle(match.p1, pm1id, playersSingle, eloSeparate, 1.0f - result, k, s2);
            rateSingle(match.p2, pm2id, playersSingle, eloSeparate, result, k, s1);

            const EloRating c1 = playersCombined[match.p1];
            const EloRating c2 = playersCombined[match.p2];
            rateSingle(match.p1, pm1id, playersCombined, eloCombined, 1.0f - result, k, c2);
            rateSingle(match.p2, pm2id, playersCombined, eloCombined, result, k, c1);
        }
        else if (match.type == MatchType::Double) {
            const int pm1id  = addPlayedMatch(match.p1,  match.id);
            const int pm11id = addPlayedMatch(match.p11, match.id);
            const int pm2id  = addPlayedMatch(match.p2,  match.id);
            const int pm22id = addPlayedMatch(match.p22, match.id);

            const EloRating d1  = playersDouble[match.p1];
            const EloRating d11 = playersDouble[match.p11];
            const EloRating d2  = playersDouble[match.p2];
            const EloRating d22 = playersDouble[match.p22];
            rateDouble(match.p1,  pm1id,  playersDouble, eloSeparate, 1.0f - result, k, d11, d2, d22);
            rateDouble(match.p11, pm11id, playersDouble, eloSeparate, 1.0f - result, k, d1,  d2, d22);
            rateDouble(match.p2,  pm2id,  playersDouble, eloSeparate, result, k, d22, d1, d11);
            rateDouble(match.p22, pm22id, playersDouble, eloSeparate, result, k, d2,  d1, d11);

            const EloRating c1  = playersCombined[match.p1];
            const EloRating c11 = playersCombined[match.p11];
            const EloRating c2  = playersCombined[match.p2];
            const EloRating c22 = playersCombined[match.p22];
            rateDouble(match.p1,  pm1id,  playersCombined, eloCombined, 1.0f - result, k, c11, c2, c22);
            rateDouble(match.p11, pm11id, playersCombined, eloCombined, 1.0f - result, k, c1,  c2, c22);
            rateDouble(match.p2,  pm2id,  playersCombined, eloCombined, result, k, c22, c1, c11);
            rateDouble(match.p22, pm22id, playersCombined, eloCombined, result, k, c2,  c1, c11);
        }
    }

    //
    // Store current ELOs as matchid=0
    //
    QVariantList playerIds;
    QVariantList playerSingleElos;
    QVariantList playerDoubleElos;
    QVariantList playerCombinedElos;
    for (auto it = m_players.cbegin(); it != m_players.cend(); ++it) {
        playerIds << it->id;
        playerSingleElos << playersSingle[it->id].abs();
        playerDoubleElos << playersDouble[it->id].abs();
        playerCombinedElos << playersCombined[it->id].abs();
    }

    //
    // Write new played_matches and ELO tables
    //
    execQuery("DELETE FROM played_matches");
    execQuery("DELETE FROM elo_separate");
    execQuery("DELETE FROM elo_combined");
    execQuery("DELETE FROM elo_current");

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
    query.prepare("INSERT INTO elo_separate (played_match_id, rating) VALUES (?, ?)");
    query.addBindValue(eloSeparate.pmIds);
    query.addBindValue(eloSeparate.ratings);
    query.execBatch();
    checkQueryStatus(query);
    m_db.commit();

    m_db.transaction();
    query.prepare("INSERT INTO elo_combined (played_match_id, rating) VALUES (?, ?)");
    query.addBindValue(eloCombined.pmIds);
    query.addBindValue(eloCombined.ratings);
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
}
