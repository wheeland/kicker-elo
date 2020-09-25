#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPair>
#include <QHash>

namespace FoosDB {

enum class EloDomain
{
    Single,
    Double,
    Combined
};

struct Player
{
    int id;
    QString firstName;
    QString lastName;
};

class Database
{
public:
    Database(const std::string &dbPath);
    ~Database();

    const Player *getPlayer(int id) const;
    QVector<const Player*> searchPlayer(const QString &pattern) const;
    QVector<QPair<const Player*, float>> getPlayersByRanking(EloDomain domain, int start = 0, int count = -1);

private:
    void execQuery(const QString &query);
    void readData();

    QSqlDatabase m_db;
    QSqlQuery m_insertPlayerQuery;
    QSqlQuery m_insertCompetitionQuery;
    QSqlQuery m_insertMatchQuery;

    QHash<int, Player> m_players;
};

} // namespace Database
