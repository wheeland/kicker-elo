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

void Database::checkQueryStatus(const QSqlQuery &query) const
{
    if (query.lastError().type() != QSqlError::NoError) {
        qWarning() << "SQL Error:" << query.lastError();
    }
}

void Database::createTables()
{
    if (!m_db.tables().contains("players"))
        execQuery("CREATE TABLE players (id INTEGER PRIMARY KEY, firstName TEXT, lastName TEXT)");

    if (!m_db.tables().contains("competitions"))
        execQuery("CREATE TABLE competitions (id INTEGER PRIMARY KEY, tfvbId INTEGER, tournament BOOL, name TEXT, date DATETIME)");

    if (!m_db.tables().contains("matches"))
        execQuery("CREATE TABLE matches (id INTEGER PRIMARY KEY, competition INTEGER, position INTEGER, "
                        "score1 INTEGER, score2 INTEGER, single BOOL, p1 INTEGER, p2 INTEGER, p11 INTEGER, p22 INTEGER)");

    if (!m_db.tables().contains("eloSingle"))
        execQuery("CREATE TABLE eloSingle (matchId INTEGER, playerID INTEGER, rating REAL)");

    if (!m_db.tables().contains("eloDouble"))
        execQuery("CREATE TABLE eloDouble (matchId INTEGER, playerID INTEGER, rating REAL)");

    if (!m_db.tables().contains("eloCombined"))
        execQuery("CREATE TABLE eloCombined (matchId INTEGER, playerID INTEGER, rating REAL)");
}

