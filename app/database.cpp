#include "database.hpp"
#include "database_orm.hpp"

#include <Wt/Dbo/backend/Sqlite3.h>
#include <Wt/Dbo/WtSqlTraits.h>

static std::string tolower(const std::string &s)
{
    std::string ret = s;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

namespace dbo = Wt::Dbo;

namespace FoosDB {

Database::Database(const std::string &dbPath)
{
    std::unique_ptr<dbo::backend::Sqlite3> sqlite3{new dbo::backend::Sqlite3(dbPath)};
    sqlite3->setDateTimeStorage(dbo::SqlDateTimeType::DateTime, dbo::backend::DateTimeStorage::UnixTimeAsInteger);
    m_session.setConnection(std::move(sqlite3));

    m_session.mapClass<DbPlayer>("players");
    m_session.mapClass<DbCompetition>("competitions");
    m_session.mapClass<DbMatch>("matches");
    m_session.mapClass<DbPlayedMatch>("played_matches");
    m_session.mapClass<DbRating<EloDomain::Single>>("elo_single");
    m_session.mapClass<DbRating<EloDomain::Double>>("elo_double");
    m_session.mapClass<DbRating<EloDomain::Combined>>("elo_combined");
}

Database::~Database()
{
}

void Database::readData()
{
    dbo::Transaction transaction{m_session};

    const dbo::collection<dbo::ptr<DbPlayer>> players = m_session.find<DbPlayer>();
    for (const dbo::ptr<DbPlayer> &player : players) {
        m_players[player->id] = Player{player->id, player->firstName, player->lastName};
    }
}

const Player *Database::getPlayer(int id) const
{
    const auto it = m_players.find(id);
    return (it != m_players.end()) ? &it->second : nullptr;
}

std::vector<const Player*> Database::searchPlayer(const std::string &pattern) const
{
    std::vector<const Player*> ret;

    const std::string lowered = tolower(pattern);

    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        if (tolower(it->second.firstName).find(pattern) >= 0 || tolower(it->second.lastName).find(pattern) >= 0)
            ret.push_back(&it->second);
    }

    return ret;
}

std::vector<std::pair<const Player*, float>> Database::getPlayersByRanking(EloDomain domain)
{
    std::vector<std::pair<const Player*, float>> ret;

    dbo::Transaction transaction{m_session};

    players = m_session.find<DbPlayer>().join();
    for (const dbo::ptr<DbPlayer> &player : players) {
        m_players[player->id] = Player{player->id, player->firstName, player->lastName};
    }

}

} // namespace FoosDB
