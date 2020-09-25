#pragma once

#include <Wt/Dbo/Dbo.h>
#include <Wt/WDateTime.h>
#include <string>

namespace FoosDB {

class Database
{
public:
    Database(const std::string &dbPath);
    ~Database();

private:
    Wt::Dbo::Session m_session;
};

} // namespace Database
