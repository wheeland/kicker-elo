#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPair>
#include <QHash>
#include <QDateTime>
#include <QReadWriteLock>

class QThread;

namespace FoosDB {

enum class EloDomain
{
    Single,
    Double,
    Combined
};

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

struct Player
{
    int id;

    QString firstName;
    QString lastName;

    float eloSingle;
    float eloDouble;
    float eloCombined;

    int matchCount;
};

struct PlayerMatch
{
    QDateTime date;
    QString competitionName;
    CompetitionType competitionType;
    MatchType matchType;

    struct Participant {
        const Player *player = nullptr;
        int eloSeparate = 0;
        int eloCombined = 0;
    };

    Participant myself;
    Participant partner;
    Participant opponent1;
    Participant opponent2;

    int myScore;
    int opponentScore;

    int eloSeparateDiff;
    int eloCombinedDiff;
};

class Database
{
public:
    Database(const std::string &dbPath);
    ~Database();

    const Player *getPlayer(int id) const;
    int getPlayerCount() const { return m_players.size(); }
    QVector<const Player*> searchPlayer(const QString &pattern) const;
    QVector<const Player*> getPlayersByRanking(EloDomain domain, int start = 0, int count = -1) const;

    QVector<PlayerMatch> getPlayerMatches(const Player *player);

private:
    void readData();
    QSqlDatabase *getOrCreateDb();

    // each thread needs to have its own database connection
    QReadWriteLock m_dbLock;
    QHash<QThread*, QSqlDatabase*> m_dbs;

    QHash<int, Player> m_players;
};

} // namespace Database
