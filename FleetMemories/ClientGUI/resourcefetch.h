/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef RESOURCEFETCH_H
#define RESOURCEFETCH_H

#include <QNetworkAccessManager>
#include <QUrl>
#include <QObject>
#include <QFile>

class ResourceFetch : public QObject
{
    Q_OBJECT
public:
    explicit ResourceFetch(QObject *parent = nullptr);
    ~ResourceFetch();

    void startRequest(const QUrl &requestedUrl);

signals:
    void finished();

public slots:
    void cancelDownload();
    void downloadFile(const QString &urlSpec =
                      "https://tsunkit.net/api/assets/images/equipTypeIcons/1",
                      const QString &fileInput = "1",
                       const QString &directory = "TsunkitMode/equipTypeIcons/");
    void httpFinished();
    void httpReadyRead();
    void networkReplyProgress(qint64 bytesRead, qint64 totalBytes);
    void sslErrors(const QList<QSslError> &errors);

private:
    std::unique_ptr<QFile> openFileForWrite(const QString &fileName);

    QUrl url;
    QNetworkAccessManager qnam;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply;
    std::unique_ptr<QFile> file;
    bool httpRequestAborted = false;
};

#endif // RESOURCEFETCH_H
