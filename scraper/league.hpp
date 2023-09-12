#pragma once

#include "downloader.hpp"
#include "database.hpp"

struct LeagueGame
{
    QString url;
    int tfvbId;
};
QVector<LeagueGame> scrapeLeagueSeason(GumboOutput *output);

bool scrapeLeageGame(Database *db, int tfvbId, GumboOutput *output);
