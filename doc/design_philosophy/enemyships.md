# Enemy Ship Design Philosophy

This is an document describing how to generate enemy ("Amnesiac") ship stats.

Combine this with [Enemy equip design philosophy](enemyequips.md) to generate their default equips. General design in [equip](equips.md) and [ship](ships.md) is also helpful.

Enemy ships have no Wikidata Identifier; countryoforigin is 0; shiporder is 0; remodel is 0 (no need); rarity is 0 (unobtainable); Allegiance is 0 (Any Qt Territory).

## Normal Enemies

### Remodel stage

- **Base**: `id & 0xFF000000 = 0x7F000000`, colored Grey, Equivalent to Lv 10 player ship at tech 1922

- **Regular**:`id & 0xFF000000 = 0x7E000000`, colored Red, Equivalent to Lv 40 player ship at tech 1925

- **Veteran**:`id & 0xFF000000 = 0x7D000000`, colored Yellow, Equivalent to Lv 100 player ship at tech 1930 (For plane equipment, 1940)

- **Elite**:`id & 0xFF000000 = 0x7C000000`, colored Green, Equivalent to Lv 100 player ship at tech 1937 (For plane equipment, 1942)

- **Chief**:`id & 0xFF000000 = 0x7B000000`, colored Blue, Equivalent to Lv 100 player ship at tech 1943 (For plane equipment, 1944)

- **Flagship**:`id & 0xFF000000 = 0x7A000000`, colored Purple, Equivalent to Lv 100 player ship at tech 1948

### Enemy classes

In the revised design pre-emptive ASW is random behaviour with chance based on ship ASW attribute and ASW equipment, not deterministic. So all such ships described as "having pre-emptive ASW" below should be construed as having high ASW attribute, ideally with sonars/depth charges (or for carriers, ASW patrol planes)

#### Torpedo boat

See description in [ships](../worldview_and_mechanics/5-ships.md)

#### Escort r(ㄖ)-class

Have pre-emptive ASW ability on regular stage or above.

#### Destroyer b(ㄅ)-class

Compared to same-stage destroyers, they have good gunshot/torpedo capability.

Have pre-emptive ASW ability on elite stage or above.

Have a midget-sub in flagship stage.

#### Destroyer p(ㄆ)-class

Compared to same-stage destroyers, they have good ASW capability.

Have pre-emptive ASW ability on all stages.

#### Destroyer m(ㄇ)-class

Compared to same-stage destroyers, they have good Anti-air capability.

Have pre-emptive ASW ability on flagship stage.

Have a midget-sub on chief stage or above.

#### Destroyer f(ㄈ)-class

Compared to same-stage destroyers, they have **very** good LOS capability. In night battle they have a high chance to discover the player while remaining concealed, thus attacking the player's fleet with impunity.

Have pre-emptive ASW ability on flagship stage.

Have a midget-sub on chief stage or above.

#### Light cruiser d(ㄉ)-class

Compared to same-stage light cruisers, they have good recon planes and ASW capability.

Have pre-emptive ASW ability on elite stage or above.

#### Light cruiser t(ㄊ)-class

Compared to same-stage light cruisers, they have good radar and anti-air.

Have pre-emptive ASW ability on flagship stage.

#### Torpedo cruiser n(ㄋ)-class

Compared to same-stage light cruisers, they have **very** good torpedo capability.

Have a midget-sub on all stages.

Have **two** midget-subs on elite stage or above.

#### Heavy Cruiser g(ㄍ)-class

Have a midget-sub on flagship stage.

#### Heavy (Aviation) Cruiser k(ㄎ)-class

As the name suggests, have seaplane fighters and bombers. Have a midget-sub on flagship stage.

#### Battlecruiser sh(ㄕ)-class

Compared to same-stage battleships, they are capable of torpedo warfare.

Have a midget-sub on veteran stage or above.

#### Battleship j(ㄐ)-class

Compared to same-stage battleships, they are superior in firepower and armor.

#### Battleship q(ㄑ)-class

Compared to same-stage battleships, they are superior in speed, evasion and accuracy.

#### Light carrier zh(ㄓ)-class

Compared to same-stage light carriers, they are superior in firepower and armor.

Have night aviation capability on chief stage or above.

#### Light Carrier ch(ㄔ)-class

Compared to same-stage light carriers, they are superior in ASW.

Have pre-emptive ASW ability on elite stage or above.

Have night aviation capability on chief stage or above.

#### Standard Carrier l(ㄌ)-class

Have ASW ability on elite stage or above.

Have night aviation capability on chief stage or above.

#### Transport h(ㄏ)-class

Lack equipment and will not attack on base stage; equips small-caliber gun on regular and veteran stages; equips mid-caliber gun on elite and chief stage; equips mid-caliber **flak** gun and anti-air radar on flagship stage.

Equips recon seaplane on chief stage and above.

#### Submarine x(ㄒ)-class

Normal submarines.

#### 浮游要塞,岸防炮台,对空炮台(To be determined)

Should be untoched for now.
