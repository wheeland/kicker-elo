#include "downloader.hpp"

#include <QNetworkReply>

Downloader::Downloader(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

void Downloader::maybeStartDownloads()
{
    while (m_activeDownloads.size() < m_maxDownloads && m_pendingDownloads.size() > 0) {
        const PendingDownload pending = m_pendingDownloads.takeFirst();

        QNetworkReply *reply = m_manager->get(pending.request);
        connect(reply, &QNetworkReply::finished, this, &Downloader::onReplyFinished);
        m_activeDownloads[reply] = pending.callback;
    }
}

void Downloader::request(const QNetworkRequest &request, const DownloadCallback &cb)
{
    m_pendingDownloads << PendingDownload{request, cb};
    maybeStartDownloads();
}

void Downloader::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    const auto it = m_activeDownloads.find(reply);
    if (it == m_activeDownloads.end())
        return;

    const DownloadCallback cb = *it;
    m_activeDownloads.erase(it);

    maybeStartDownloads();

    const QNetworkReply::NetworkError error = reply->error();
    const QByteArray data = reply->readAll();

    GumboOutput* output = gumbo_parse(data.data());
    cb(error, output);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}
