#pragma once

#include "downloader.hpp"
#include "database.hpp"

struct Tournament
{
    QString url;
    int tfvbId;
};
QVector<Tournament> scrapeTournamentPage(GumboOutput *output);

void scrapeTournament(Database *db, int tfvbId, GumboOutput *output);
