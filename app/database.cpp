#include "database.hpp"
#include "database_orm.hpp"

#include <Wt/Dbo/backend/Sqlite3.h>
#include <Wt/Dbo/WtSqlTraits.h>

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
    m_session.mapClass<DbRating>("elo_single");
    m_session.mapClass<DbRating>("elo_double");
    m_session.mapClass<DbRating>("elo_combined");

    m_session.createTables();

//    dbo::ptr<DbPlayer> me = m_session.find<DbPlayer>().where("lastName = ?").bind("Hagen");
//    std::cout << me->id;
}

Database::~Database()
{

}

} // namespace FoosDB
