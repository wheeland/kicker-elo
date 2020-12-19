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

enum TournamentSource {
    TFVB,
    DTFB
};

void scrapeTournament(Database *db, int tfvbId, TournamentSource src, GumboOutput *output);
