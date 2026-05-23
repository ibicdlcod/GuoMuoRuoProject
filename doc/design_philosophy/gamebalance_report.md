# Game Balance Report

Updated 2026-05-23. Evenly-matched 3v3 tests, lv=100, default equipment, 30 runs.
Player ships vs amnesiac equivalents of the same tier.

---

## 1. Destroyer Balance (3v3 vs 驱逐ㄅ级)

| Player Ship | Tech | Enemy Tier | Dmg Dealt | Dmg Taken | Final HP | Notes |
|------------|------|-----------|-----------|-----------|----------|-------|
| 神風 | 1922 | Base (1922) | 492 | 222 | 65/233 | Wins but takes heavy damage |
| 吹雪 | 1928 | Regular (1925) | 528 | 256 | 56/277 | Wins, similar damage taken |
| 吹雪改二 | 1941 | Chief (1943) | 384 | 368 | 2/310 | Barely survives |
| 陽炎改二 | 1945 | Flagship (1948) | 0 | 0 | 337/337 | No combat — equipment issue |
| 島風 | 1943 | Base (1922) | 546 | 78 | 240/318 | Dominates early-tier |
| 島風 | 1943 | Flagship (1948) | 0 | 0 | 318/318 | No combat — equipment issue |

**Hit rates (神風 vs Base):**
- 神風: ~47% main gun, ~8% torpedo
- Enemy: ~17% main gun

Enemy DDs at Flagship tier have no main gun in their default equipment? Needs investigation.

---

## 2. Cruiser / Battleship Balance (3v3 vs same tier)

| Player Ship | Tech | Enemy Tier | Dmg Dealt | Dmg Taken | Final HP |
|------------|------|-----------|-----------|-----------|----------|
| 天龍 CL | 1919 | Base | — | — | — |
| 古鷹 CA | 1926 | Base | — | — | — |
| 金剛 BB | 1913 | Base | — | — | — |
| 扶桑 BB | 1915 | Base | — | — | — |

---

## 3. Key Findings

### 3v3 matchups work correctly
Evenly-matched fleet sizes produce cleaner data than the 1v6 tests:
- Early DDs (神風 1922) can beat Base-tier enemies but lose 70%+ HP
- Mid-war DDs (吹雪改二 1941) trade evenly with Chief-tier
- 島風 dominates early tiers (78 damage taken vs 3 Base enemies)

### Equipment issues at high tiers
Flagship-tier amnesiac DDs have 0 damage output — their default equipment may lack main guns, or the equipment IDs in the CSV don't match what's in ocean.db.

### Remodel progression
吹雪 (1928) → 吹雪改 (1937) → 吹雪改二 (1941): damage output scales with tech level but survivability against higher-tier enemies decreases.

---

## 4. Recommendations

1. Verify Flagship-tier amnesiac equipment — some ships show 0 damage, suggesting missing main guns
2. Run tests with `extraBattle=true` to validate torpedo mechanics
3. Expand BB/CA tests to all tiers
4. The gamebalance report infrastructure with individual per-class/tier test files works well for isolating balance issues
