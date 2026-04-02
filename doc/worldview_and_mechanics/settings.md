|                Field                 |               Default               |                             Note                             |
| :----------------------------------: | :---------------------------------: | :----------------------------------------------------------: |
|               alias/*                |                                     |                     created by the user                      |
|           client/language            |                ja_JP                |                  default value is temporary                  |
|            client/logfile            |            ClientLog.log            |                                                              |
|      client/licenseareapersist       |                5000                 |                       in milliseconds                        |
|       client/equipdbtimestamp        |               dynamic               |                                                              |
|         client/equipdbcache          |               dynamic               |                                                              |
|        client/shipdbtimestamp        |               dynamic               |                                                              |
|          client/shipdbcache          |               dynamic               |                                                              |
|        client/mapdbtimestamp         |               dynamic               |                                                              |
|          client/mapdbcache           |               dynamic               |                                                              |
|     networkclient/retransmitmax      |                  2                  |                    Client retransmit time                    |
|  networkclient/connectwaittimemsec   |                8000                 |                                                              |
|  networkclient/downloadwaittimemsec  |                80000                |                                                              |
|          networkclient/pem           |           :/harusoft.pem            |                           Embedded                           |
|    networkclient/autopasswordtime    |                1000                 |                                                              |
|     networkclient/requestEATCall     | datetime at last successful attempt |                                                              |
|    networkshared/maxmsgdelayinms     |                1000                 |                                                              |
| networkshared/mintimebetweenmsgsinms |                  0                  |                                                              |
|          networkserver/pem           |           :/harusoft.pem            |                                                              |
|          networkserver/key           |          serverprivate.key          |                                                              |
|          msg_disabled/debug          |                false                |                    ignored in releasemode                    |
|          msg_disabled/info           |                false                |                                                              |
|          msg_disabled/warn           |                false                |                                                              |
|          msg_disabled/crit           |                false                |                                                              |
|          msg_disabled/fatal          |                false                |                                                              |
|           rule/devresscale           |                 10                  | the more this value, the more resources and time is required to develop equipment |
|           rule/devtimebase           |                  6                  | the more this value, the more time is required to develop equipment |
|        rule/skillpointfactor         |                1.25                 | the more this value, the more tech affects an equipment's standard skill points |
|         rule/skillpointbase          |               10000.0               | the more this value, the more any equipment's standard skill points |
|       rule/techcombinedeffects       |                 3.0                 | maximum deterimental effects of inferior local/global technology on global/local technology |
|         rule/techglobalscope         |                1.02                 | controls the decay speed when calculating global tech level. The closer this is to 1, the more tech components is required to keep the overall tech level high. |
|         rule/techlocalscope          |                 1.1                 |           same as above, but applys to local tech            |
|       rule/shiplevelperweight        |                10.0                 | the higher this value, the less weight ship levels contribute to ship tech |
|     rule/skillpointweightcontrib     |                 9.0                 | the higher this value, the more weight equip skill points contribute to equip tech |
|  rule/penguinskillpointsdifficulty   |                10.0                 | the higher this value, the more difficult to accumulate skill points by failing to develop an equipment |
|     rule/maxskillpointsamplifier     |                 5.0                 | the higher this value, the more mother skill point requirement of son equipment |
|    rule/normalproductionstockpile    |                30.0                 |            normal possessing limit for equipments            |
|         rule/antiregenpower          |                 4.0                 | the higher this value, the less global tech applys to extra resource natural regeneration |
|         rule/baseregennormal         |                 10                  |         regenerate speed for oil, explosives, steel          |
|        rule/baseregenaluminum        |                  5                  |                regenerate speed for aluminum                 |
|          rule/baseregenrare          |                  2                  |      regenerate speed for rubber, tungsten and chromium      |
|         rule/regencapnormal          |                2500                 |        base regenerate cap for oil, explosives, steel        |
|        rule/regencapaluminum         |                2000                 |               base regenerate cap for aluminum               |
|          rule/regencaprare           |                1500                 |    base regenerate cap for rubber, tungsten and chromium     |
|          rule/regenpertech           |                 8.0                 |               regenerate factor per tech level               |
|          rule/regenattech0           |                 24                  |                 regenerate factor at tech 0                  |
|          rule/motherspscale          |                 0.2                 | the higher this value, the more son equipment's tech matters in determining mother skill point requirement of son equipment |
|          rule/maxresources           |               3600000               |                   max resources stockpile                    |
|          rule/sigmaconstant          |                 2.0                 | the larger this value, the less effect tech has on development/construction success rate |
|      rule/equipmentstandardstar      |                 10                  | the larger this value, the less effective improving equipment does |
|          rule/shipexpscale           |                 100                 |     the larger this value, the harder ship can level up      |
|       rule/navalsupremacydecay       |                2880                 | the larger this value, the easier it is to maintain naval supermacy in maps |
|           rule/loscontrol            |                 0.9                 | geometric decay factor for fleet LoS: the nth highest-LoS ship contributes a^(n-1) × its LoS value; lower values make top-LoS ships dominate more |
|      rule/techfactorcontroller       |                  5                  | the larger this value, the ship construction time and repair time/resources is shorter/fewer |
|       rule/badconditionpenalty       |                1.001                | the larger this value, the faster your ship exp decreases when any of them is in negative condition |
|       rule/mapresourcecontrol        |                1000                 | the larger this value, the harder gaining naval supremacy in coastal maps would affect resource gain |
|       rule/waivemotherconditon       |                0.99                 | the lower this value, the lower mother skill points is required when naval supremacy in appropriate map is high |
|            server/logfile            |            ServerLog.log            |                                                              |
|           server/language            |                en_US                |                                                              |
|      server/displaypromptdelay       |                 100                 |                                                              |
|        server/apikeylocation         |             APIPrivate              | path to the SSL private key file loaded at listen-start      |
|           server/servername          |               Alice                 | TLS PSK identity hint broadcast to connecting clients        |
|          server/packetallowed        |                3600                 | initial anti-DDoS packet budget assigned to each authenticated connection |
|       server/packetallowedregen      |                 60                  | anti-DDoS packet budget regenerated per minute per connection |
|       server/cachetolerancemsec      |                10000                | ms tolerance before the server considers its DB cache stale and resends timestamps to the client |
|        server/lastrecvcondtime       |               dynamic               | last condition-recovery pulse timestamp; written by server, do not edit manually |
|       server/nextsettleranktime      |               dynamic               | next ranking-settlement timestamp; written by server, do not edit manually |
|         server/equip_reg_csv         |             Equip.csv               | filename of the equipment definition CSV loaded at startup   |
|          server/ship_reg_csv         |              Ship.csv               | filename of the ship definition CSV loaded at startup        |
|       server/map_node_reg_csv        |           Map_nodes.csv             | filename of the map node definition CSV loaded at startup    |
|     server/map_relation_reg_csv      |         Map_relations.csv           | filename of the map relation CSV loaded at startup           |
|          server/vcr_reg_csv          |   Precondition_relations.csv        | filename of the virtual-condition-relation CSV loaded at startup |
|              sql/driver              |               QSQLITE               |                                                              |
|             sql/hostname             |            SpearofTanaka            |                                                              |
|              sql/dbname              |                ocean                |                                                              |
|            sql/adminname             |                admin                |                                                              |
|             sql/adminpw              |              10000826               |  You must change this or your database would be vulnerable   |
|       server/equipdbtimestamp        |               dynamic               |                                                              |
|        server/shipdbtimestamp        |               dynamic               |                                                              |
|        server/mapdbtimestamp         |               dynamic               |                                                              |
|            steam/webkey              |               (empty)               | Steam Web API key; required for ARD coupon purchases and refund polling |
|        steam/lastrefundpolltime      |               dynamic               | last Steam refund poll timestamp; written by server, do not edit manually |
|           license_notice             |       :/openingwords.txt            | path to the opening/license text shown at client startup and in the server CLI |
|        connect_wait_time_msec        |                8000                 | server-side timeout in ms when waiting for a client to disconnect during shutdown |