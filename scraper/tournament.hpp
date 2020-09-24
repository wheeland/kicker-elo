#pragma once

#include "downloader.hpp"
#include "database.hpp"

QStringList scrapeTournamentOverview(GumboOutput *output);

struct Tournament
{
    QString url;
    int tfvbId;
};
QVector<Tournament> scrapeTournamentPage(GumboOutput *output);

void scrapeTournament(Database *db, int tfvbId, GumboOutput *output);
