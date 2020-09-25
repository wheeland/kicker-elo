#pragma once

#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>

struct DbPlayer;
struct DbCompetition;
struct DbMatch;
struct DbPlayedMatch;
struct DbRating;

namespace Wt { namespace Dbo {
    template<> struct dbo_traits<DbPlayer> : public dbo_default_traits {
        static const char *versionField() { return nullptr; }
        static const char *surrogateIdField() { return nullptr; }
    };
    template<> struct dbo_traits<DbCompetition> : public dbo_default_traits {
        static const char *versionField() { return nullptr; }
        static const char *surrogateIdField() { return nullptr; }
    };
    template<> struct dbo_traits<DbMatch> : public dbo_default_traits {
        static const char *versionField() { return nullptr; }
        static const char *surrogateIdField() { return nullptr; }
    };
    template<> struct dbo_traits<DbPlayedMatch> : public dbo_default_traits {
        static const char *versionField() { return nullptr; }
        static const char *surrogateIdField() { return nullptr; }
    };
    template<> struct dbo_traits<DbRating> : public dbo_default_traits {
        typedef Wt::Dbo::ptr<DbPlayedMatch> IdType;
        static IdType invalidId() { return IdType(); }
        static const char *versionField() { return nullptr; }
        static const char *surrogateIdField() { return 0; }
    };
} }

enum class DbCompetitionType {
    Invalid = 0,
    League = 1,
    Cup = 2,
    Tournament = 3
};

enum class DbMatchType {
    Invalid = 0,
    Single = 1,
    Double = 2
};

struct DbPlayer
{
    int id;
    std::string firstName;
    std::string lastName;

    Wt::Dbo::collection<Wt::Dbo::ptr<DbPlayedMatch>> playedMatches;

    template<class Action>
    void persist(Action& a)
    {
        Wt::Dbo::id(a, id, "id");
        Wt::Dbo::field(a, firstName, "firstName");
        Wt::Dbo::field(a, lastName, "lastName");
        Wt::Dbo::hasMany(a, playedMatches, Wt::Dbo::ManyToOne, "player");
    }
};

struct DbCompetition
{
    int id;
    int tfvbId;
    DbCompetitionType type;
    std::string name;
    Wt::WDateTime date;

    Wt::Dbo::collection<Wt::Dbo::ptr<DbMatch>> matches;

    template<class Action>
    void persist(Action& a)
    {
        Wt::Dbo::id(a, id, "id");
        Wt::Dbo::field(a, tfvbId, "tfvbId");
        Wt::Dbo::field(a, type, "type");
        Wt::Dbo::field(a, name, "name");
        Wt::Dbo::field(a, date, "date");
        Wt::Dbo::hasMany(a, matches, Wt::Dbo::ManyToOne, "competition");
    }
};

struct DbMatch
{
    int id;
    Wt::Dbo::ptr<DbCompetition> competition;
    int position;
    DbMatchType type;
    int score1;
    int score2;
    int p1;
    int p2;
    int p11;
    int p22;

    Wt::Dbo::collection<Wt::Dbo::ptr<DbPlayedMatch>> playedMatches;

    template<class Action>
    void persist(Action& a)
    {
        Wt::Dbo::id(a, id, "id");
        Wt::Dbo::belongsTo(a, competition, "competition");
        Wt::Dbo::field(a, position, "position");
        Wt::Dbo::field(a, type, "type");
        Wt::Dbo::field(a, score1, "score1");
        Wt::Dbo::field(a, score2, "score2");
        Wt::Dbo::field(a, p1, "p1");
        Wt::Dbo::field(a, p2, "p2");
        Wt::Dbo::field(a, p11, "p11");
        Wt::Dbo::field(a, p22, "p22");
        Wt::Dbo::hasMany(a, playedMatches, Wt::Dbo::ManyToOne, "match");
    }
};

struct DbPlayedMatch
{
    int id;
    Wt::Dbo::ptr<DbPlayer> player;
    Wt::Dbo::ptr<DbMatch> match;

    Wt::Dbo::collection<Wt::Dbo::ptr<DbRating>> ratings;

    template<class Action>
    void persist(Action& a)
    {
        Wt::Dbo::id(a, id, "id");
        Wt::Dbo::belongsTo(a, player, "player");
        Wt::Dbo::belongsTo(a, match, "match");
        Wt::Dbo::hasMany(a, ratings, Wt::Dbo::ManyToOne, "played_match");
    }
};

struct DbRating
{
    Wt::Dbo::ptr<DbPlayedMatch> played_match;
    float rating;

    Wt::Dbo::collection<Wt::Dbo::ptr<DbRating>> ratings;

    template<class Action>
    void persist(Action& a)
    {
        Wt::Dbo::id(a, played_match, "played_match", Wt::Dbo::OnDeleteCascade);
        Wt::Dbo::field(a, rating, "rating");
    }
};
