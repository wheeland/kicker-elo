#pragma once

#include <QString>
#include <QDateTime>
#include <QHash>

#include <QSqlDatabase>
#include <QSqlQuery>

class Database
{
public:
    Database(const QString &sqlitePath);
    ~Database();

    int addCompetition(int tfvbId, bool tournament, const QString &name, QDateTime dt);

    void addPlayer(int id, const QString &firstName, const QString &lastName);
    void addMatch(int competition, int position, int score1, int score2, int p1, int p2);
    void addMatch(int competition, int position, int score1, int score2, int p1a, int p1b, int p2a, int p2b);

private:
    void createTables();
    void createQueries();
    void readData();

    QSqlDatabase m_db;
    QSqlQuery m_insertPlayerQuery;
    QSqlQuery m_insertCompetitionQuery;
    QSqlQuery m_insertMatchQuery;

    void checkQueryStatus(const QSqlQuery &query);

    struct Player {
        int id;
        QString firstName;
        QString lastName;
    };

    struct Competition {
        int id;
        int tfvbId;
        bool tournament;
        QString name;
        QDateTime dateTime;
    };

    struct Match {
        int id;
        int competition;
        int position;
        int score1;
        int score2;
        bool single;
        int p1;
        int p2;
        int p11;
        int p22;
    };

    QHash<int, Player> m_players;
    QHash<int, Competition> m_competitions;
    QHash<int, Match> m_matches;
    int m_nextMatchId = 1;
};
