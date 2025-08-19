#ifndef EQUIPTYPE_H
#define EQUIPTYPE_H

#include <QString>
#include <QMap>
#include <QSet>
#include "resord.h"

class EquipType
{
public:
    EquipType();
    EquipType(int);
    EquipType(QString);

    bool operator==(const EquipType &) const;

    const ResOrd devResBase() const;
    static const QList<QString> getDisplayGroupsSorted();
    QString getDisplayGroup();
    int getTypeSort() const;
    QString getPrimaryAttr() const;
    int iconGroup() const;
    static const QString intToStrRep(int);
    static int strToIntRep(QString);
    QString toString() const;
    int toInt() const;

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
    static bool isBomber(const int);
    static bool isJet(const int);
    static bool isVirtual(const int);

    int getSize() const;
    bool isMainGun() const;
    bool isSecGun() const;
    bool isFlak() const;
    bool isSurface() const;
    bool isTorp() const;
    bool isFighter() const;
    bool isTorpBomber() const;
    bool isDiveBomber() const;
    bool isRecon() const;
    bool isPatrol() const;
    bool isLb() const;
    bool isNight() const;
    bool isSeaplane() const;
    bool isRadar() const;
    int getSpecial() const;
    bool isNight2() const;
    bool isBomber() const;
    bool isJet() const;
    bool isVirtual() const;

    static QList<QString> allEquipTypes();

private:
#pragma message(USED_CXX17)
    /* see equiptype.xlsx in doc for explanation */
    inline static const QMap<QString, int> result = {
        std::pair("Small-gun-flat",         0x8001),
        std::pair("Small-gun-flak",         0xA001),
        std::pair("Mid-gun-flat",           0x8002),
        std::pair("Mid-gun-flak",           0xA002),
        std::pair("Mid-gun-flat-ca",        0x8003),
        std::pair("Big-gun",                0x8004),
        std::pair("Superbig-gun",           0x8005),
        std::pair("Supremebig-gun",         0x8006),
        std::pair("Second-gun-flat",        0x4002),
        std::pair("Second-gun-flak",        0x6002),
        std::pair("Second-gun-flak-big",    0x6003),
        std::pair("Torp",                   0x1800),
        std::pair("Torp-sub",               0x0800),
        std::pair("Midget-sub",             0x00010000),
        std::pair("Fighter",                0x0400),
        std::pair("Fighter-night",          0x0410),
        std::pair("Fighter-lb",             0x0420),
        std::pair("Fighter-lb-interc",      0x001B0420),
        std::pair("Bomb-torp",              0x0200),
        std::pair("Bomb-torp-night",        0x0210),
        std::pair("Bomb-torp-n2",           0x00180200),
        std::pair("Bomb-torp-fight",        0x0600),
        std::pair("Bomb-torp-dive",         0x0300),
        std::pair("Bomb-dive",              0x0100),
        std::pair("Bomb-dive-fight",        0x0500),
        std::pair("Bomb-dive-fight-jet",    0x001C0500),
        std::pair("Bomb-dive-fight-n2",     0x00180500),
        std::pair("Bomb-dive-n2",           0x00180100),
        std::pair("Bomb-dive-torp-fight",   0x0700),
        std::pair("Attack-lb",              0x0320),
        std::pair("Attack-lb-fight",        0x0720),
        std::pair("Attack-lb-big",          0x0325),
        std::pair("Patrol-lb",              0x0060),
        std::pair("Recon",                  0x0080),
        std::pair("Recon-lb",               0x00A0),
        std::pair("Recon-fight",            0x0480),
        std::pair("Recon-jet",              0x001C0080),
        std::pair("Sp-bomb-small",          0x0109),
        std::pair("Sp-bomb",                0x010A),
        std::pair("Sp-bomb-night",          0x011A),
        std::pair("Sp-recon-small",         0x0089),
        std::pair("Sp-recon",               0x008A),
        std::pair("Sp-recon-night",         0x009A),
        std::pair("Sp-fight",               0x0408),
        std::pair("Flyingboat",             0x001A0000),
        std::pair("Patrol-autogyro",        0x0042),
        std::pair("Patrol-liaison",         0x0041),
        std::pair("Patrol-liaison-f",       0x0441),
        std::pair("Depthc-projector",       0x00020002),
        std::pair("Depthc-racks",           0x00020001),
        std::pair("Sonar-passive-big",      0x00040003),
        std::pair("Sonar-passive",          0x00040001),
        std::pair("Sonar-active",           0x00040002),
        std::pair("AP-shell",               0x00060000),
        std::pair("AL-shell",               0x00070000),
        std::pair("AL-rocket",              0x00080000),
        std::pair("Landing-craft",          0x00090000),
        std::pair("Landing-tank",           0x000A0000),
        std::pair("Drum",                   0x000B0000),
        std::pair("Tp-material",            0x000C0000),
        std::pair("Radar-small-flak",       0x2001),
        std::pair("Radar-small-flat",       0x1001),
        std::pair("Radar-small-dual",       0x3001),
        std::pair("Radar-big-flak",         0x2002),
        std::pair("Radar-big-flat",         0x1002),
        std::pair("Radar-big-dual",         0x3002),
        std::pair("Radar-superbig-dual",    0x3003),
        std::pair("Radar-sub",              0x0007),
        std::pair("Engine-turbine",         0x000D0000),
        std::pair("Engine-boiler",          0x000E0000),
        std::pair("Bulge-small",            0x001D0001),
        std::pair("Bulge-medium",           0x001D0002),
        std::pair("Bulge-large",            0x001D0003),
        std::pair("Searchlight",            0x000F0001),
        std::pair("Searchlight-big",        0x000F0003),
        std::pair("Starshell",              0x00100000),
        std::pair("Repair-item",            0x00110000),
        std::pair("Underway-replenish",     0x00120000),
        std::pair("Food",                   0x00130000),
        std::pair("Command-fac",            0x00140000),
        std::pair("Aircraft-personnel",     0x00150000),
        std::pair("Repair-fac",             0x00160000),
        std::pair("Surface-personnel",      0x00170000),
        std::pair("Smoke",                  0x00030000),
        std::pair("Ballon",                 0x00050000),
        std::pair("AA-gun",                 0x00192000),
        std::pair("AA-cannon",              0x00192001),
        std::pair("AA-control-device",      0x001E2000),
        std::pair("Land-corps",             0x001F0000),
        std::pair("Virtual-precondition",   0x01000000),
    };

