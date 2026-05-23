#!/usr/bin/env python3
"""
Generate amnesiac (enemy) ship rows for Ship.csv.

ID = (tier_byte << 24) | (type_code << 12) | class_index

Stats: averaged from player ships of matching *major* type within the
tech-year interval bounded by midpoints of adjacent tier tech years,
then scaled by level efficiency.

Level scaling: Base(Lv10)=0.570, Regular(Lv40)=0.763, Veteran+(Lv100)=1.0

Default equipment from amnesiac equip IDs (8192+).

The `remodel` column of each generated ship points to the corresponding ship
of the same class (type_enc) and tier from Ship.xlsx.bak (loaded via its CSV
backup).
"""

import csv, math, os, sys

HERE = os.path.dirname(__file__)
SHIP_CSV = os.path.join(HERE, "Ship.csv")
SHIP_BAK_CSV = os.path.join(HERE, "Ship.csv.bak")
EQUIP_CSV = os.path.join(HERE, "..", "equip", "Equip.csv")

# ── tiers ──────────────────────────────────────────────────
TIERS = [
    {"name":"Base",     "byte":0x7F, "lv":10,  "ship":1922, "plane":1930},
    {"name":"Regular",  "byte":0x7E, "lv":40,  "ship":1925, "plane":1937},
    {"name":"Veteran",  "byte":0x7D, "lv":100, "ship":1930, "plane":1940},
    {"name":"Elite",    "byte":0x7C, "lv":100, "ship":1937, "plane":1942},
    {"name":"Chief",    "byte":0x7B, "lv":100, "ship":1943, "plane":1944},
    {"name":"Flagship", "byte":0x7A, "lv":100, "ship":1948, "plane":1948},
]

SQ2_INV = 1.0 / math.sqrt(2)
LV_EFF = {
    10:  0.5 + SQ2_INV * 10  / math.sqrt(10000 + 100),
    40:  0.5 + SQ2_INV * 40  / math.sqrt(10000 + 1600),
    100: 1.0,
}


def tech_intervals(is_plane):
    yrs = sorted(t["plane" if is_plane else "ship"] for t in TIERS)
    mids = [yrs[0] - (yrs[1] - yrs[0]) / 2]
    for i in range(len(yrs) - 1):
        mids.append((yrs[i] + yrs[i + 1]) / 2)
    mids.append(yrs[-1] + (yrs[-1] - yrs[-2]) / 2)
    return mids  # len = 7, interval i = [mids[i], mids[i+1])


# ── enemy class definitions ────────────────────────────────
# (name_base, class_text, type_code, is_plane,
#  bak_target_type, bak_target_class,
#  [(equiptype, first_tier_or_None), ...],
#  stat_modifiers_dict)
#
# bak_target_type / bak_target_class identify the ship in Ship.xlsx.bak
# to reference via the `remodel` column.

