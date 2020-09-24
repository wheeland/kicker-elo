#include <QCoreApplication>
#include <QDebug>

#include "downloader.hpp"
#include "database.hpp"
#include "league.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Downloader downloader;
    Database database("foo.sqlite");

//    const QString url = "https://tfvb.de/index.php/landesliga?task=begegnung_spielplan&veranstaltungid=25&teamid=332&id=1020";
    const QString url = "https://tfvb.de/index.php/landesliga?task=begegnung_spielplan&veranstaltungid=25&id=1028";

    downloader.request(QNetworkRequest(QUrl(url)), [&](QNetworkReply::NetworkError err, GumboOutput *out) {
        scrapeLeageGame(&database, 1028, out);
    });

    return app.exec();
}
