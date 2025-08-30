#ifndef STEAMAUTH_H
#define STEAMAUTH_H

#include "../steam/isteamuser.h"
#include <QObject>

class SteamAuth : public QObject {
    Q_OBJECT

public:
    void RetrieveEncryptedAppTicket();

signals:
    void eATFailed();

private:
    void OnEncryptedAppTicketResponse(
        EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse,
        bool bIOFailure);
    CCallResult<SteamAuth, EncryptedAppTicketResponse_t>
        m_EncryptedAppTicketResponseCallResult;
};

#endif // STEAMAUTH_H