ENEMY_CLASSES = [

    ("魚雷艇", "魚雷艇", 0x10, False,
     0x010, 0x100,
     [("Small-gun-flat", 0)],
     {"DPM":0.5, "Hitpoints":0.4, "Evasion":1.5, "Armor":0.3}),

    ("海防ㄖ級", "海防ㄖ級", 0x11, False,
     0x011, 0x100,
     [("Small-gun-flat", 0), ("Sonar-passive", 1), ("Depthc-projector", 1)],
     {"Asw":1.4, "DPM":0.8}),

    ("駆逐ㄅ級", "駆逐ㄅ級", 0x20, False,
     0x020, 0x100,
     [("Small-gun-flat", 0), ("Torp", 0), ("Midget-sub", 5)],
     {"DPM":1.1, "Torpedo":1.15}),

    ("駆逐ㄆ級", "駆逐ㄆ級", 0x20, False,
     0x020, 0x200,
     [("Small-gun-flat", 0), ("Sonar-passive", 0), ("Depthc-projector", 0)],
     {"Asw":1.4, "Torpedo":0.7, "DPM":0.85}),

    ("駆逐ㄇ級", "駆逐ㄇ級", 0x20, False,
     0x020, 0x300,
     [("Small-gun-flat", 0), ("AA-gun", 0), ("Midget-sub", 4)],
     {"Antiair":1.3, "DPM":0.85, "Torpedo":0.85}),

    ("駆逐ㄈ級", "駆逐ㄈ級", 0x20, False,
     0x020, 0x400,
     [("Small-gun-flat", 0), ("Radar-small-flat", 0), ("Midget-sub", 4)],
     {"Los":1.5, "Concealment":1.4, "Accuracy":1.2}),

    ("軽巡ㄉ級", "軽巡ㄉ級", 0x30, False,
     0x030, 0x100,
     [("Mid-gun-flat", 0), ("Sp-recon", 0), ("Sonar-passive", 0), ("Depthc-projector", 0)],
     {"Asw":1.3, "Los":1.1}),

    ("軽巡ㄊ級", "軽巡ㄊ級", 0x30, False,
     0x030, 0x200,
     [("Mid-gun-flat", 0), ("Radar-small-flak", 0), ("AA-gun", 0)],
     {"Antiair":1.4, "Los":1.2, "Accuracy":1.1}),

    ("雷巡ㄋ級", "雷巡ㄋ級", 0x32, False,
     0x032, 0x300,
     [("Mid-gun-flat", 0), ("Torp", 0), ("Midget-sub", 0), ("Midget-sub", 3)],
     {"Torpedo":1.5, "DPM":0.85}),

    ("重巡ㄍ級", "重巡ㄍ級", 0x40, False,
     0x040, 0x100,
     [("Mid-gun-flat-ca", 0), ("Midget-sub", 5)],
     {"DPM":1.0, "Armor":1.0}),

    ("航巡ㄎ級", "航巡ㄎ級", 0x44, False,
     0x044, 0x200,
     [("Mid-gun-flat-ca", 0), ("Sp-fight", 0), ("Sp-bomb", 0), ("Midget-sub", 5)],
     {"Los":1.2, "Antiair":1.1}),

    ("戦巡ㄕ級", "戦巡ㄕ級", 0x51, False,
     0x050, 0x100,
     [("Big-gun", 0), ("Torp", 0), ("Midget-sub", 2)],
     {"Speed":1.15, "Torpedo":1.2, "Evasion":1.1}),

    ("戦艦ㄐ級", "戦艦ㄐ級", 0x50, False,
     0x050, 0x100,
     [("Big-gun", 0)],
     {"DPM":1.2, "Armor":1.15}),

    ("戦艦ㄑ級", "戦艦ㄑ級", 0x52, False,
     0x052, 0x200,
     [("Big-gun", 0)],
     {"Speed":1.2, "Evasion":1.15, "Accuracy":1.15, "DPM":0.9, "Armor":0.9}),

    ("軽母ㄓ級", "軽母ㄓ級", 0x61, True,
     0x061, 0x100,
     [("Fighter", 0), ("Bomb-torp", 0), ("Bomb-dive", 0)],
     {"DPM":1.1, "Armor":1.1, "Planes":1.1}),

    ("軽母ㄔ級", "軽母ㄔ級", 0x62, True,
     0x061, 0x200,
     [("Fighter", 0), ("Bomb-dive", 0)],
     {"Asw":1.8, "Planes":1.1}),

    ("空母ㄌ級", "空母ㄌ級", 0x60, True,
     0x060, 0x100,
     [("Fighter", 0), ("Bomb-dive", 0), ("Bomb-torp", 0)],
     {"Planes":1.2}),

    ("潜水ㄒ級", "潜水ㄒ級", 0x70, False,
     0x070, 0x100,
     [("Torp-sub", 0)],
     {"Torpedo":1.0}),

    ("輸送ㄏ級", "輸送ㄏ級", 0x90, False,
     0x090, 0x100,
     [("Small-gun-flat", 1), ("Mid-gun-flat", 3), ("Mid-gun-flak", 5),
      ("Radar-small-flak", 5), ("Sp-recon", 4)],
     {"DPM":0.7, "Hitpoints":0.8}),
]


# ── helpers ─────────────────────────────────────────────────
def sv(s):
    try: return float(s.strip())
    except: return 0.0


# ── parse headers once ─────────────────────────────────────
_HEADER_INDICATORS = None
_HEADER_TITLES = None

