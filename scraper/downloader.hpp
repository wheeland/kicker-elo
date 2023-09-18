#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <functional>

#include "gumbo.h"

using DownloadCallback = std::function<void(QNetworkReply::NetworkError, GumboOutput*)>;

class Downloader : public QObject
{
    Q_OBJECT

public:
    Downloader(QObject *parent = nullptr);
    ~Downloader() = default;

    void request(const QNetworkRequest &request, const DownloadCallback &dcb);

signals:
    void completed();

private slots:
    void onReplyFinished();
    void maybeStartDownloads();

private:
    QNetworkAccessManager *m_manager;
    int m_maxDownloads = 10;

    struct PendingDownload
    {
        QNetworkRequest request;
        DownloadCallback callback;
    };
    QVector<PendingDownload> m_pendingDownloads;
    QHash<QNetworkReply*, DownloadCallback> m_activeDownloads;
};
