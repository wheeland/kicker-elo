#pragma once

#include <QString>
#include <QDateTime>

class Database
{
public:
    int addCompetition(const QString &name, QDateTime dt);

    void addPlayer(int id, const QString &firstName, const QString &lastName);
    void addMatch(int competition, int position, int score1, int score2, int p1, int p2);
    void addMatch(int competition, int position, int score1, int score2, int p1a, int p1b, int p2a, int p2b);

private:
};