    inline static const QHash<QString, int> groupToIcon = {
        std::pair("AA-gun",                 15),
        std::pair("AA-cannon",              15),
        std::pair("AA-control-device",      30),
        std::pair("Depthc-projector",       17),
        std::pair("Depthc-racks",           17),
        std::pair("Sonar-passive-big",      18),
        std::pair("Sonar-passive",          18),
        std::pair("Sonar-active",           18),
        std::pair("Patrol-autogyro",        21),
        std::pair("Patrol-liaison",         22),
        std::pair("Patrol-liaison-f",       22),
        std::pair("Patrol-lb",              47),
        std::pair("Attack-lb",              37),
        std::pair("Attack-lb-fight",        37),
        std::pair("Attack-lb-big",          49),
        std::pair("Big-gun",                3),
        std::pair("Bomb-dive",              7),
        std::pair("Bomb-dive-fight",        7),
        std::pair("Bomb-dive-fight-jet",    40),
        std::pair("Bomb-dive-fight-n2",     7),
        std::pair("Bomb-dive-n2",           7),
        std::pair("Bomb-dive-torp-fight",   7),
        std::pair("Bomb-torp",              8),
        std::pair("Bomb-torp-night",        46),
        std::pair("Bomb-torp-fight",        8),
        std::pair("Bomb-torp-dive",         8),
        std::pair("Bomb-torp-n2",           8),
        std::pair("Bulge-small",            23),
        std::pair("Bulge-medium",           23),
        std::pair("Bulge-large",            23),
        std::pair("Fighter",                6),
        std::pair("Fighter-night",          45),
        std::pair("Fighter-lb",             44),
        std::pair("Fighter-lb-interc",      38),
        std::pair("Landing-craft",          20),
        std::pair("Landing-tank",           36),
        std::pair("Land-corps",             52),
        std::pair("Mid-gun-flat",           2),
        std::pair("Mid-gun-flak",           16),
        std::pair("Mid-gun-flat-ca",        2),
        std::pair("AL-rocket",              31),
        std::pair("Ballon",                 55),
        std::pair("Command-fac",            28),
        std::pair("Drum",                   25),
        std::pair("Engine-boiler",          19),
        std::pair("Engine-turbine",         19),
        std::pair("Flyingboat",             33),
        std::pair("Food",                   34),
        std::pair("Repair-fac",             26),
        std::pair("Repair-item",            14),
        std::pair("Searchlight",            24),
        std::pair("Searchlight-big",        24),
        std::pair("Smoke",                  54),
        std::pair("Starshell",              27),
        std::pair("Tp-material",            41),
        std::pair("Underway-replenish",     35),
        std::pair("Aircraft-personnel",     29),
        std::pair("Surface-personnel",      32),
        std::pair("Radar-small-flak",       11),
        std::pair("Radar-small-flat",       11),
        std::pair("Radar-small-dual",       11),
        std::pair("Radar-big-flak",         11),
        std::pair("Radar-big-flat",         11),
        std::pair("Radar-big-dual",         11),
        std::pair("Radar-superbig-dual",    11),
        std::pair("Radar-sub",              42),
        std::pair("Recon",                  9),
        std::pair("Recon-lb",               9),
        std::pair("Recon-fight",            9),
        std::pair("Recon-jet",              9),
        std::pair("Sp-recon-small",         10),
        std::pair("Sp-recon",               10),
        std::pair("Sp-recon-night",         50),
        std::pair("Sp-bomb-small",          10),
        std::pair("Sp-bomb",                10),
        std::pair("Sp-bomb-night",          51),
        std::pair("Sp-fight",               43),
        std::pair("Second-gun-flat",        4),
        std::pair("Second-gun-flak",        16),
        std::pair("Second-gun-flak-big",    16),
        std::pair("AP-shell",               13),
        std::pair("AL-shell",               12),
        std::pair("Small-gun-flat",         1),
        std::pair("Small-gun-flak",         16),
        std::pair("Superbig-gun",           3),
        std::pair("Supremebig-gun",         3),
        std::pair("Torp",                   5),
        std::pair("Torp-sub",               5),
        std::pair("Midget-sub",             5),
        std::pair("Virtual-precondition",   0),
    };

