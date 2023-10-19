#ifndef EQUIPTYPE_H
#define EQUIPTYPE_H

#include <QString>
#include <QMap>
#include <QSet>

class EquipType
{
public:
    EquipType();
    EquipType(int);
    EquipType(QString);

    static const QString intToStrRep(int);
    static int strToIntRep(QString);
    QString toString() const;
    int toInt() const;
    static QSet<QString> getDisplayGroups();

    static int getSize(const int);
    static bool isMainGun(const int);
    static bool isSecGun(const int);
    static bool isFlak(const int);
    static bool isSurface(const int);
    static bool isTorp(const int);
    static bool isFighter(const int);
    static bool isTorpBomber(const int);
    static bool isDiveBomber(const int);
    static bool isRecon(const int);
    static bool isPatrol(const int);
    static bool isLb(const int);
    static bool isNight(const int);
    static bool isSeaplane(const int);
    static bool isRadar(const int);
    static int getSpecial(const int);
    static bool isNight2(const int);

    static QList<QString> allEquipTypes();

private:
    /* this is C++17 */
    inline static const QMap<QString, int> result = {
        std::pair("Small-gun-flat",     0x8001),
        std::pair("Small-gun-flak",     0xA001),
        std::pair("Mid-gun-flat",       0x8002),
        std::pair("Mid-gun-flak",       0xA002),
        std::pair("Mid-gun-flat-ca",    0x8003),
        std::pair("Big-gun",            0x8004),
        std::pair("Superbig-gun",       0x8005),
        std::pair("Supremebig-gun",     0x8006),
        std::pair("Second-gun-flat",    0x4002),
        std::pair("Second-gun-flak",    0x6002),
        std::pair("Second-gun-flak-big",0x6003),
        std::pair("Torp",               0x1800),
        std::pair("Torp-sub",           0x0800),
        std::pair("Midget-sub",         0x00010000),
        std::pair("Fighter",            0x0400),
        std::pair("Fighter-night",      0x0410),
        std::pair("Fighter-lb",         0x0420),
        std::pair("Fighter-lb-interc",  0x001B0420),
        std::pair("Bomb-torp",          0x0200),
        std::pair("Bomb-torp-night",    0x0210),
        std::pair("Bomb-torp-fight",    0x0600),
        std::pair("Bomb-torp-dive",     0x0300),
        std::pair("Attack-lb",          0x0320),
        std::pair("Attack-lb-fight",    0x0720),
        std::pair("Attack-lb-big",      0x0325),
        std::pair("Patrol-lb",          0x0060),
        std::pair("Bomb-dive",          0x0100),
        std::pair("Bomb-dive-fight",    0x0500),
        std::pair("Bomb-dive-fight-jet",0x001C0500),
        std::pair("Recon",              0x0080),
        std::pair("Recon-lb",           0x00A0),
        std::pair("Recon-fight",        0x0480),
        std::pair("Sp-bomb",            0x0108),
        std::pair("Sp-bomb-night",      0x0118),
        std::pair("Sp-recon",           0x0088),
        std::pair("Sp-recon-night",     0x0098),
        std::pair("Sp-fight",           0x0408),
        std::pair("Flyingboat",         0x001A0000),
        std::pair("Patrol-autogyro",    0x0042),
        std::pair("Patrol-liaison",     0x0041),
        std::pair("Patrol-liaison-f",   0x0441),
        std::pair("Depthc-projector",   0x00020002),
        std::pair("Depthc-racks",       0x00020001),
        std::pair("Sonar-passive-big",  0x00040003),
        std::pair("Sonar-passive",      0x00040002),
        std::pair("Sonar-active",       0x00040001),
        std::pair("AP-shell",           0x00060000),
        std::pair("AL-shell",           0x00070000),
        std::pair("AL-rocket",          0x00080000),
        std::pair("Landing-craft",      0x00090000),
        std::pair("Landing-tank",       0x000A0000),
        std::pair("Drum",               0x000B0000),
        std::pair("Tp-material",        0x000C0000),
        std::pair("Radar-small-flak",   0x2001),
        std::pair("Radar-small-flat",   0x1001),
        std::pair("Radar-small-dual",   0x3001),
        std::pair("Radar-big-flak",     0x2002),
        std::pair("Radar-big-flat",     0x1002),
        std::pair("Radar-big-dual",     0x3002),
        std::pair("Radar-superbig-dual",0x3003),
        std::pair("Radar-sub",          0x0007),
        std::pair("Engine-turbine",     0x000D0000),
        std::pair("Engine-boiler",      0x000E0000),
        std::pair("Bulge-medium",       0x001D0002),
        std::pair("Bulge-large",        0x001D0003),
        std::pair("Searchlight",        0x000F0001),
        std::pair("Searchlight-big",    0x000F0003),
        std::pair("Starshell",          0x00100000),
        std::pair("Repair-item",        0x00110000),
        std::pair("Underway-replenish", 0x00120000),
        std::pair("Food",               0x00130000),
        std::pair("Command-fac",        0x00140000),
        std::pair("Aircraft-personnel", 0x00150000),
        std::pair("Repair-fac",         0x00160000),
        std::pair("Surface-personnel",  0x00170000),
        std::pair("Bomb-torp-n2",       0x00180200),
        std::pair("Bomb-dive-fight-n2", 0x00180500),
        std::pair("Smoke",              0x00030000),
        std::pair("Ballon",             0x00050000),
        std::pair("AA-gun",             0x00192000),
        std::pair("AA-control-device",  0x001E2000),
        std::pair("Land-corps",         0x001F0000),
        std::pair("Virtual-precondition",   0x01000000),
    };

