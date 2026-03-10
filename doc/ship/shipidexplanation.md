|                       Field                       |                           Meaning                            |
| :-----------------------------------------------: | :----------------------------------------------------------: |
|                    0xRR??????                     |                        Remodel level                         |
|               0x??1???00-0x??1???FF               |                           Japanese                           |
|               0x??2???00-0x??2???FF               |                            German                            |
|               0x??3???00-0x??3???9F               |                           Italian                            |
|               0x??3???A0-0x??3???AF               |                           Albanian                           |
|               0x??3???E0-0x??3???FF               |           Italian East African*->Eritrean/Somalian           |
|               0x??4???00-0x??4???EF               |                           American                           |
|               0x??4???F0-0x??4???FF               |                          Filipino*                           |
|               0x??5???00-0x??5???FF               |                           British                            |
|               0x??6???00-0x??6???DF               |                            French                            |
|               0x??6???E0-0x??6???EF               |          French Indochinese*->Vietnamese/Cambodian           |
|               0x??6???F0-0x??6???FF               |            (Reserved for Francophone countries)*             |
|               0x??7???00-0x??7???BF               |                        Soviet/Russian                        |
|               0x??7???C0-0x??7???DF               |       (Reserved for Ukrainian, only when independent)        |
|               0x??7???E0-0x??7???FF               | (Reserved for other non-Baltic post-Soviet states, only when independent) |
|               0x??8???00-0x??8???7F               |               Chinese (Nanjing/Taipei Regime)                |
|               0x??8???80-0x??8???FF               |                       Chinese (modern)                       |
|               0x??9???00-0x??9???7F               |                Dutch East Indies*->Indonesian                |
|               0x??9???80-0x??9???AF               |                            Dutch                             |
|               0x??9???B0-0x??9???BF               |                           Belgian                            |
|               0x??9???C0-0x??9???FF               |                          (Reserved)                          |
|               0x??A???00-0x??A???3F               |                           Swedish                            |
|               0x??A???40-0x??A???7F               |                            Danish                            |
|               0x??A???80-0x??A???AF               |                          Norwegian                           |
|               0x??A???B0-0x??A???BF               |                   (Reserved for Icelandic)                   |
|               0x??A???C0-0x??A???FF               |                           Finnish                            |
|               0x??B???00-0x??B???1F               |                          Australian                          |
|               0x??B???20-0x??B???2F               |                        New Zealander                         |
|               0x??B???30-0x??B???3F               | (Reserved for Papua New Guinean and other Oceanian countries) |
|               0x??B???40-0x??B???4F               |                 (Reserved for South African)                 |
|               0x??B???50-0x??B???5F               |                     (Reserved for Irish)                     |
|               0x??B???60-0x??B???6F               |                  (Reserved for Malaysian*)                   |
|               0x??B???70-0x??B???7F               |                 (Reserved for Singaporean*)                  |
|               0x??B???80-0x??B???9F               |                            Indian                            |
|               0x??B???A0-0x??B???AF               |                   (Reserved for Pakistani)                   |
|               0x??B???B0-0x??B???BF               |                  (Reserved for Bangladeshi)                  |
|               0x??B???C0-0x??B???DF               |                           Canadian                           |
|               0x??B???E0-0x??B???FF               |         (Reserved for other Commonwealth countries)          |
|               0x??C???00-0x??C???1F               |                    (Reserved for Spanish)                    |
|               0x??C???20-0x??C???3F               |                  (Reserved for Portuguese)                   |
|               0x??C???40-0x??C???5F               |                          Brazilian                           |
|               0x??C???60-0x??C???7F               |                         Argentinian                          |
|               0x??C???80-0x??C???8F               |                           Peruvian                           |
|               0x??C???90-0x??C???9F               |                           Chilean                            |
|               0x??C???A0-0x??C???BF               |                    (Reserved for Mexican)                    |
|               0x??C???C0-0x??C???CF               |              (Reserved for Columbian/Ecuadoran)              |
|               0x??C???D0-0x??C???DF               |                  (Reserved for Venezuelan)                   |
|               0x??C???E0-0x??C???EF               |                     (Reserved for Cuban)                     |
|               0x??C???F0-0x??C???FF               |        (Reserved for other Latin American countries)         |
|               0x??D???00-0x??D???1F               |                         Yugoslavian                          |
|               0x??D???20-0x??D???3F               |                            Polish                            |
|               0x??D???40-0x??D???5F               |                          Bulgarian                           |
|               0x??D???60-0x??D???7F               |                 Greek/(Reserved for Cypriot)                 |
|               0x??D???80-0x??D???9F               |                           Romanian                           |
|               0x??D???A0-0x??D???BF               |                           Turkish                            |
|               0x??D???C0-0x??D???CF               |             Baltic states (ony when independent)             |
|               0x??D???E0-0x??D???EF               |                           Israeli                            |
| 0x??D???D0-0x??D???DF,<br />0x??D???F0-0x??D???FF |           (Reserved for other European countries)            |
|               0x??E???00-0x??E???1F               |                             Thai                             |
|               0x??E???20-0x??E???2F               |                    (Reserved for Iranian)                    |
|               0x??E???40-0x??E???7F               |               (Reserved for Arabic countries)                |
|               0x??E???80-0x??E???9F               |                 (Reserved for South Korean)                  |
|               0x??E???A0-0x??E???BF               |                 (Reserved for North Korean)                  |
|               0x??E???C0-0x??E???FF               |                          (Reserved)                          |
|                    0x??F?????                     |                        Fantasy ships                         |
|                    0x???10???                     |                           Escorts                            |
|                    0x???11???                     |            Escort Destroyers (have base torpedo)             |
|                    0x???20???                     |                          Destroyers                          |
|                    0x???21???                     |  Destroyers with Daihatsu capabilities (bitwise operation)   |
|                    0x???22???                     | Destroyers with Amphibious tank capabilities (bitwise operation) |
|                    0x???24???                     |    Destroyers with Bulge capabilities (bitwise operation)    |
|                    0x???28???                     |            Leading destroyer (bitwise operation)             |
|                    0x???30???                     |                        Light cruisers                        |
|                    0x???31???                     |                      Training cruisers                       |
|                    0x???32???                     | Light cruisers with advanced torpedo capabilities (bitwise operation) |
|                    0x???34???                     |        Light (Aviation) cruisers (bitwise operation)         |
|                    0x???35???                     | Submarine tenders (count as both training cruiser and light aviation cruiser) |
|                    0x???38???                     |                  Light (Anti-air) cruisers                   |
|                    0x???40???                     |                        Heavy cruisers                        |
|                    0x???42???                     | Heavy cruisers with advanced torpedo capabilities (bitwise operation) |
|                    0x???44???                     |        Heavy (Aviation) cruisers (bitwise operation)         |
|                    0x???48???                     |                  Heavy (Anti-air) cruisers                   |
|                    0x???50???                     |                         Battleships                          |
|                    0x???51???                     |              Battlecruisers (bitwise operation)              |
|                    0x???52???                     |         Battleships (high-speed) (bitwise operation)         |
|                    0x???54???                     |           Aviation battleships (bitwise operation)           |
|                    0x???58???                     |            Super battleships (bitwise operation)             |
|                    0x???60???                     |                           Carriers                           |
|                    0x???61???                     |              Light carriers (bitwise operation)              |
|                    0x???62???                     | Carriers with advanced anti-sub capabilities (bitwise operation) |
|                    0x???63???                     |      Escort carriers (count as both light and anti-sub)      |
|                    0x???64???                     |             Armored Carriers (bitwise operation)             |
|                    0x???68???                     | Carriers with night aviation capabilities (bitwise operation) |
|                    0x???70???                     |                          Submarines                          |
|                    0x???74???                     |             Submarine with aviation capabilities             |
|                    0x???80???                     |                      Seaplane carriers                       |
|                    0x???82???                     |     Seaplane carriers with advanced torpedo capabilities     |
|                    0x???90???                     |                         Supply ships                         |
|                    0x???A0???                     |                   Amphibious assault ships                   |
|                    0x???B0???                     |                         Repair ships                         |
|                    0x???C0???                     |                        Land Structure                        |
|                    0x?????S??                     |                          Ship class                          |
|                    0x??????S?                     |                       Ship subclasses                        |
|                    0x??????ID                     |                           Ship id                            |

(*) Ships that were primarily deployed in colonies have their nationality count as that of the colony.