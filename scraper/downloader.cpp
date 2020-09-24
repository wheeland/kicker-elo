#include "downloader.hpp"

#include <QNetworkReply>

Downloader::Downloader(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

void Downloader::request(const QNetworkRequest &request, const DownloadCallback &cb)
{
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &Downloader::onReplyFinished);
    m_downloads[reply] = cb;
}

void Downloader::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    const auto it = m_downloads.find(reply);
    if (it == m_downloads.end())
        return;

    QNetworkReply::NetworkError error = reply->error();
    const QByteArray data = reply->readAll();

    GumboOutput* output = gumbo_parse(data.data());

    (*it)(error, output);
    m_downloads.erase(it);

    gumbo_destroy_output(&kGumboDefaultOptions, output);
}
