#include "kp.h"
#include "qjsonarray.h"
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
        //% "This program must be run in the terminal."
        qFatal(qtTrId("terminial-required").toUtf8());
        throw GetLastError();
    }
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        throw GetLastError();
    }
}
#endif

QByteArray KP::accessDenied() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AccessDenied;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::catbomb() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AllowClientFinish;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientAddEquip(int equipid) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::AdminAddEquip;
    result["equipid"] = equipid;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandEquipInfo() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandEquipInfo;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandGlobalTech() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandGlobalTech;
    return QCborValue::fromJsonValue(result).toCbor();

}

QByteArray KP::clientDevelop(int equipid, bool convert, int factoryID) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Develop;
    result["equipid"] = equipid;
    result["convert"] = convert;
    result["factory"] = factoryID;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientFactoryRefresh() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Refresh;
    result["view"] = GameState::Factory;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientFetch(int factoryID) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Fetch;
    result["factory"] = factoryID;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientHello() {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::CHello;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientStateChange(GameState state) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ChangeState;
    result["state"] = state;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientSteamAuth(uint8 rgubTicket [], uint32 cubTicket) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::SteamAuth;
    QJsonArray rgubArray = QJsonArray();
    for(unsigned int i = 0; i < cubTicket; ++i) {
        rgubArray.append(rgubTicket[i]);
    }
    result["rgubTicket"] = rgubArray;
    result["cubTicket"] = (qint64)cubTicket;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientSteamLogout() {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::SteamLogout;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverDevelopFailed(GameError error) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopFailed;
    result["reason"] = error;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverEquipLackFather(GameError error, int father) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopFailed;
    result["reason"] = error;
    result["father"] = father;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverDevelopStart() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopStart;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverEquipInfo(QJsonArray &input) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::EquipInfo;
    result["content"] = input;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverFairyBusy(int jobID) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::FairyBusy;
    result["job"] = jobID;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverGlobalTech(double tech) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::GlobalTechInfo;
    result["value"] = tech;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverGlobalTech(QList<std::tuple<int, int, double>> &content,
                                bool initial, bool final) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::GlobalTechInfo;
    result["initial"] = initial; // a true result will refresh the client table
    result["final"] = final; // a true result will trigger sorting in client
    QJsonArray contentJSON;
    for(auto &contentPart: content) {
        QJsonObject part;
        part["serial"] = std::get<0>(contentPart);
        part["def"] = std::get<1>(contentPart);
        part["weight"] = std::get<2>(contentPart);
        contentJSON.append(part);
    }
    result["content"] = contentJSON;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverHello() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::Hello;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLackPrivate() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::LackPrivate;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLogFail(KP::AuthFailType reason) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = AuthMode::NewLogin;
    result["success"] = false;
    result["reason"] = reason;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLogSuccess(bool newUser) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = AuthMode::NewLogin;
    result["success"] = true;
    result["newuser"] = newUser;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLogout(KP::LogoutType reason) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = AuthMode::Logout;
    result["reason"] = reason;
    result["success"] = (reason != KP::LogoutFailure);
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverNewEquip(int serial, int equipDid) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::NewEquip;
    result["serial"] = serial;
    result["equipdef"] = equipDid;
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

QByteArray KP::serverPenguin() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::Penguin;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverVerifyComplete() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::VerifyComplete;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::weighAnchor() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AllowClientStart;
    return QCborValue::fromJsonValue(result).toCbor();
}
