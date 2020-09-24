#pragma once

#include "downloader.hpp"
#include "database.hpp"

struct LeagueGame
{
    QString url;
    int tfvbId;
};
QVector<LeagueGame> scrapeLeagueSeason(Database *db, GumboOutput *output);

void scrapeLeageGame(Database *db, int tfvbId, GumboOutput *output);
