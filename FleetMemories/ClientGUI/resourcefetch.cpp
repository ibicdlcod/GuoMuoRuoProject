#include "resourcefetch.h"
#include <QNetworkReply>
#include <QFileInfo>
#include <QDir>
#include <QTimer>

ResourceFetch::ResourceFetch(QObject *parent)
    : QObject{parent}
{

}

ResourceFetch::~ResourceFetch() = default;

void ResourceFetch::networkReplyProgress(qint64 bytesRead, qint64 totalBytes)
{
    //% "Read %1 of %2 bytes"
    qInfo() << qtTrId("bytes-read-total").arg(bytesRead).arg(totalBytes);
}

void ResourceFetch::startRequest(const QUrl &requestedUrl)
{
    url = requestedUrl;
    httpRequestAborted = false;

    //! [qnam-download]
    reply.reset(qnam.get(QNetworkRequest(url)));
    //! [qnam-download]
    //! [connecting-reply-to-slots]
    connect(reply.get(), &QNetworkReply::finished, this, &ResourceFetch::httpFinished);
    //! [networkreply-readyread-1]
    connect(reply.get(), &QIODevice::readyRead, this, &ResourceFetch::httpReadyRead);
    //! [networkreply-readyread-1]
#if QT_CONFIG(ssl)
    //! [sslerrors-1]
    connect(reply.get(), &QNetworkReply::sslErrors, this, &ResourceFetch::sslErrors);
    //! [sslerrors-1]
#endif
    //! [connecting-reply-to-slots]

    connect(reply.get(), &QNetworkReply::downloadProgress,
            this, &ResourceFetch::networkReplyProgress);
}

void ResourceFetch::downloadFile(const QString &urlSpec,
                                 const QString &fileInput,
                                 const QString &directory)
{
    if (urlSpec.isEmpty())
        return;

    const QUrl newUrl = QUrl::fromUserInput(urlSpec);
    if (!newUrl.isValid()) {
        //% "Invalid URL: %1: %2"
        qCritical() << qtTrId("url-invalid").arg(urlSpec, newUrl.errorString());
        return;
    }

    QString fileName = fileInput;
    QString downloadDirectory = directory;
    if(!QDir(directory).exists()) {
        QDir().mkdir(directory);
    }
    bool useDirectory = !downloadDirectory.isEmpty() && QFileInfo(downloadDirectory).isDir();
    if (useDirectory)
        fileName.prepend(downloadDirectory + '/');

    if(QFile::exists(fileName)) {
        //% "File %1 exists."
        qDebug() << qtTrId("file-exists").arg(fileName);
        QTimer::singleShot(1, this, [this]{emit finished();});
        return;
    }
    file = openFileForWrite(fileName);
    if (!file)
        return;

    // schedule the request
    startRequest(newUrl);
}

std::unique_ptr<QFile> ResourceFetch::openFileForWrite(const QString &fileName)
{
    std::unique_ptr<QFile> file = std::make_unique<QFile>(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        //% "Unable to save the file %1: %2."
        qCritical() << qtTrId("unable-to-save-file")
                           .arg(QDir::toNativeSeparators(fileName),
                                file->errorString());
        return nullptr;
    }
    return file;
}

void ResourceFetch::cancelDownload()
{
    httpRequestAborted = true;
    reply->abort();
}

void ResourceFetch::httpFinished()
{
    QFileInfo fi;
    if (file) {
        fi.setFile(file->fileName());
        file->close();
        file.reset();
    }

    //! [networkreply-error-handling-1]
    QNetworkReply::NetworkError error = reply->error();
    const QString &errorString = reply->errorString();
    //! [networkreply-error-handling-1]
    reply.reset();
    //! [networkreply-error-handling-2]
    if (error != QNetworkReply::NoError) {
        QFile::remove(fi.absoluteFilePath());
        // For "request aborted" we handle the label and button in cancelDownload()
        if (!httpRequestAborted) {
            //% "Download failed: %1"
            qCritical() << qtTrId("download-failed").arg(errorString);
        }
        return;
    }
    //! [networkreply-error-handling-2]

    //% "Downloaded %1 bytes to %2 in %3"
    qInfo() << qtTrId("download-success")
                   .arg(fi.size())
                   .arg(fi.fileName(), QDir::toNativeSeparators(fi.absolutePath()));
    emit finished();
}

//! [networkreply-readyread-2]
void ResourceFetch::httpReadyRead()
{
    // This slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file)
        file->write(reply->readAll());
}
//! [networkreply-readyread-2]

#if QT_CONFIG(ssl)
//! [sslerrors-2]
void ResourceFetch::sslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    //% "One or more TLS errors has occurred: %1"
    qWarning() << qtTrId("ssl-errors").arg(errorString);
}
//! [sslerrors-2]
#endif