    inline static const QHash<QString, QString> primaryAttr = {
        std::pair("Small-gun-flat",         "Firepower"),
        std::pair("Small-gun-flak",         "Firepower"),
        std::pair("Mid-gun-flat",           "Firepower"),
        std::pair("Mid-gun-flat-ca",        "Firepower"),
        std::pair("Mid-gun-flak",           "Firepower"),
        std::pair("Big-gun",                "Firepower"),
        std::pair("Superbig-gun",           "Firepower"),
        std::pair("Supremebig-gun",         "Firepower"),
        std::pair("Second-gun-flat",        "Accuracy"),
        std::pair("Second-gun-flak",        "Antiair"),
        std::pair("Second-gun-flak-big",    "Antiair"),
        std::pair("Torp",                   "Torpedo"),
        std::pair("Torp-sub",               "Torpedo"),
        std::pair("Midget-sub",             "Torpedo"),
        std::pair("Fighter",                "Antiair"),
        std::pair("Fighter-night",          "Antiair"),
        std::pair("Bomb-dive",              "Bombing"),
        std::pair("Bomb-dive-fight",        "Bombing"),
        std::pair("Bomb-dive-fight-n2",     "Bombing"),
        std::pair("Bomb-dive-n2",           "Bombing"),
        std::pair("Bomb-dive-torp-fight",   "Bombing"),
        std::pair("Bomb-dive-fight-jet",    "Bombing"),
        std::pair("Bomb-torp",              "Airtorpedo"),
        std::pair("Bomb-torp-fight",        "Airtorpedo"),
        std::pair("Bomb-torp-dive",         "Airtorpedo"),
        std::pair("Bomb-torp-n2",           "Airtorpedo"),
        std::pair("Bomb-torp-night",        "Airtorpedo"),
        std::pair("Recon",                  "Los"),
        std::pair("Recon-fight",            "Los"),
        std::pair("Recon-jet",              "Los"),
        std::pair("Fighter-lb",             "Antiair"),
        std::pair("Fighter-lb-interc",      "Antiair"),
        std::pair("Attack-lb",              "Bombing"),
        std::pair("Attack-lb-fight",        "Bombing"),
        std::pair("Attack-lb-big",          "Bombing"),
        std::pair("Recon-lb",               "LOS"),
        std::pair("Sp-recon-small",         "Los"),
        std::pair("Sp-recon",               "Los"),
        std::pair("Sp-recon-night",         "Los"),
        std::pair("Sp-bomb-small",          "Bombing"),
        std::pair("Sp-bomb",                "Bombing"),
        std::pair("Sp-bomb-night",          "Bombing"),
        std::pair("Sp-fight",               "Antiair"),
        std::pair("Radar-small-flak",       "Los"),
        std::pair("Radar-small-flat",       "Los"),
        std::pair("Radar-small-dual",       "Los"),
        std::pair("Radar-big-flak",         "Los"),
        std::pair("Radar-big-flat",         "Los"),
        std::pair("Radar-big-dual",         "Los"),
        std::pair("Radar-superbig-dual",    "Los"),
        std::pair("Radar-sub",              "Evasion"),
        std::pair("AL-shell",               "Antiland"),
        std::pair("AP-shell",               "Armorpenetration"),
        std::pair("Repair-item",            "Tech"),
        std::pair("AA-gun",                 "Antiair"),
        std::pair("AA-cannon",              "Antiair"),
        std::pair("AA-control-device",      "Antiair"),
        std::pair("Sonar-passive-big",      "Asw"),
        std::pair("Sonar-passive",          "Asw"),
        std::pair("Sonar-active",           "Asw"),
        std::pair("Depthc-projector",       "Asw"),
        std::pair("Depthc-racks",           "Asw"),
        std::pair("Engine-boiler",          "Speed"),
        std::pair("Engine-turbine",         "Speed"),
        std::pair("Landing-craft",          "Antiland"),
        std::pair("Landing-tank",           "Antiland"),
        std::pair("Land-corps",             "Antiland"),
        std::pair("Patrol-autogyro",        "Asw"),
        std::pair("Patrol-liaison",         "Asw"),
        std::pair("Patrol-liaison-f",       "Asw"),
        std::pair("Patrol-lb",              "Asw"),
        std::pair("Bulge-small",            "Armor"),
        std::pair("Bulge-medium",           "Armor"),
        std::pair("Bulge-large",            "Armor"),
        std::pair("Searchlight",            "Los"),
        std::pair("Searchlight-big",        "Los"),
        std::pair("Drum",                   "Transport"),
        std::pair("Repair-fac",             "Tech"),
        std::pair("Starshell",              "Tech"),
        std::pair("Command-fac",            "Los"),
        std::pair("Aircraft-personnel",     "Firingrange"),
        std::pair("AL-rocket",              "Antiland"),
        std::pair("Surface-personnel",      "Los"),
        std::pair("Flyingboat",             "Los"),
        std::pair("Food",                   "Tech"),
        std::pair("Underway-replenish",     "Tech"),
        std::pair("Tp-material",            "Transport"),
        std::pair("Smoke",                  "Concealment"),
        std::pair("Ballon",                 "Evasion"),
        std::pair("Virtual-precondition",   "Tech"),
    };

