#include <QCoreApplication>
#include <QDebug>

#include "downloader.hpp"
#include "database.hpp"
#include "league.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Downloader downloader;
    Database database;

    const QString url = "https://tfvb.de/index.php/landesliga?task=begegnung_spielplan&veranstaltungid=25&teamid=332&id=1020";

    downloader.request(QNetworkRequest(QUrl(url)), [&](QNetworkReply::NetworkError err, GumboOutput *out) {
        scrapeLeageGame(&database, out);
    });

    return app.exec();
}
