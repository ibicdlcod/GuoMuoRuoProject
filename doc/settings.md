|                Field                 |      Default      |                           Note                            |
| :----------------------------------: | :---------------: | :-------------------------------------------------------: |
|               alias/*                |                   |                                                           |
|           client/language            |       ja_JP       |                default value is temporary                 |
|            client/logfile            |   ClientLog.log   |                                                           |
|      client/licenseareapersist       |       5000        |                                                           |
|     networkclient/retransmitmax      |         2         |                  Client retransmit time                   |
|  networkclient/connectwaittimemsec   |       8000        |                                                           |
|          networkclient/pem           |  :/harusoft.pem   |                         Embedded                          |
|    networkclient/autopasswordtime    |       1000        |                                                           |
|     networkclient/requestEATCall     |      dynamic      |                                                           |
|    networkshared/maxmsgdelayinms     |       1000        |                                                           |
| networkshared/mintimebetweenmsgsinms |        100        |                                                           |
|          networkserver/pem           |  :/harusoft.pem   |                                                           |
|          networkserver/key           | serverprivate.key |                                                           |
|          msg_disabled/debug          |       false       |                  ignored in releasemode                   |
|          msg_disabled/info           |       false       |                                                           |
|          msg_disabled/warn           |       false       |                                                           |
|          msg_disabled/crit           |       false       |                                                           |
|          msg_disabled/fatal          |       false       |                                                           |
|           rule/devresscale           |        10         |                                                           |
|           rule/devtimebase           |         6         |                                                           |
|        rule/skillpointfactor         |       1.25        |                                                           |
|         rule/skillpointbase          |      10000.0      |                                                           |
|       rule/techcombinedeffects       |        3.0        |                                                           |
|         rule/techglobalscope         |       1.02        |                                                           |
|         rule/techlocalscope          |        1.1        |                                                           |
|       rule/shiplevelperweight        |       10.0        |                                                           |
|     rule/skillpointweightcontrib     |        9.0        |                                                           |
|  rule/penguinskillpointsdifficulty   |       10.0        |                                                           |
|     rule/maxskillpointsamplifier     |        5.0        |                                                           |
|    rule/normalproductionstockpile    |       30.0        |                                                           |
|         rule/antiregenpower          |        4.0        |                                                           |
|         rule/baseregennormal         |        10         |                                                           |
|        rule/baseregenaluminum        |         5         |                                                           |
|          rule/baseregenrare          |         2         |                                                           |
|          rule/motherspscale          |        0.2        |                                                           |
|          rule/maxresources           |      3600000      |                                                           |
|            server/logfile            |   ServerLog.log   |                                                           |
|           server/language            |       en_US       |                                                           |
|      server/displaypromptdelay       |        100        |                                                           |
|              sql/driver              |      QSQLITE      |                                                           |
|             sql/hostname             |   SpearofTanaka   |                                                           |
|              sql/dbname              |       ocean       |                                                           |
|            sql/adminname             |       admin       |                                                           |
|             sql/adminpw              |     10000826      | You must change this or your database would be vulnerable |