#pragma once

#include <Wt/Dbo/Dbo.h>
#include <Wt/WDateTime.h>
#include <string>
#include <unordered_map>

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
    std::string firstName;
    std::string lastName;
};

class Database
{
public:
    Database(const std::string &dbPath);
    ~Database();

    const Player *getPlayer(int id) const;
    std::vector<const Player*> searchPlayer(const std::string &pattern) const;
    std::vector<std::pair<const Player*, float>> getPlayersByRanking(EloDomain domain);

private:
    void readData();

    Wt::Dbo::Session m_session;

    std::unordered_map<int, Player> m_players;
};

} // namespace Database
