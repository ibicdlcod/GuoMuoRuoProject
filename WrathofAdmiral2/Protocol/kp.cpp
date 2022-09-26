#include "kp.h"
#include <QFile>
#include <QSettings>

extern QFile *logFile;
extern std::unique_ptr<QSettings> settings;

void KP::initLog(bool server) {
    QString logFileName;
    if(server) {
        logFileName = settings->value("server/logfile",
                                      "ServerLog.log").toString();
    }
    else {
        logFileName = settings->value("client/logfile",
                                      "ClientLog.log").toString();
    }
    logFile = new QFile(logFileName);
    if(Q_UNLIKELY(!logFile)
            || !logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qFatal("Log file cannot be opened");
    }
}

#if defined (Q_OS_WIN)
void KP::winConsoleCheck() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        throw GetLastError();
    }
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        throw GetLastError();
    }
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        throw GetLastError();
    }
}
#endif

QByteArray KP::clientAuth(AuthMode mode, const QString &uname,
                          const QByteArray &shadow) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = mode;
    if(mode != AuthMode::Logout) {
        result["username"] = uname;
        /* directly using QString is even less efficient */
        result["shadow"] =
                QString(shadow.toBase64(QByteArray::Base64Encoding));
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverAuth(AuthMode mode, const QString &uname,
                          bool success, AuthError reason) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = mode;
    result["username"] = uname;
    result["success"] = success;
    result["reason"] = reason;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverAuth(AuthMode mode, const QString &uname,
                          bool success, AuthError reason,
                          QDateTime reEnable) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = mode;
    result["username"] = uname;
    result["success"] = success;
    result["reason"] = reason;
    result["reenable"] = reEnable.toString();
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverAuth(AuthMode mode, const QString &uname,
                          bool success) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = mode;
    result["username"] = uname;
    result["success"] = success;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverParseError(MsgType pe, const QString &uname,
                                const QString &content) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = pe;
    result["username"] = uname;
    result["content"] = content;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDevelop(int equipid, bool convert) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Develop;
    result["equipid"] = equipid;
    result["convert"] = convert;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::accessDenied() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AccessDenied;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverDevelopFailed(bool ruleBased) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopFailed;
    result["rulebased"] = ruleBased;
    return QCborValue::fromJsonValue(result).toCbor();
}