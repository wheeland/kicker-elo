#include <QCoreApplication>
#include <QCommandLineParser>
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
                if (!line.isEmpty())
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
    parser.process(app);
    if (parser.positionalArguments().isEmpty())
        parser.showHelp();

    const QString sqlitePath = parser.positionalArguments().first();

    Downloader downloader;
    Database database(sqlitePath);

    //
    // Parse and scrape league source files
    //
    const QStringList leagueSourceFiles = parser.values(leagueSourcesOption);
    const QStringList leagueSources = readSourceFiles(leagueSourceFiles);
    qDebug() << "Scraping" << leagueSources.size() << "League URLs";

//    for (const QString &source : leagueSources) {
//        const auto cb = [&,source] (QNetworkReply::NetworkError err, GumboOutput *out) {
//            const QVector<LeagueGame> games = scrapeLeagueSeason(&database, out);
//            qDebug() << "Scraping" << games.size() << "games from League URL" << source;

//            for (const LeagueGame &game : scrapeLeagueSeason(&database, out)) {
//                const int count = database.competitionGameCount(game.tfvbId, false);
//                if (count > 0) {
//                    qDebug() << "Skipping" << game.tfvbId << game.url << "(from" << source << "): has" << count << "matches already";
//                    continue;
//                }

//                downloader.request(QNetworkRequest(game.url), [&,source,game](QNetworkReply::NetworkError err, GumboOutput *out) {
//                    qDebug() << "Scraping" << game.tfvbId << game.url << "(from" << source << ")";
//                    scrapeLeageGame(&database, game.tfvbId, out);
//                });
//            }
//        };

//        downloader.request(QNetworkRequest(source), cb);
//    }

    const auto url1 = "https://tfvb.de/index.php/turniere?task=turnierdisziplin&id=539";
    const auto url2 = "https://tfvb.de/index.php/turniere?task=turnierdisziplin&id=219";
    const auto url3 = "https://tfvb.de/index.php/turniere?task=turnierdisziplinen&turnierid=63";
    // https://tfvb.de/index.php/turniere?task=turnierdisziplin&id=219
    downloader.request(QNetworkRequest(QUrl(url3)), [&](QNetworkReply::NetworkError err, GumboOutput *out) {
        scrapeTournamentPage(out);
    });

    QObject::connect(&downloader, &Downloader::completed, &app, &QCoreApplication::quit);

    return app.exec();
}