def _init_headers():
    global _HEADER_INDICATORS, _HEADER_TITLES
    if _HEADER_INDICATORS is not None:
        return
    with open(SHIP_CSV, encoding="utf-8") as f:
        r = csv.reader(f)
        _HEADER_INDICATORS = next(r)
        _HEADER_TITLES = next(r)


def header_cols():
    _init_headers()
    return _HEADER_INDICATORS, _HEADER_TITLES


# ── bak ship loader (for remodel targeting) ────────────────
_BAK_ENEMY = None

def load_bak_enemy():
    global _BAK_ENEMY
    if _BAK_ENEMY is not None:
        return _BAK_ENEMY
    bak = {}
    fp = os.path.normpath(SHIP_BAK_CSV)
    if not os.path.exists(fp):
        print(f"WARN: {fp} not found, remodel will be 0", file=sys.stderr)
        _BAK_ENEMY = {}
        return {}
    inds, titles = header_cols()
    with open(fp, encoding="utf-8") as f:
        r = csv.reader(f)
        next(r); next(r)
        for line in r:
            if not line or not line[0]:
                continue
            sid = int(line[0])
            if (sid & 0xFF000000) < 0x70000000:
                continue
            type_enc = (sid >> 12) & 0xFFF
            tier_byte = (sid >> 24) & 0xFF
            cls = sid & 0xFFF
            key = (type_enc, tier_byte, cls)
            bak[key] = sid
    _BAK_ENEMY = bak
    print(f"Loaded {len(bak)} bak enemy ships for remodel targeting",
          file=sys.stderr)
    return bak


def find_remodel_target(bak_type, bak_class, tier_byte):
    bak = load_bak_enemy()
    return bak.get((bak_type, tier_byte, bak_class), 0)


# ── player ship loading ────────────────────────────────────
def load_player_ships():
    inds, titles = header_cols()
    out = []
    with open(SHIP_CSV, encoding="utf-8") as f:
        r = csv.reader(f)
        next(r); next(r)
        for line in r:
            if len(line) < 38:
                continue
            sid = int(line[0])
            if sid == 0:
                continue
            if (sid & 0xFF000000) >= 0x70000000:
                continue  # skip existing enemy ships
            type_code = (sid & 0x000ff000) >> 12
            tech = sv(line[14])
            if tech <= 0:
                continue
            stats = {}
            for i, (ind, title) in enumerate(zip(inds, titles)):
                if ind == "attr" and title not in ("OldNo", "OldInternalNo"):
                    stats[title] = sv(line[i])
                elif ind == "customflags":
                    stats["CUSTOM" + title] = (
                        sv(line[i]) if line[i].strip() else 0)
            out.append((sid, type_code, tech, stats))
    return out


# ── amnesiac equip cache ───────────────────────────────────
_EQUIP_CACHE = None

def load_amnesiac_equips():
    global _EQUIP_CACHE
    if _EQUIP_CACHE is not None:
        return _EQUIP_CACHE
    el = []
    ep = os.path.normpath(EQUIP_CSV)
    if not os.path.exists(ep):
        print(f"WARN: {ep} not found", file=sys.stderr)
        _EQUIP_CACHE = []
        return []
    with open(ep, encoding="utf-8") as f:
        r = csv.reader(f)
        next(r); next(r)
        for line in r:
            if len(line) < 5:
                continue
            eid = int(sv(line[0]))
            if eid < 8192:
                continue
            etype = line[4].strip()
            tech = int(sv(line[5]))
            for idx, t in enumerate(TIERS):
                yr = t["plane" if etype in _PLANE_TYPES else "ship"]
                if tech == yr:
                    el.append((eid, etype, idx))
                    break
    _EQUIP_CACHE = el
    return el

_PLANE_TYPES = {"Fighter","Bomb-dive","Bomb-torp","Sp-recon","Sp-fight","Sp-bomb"}

def min_tier_for_type(equip_type):
    """Return the lowest tier index where amnesiac equip of this type exists."""
    tiers = set()
    for eid, et, t in load_amnesiac_equips():
        if et == equip_type:
            tiers.add(t)
    return min(tiers) if tiers else 6


def find_amnesiac_equip(equip_type, tier_idx):
    for eid, et, t in load_amnesiac_equips():
        if et == equip_type and t == tier_idx:
            return eid
    return 0


