#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPair>
#include <QHash>
#include <QDateTime>

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
};

struct PlayerMatch
{
    QDateTime date;
    QString competitionName;
    CompetitionType competitionType;
    MatchType matchType;

    const Player *partner;
    const Player *opponent1;
    const Player *opponent2;

    int myScore;
    int opponentScore;

    float eloDiffSeparate;
    float eloDiffCombined;
};

class Database
{
public:
    Database(const std::string &dbPath);
    ~Database();

    const Player *getPlayer(int id) const;
    QVector<const Player*> searchPlayer(const QString &pattern) const;
    QVector<const Player*> getPlayersByRanking(EloDomain domain, int start = 0, int count = -1) const;

    QVector<PlayerMatch> getRecentMatches(const Player *player, int start = 0, int count = -1);

private:
    void execQuery(const QString &query);
    void readData();

    QSqlDatabase m_db;

    QHash<int, Player> m_players;
};

} // namespace Database
