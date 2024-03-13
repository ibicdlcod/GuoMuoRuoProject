#include "steamauth.h"
#include "clientv2.h"
#include <QtLogging>
#include <QtTranslation>
#include <QString>
#include <QTextStream>

extern std::unique_ptr<QSettings> settings;

/* Part of Steam Authentication */
void SteamAuth::RetrieveEncryptedAppTicket() {
#pragma message(NOT_M_CONST)
    uint32 k_unSecretData = 0x5444;
    int steamRateLimit = 60;

    if(settings->contains("networkclient/requestEATCall")
        && settings->value("networkclient/requestEATCall").toDateTime()
                   .secsTo(QDateTime::currentDateTimeUtc()) < steamRateLimit)
    {
        //% "Steam API 'RequestEncryptedAppTicket' is subject to a 60 second rate limit."
        qWarning() << qtTrId("steam-60-sec");
    }

    settings->setValue("networkclient/requestEATCall", QDateTime::currentDateTimeUtc());
    SteamAPICall_t hSteamAPICall = SteamUser()->RequestEncryptedAppTicket(
        &k_unSecretData, sizeof(k_unSecretData));
    m_EncryptedAppTicketResponseCallResult.Set(
        hSteamAPICall, this,
        &SteamAuth::OnEncryptedAppTicketResponse);
}

void SteamAuth::OnEncryptedAppTicketResponse(
    EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse,
    bool bIOFailure) {
    if(bIOFailure) {
        qWarning() << qtTrId("There has been an IO Failure when requesting the "
                             "Encrypted App Ticket.\n").toUtf8();
        return;
    }

    switch(pEncryptedAppTicketResponse->m_eResult)
    {
    case k_EResultOK:
    {
/* better not to modify it */
#pragma message(NOT_M_CONST)
        uint8 rgubTicket[1024];
        uint32 cubTicket;

        if(SteamUser()->GetEncryptedAppTicket(rgubTicket, sizeof(rgubTicket), &cubTicket)) {
            qDebug() << "GetEncryptedAppTicket success!";
            Clientv2::getInstance().sendEncryptedAppTicket(rgubTicket, cubTicket);
        }
        else {
            qWarning() << qtTrId("GetEncryptedAppTicket failed.\n").toUtf8();
        }
    }
    break;
    case k_EResultNoConnection:
        qWarning() << qtTrId("Calling RequestEncryptedAppTicket while not "
                             "connected to steam results in this error.\n").toUtf8();
        break;
    case k_EResultDuplicateRequest:
        qWarning() << qtTrId("Calling RequestEncryptedAppTicket while there is "
                             "already a pending request results in this error.\n").toUtf8();
        break;
    case k_EResultLimitExceeded:
        qWarning() << qtTrId("Calling RequestEncryptedAppTicket more than once per "
                             "minute returns this error.\n").toUtf8();
        break;
    default:
        qWarning() << qtTrId("Calling RequestEncryptedAppTicket encountered "
                             "unknown error %1.\n")
                          .arg(pEncryptedAppTicketResponse->m_eResult).toUtf8();
        break;
    }
}
/* End Part of Steam Authentication */
