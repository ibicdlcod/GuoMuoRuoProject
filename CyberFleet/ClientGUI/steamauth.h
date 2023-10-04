#ifndef STEAMAUTH_H
#define STEAMAUTH_H

#include "../steam/isteamuser.h"

class SteamAuth {
public:
    void RetrieveEncryptedAppTicket();

private:
    void OnEncryptedAppTicketResponse(
        EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse,
        bool bIOFailure);
    CCallResult<SteamAuth, EncryptedAppTicketResponse_t>
        m_EncryptedAppTicketResponseCallResult;
};

#endif // STEAMAUTH_H
