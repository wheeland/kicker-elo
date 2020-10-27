#include <QCoreApplication>
#include <QCommandLineParser>
#include <QNetworkCookie>
#include <QFile>
#include <QDebug>

#include "downloader.hpp"
#include "database.hpp"
#include "league.hpp"
#include "tournament.hpp"

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
    QCommandLineOption tournamentSeasonOption({"tournament-season", "t"}, "Season to query for tournaments (1-15)", "15");
    parser.addOption(tournamentSeasonOption);
    parser.process(app);
    if (parser.positionalArguments().isEmpty())
        parser.showHelp();

    const QString sqlitePath = parser.positionalArguments().first();

    Downloader *downloader = new Downloader();
    Database *database = new Database(sqlitePath);

    //
    // Parse and scrape league source files
    //
    const QStringList leagueSourceFiles = parser.values(leagueSourcesOption);
    const QStringList leagueSources = readSourceFiles(leagueSourceFiles);
    qDebug() << "Scraping" << leagueSources.size() << "League URLs";

    for (const QString &source : leagueSources) {
        const QUrl url(source);
        const QString prefix = url.scheme() + "://" + url.host();

        const auto cb = [=] (QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
            const QVector<LeagueGame> games = scrapeLeagueSeason(out);
            qDebug() << "Scraping" << games.size() << "games from League URL" << source;

            for (const LeagueGame &game : games) {
                const int count = database->competitionGameCount(game.tfvbId, CompetitionType::League);

                QString fullUrl = game.url;
                if (!fullUrl.startsWith(prefix))
                    fullUrl = prefix + fullUrl;

                if (count > 0) {
                    qDebug() << "Skipping" << game.tfvbId << fullUrl << "(from" << source << "): has" << count << "matches already";
                    continue;
                }

                downloader->request(QNetworkRequest(fullUrl), [=](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
                    qDebug() << "Scraping" << game.tfvbId << fullUrl << "(from" << source << ")";
                    scrapeLeageGame(database, game.tfvbId, out);
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
    const int season = parser.value(tournamentSeasonOption).toInt();
    if (season > 0) {
        QNetworkRequest request(QUrl("https://tfvb.de/index.php/turniere"));
        QNetworkCookie cookie("sportsmanager_filter_saison_id", QByteArray::number(season));
        QList<QNetworkCookie> cookies{cookie};
        request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookies));

        downloader->request(request, [=](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
            const QStringList tournamentPages = scrapeTournamentOverview(out);
            qDebug() << "Scraping" << tournamentPages.size() << "Tournaments from season" << season;

            for (const QString &page : tournamentPages) {
                downloader->request(QNetworkRequest(QUrl(page)), [=](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
                    const QVector<Tournament> tournaments = scrapeTournamentPage(out);

                    for (const Tournament &tnm : tournaments) {
                        const int count = database->competitionGameCount(tnm.tfvbId, CompetitionType::Tournament);
                        if (count > 0) {
                            qDebug() << "Skipping" << tnm.tfvbId << tnm.url << "(from season" << season << "), has" << count << "matches already";
                            continue;
                        }

                        downloader->request(QNetworkRequest(tnm.url), [=](QNetworkReply::NetworkError /*err*/, GumboOutput *out) {
                            qDebug() << "Scraping" << tnm.tfvbId << tnm.url << "(from season" << season << ")";
                            scrapeTournament(database, tnm.tfvbId, out);
                        });
                    }
                });
            }
        });
    }

    QObject::connect(downloader, &Downloader::completed, [&]() {
        database->recompute();
        app.quit();
    });

    return app.exec();
}
