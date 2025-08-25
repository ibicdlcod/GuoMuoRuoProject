#include "steamauth.h"
#include <QtLogging>
#include <QtTranslation>
#include <QString>
#include <QTextStream>
#include "clientv2.h"
#include "../steam/isteamfriends.h"

extern std::unique_ptr<QSettings> settings;

/* Part of Steam Authentication */
void SteamAuth::RetrieveEncryptedAppTicket() {
#pragma message(NOT_M_CONST)
    uint32 k_unSecretData = 0x5444;

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
        //% "There has been an IO Failure when requesting the Encrypted App Ticket.\n"
        qWarning() << qtTrId("steam-bIOFailure").toUtf8();
        emit eATFailed();
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
            //% "GetEncryptedAppTicket success!"
            qDebug() << qtTrId("appticket-success");
            Clientv2::getInstance().sendEncryptedAppTicket(rgubTicket, cubTicket);
        }
        else {
            //% "GetEncryptedAppTicket failed!"
            qWarning() << qtTrId("appticket-failure");
            emit eATFailed();
        }
    }
    break;
    case k_EResultNoConnection:
        //% "Calling RequestEncryptedAppTicket while not connected to steam results in this error."
        qWarning() << qtTrId("k_EResultNoConnection");
        qInfo() << SteamFriends()->GetPersonaName();
        emit eATFailed();
        break;
    case k_EResultDuplicateRequest:
        //% "Calling RequestEncryptedAppTicket while there is already a pending request results in this error."
        qWarning() << qtTrId("k_EResultDuplicateRequest");
        emit eATFailed();
        break;
    case k_EResultLimitExceeded:
        //% "Calling RequestEncryptedAppTicket more than once per minute returns this error."
        qWarning() << qtTrId("k_EResultLimitExceeded");
        emit eATFailed();
        break;
    default:
        //% "Calling RequestEncryptedAppTicket encountered unknown error %1."
        qWarning() << qtTrId("request-app-ticket-fail-unknown")
                          .arg(pEncryptedAppTicketResponse->m_eResult);
        emit eATFailed();
        break;
    }
}
/* End Part of Steam Authentication */
