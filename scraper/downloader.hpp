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
    QNetworkAccessManager *m_manager;
    QHash<QNetworkReply*, DownloadCallback> m_downloads;
};
