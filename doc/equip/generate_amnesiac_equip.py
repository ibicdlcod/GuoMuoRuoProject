#!/usr/bin/env python3
"""Generate amnesiac (enemy) equipment rows for Equip.csv.

ID starts at 8192. Stats grow monotonically with tier — every stat for a type
is >= its value at lower tiers. All 6 tiers generated for every type — where
target tech is before the player equip range, stats are backward-extrapolated
to produce weaker (but functional) versions.
"""

import csv
import os
import sys

CSV_PATH = os.path.join(os.path.dirname(__file__), "Equip.csv")

TIERS = [
    (1922, 1930),  # Base
    (1925, 1937),  # Regular
    (1930, 1940),  # Veteran
    (1937, 1942),  # Elite
    (1943, 1944),  # Chief
    (1948, 1948),  # Flagship
]

NEEDED = [
    ("Small-gun-flat",     "小口径主砲", False),
    ("Small-gun-flak",     "小口径主砲", False),
    ("Mid-gun-flat",       "中口径主砲", False),
    ("Mid-gun-flat-ca",    "中口径主砲", False),
    ("Big-gun",            "大口径主砲", False),
    ("Second-gun-flat",    "副砲", False),
    ("Second-gun-flak",    "副砲", False),
    ("Torp",               "魚雷", False),
    ("Torp-sub",           "潜水艦魚雷", False),
    ("Midget-sub",         "特殊潜航艇", False),
    ("Fighter",            "艦上戦闘機", True),
    ("Bomb-dive",          "艦上爆撃機", True),
    ("Bomb-torp",          "艦上攻撃機", True),
    ("Sp-recon",           "水上偵察機", True),
    ("Sp-fight",           "水上戦闘機", True),
    ("Sp-bomb",            "多用途水上機/水上爆撃機", True),
    ("AA-gun",             "対空機銃", False),
    ("AA-cannon",          "対空機銃", False),
    ("Sonar-passive",      "ソナー", False),
    ("Sonar-active",       "ソナー", False),
    ("Depthc-projector",   "爆雷", False),
    ("Depthc-racks",       "爆雷", False),
    ("Radar-small-flat",   "小型電探", False),
    ("Radar-small-flak",   "小型電探", False),
    ("Radar-small-dual",   "小型電探", False),
    ("Radar-big-flat",     "大型電探", False),
    ("Radar-big-flak",     "大型電探", False),
    ("Drum",               "簡易輸送部材", False),
    ("Searchlight",        "探照灯", False),
]

C = {
    "id": 0, "name": 1, "ident": 2, "cat": 3, "equiptype": 4,
    "Tech": 5, "Father": 6, "Father2": 7, "Mother": 8, "MaxProd": 9,
    "Hitpoints": 10, "DPM": 11, "Armor": 12, "AP": 13,
    "Accuracy": 14, "TorpAcc": 15, "Evasion": 16,
    "Los": 17, "Concealment": 18,
    "Firingrange": 19, "Firingspeed": 20, "Speed": 21,
    "Torpedo": 22, "Airtorpedo": 23, "Bombing": 24,
    "Antiair": 25, "Asw": 26, "Interception": 27,
    "Antibomber": 28, "Antiland": 29, "Transport": 30,
    "Flightrange": 31, "Homeport": 32, "Storeprice": 33,
    "Planes": 34,
}

STATS = [
    "Hitpoints", "DPM", "Armor", "AP", "Accuracy", "TorpAcc",
    "Evasion", "Los", "Concealment", "Firingrange", "Firingspeed",
    "Speed", "Torpedo", "Airtorpedo", "Bombing", "Antiair", "Asw",
    "Interception", "Antibomber", "Antiland", "Transport", "Flightrange",
    "Planes",
]


def sv(s):
    try: return float(s)
    except: return 0.0


def load():
    rows = []
    with open(CSV_PATH, encoding="utf-8") as f:
        r = csv.reader(f)
        next(r); next(r)
        for row in r:
            if len(row) >= 34:
                rows.append(row)
    return rows


def player_of_type(rows, etype):
    out = [(r, sv(r[C["Tech"]])) for r in rows
           if r[C["equiptype"]] == etype
           and sv(r[C["Tech"]]) > 0
           and int(sv(r[C["id"]])) < 8192]
    out.sort(key=lambda x: x[1])
    return [r for r, _ in out]


