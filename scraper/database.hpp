#pragma once

#include <QString>
#include <QDateTime>
#include <QHash>

#include <QSqlDatabase>
#include <QSqlQuery>

enum class CompetitionType {
    Invalid = 0,
    League = 1,
    Cup = 2,
    Tournament = 3
};

enum class MatchType {
    Invalid = 0,
    Single = 1,
    Double = 2
};

class Database
{
public:
    Database(const QString &sqlitePath);
    ~Database();

    int addCompetition(int tfvbId, CompetitionType type, const QString &name, QDateTime dt);
    int competitionGameCount(int tfvbId, CompetitionType type);

    void addPlayer(int id, const QString &firstName, const QString &lastName);
    void addMatch(int competition, int position, int score1, int score2, int p1, int p2);
    void addMatch(int competition, int position, int score1, int score2, int p1a, int p1b, int p2a, int p2b);

    void recompute();

private:
    void execQuery(const QString &query);

    void createQueries();
    void readData();

    QSqlDatabase m_db;
    QSqlQuery m_insertPlayerQuery;
    QSqlQuery m_insertCompetitionQuery;
    QSqlQuery m_insertMatchQuery;

    void checkQueryStatus(const QSqlQuery &query) const;

    struct Player {
        int id;
        QString firstName;
        QString lastName;
    };

    struct Competition {
        int id;
        int tfvbId;
        CompetitionType type;
        QString name;
        QDateTime dateTime;
    };

    struct Match {
        int id;
        int competition;
        int position;
        MatchType type;
        int score1;
        int score2;
        int p1;
        int p2;
        int p11;
        int p22;
    };

    struct PlayedMatch {
        int id;
        int player;
        int match;
    };

    QHash<int, Player> m_players;
    QHash<int, Competition> m_competitions;
    QHash<int, Match> m_matches;
    int m_nextMatchId = 1;

    void recomputeElo(
            const QVector<Match> &sortedMatches,
            const QString &table,
            float kLeague, float kTournament, bool singles, bool doubles
    );
};
