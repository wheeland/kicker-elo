#include <QCoreApplication>
#include <QCommandLineParser>
#include <QNetworkCookie>
#include <QFile>
#include <QDebug>

#include "downloader.hpp"
#include "database.hpp"
#include "league.hpp"
#include "tournament.hpp"

static QString prepend(const QString &str, const QString &prefix)
{
    return str.startsWith(prefix) ? str : (prefix + str);
}

QStringList readSourceFiles(const QStringList &paths)
{
    QStringList ret;

    for (const QString &path : paths) {
        QFile file(path);
        if (file.open(QFile::ReadOnly)) {
            while (!file.atEnd()) {
                const QString line = QString::fromUtf8(file.readLine()).trimmed();
                if (!line.isEmpty() && !line.startsWith("#"))
                    ret << line;
            }
        }
    }

    return ret;
}

bool readFloatValue(QCommandLineParser &parser, QCommandLineOption &option, float &dst)
{
    if (!parser.isSet(option)) {
        dst = option.defaultValues().first().toFloat();
        return true;
    }
    bool ok = true;
    dst = parser.value(option).toFloat(&ok);
    if (!ok) {
        qCritical() << "Not a valid value for" << option.names().first();
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("tfvb-scraper");
    QCoreApplication::setApplicationVersion("0.1");

    //
    // Setup command line parsing
    //
    QCommandLineParser parser;
    parser.setApplicationDescription("Scrapes delicious foos data off TFVB");
    parser.addHelpOption();
    parser.addPositionalArgument("sqlite", "Path to SQLite database");
    QCommandLineOption leagueSourcesOption({"league-sources", "l"}, "Sources for league games", "path");
    parser.addOption(leagueSourcesOption);
    QCommandLineOption tournamentSeasonOption({"tournament-season", "t"}, "Season to query for tournaments (1-15)", "season", "15");
    parser.addOption(tournamentSeasonOption);
    QCommandLineOption tournamentSourceOption({"tournament-source", "s"}, "What website to query tournaments from (dtfb, tfvb)", "source", "tfvb");
    parser.addOption(tournamentSourceOption);
    QCommandLineOption kLeagueOption(QStringList{"kleague"}, "k-factor used for 2-set league games (1-set games count half)", "k", "18");
    parser.addOption(kLeagueOption);
    QCommandLineOption kTournamentOption(QStringList{"ktournament"}, "k-factor used for tournament games (mini-challengers count half)", "k", "24");
    parser.addOption(kTournamentOption);
    QCommandLineOption forceRecompute(QStringList{{"recompute", "r"}}, "Force recomputation of ELO");
    parser.addOption(forceRecompute);

    parser.process(app);
    if (parser.positionalArguments().isEmpty())
        parser.showHelp();

    float kl, kt;
    if (!readFloatValue(parser, kLeagueOption, kl) || !readFloatValue(parser, kTournamentOption, kt))
        return 1;
    qDebug() << "kLeague =" << kl;
    qDebug() << "kTournament =" << kt;

    const QString sqlitePath = parser.positionalArguments().first();

    Downloader *downloader = new Downloader();
    Database *database = new Database(sqlitePath, kl, kt);
	bool recomputeElo = parser.isSet(forceRecompute);

    //
    // Parse and scrape league source files
    //
    const QStringList leagueSourceFiles = parser.values(leagueSourcesOption);
    const QStringList leagueSources = readSourceFiles(leagueSourceFiles);
    qDebug() << "Scraping" << leagueSources.size() << "League URLs";

    for (const QString &source : leagueSources) {
        const QUrl url(source);
        const QString prefix = url.scheme() + "://" + url.host();

        const auto cb = [=, &recomputeElo] (QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
            const QVector<LeagueGame> games = scrapeLeagueSeason(out);
            qDebug() << "Scraping" << games.size() << "games from League URL" << source;

            for (const LeagueGame &game : games) {
                const int count = database->competitionGameCount(game.tfvbId, CompetitionType::League);
                const QString fullUrl = prepend(game.url, prefix);

                if (count > 0) {
                    qDebug() << "Skipping" << game.tfvbId << fullUrl << "(from" << source << "): has" << count << "matches already";
                    continue;
                }

                downloader->request(QNetworkRequest(fullUrl), [=, &recomputeElo](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
                    qDebug() << "Scraping" << game.tfvbId << fullUrl << "(from" << source << ")";
                    recomputeElo |= scrapeLeageGame(database, game.tfvbId, out);
                });
            }
        };

        downloader->request(QNetworkRequest(url), cb);
    }

    //
    // Scrape tournaments. For some reason, when we send multiple requests with
    // different sportsmanager_filter_saison_id, we get the same result all over (for the current season).
    // might be a server-side error, so we just request one season at a time for now -_-
    //
    if (parser.isSet(tournamentSeasonOption)) {
        bool ok = true;
        const int season = parser.value(tournamentSeasonOption).toInt(&ok);
        if (!ok) {
            qCritical() << "Not a season:" << parser.value(tournamentSeasonOption);
            return 1;
        }

        const QString tournamentSourceValue = parser.value(tournamentSourceOption);
        TournamentSource tournamentSource;
        QUrl url;
        if (tournamentSourceValue == "dtfb") {
            tournamentSource = DTFB;
            url = "https://dtfb.de/wettbewerbe/turnierserie/turnierergebnisse";
        }
        else if (tournamentSourceValue == "tfvb") {
            tournamentSource = TFVB;
            url = "https://tfvb.de/index.php/turniere";
        }
        else {
            qWarning() << "Tournament source must be set to 'dtfb' or 'tfvb'.";
            return 1;
        }

        QNetworkRequest request(url);
        const QString prefix = url.scheme() + "://" + url.host();
        
        QNetworkCookie cookie;
        cookie.setName("sportsmanager_filter_saison_id");
        cookie.setValue(QByteArray::number(season));
        cookie.setPath("/wettbewerbe/turnierserie");
        cookie.setHttpOnly(false);
        cookie.setSecure(false);
        QList<QNetworkCookie> cookies{cookie};
        request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookies));

        downloader->request(request, [=, &recomputeElo](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
            const QStringList tournamentPages = scrapeTournamentOverview(out);
            qDebug() << "Scraping" << tournamentPages.size() << "Tournaments from season" << season;

            for (const QString &page : tournamentPages) {
                const QString fullUrl = prepend(page, prefix);

                downloader->request(QNetworkRequest(QUrl(fullUrl)), [=, &recomputeElo](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
                    const QVector<Tournament> tournaments = scrapeTournamentPage(out);

                    for (const Tournament &tnm : tournaments) {
                        const QString fullTournamentUrl = prepend(tnm.url, prefix);
                        const int count = database->competitionGameCount(tnm.tfvbId, CompetitionType::Tournament);
                        if (count > 0) {
                            qDebug() << "Skipping" << tnm.tfvbId << fullTournamentUrl << "(from season" << season << "), has" << count << "matches already";
                            continue;
                        }

                        downloader->request(QNetworkRequest(fullTournamentUrl), [=, &recomputeElo](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
                            qDebug() << "Scraping" << tnm.tfvbId << fullTournamentUrl << "(from season" << season << ")";
                            recomputeElo |= scrapeTournament(database, tnm.tfvbId, tournamentSource, out);
                        });
                    }
                });
            }
        });
    }

    QObject::connect(downloader, &Downloader::completed, [&]() {
        static bool done = false;
        if (!done) {
            if (recomputeElo) {
                database->recompute();
            } else {
                qDebug() << "Not recomputing, since nothing changed";
            }
            app.quit();
            done = true;
        }
    });

    return app.exec();
}