def extrap_one(tgt, techs, vals, is_plane):
    """Interpolate/extrapolate a single stat at target year `tgt`.

    Between two player techs: linear interp.
    Before the first: extrapolate backward using slope of first two equips.
    After the last: hold the last value (or use slope of last two).
    """
    n = len(techs)
    if n == 0:
        return 0.0
    if n == 1:
        return max(vals[0], 0.0)

    # If within range, bracket-interpolate
    for i in range(n - 1):
        if techs[i] <= tgt <= techs[i + 1]:
            d = techs[i + 1] - techs[i]
            if d == 0:
                return max(vals[i], 0.0)
            t = (tgt - techs[i]) / d
            return max(int(round(vals[i] + t * (vals[i + 1] - vals[i]))), 0)

    # Before earliest player equip: extrapolate backward
    if tgt < techs[0]:
        slope = (vals[1] - vals[0]) / max(techs[1] - techs[0], 1)
        dt = tgt - techs[0]
        return max(int(round(vals[0] + slope * dt)), 0)

    # After latest player equip: extrapolate forward using last two
    if tgt > techs[-1]:
        slope = (vals[-1] - vals[-2]) / max(techs[-1] - techs[-2], 1)
        dt = tgt - techs[-1]
        return max(int(round(vals[-1] + slope * dt)), 0)

    return max(vals[-1], 0)


def gen_type(etype, jp_cat, is_plane, player_rows, start_id):
    peqs = player_of_type(player_rows, etype)
    if not peqs:
        return [], start_id

    for r in peqs:
        while len(r) < 35:
            r.append("")

    techs = [sv(r[C["Tech"]]) for r in peqs]
    # Stats of all player equips, one list per stat
    pstats = {}
    for stat in STATS:
        ci = C[stat]
        pstats[stat] = [sv(r[ci]) for r in peqs]

    # For backward extrapolation, we want at least 2 data points.
    # If only 1 player equip exists, duplicate it at tech+1 to create a
    # shallow slope.
    if len(techs) == 1:
        techs.append(techs[0] + 1)
        for stat in STATS:
            pstats[stat].append(pstats[stat][0])

    rows_out = []
    prev_value = {s: -1 for s in STATS}

    for tier_i, (ship_yr, plane_yr) in enumerate(TIERS):
        tgt = plane_yr if is_plane else ship_yr
        if techs and tgt < techs[0]:
            continue  # skip tiers before earliest player equip
        row = [""] * 35
        row[C["id"]] = str(start_id)
        row[C["name"]] = f"Amnesiac {etype.replace('-', ' ')} Version {int(tgt)}"
        row[C["cat"]] = jp_cat
        row[C["equiptype"]] = etype
        row[C["Tech"]] = str(int(tgt))
        row[C["MaxProd"]] = "30"

        for stat in STATS:
            ci = C[stat]
            if stat == "Planes" and is_plane:
                # Tier-dependent plane count, matching player plane progression
                val = [5, 8, 12, 16, 20, 24][tier_i]
            else:
                val = extrap_one(tgt, techs, pstats[stat], is_plane)
            # monotonic enforcement
            if stat in prev_value and prev_value[stat] >= 0:
                val = max(val, prev_value[stat])
            val = max(val, 0)
            row[ci] = str(int(val))
            prev_value[stat] = val

        row[C["Homeport"]] = ""
        row[C["Storeprice"]] = ""
        rows_out.append(row)
        start_id += 1

    return rows_out, start_id


def fmt(row):
    return ",".join(str(v) for v in row)


def main():
    player_rows = load()
    print(f"Parsed {len(player_rows)} player entries", file=sys.stderr)

    # Separate player (id < 8192) from old amnesiac rows
    player_part = [r for r in player_rows
                   if int(sv(r[C['id']])) < 8192]

    nid = 8192
    all_out = []
    for etype, jp_cat, is_plane in NEEDED:
        rows, nid = gen_type(etype, jp_cat, is_plane,
                             [r for r in player_rows
                              if int(sv(r[C['id']])) < 8192],
                             nid)
        for r in rows:
            print(f"  [{r[C['id']]}] {r[C['name']]}",
                  file=sys.stderr)
        all_out.extend(rows)

    # Rewrite CSV: headers first, then player rows, then amnesiac rows
    HEADER1 = "id,name,,,type,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr,attr"
    HEADER2 = "No.,ja_JP,Identifier,種別,equiptype,Tech,Father,Father2,Mother,Disallowmassproduction,Hitpoints,DPM,Armor,Armorpenetration,Accuracy,Torpedoaccuracy,Evasion,Los,Concealment,Firingrange,Firingspeed,Speed,Torpedo,Airtorpedo,Bombing,Antiair,Asw,Interception,Antibomber,Antiland,Transport,Flightrange,Homeport,Storeprice,Planes"
    with open(CSV_PATH, "w", encoding="utf-8") as f:
        f.write(HEADER1 + "\n")
        f.write(HEADER2 + "\n")
        for row in player_part:
            try:
                if int(sv(row[0])) > 0 and int(sv(row[0])) < 8192:
                    while len(row) < 35:
                        row.append("")
                    f.write(fmt(row) + "\n")
            except:
                pass
        for row in all_out:
            f.write(fmt(row) + "\n")
    print(f"Generated {len(all_out)} entries (IDs 8192-{nid - 1})",
          file=sys.stderr)


if __name__ == "__main__":
    main()
