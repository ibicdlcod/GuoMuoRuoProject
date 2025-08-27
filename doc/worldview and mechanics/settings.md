|                Field                 |               Default               |                             Note                             |
| :----------------------------------: | :---------------------------------: | :----------------------------------------------------------: |
|               alias/*                |                                     |                     created by the user                      |
|           client/language            |                ja_JP                |                  default value is temporary                  |
|            client/logfile            |            ClientLog.log            |                                                              |
|      client/licenseareapersist       |                5000                 |                                                              |
|       client/equipdbtimestamp        |               dynamic               |                                                              |
|         client/equipdbcache          |               dynamic               |                                                              |
|        client/shipdbtimestamp        |               dynamic               |                                                              |
|        client/mapdbtimestamp         |               dynamic               |                                                              |
|     networkclient/retransmitmax      |                  2                  |                    Client retransmit time                    |
|  networkclient/connectwaittimemsec   |                8000                 |                                                              |
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
|          rule/sigmaconstant          |                 1.0                 | the larger this value, the less effect tech has on development/construction success rate |
|      rule/equipmentstandardstar      |                 15                  | the larger this value, the less effective improving equipment does |
|            server/logfile            |            ServerLog.log            |                                                              |
|           server/language            |                en_US                |                                                              |
|      server/displaypromptdelay       |                 100                 |                                                              |
|              sql/driver              |               QSQLITE               |                                                              |
|             sql/hostname             |            SpearofTanaka            |                                                              |
|              sql/dbname              |                ocean                |                                                              |
|            sql/adminname             |                admin                |                                                              |
|             sql/adminpw              |              10000826               |  You must change this or your database would be vulnerable   |
|       server/equipdbtimestamp        |               dynamic               |                                                              |
|        server/shipdbtimestamp        |               dynamic               |                                                              |
|        server/mapdbtimestamp         |               dynamic               |                                                              |