void Database::createQueries()
{
    if (!m_insertPlayerQuery.prepare("INSERT INTO players (id, firstName, lastName) VALUES (?, ?, ?)")) {
        qWarning() << "Failed to prepare query";
    }
    if (!m_insertCompetitionQuery.prepare("INSERT INTO competitions (id, tfvbId, tournament, name, date) VALUES (?, ?, ?, ?, ?)")) {
        qWarning() << "Failed to prepare query";
    }
    if (!m_insertMatchQuery.prepare("INSERT INTO matches (id, competition, position, score1, score2, single, p1, p2, p11, p22) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
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

    QSqlQuery competitionQuery("SELECT id, tfvbId, tournament, name, date FROM competitions");
    checkQueryStatus(competitionQuery);
    while (competitionQuery.next()) {
        const int id = competitionQuery.value(0).toInt();
        const int tfvbId = competitionQuery.value(1).toInt();
        const bool tournament = competitionQuery.value(2).toBool();
        const QString name = competitionQuery.value(3).toString();
        const QDateTime dt = competitionQuery.value(4).toDateTime();
        m_competitions[id] = Competition{id, tfvbId, tournament, name, dt};
    }

    QSqlQuery matchQuery("SELECT id, competition, position, score1, score2, single, p1, p2, p11, p22 FROM matches");
    checkQueryStatus(matchQuery);
    while (matchQuery.next()) {
        const int id = matchQuery.value(0).toInt();
        const int competition = matchQuery.value(1).toInt();
        const int position = matchQuery.value(2).toInt();
        const int score1 = matchQuery.value(3).toInt();
        const int score2 = matchQuery.value(4).toInt();
        const bool single = matchQuery.value(5).toBool();
        const int p1 = matchQuery.value(6).toInt();
        const int p2 = matchQuery.value(7).toInt();
        const int p11 = matchQuery.value(8).toInt();
        const int p22 = matchQuery.value(9).toInt();
        m_matches[id] = Match{id, competition, position, score1, score2, single, p1, p2, p11, p22};
        m_nextMatchId = qMax(m_nextMatchId, id + 1);
    }
}

int Database::addCompetition(int tfvbId, bool tournament, const QString &name, QDateTime dt)
{
    int lastId = 0;

    for (auto it = m_competitions.begin(); it != m_competitions.end(); ++it) {
        if (it->tfvbId == tfvbId && it->tournament == tournament)
            return it->id;
        lastId = qMax(lastId, it->id);
    }

    m_insertCompetitionQuery.bindValue(0, lastId + 1);
    m_insertCompetitionQuery.bindValue(1, tfvbId);
    m_insertCompetitionQuery.bindValue(2, tournament);
    m_insertCompetitionQuery.bindValue(3, name);
    m_insertCompetitionQuery.bindValue(4, dt);
    m_insertCompetitionQuery.exec();
    checkQueryStatus(m_insertCompetitionQuery);

    m_competitions[lastId + 1] = Competition{lastId + 1, tfvbId, tournament, name, dt};

    return lastId + 1;
}

int Database::competitionGameCount(int tfvbId, bool tournament)
{
    int id = -1;
    for (auto it = m_competitions.begin(); it != m_competitions.end(); ++it) {
        if (it->tfvbId == tfvbId && it->tournament == tournament) {
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
    m_insertMatchQuery.bindValue(3, score1);
    m_insertMatchQuery.bindValue(4, score2);
    m_insertMatchQuery.bindValue(5, true);
    m_insertMatchQuery.bindValue(6, p1);
    m_insertMatchQuery.bindValue(7, p2);
    m_insertMatchQuery.bindValue(8, 0);
    m_insertMatchQuery.bindValue(9, 0);
    m_insertMatchQuery.exec();
    checkQueryStatus(m_insertMatchQuery);

    m_matches[id] = Match{id, competition, position, score1, score2, true, p1, p2, 0, 0};
}

void Database::addMatch(int competition, int position, int score1, int score2, int p1a, int p1b, int p2a, int p2b)
{
    const int id = m_nextMatchId++;

    m_insertMatchQuery.bindValue(0, id);
    m_insertMatchQuery.bindValue(1, competition);
    m_insertMatchQuery.bindValue(2, position);
    m_insertMatchQuery.bindValue(3, score1);
    m_insertMatchQuery.bindValue(4, score2);
    m_insertMatchQuery.bindValue(5, false);
    m_insertMatchQuery.bindValue(6, p1a);
    m_insertMatchQuery.bindValue(7, p2a);
    m_insertMatchQuery.bindValue(8, p1b);
    m_insertMatchQuery.bindValue(9, p2b);
    m_insertMatchQuery.exec();
    checkQueryStatus(m_insertMatchQuery);

    m_matches[id] = Match{id, competition, position, score1, score2, false, p1a, p2a, p1b, p2b};
}

struct RatingChange
{
    int matchId;
    int playerId;
    float ratingAfter;
};

template <class Rating, class Player, class Competition, class Match>
QVector<RatingChange> computeRankings(
        const QHash<int, Player> &players,
        const QHash<int, Competition> &competitions,
        const QHash<int, Match> &matches,
        float kLeague, float kTournament,
        bool singles, bool doubles)
{
    QVector<RatingChange> ret;

    //
    // build a list of all matches, sorted by time/pos
    //
    QVector<Match> sortedMatches;
    for (auto it = matches.cbegin(); it != matches.cend(); ++it)
        sortedMatches << it.value();
    qSort(sortedMatches.begin(), sortedMatches.end(), [&](const Match &m1, const Match &m2) {
        if (m1.competition == m2.competition)
            return m1.position < m2.position;
        const QDateTime t1 = competitions[m1.competition].dateTime;
        const QDateTime t2 = competitions[m2.competition].dateTime;
        return t1 < t2;
    });

    //
    // build list of players
    //
    struct RatedPlayer { int id; Rating rating; };
    QHash<int, RatedPlayer> playerRatings;
    for (auto it = players.begin(); it != players.end(); ++it)
        playerRatings[it->id] = RatedPlayer{ it->id, Rating() };

    //
    // iterate through matches and adjust ELO
    //
    for (const Match &match : sortedMatches)
    {
        const float result = (match.score1 > match.score2) ? 0.0f :
                             (match.score1 < match.score2) ? 1.0f : 0.5f;
        const float k = competitions[match.competition].tournament ? kTournament : kLeague;

        if (match.single && singles) {
            RatedPlayer *p1 = &playerRatings[match.p1];
            RatedPlayer *p2 = &playerRatings[match.p2];
            const Rating r1 = p1->rating;
            const Rating r2 = p1->rating;
            p1->rating.adjust(k, 1.0f-result, r2);
            p2->rating.adjust(k, result,  r1);
            ret << RatingChange{match.id, p1->id, p1->rating.abs()};
            ret << RatingChange{match.id, p2->id, p2->rating.abs()};
        }
        else if (doubles) {
            RatedPlayer *p1 = &playerRatings[match.p1];
            RatedPlayer *p11 = &playerRatings[match.p11];
            RatedPlayer *p2 = &playerRatings[match.p2];
            RatedPlayer *p22 = &playerRatings[match.p22];
            const Rating r1  = p1->rating;
            const Rating r11 = p11->rating;
            const Rating r2  = p2->rating;
            const Rating r22 = p22->rating;
            p1 ->rating.adjust(k, r11, 1.0f-result, r2, r22);
            p11->rating.adjust(k, r1,  1.0f-result, r2, r22);
            p2 ->rating.adjust(k, r22, result,  r1, r11);
            p22->rating.adjust(k, r2,  result,  r1, r11);
            ret << RatingChange{match.id, p1 ->id, p1 ->rating.abs()};
            ret << RatingChange{match.id, p11->id, p11->rating.abs()};
            ret << RatingChange{match.id, p2 ->id, p2 ->rating.abs()};
            ret << RatingChange{match.id, p22->id, p22->rating.abs()};
        }
    }

    // Add current ratings as a special entry with matchId=0
    for (auto it = playerRatings.begin(); it != playerRatings.end(); ++it) {
        ret << RatingChange{0, it->id, it->rating.abs() };
    }

    return ret;
}

void Database::recomputeElo(const QString &table, float kLeague, float kTournament, bool singles, bool doubles)
{
    QVector<RatingChange> changes = computeRankings<EloRating>(
                m_players, m_competitions, m_matches, kLeague, kTournament, singles, doubles);

    execQuery(QString("DELETE FROM ") + table);

    QVariantList matchIds;
    QVariantList playerIds;
    QVariantList ratings;

    for (const RatingChange &change : changes) {
        matchIds << change.matchId;
        playerIds << change.playerId;
        ratings << change.ratingAfter;
    }

    m_db.transaction();

    QSqlQuery query;
    query.prepare(QString("INSERT INTO %1 (matchId, playerId, rating) VALUES (?, ?, ?)").arg(table));
    query.addBindValue(matchIds);
    query.addBindValue(playerIds);
    query.addBindValue(ratings);
    query.execBatch();
    checkQueryStatus(query);

    m_db.commit();
}


void Database::recompute()
{
    recomputeElo("eloSingle", 12.0f, 24.0f, true, false);
    recomputeElo("eloDouble", 12.0f, 24.0f, false, true);
    recomputeElo("eloCombined", 12.0f, 24.0f, true, true);
}
