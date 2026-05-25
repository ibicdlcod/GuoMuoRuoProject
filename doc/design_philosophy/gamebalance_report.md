# Game Balance Report

Updated 2026-05-23. 3v3 matchups, lv=100, default equipment, 50 runs.
Player ships (base + remodels) vs amnesiac equivalents. Extra battle = false.

---

## 1. Destroyer Balance

| Ship | Tech | HP | vs Enemy | Dmg Dealt | Dmg Taken | Final HP | Result |
|------|------|-----|----------|-----------|-----------|----------|--------|
| 神風 | 1922 | 233 | Base (1922) | 584 | 164 | 71/233 | Wins, 30% HP left |
| 吹雪 | 1928 | 277 | Regular (1925) | 575 | 226 | 55/277 | Wins, 20% HP left |
| 白露 | 1936 | 286 | Elite (1937) | 138 | 365 | **0**/286 | Dies every run |
| 陽炎改二 | 1945 | 337 | Flagship (1948) | 127 | 428 | **0**/337 | Dies every run |
| 島風 | 1943 | 318 | Base (1922) | 536 | 69 | 249/318 | Dominates |
| 島風 | 1943 | 318 | Flagship (1948) | 51 | 398 | **0**/318 | Dies every run |

**Enemy DD (Flagship):** equips 8197/8235/8240 (Small-gun 1948, Torp 1948, Midget-sub). Hit rate 9-15% against player ship evasion.

---

## 2. Cruiser Balance

| Ship | Tech | HP | vs Enemy | Dmg Dealt | Dmg Taken | Final HP |
|------|------|-----|----------|-----------|-----------|----------|
| 天龍 CL | 1919 | 345 | Base | 565 | 309 | 52/345 |
| 天龍改二 CL | 1940 | 420 | Flagship | 22 | 472 | **0**/420 |
| 古鷹 CA | 1926 | 441 | Base CA | 1122 | **0** | 441/441 |
| 妙高改二 CA | 1943 | 565 | Flagship CA | 417 | 661 | 6/565 |

**古鷹 vs Base heavy cruiser:** enemy deals 0 damage. The amnesiac heavy cruiser at Base tier may lack main guns in its default equipment — needs investigation.

---

## 3. Battleship Balance

| Ship | Tech | HP | vs Enemy | Dmg Dealt | Dmg Taken | Final HP |
|------|------|-----|----------|-----------|-----------|----------|
| 金剛 | 1913 | 635 | Base | 773 | 473 | 188/635 |
| 金剛改二丙 | 1944 | 802 | Flagship | 269 | 888 | 42/802 |
| 扶桑 | 1915 | 666 | Base | 744 | 478 | 204/666 |

---

## 4. Battle Formula Assessment

No battle formula changes needed. The formulas produce:
- Reasonable hit rates (9-50% depending on accuracy/evasion match)
- Proper damage scaling with tech level
- Appropriate cut-in rates (small but present)

Issues are purely in **equipment/ship stat data**, not formulas.

---

## 5. Data Issues Found

### A. Enemy CA at Base tier has no main gun
古鷹 takes 0 damage from Base-tier enemy CA — the enemy cannot attack. Check `重巡ㄍ級Base` default equipment.

### B. Mid-tier DDs are too weak vs higher-tier enemies  
白露 (1936) dies every run against Elite (1937). The 1-year tech gap shouldn't produce a 100% loss rate. Possible causes: equipment gap (enemy Elite has much better guns), or armor/HP scaling favors enemies too strongly.

### C. DD vs Flagship is hopeless across the board
Even 陽炎改二 (1945) and 島風 (1943) die every run against Flagship. Flagship-tier enemies have Midget-subs, better armor, and higher DPM. This may be intentional but should be confirmed.

### D. Rarity/Allegiance now correct
Per `enemyships.md`: rarity=0, allegiance=0, no Wikidata ID. Confirmed working.

---

## 6. Recommendations (data only, no formula changes)

1. Audit Base-tier enemy CA equipment — missing main guns
2. Consider reducing the DPM/armor gap between Elite and mid-tier player ships
3. Run tests with extraBattle=true to validate torpedo mechanics
4. Compare specific equipment stats (DPM, armor) between same-tier player and amnesiac ships to pinpoint imbalances