    inline static const QMap<QString, QString> displaygroup = {
        std::pair("AA-gun",             "AA"),
        std::pair("AA-control-device",  "AA"),
        std::pair("Depthc-projector",   "ASW"),
        std::pair("Depthc-racks",       "ASW"),
        std::pair("Sonar-passive-big",  "ASW"),
        std::pair("Sonar-passive",      "ASW"),
        std::pair("Sonar-active",       "ASW"),
        std::pair("Patrol-autogyro",    "ASW"),
        std::pair("Patrol-liaison",     "ASW"),
        std::pair("Patrol-liaison-f",   "ASW"),
        std::pair("Patrol-lb",          "ASW"),
        std::pair("Attack-lb",          "ATTACKLB"),
        std::pair("Attack-lb-fight",    "ATTACKLB"),
        std::pair("Attack-lb-big",      "ATTACKLB"),
        std::pair("Big-gun",            "BIGGUN"),
        std::pair("Bomb-dive",          "BOMBDIVE"),
        std::pair("Bomb-dive-fight",    "BOMBDIVE"),
        std::pair("Bomb-dive-fight-jet","BOMBDIVE"),
        std::pair("Bomb-dive-fight-n2", "BOMBDIVE"),
        std::pair("Bomb-torp",          "BOMBTORP"),
        std::pair("Bomb-torp-night",    "BOMBTORP"),
        std::pair("Bomb-torp-fight",    "BOMBTORP"),
        std::pair("Bomb-torp-dive",     "BOMBTORP"),
        std::pair("Bomb-torp-n2",       "BOMBTORP"),
        std::pair("Bulge-medium",       "BULGE"),
        std::pair("Bulge-large",        "BULGE"),
        std::pair("Fighter",            "FIGHTER"),
        std::pair("Fighter-night",      "FIGHTER"),
        std::pair("Fighter-lb",         "FIGHTERLB"),
        std::pair("Fighter-lb-interc",  "FIGHTERLB"),
        std::pair("Landing-craft",      "LAND"),
        std::pair("Landing-tank",       "LAND"),
        std::pair("Land-corps",         "LAND"),
        std::pair("Mid-gun-flat",       "MIDGUN"),
        std::pair("Mid-gun-flak",       "MIDGUN"),
        std::pair("Mid-gun-flat-ca",    "MIDGUN"),
        std::pair("AL-rocket",          "OTHER"),
        std::pair("Ballon",         	"OTHER"),
        std::pair("Command-fac",    	"OTHER"),
        std::pair("Drum",           	"OTHER"),
        std::pair("Engine-boiler",  	"OTHER"),
        std::pair("Engine-boiler",  	"OTHER"),
        std::pair("Engine-turbine", 	"OTHER"),
        std::pair("Flyingboat",     	"OTHER"),
        std::pair("Food",           	"OTHER"),
        std::pair("Repair-fac",     	"OTHER"),
        std::pair("Repair-item",    	"OTHER"),
        std::pair("Searchlight",       	"OTHER"),
        std::pair("Searchlight-big",	"OTHER"),
        std::pair("Smoke",          	"OTHER"),
        std::pair("Starshell",      	"OTHER"),
        std::pair("Tp-material",    	"OTHER"),
        std::pair("Underway-replenish",	"OTHER"),
        std::pair("Aircraft-personnel", "PERS"),
        std::pair("Surface-personnel",  "PERS"),
        std::pair("Radar-small-flak",   "RADAR"),
        std::pair("Radar-small-flat",   "RADAR"),
        std::pair("Radar-small-dual",   "RADAR"),
        std::pair("Radar-big-flak",     "RADAR"),
        std::pair("Radar-big-flat",     "RADAR"),
        std::pair("Radar-big-dual",     "RADAR"),
        std::pair("Radar-superbig-dual","RADAR"),
        std::pair("Radar-sub",          "RADAR"),
        std::pair("Recon",              "RECON"),
        std::pair("Recon-lb",           "RECON"),
        std::pair("Recon-fight",        "RECON"),
        std::pair("Sp-recon",           "RECON"),
        std::pair("Sp-recon-night",     "RECON"),
        std::pair("Sp-bomb",            "SEAPLANEBF"),
        std::pair("Sp-bomb-night",      "SEAPLANEBF"),
        std::pair("Sp-fight",           "SEAPLANEBF"),
        std::pair("Second-gun-flat",    "SECGUN"),
        std::pair("Second-gun-flak",    "SECGUN"),
        std::pair("Second-gun-flak-big","SECGUN"),
        std::pair("AP-shell",           "SHELL"),
        std::pair("AL-shell",           "SHELL"),
        std::pair("Small-gun-flat",     "SMALLGUNFLAT"),
        std::pair("Small-gun-flak",     "SMALLGUNFLAK"),
        std::pair("Superbig-gun",       "SUPERBIGGUN"),
        std::pair("Supremebig-gun",     "SUPERBIGGUN"),
        std::pair("Torp",               "TORP"),
        std::pair("Torp-sub",           "TORP"),
        std::pair("Midget-sub",         "TORP"),
        std::pair("Virtual-precondition",   "VIRTUAL"),
    };

    int internalRep;
};

#endif // EQUIPTYPE_H
