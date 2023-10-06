#include "steamauth.h"
#include "clientv2.h"
#include "networkerror.h"
#include <QtLogging>
#include <QtTranslation>
#include <QString>
#include <QTextStream>

void SteamAuth::RetrieveEncryptedAppTicket() {
    uint32 k_unSecretData = 0x5444;

    SteamAPICall_t hSteamAPICall = SteamUser()->RequestEncryptedAppTicket(
        &k_unSecretData, sizeof(k_unSecretData));
    m_EncryptedAppTicketResponseCallResult.Set(
        hSteamAPICall, this,
        &SteamAuth::OnEncryptedAppTicketResponse);
}

void SteamAuth::OnEncryptedAppTicketResponse(
    EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse,
    bool bIOFailure) {
    if(bIOFailure){
        qWarning(qtTrId("There has been an IO Failure when requesting the "
                        "Encrypted App Ticket.\n").toUtf8());
        return;
    }

    switch(pEncryptedAppTicketResponse->m_eResult)
    {
    case k_EResultOK:
    {
        uint8 rgubTicket[1024];
        uint32 cubTicket;

        if(SteamUser()->GetEncryptedAppTicket(rgubTicket, sizeof(rgubTicket), &cubTicket)) {
            try {
                Clientv2::getInstance().sendEncryptedAppTicket(rgubTicket, cubTicket);
            } catch (NetworkError e) {
                qCritical("Network error when sending Encrypted Ticket");
                qCritical(e.what());
            }
        }
        else {
            qWarning(qtTrId("GetEncryptedAppTicket failed.\n").toUtf8());
        }
    }
    break;
    case k_EResultNoConnection:
        qWarning(qtTrId("Calling RequestEncryptedAppTicket while not "
                        "connected to steam results in this error.\n").toUtf8());
        break;
    case k_EResultDuplicateRequest:
        qWarning(qtTrId("Calling RequestEncryptedAppTicket while there is "
                        "already a pending request results in this error.\n").toUtf8());
        break;
    case k_EResultLimitExceeded:
        qWarning(qtTrId("Calling RequestEncryptedAppTicket more than once per "
                        "minute returns this error.\n").toUtf8());
        break;
    default:
        qWarning(qtTrId("Calling RequestEncryptedAppTicket encountered "
                        "unknown error %1.\n")
                     .arg(pEncryptedAppTicketResponse->m_eResult).toUtf8());
        break;
    }
}