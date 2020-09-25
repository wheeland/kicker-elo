#include "database.hpp"

#include <QDebug>
#include <QSqlError>

static QString eloDbName(FoosDB::EloDomain domain)
{
    switch (domain) {
    case FoosDB::EloDomain::Single: return "elo_single";
    case FoosDB::EloDomain::Double: return "elo_double";
    case FoosDB::EloDomain::Combined: return "elo_combined";
    }
}

namespace FoosDB {

Database::Database(const std::string &dbPath)
    : m_db(QSqlDatabase::addDatabase("QSQLITE"))
{
    m_db.setDatabaseName(QString::fromStdString(dbPath));

    if (!m_db.open()) {
        qWarning() << "Error opening database:" << m_db.lastError();
    }

    readData();
}

Database::~Database()
{
}

void Database::execQuery(const QString &query)
{
    QSqlQuery sqlQuery(query);
    if (sqlQuery.lastError().type() != QSqlError::NoError) {
        qWarning() << "SQL Error:" << sqlQuery.lastError();
    }
}

void Database::readData()
{
    QSqlQuery playerQuery("SELECT id, firstName, lastName FROM players");

    while (playerQuery.next()) {
        const int id = playerQuery.value(0).toInt();
        const QString firstName = playerQuery.value(1).toString();
        const QString lastName = playerQuery.value(2).toString();
        m_players[id] = Player{id, firstName, lastName};
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

QVector<QPair<const Player*, float>> Database::getPlayersByRanking(EloDomain domain, int start, int count)
{
    QVector<QPair<const Player*, float>> ret;

    QString queryString = QString(
        "SELECT played_matches.player_id, %1.rating\n"
        "FROM played_matches\n"
        "INNER JOIN %1 ON played_matches.id = %1.played_match_id\n"
        "WHERE played_matches.match_id = 0\n"
        "ORDER BY %1.rating DESC\n"
    ).arg(eloDbName(domain));

    if (start > 0 || count > 0) {
        if (count <= 0) {
            count = m_players.size();
        }
        queryString += QString("LIMIT %1 OFFSET %2").arg(count).arg(start + 1);
    }

    QSqlQuery query(queryString);
    while (query.next()) {
        const int id = query.value(0).toInt();
        const float rating = query.value(1).toFloat();

        const auto it = m_players.find(id);
        if (it != m_players.cend()) {
            ret << qMakePair(&it.value(), rating);
        }
    }

    return ret;
}

} // namespace FoosDB