    inline static const QHash<QString, int> displaySort = {
        std::pair("Small-gun-flat",         1),
        std::pair("Small-gun-flak",         2),
        std::pair("Mid-gun-flat",           3),
        std::pair("Mid-gun-flat-ca",        4),
        std::pair("Mid-gun-flak",           5),
        std::pair("Big-gun",                6),
        std::pair("Superbig-gun",           7),
        std::pair("Supremebig-gun",         8),
        std::pair("Second-gun-flat",        9),
        std::pair("Second-gun-flak",        10),
        std::pair("Second-gun-flak-big",    11),
        std::pair("Torp",                   12),
        std::pair("Torp-sub",               13),
        std::pair("Midget-sub",             14),
        std::pair("Fighter",                15),
        std::pair("Fighter-night",          16),
        std::pair("Bomb-dive",              17),
        std::pair("Bomb-dive-fight",        17),
        std::pair("Bomb-dive-fight-n2",     17),
        std::pair("Bomb-dive-n2",           17),
        std::pair("Bomb-dive-torp-fight",   17),
        std::pair("Bomb-dive-fight-jet",    18),
        std::pair("Bomb-torp",              19),
        std::pair("Bomb-torp-fight",        19),
        std::pair("Bomb-torp-dive",         19),
        std::pair("Bomb-torp-n2",           19),
        std::pair("Bomb-torp-night",        20),
        std::pair("Recon",                  21),
        std::pair("Recon-fight",            21),
        std::pair("Recon-jet",              21),
        std::pair("Fighter-lb",             22),
        std::pair("Fighter-lb-interc",      23),
        std::pair("Attack-lb",              24),
        std::pair("Attack-lb-fight",        24),
        std::pair("Attack-lb-big",          25),
        std::pair("Recon-lb",               26),
        std::pair("Sp-recon-small",         27),
        std::pair("Sp-recon",               28),
        std::pair("Sp-recon-night",         29),
        std::pair("Sp-bomb-small",          30),
        std::pair("Sp-bomb",                31),
        std::pair("Sp-bomb-night",          32),
        std::pair("Sp-fight",               33),
        std::pair("Radar-small-flak",       34),
        std::pair("Radar-small-flat",       34),
        std::pair("Radar-small-dual",       34),
        std::pair("Radar-big-flak",         35),
        std::pair("Radar-big-flat",         35),
        std::pair("Radar-big-dual",         35),
        std::pair("Radar-superbig-dual",    36),
        std::pair("Radar-sub",              37),
        std::pair("AL-shell",               38),
        std::pair("AP-shell",               39),
        std::pair("Repair-item",            40),
        std::pair("AA-gun",                 41),
        std::pair("AA-cannon",              41),
        std::pair("AA-control-device",      42),
        std::pair("Sonar-passive-big",      43),
        std::pair("Sonar-passive",          43),
        std::pair("Sonar-active",           43),
        std::pair("Depthc-projector",       44),
        std::pair("Depthc-racks",           44),
        std::pair("Engine-boiler",          45),
        std::pair("Engine-turbine",         46),
        std::pair("Landing-craft",          47),
        std::pair("Landing-tank",           48),
        std::pair("Land-corps",             49),
        std::pair("Patrol-autogyro",        50),
        std::pair("Patrol-liaison",         51),
        std::pair("Patrol-liaison-f",       52),
        std::pair("Patrol-lb",              53),
        std::pair("Bulge-small",            54),
        std::pair("Bulge-medium",           55),
        std::pair("Bulge-large",            56),
        std::pair("Searchlight",            57),
        std::pair("Searchlight-big",        58),
        std::pair("Drum",                   59),
        std::pair("Repair-fac",             60),
        std::pair("Starshell",              61),
        std::pair("Command-fac",            62),
        std::pair("Aircraft-personnel",     63),
        std::pair("AL-rocket",              64),
        std::pair("Surface-personnel",      65),
        std::pair("Flyingboat",             66),
        std::pair("Food",                   67),
        std::pair("Underway-replenish",     68),
        std::pair("Tp-material",            69),
        std::pair("Smoke",                  70),
        std::pair("Ballon",                 71),
        std::pair("Virtual-precondition",   0),
    };

    int iRep; // internal representation
};

#endif // EQUIPTYPE_H
