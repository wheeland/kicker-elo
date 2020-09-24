#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "gumbo.h"

using DownloadCallback = std::function<void(QNetworkReply::NetworkError, GumboOutput*)>;

class Downloader : public QObject
{
    Q_OBJECT

public:
    Downloader(QObject *parent = nullptr);
    ~Downloader() = default;

    void request(const QNetworkRequest &request, const DownloadCallback &dcb);

private slots:
    void onReplyFinished();

private:
    void maybeStartDownloads();

    QNetworkAccessManager *m_manager;
    int m_maxDownloads = 10;

    struct PendingDownload
    {
        const QNetworkRequest request;
        const DownloadCallback callback;
    };
    QVector<PendingDownload> m_pendingDownloads;
    QHash<QNetworkReply*, DownloadCallback> m_activeDownloads;
};