# ── stat averaging ─────────────────────────────────────────
def avg_stats(player_ships, major_type, low_yr, high_yr):
    """Average stats of player ships whose type major nibble (bits 16-19)
    matches, and whose Tech is in [low_yr, high_yr)."""
    mt = major_type & 0xF0
    matches = []
    for sid, tc, tech, stats in player_ships:
        if (tc & 0xF0) != mt:
            continue
        if not (low_yr <= tech < high_yr):
            continue
        matches.append(stats)
    if not matches:
        return {}, 0
    n = len(matches)
    avg = {}
    for key in matches[0]:
        total = sum(m.get(key, 0) for m in matches)
        avg[key] = total / n
    return avg, n


def apply_mods(stats, mods):
    for k, v in mods.items():
        if k in stats:
            stats[k] *= v


def level_scale(stats, lv):
    eff = LV_EFF[lv]
    scale_keys = {"Hitpoints","DPM","Armor","Evasion","Los","Concealment",
                  "Speed","Torpedo","Antiair","Asw","Antiland","Transport",
                  "Equipslots","Planes",
                  "Accuracy","TorpAcc","Firingrange","Firingspeed",
                  "Airtorpedo","Bombing","Interception","Antibomber",
                  "Flightrange","FuelConsumption","AmmoConsumption"}
    for k in list(stats.keys()):
        if k in scale_keys:
            stats[k] *= eff
        elif k.startswith("CUSTOM"):
            stats[k] *= eff
    return eff


# ── row building ───────────────────────────────────────────
def build_row(sid, name_ja, cls_text, type_code, tech_yr,
              equips, stats, tier_i, remodel_target):
    inds, titles = header_cols()
    ncols = len(inds)
    row = [""] * ncols
    row[0] = str(sid)

    for i in range(ncols):
        ind = inds[i]
        ttl = titles[i]

        if ind == "attr":
            if ttl.startswith("OldNo") or ttl.startswith("OldInternalNo"):
                continue
            if ttl == "remodel":
                row[i] = str(remodel_target)
            elif ttl == "Defaultequip1":
                row[i] = str(int(equips[0])) if len(equips) > 0 else ""
            elif ttl == "Defaultequip2":
                row[i] = str(int(equips[1])) if len(equips) > 1 else ""
            elif ttl == "Defaultequip3":
                row[i] = str(int(equips[2])) if len(equips) > 2 else ""
            elif ttl == "Defaultequip4":
                row[i] = str(int(equips[3])) if len(equips) > 3 else ""
            elif ttl == "Defaultequip5":
                row[i] = str(int(equips[4])) if len(equips) > 4 else ""
            elif ttl == "Tech":
                row[i] = str(int(tech_yr))
            elif ttl == "Rarity":
                row[i] = str(int(stats.get("Rarity", 0)))
            elif ttl == "Allegiance":
                row[i] = "120"
            elif ttl in stats:
                v = stats[ttl]
                if isinstance(v, float):
                    v = int(round(v))
                row[i] = str(max(v, 0))
        elif ind == "customflags":
            key = "CUSTOM" + ttl
            if key in stats:
                v = stats[key]
                if isinstance(v, float):
                    v = int(round(v))
                row[i] = str(max(v, 0))
        elif ind == "name" and ttl == "ja_JP":
            row[i] = name_ja
        elif ind == "shipclasstext" and ttl == "ja_JP":
            row[i] = cls_text
        elif ind == "shipordertext" and ttl == "ja_JP":
            row[i] = ""

    # Set empty-indicator columns that import code checks by title
    for i, ttl in enumerate(titles):
        if ttl == "remodelstage":
            row[i] = "0"
        if ttl == "countryoforigin":
            row[i] = "0"
        if ttl == "shiptypeHEX":
            row[i] = format(0x100000, "X")
        if ttl == "shipclassHEX":
            row[i] = format(0x10000, "X")
        if ttl == "shiporder":
            row[i] = str(tier_i)

    return row


