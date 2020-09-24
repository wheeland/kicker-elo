#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>

#include "downloader.hpp"
#include "database.hpp"
#include "league.hpp"

QStringList readSourcesFile(const QString &path)
{
    QStringList ret;

    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        while (!file.atEnd()) {
            const QString line = QString::fromUtf8(file.readLine()).trimmed();
            if (!line.isEmpty())
                ret << line;
        }
    }

    return ret;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("tfvb-scraper");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Scrapes delicious foos data off TFVB");
    parser.addHelpOption();
    parser.addPositionalArgument("sqlite", "Path to SQLite database");
    QCommandLineOption leagueSourcesOption({"league-sources", "l"}, "Sources for league games", "path");
    parser.addOption(leagueSourcesOption);
    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        parser.showHelp();
    }

    const QString sqlitePath = parser.positionalArguments().first();

    Downloader downloader;
    Database database(sqlitePath);

    if (parser.isSet(leagueSourcesOption)) {
        const QStringList leagueSources = readSourcesFile(parser.value(leagueSourcesOption));
        qDebug() << "Scraping" << leagueSources.size() << "League URLs";

        for (const QString &source : leagueSources) {
            const auto cb = [&,source] (QNetworkReply::NetworkError err, GumboOutput *out) {
                const QVector<LeagueGame> games = scrapeLeagueSeason(&database, out);
                qDebug() << "Scraping" << games.size() << "games from League URL" << source;

                for (const LeagueGame &game : scrapeLeagueSeason(&database, out)) {
                    const int count = database.competitionGameCount(game.tfvbId, false);
                    if (count > 0) {
                        qDebug() << "Skipping" << game.tfvbId << game.url << "(from" << source << "): has" << count << "games";
                        continue;
                    }

                    downloader.request(QNetworkRequest(game.url), [&,source,game](QNetworkReply::NetworkError err, GumboOutput *out) {
                        qDebug() << "Scraping" << game.tfvbId << game.url << "(from" << source << ")";
                        scrapeLeageGame(&database, game.tfvbId, out);
                    });
                }
            };

            downloader.request(QNetworkRequest(source), cb);
        }
    }

    QString urrl = "https://tfvb.de/index.php/kreisliga?task=begegnung_spielplan&veranstaltungid=130&id=8276";
    downloader.request(QNetworkRequest(urrl), [&](QNetworkReply::NetworkError err, GumboOutput *out) {
        scrapeLeageGame(&database, 8276, out);
    });


    QObject::connect(&downloader, &Downloader::completed, &app, &QCoreApplication::quit);

    return app.exec();
}