# ── main ───────────────────────────────────────────────────
def generate_all():
    player_ships = load_player_ships()
    print(f"Loaded {len(player_ships)} player ships", file=sys.stderr)

    # Pre-load bak enemy data for remodel targeting
    load_bak_enemy()

    # Default slot counts by major type
    DEFAULT_SLOTS = {
        0x10: 2, 0x20: 2, 0x30: 3, 0x40: 3,
        0x50: 4, 0x60: 4, 0x70: 1, 0x90: 2,
    }
    # Default planes by type (for carriers)
    DEFAULT_PLANES = {0x60: 20, 0x61: 12, 0x62: 12}

    all_rows = []
    for cl_idx, eclass in enumerate(ENEMY_CLASSES):
        (name_base, cls_text, type_code, is_plane,
         bak_type, bak_class, eq_specs, mods) = eclass
        mids = tech_intervals(is_plane)
        class_index = cl_idx + 1

        class_rows = []
        for tier_i in range(6):
            ti = TIERS[tier_i]
            lv = ti["lv"]
            tech_yr = ti["plane" if is_plane else "ship"]

            low = mids[tier_i]
            high = mids[tier_i + 1]

            avg, cnt = avg_stats(player_ships, type_code, low, high)

            if cnt == 0:
                avg, cnt = avg_stats(player_ships, type_code, 0, 9999)
            if cnt == 0:
                print(f"  WARN: no player ships for {name_base} tier {tier_i}",
                      file=sys.stderr)
                continue

            stats = dict(avg)
            apply_mods(stats, mods)
            level_scale(stats, lv)

            # Override Equipslots and Planes with reasonable values
            mt = type_code & 0xF0
            if mt in DEFAULT_SLOTS:
                stats["Equipslots"] = DEFAULT_SLOTS[mt]
            if type_code in DEFAULT_PLANES:
                stats["Planes"] = DEFAULT_PLANES[type_code] * LV_EFF[lv]

            # ID
            sid = (ti["byte"] << 24) | (type_code << 12) | class_index

            # Equipment
            equips = []
            for eq_type, start_t in eq_specs:
                effective_start = max(start_t if start_t is not None else 0,
                                      min_tier_for_type(eq_type))
                if tier_i < effective_start:
                    continue
                eid = find_amnesiac_equip(eq_type, tier_i)
                if eid:
                    equips.append(eid)
            while len(equips) < 5:
                equips.append(0)

            tier_name = ti["name"]
            # ja_JP name: Japanese class name + English tier (no separator)
            full_name = f"{name_base}{tier_name}"

            # Find target from bak: same (type, tier)
            remodel_id = find_remodel_target(bak_type, bak_class,
                                             ti["byte"])

            row = build_row(sid, full_name, cls_text, type_code,
                            tech_yr, equips, stats, tier_i, remodel_id)
            class_rows.append(row)
            print(f"  [{sid}] {name_base}{tier_name} tier={tier_i} "
                  f"remodel=0x{remodel_id:08X} ({cnt} ref ships)",
                  file=sys.stderr)

        # Monotonic enforcement: each stat >= previous tier
        _ROUND_KEYS = {"Hitpoints","DPM","Armor","Evasion","Los","Concealment",
                       "Speed","Torpedo","Antiair","Asw","Antiland","Transport",
                       "Planes","Accuracy"}
        for i in range(1, len(class_rows)):
            prev = class_rows[i - 1]
            curr = class_rows[i]
            for k in _ROUND_KEYS:
                for ci, (ind, ttl) in enumerate(zip(*header_cols())):
                    if ind == "attr" and ttl == k:
                        pv = int(prev[ci]) if prev[ci] else 0
                        cv = int(curr[ci]) if curr[ci] else 0
                        if cv < pv:
                            curr[ci] = str(pv)
                        break

        all_rows.extend(class_rows)

    # Read existing CSV, strip old amnesiac rows, then append new ones
    existing = []
    with open(SHIP_CSV, "r", encoding="utf-8") as f:
        existing = f.readlines()
    with open(SHIP_CSV, "w", encoding="utf-8") as f:
        for line in existing:
            # Keep header rows and non-amnesiac (id < 0x70000000) entries
            sid_str = line.split(",")[0].strip()
            if not sid_str or not sid_str.isdigit():
                f.write(line)
            elif int(sid_str) < 0x70000000:
                f.write(line)
        for row in all_rows:
            f.write(",".join(str(v) for v in row) + "\n")

    print(f"Generated {len(all_rows)} ship entries", file=sys.stderr)


if __name__ == "__main__":
    generate_all()
