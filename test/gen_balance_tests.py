#!/usr/bin/env python3
"""Generate balance test lua files — evenly-matched fleets (3v3 per tier)."""

import csv, os

PROJ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHIP_CSV = os.path.join(PROJ, "doc/ship/Ship.csv")
OUTDIR = os.path.join(PROJ, "test/balance")

os.makedirs(OUTDIR, exist_ok=True)

# Load amnesiac IDs from CSV: class_name -> [base, regular, veteran, elite, chief, flagship]
def load_amn():
    tiers = {}
    with open(SHIP_CSV, encoding="utf-8") as f:
        reader = csv.reader(f)
        next(reader); next(reader)
        for row in reader:
            try: sid = int(row[0])
            except: continue
            if sid < 0x70000000: continue
            name = row[5].strip()
            if not name: continue
            for tn in ["Base","Regular","Veteran","Elite","Chief","Flagship"]:
                if name.endswith(tn):
                    cls = name[:-len(tn)]
                    tiers.setdefault(cls, {})[tn] = sid
                    break
            else:
                # Name without tier suffix = Base tier
                tiers.setdefault(name, {})["Base"] = sid
    return tiers

AMN = load_amn()
AMN_ORDER = ["Base","Regular","Veteran","Elite","Chief","Flagship"]

SHIP_NAMES = {}
with open(SHIP_CSV, encoding="utf-8") as f:
    reader = csv.reader(f)
    next(reader); next(reader)
    for row in reader:
        try: SHIP_NAMES[int(row[0])] = row[5]
        except: pass

FLEET_SIZE = 3  # evenly matched

def write_test(tag, friend_sids, friend_names, enemy_cls):
    """Create one test file per tier: FLEET_SIZE friend copies vs FLEET_SIZE enemy copies."""
    for ti, tn in enumerate(AMN_ORDER):
        enemy_sid = AMN[enemy_cls].get(tn, 0)
        if not enemy_sid:
            continue

        eids = [enemy_sid] * FLEET_SIZE
        nf = len(friend_sids)
        ne = len(eids)
        name_str = '+'.join(f.replace(' ', '') for f in friend_names)
        suffix = f"_{enemy_cls}_{tn}"
        fname = os.path.join(OUTDIR, f"balance_{tag.replace(' ', '_')}{suffix}.lua")

        fships = "\n".join(f"            [{i}] = {s}," for i, s in enumerate(friend_sids))
        eships = "\n".join(f"            [{i}] = {s}," for i, s in enumerate(eids))
        fdy = "\n".join(f'            [{i}] = {{ lv = 100, fuel = 1.0, ammo = 1.0 }},' for i in range(nf))
        edy = "\n".join(f'            [{i}] = {{ lv = 100, fuel = 1.0, ammo = 1.0 }},' for i in range(ne))

        lua = f"""-- {name_str} vs {enemy_cls}{tn} ({nf}v{ne})
return {{
    FriendFleetInfo = {{
        ships = {{
{fships}
        }},
        shipDynamics = {{
{fdy}
        }},
        shipTags = {{{", ".join("0" for _ in range(nf))}}},
        equipSkillEffects = {{}},
    }},
    EnemyFleetInfo = {{
        ships = {{
{eships}
        }},
        shipDynamics = {{
{edy}
        }},
    }},
    BattlePlan = {{
        friendFleetPriority = 0,
        enemyFleetPriority = 0,
        extraBattle = false,
    }},
}}
"""
        with open(fname, "w") as f:
            f.write(lua)


def main():
    # DD: test each flagship at all remodel stages against matching amnesiac tier
    dd_tests = [
        (269615617, "DD_神風",     "驱逐ㄅ级"),
        (538051073,  "DD_神風改",   "驱逐ㄅ级"),
        (269616129, "DD_吹雪",     "驱逐ㄅ级"),
        (538051585, "DD_吹雪改",   "驱逐ㄅ级"),
        (806487041, "DD_吹雪改二", "驱逐ㄅ级"),
        (269616641, "DD_白露",     "驱逐ㄅ级"),
        (538052097, "DD_白露改",   "驱逐ㄅ级"),
        (806495745, "DD_白露改二", "驱逐ㄅ级"),
        (269617153, "DD_陽炎",     "驱逐ㄅ级"),
        (538052609, "DD_陽炎改",   "驱逐ㄅ级"),
        (806504449, "DD_陽炎改二", "驱逐ㄅ级"),
        (269617409, "DD_夕雲",     "驱逐ㄅ级"),
        (538052865, "DD_夕雲改",   "驱逐ㄅ级"),
        (806504705, "DD_夕雲改二", "驱逐ㄅ级"),
        (269617665, "DD_島風",     "驱逐ㄅ级"),
        (538053121, "DD_島風改",   "驱逐ㄅ级"),
    ]
    dd_mix = [
        ([269615617, 806487041, 806495745, 806504449, 806504705, 269617665],
         "DD_mix", "驱逐ㄅ级"),
    ]
    cl_tests = [
        (269680897, "CL_天龍",     "轻巡ㄉ级"),
        (538116353, "CL_天龍改",   "轻巡ㄉ级"),
        (806551809, "CL_天龍改二", "轻巡ㄉ级"),
    ]
    ca_tests = [
        (269746433, "CA_古鷹",     "战舰ㄐ级"),
        (538181889, "CA_古鷹改",   "战舰ㄐ级"),
        (806617345, "CA_古鷹改二", "战舰ㄐ级"),
        (269746945, "CA_妙高",     "战舰ㄐ级"),
        (538182401, "CA_妙高改",   "战舰ㄐ级"),
        (806617857, "CA_妙高改二", "战舰ㄐ级"),
    ]
    bb_tests = [
        (269816065, "BB_金剛",         "战舰ㄐ级"),
        (538251521, "BB_金剛改",       "战舰ㄐ级"),
        (806686977, "BB_金剛改二",     "战舰ㄐ级"),
        (1008029953,"BB_金剛改二丙",   "战舰ㄐ级"),
        (269811969, "BB_扶桑",         "战舰ㄐ级"),
        (538263809, "BB_扶桑改",       "战舰ㄐ级"),
        (806699265, "BB_扶桑改二",     "战舰ㄐ级"),
    ]
    # Cross-class DD test (mix of DDs)
    dd_mix = [
        ([269615617, 806487041, 806495745, 806504449, 806504705, 269617665],
         "DD_mix", "駆逐ㄅ級"),
    ]
    # CL tests
    cl_tests = [
        (269680897, "CL_天龍",     "轻巡ㄉ级"),
        (538116353, "CL_天龍改",   "轻巡ㄉ级"),
        (806551809, "CL_天龍改二", "轻巡ㄉ级"),
    ]
    # CA tests
    ca_tests = [
        (269746433, "CA_古鷹",     "战舰ㄐ级"),
        (538181889, "CA_古鷹改",   "战舰ㄐ级"),
        (806617345, "CA_古鷹改二", "战舰ㄐ级"),
        (269746945, "CA_妙高",     "战舰ㄐ级"),
        (538182401, "CA_妙高改",   "战舰ㄐ级"),
        (806617857, "CA_妙高改二", "战舰ㄐ级"),
    ]
    # BB tests
    bb_tests = [
        (269816065, "BB_金剛",         "战舰ㄐ级"),
        (538251521, "BB_金剛改",       "战舰ㄐ级"),
        (806686977, "BB_金剛改二",     "战舰ㄐ级"),
        (1008029953,"BB_金剛改二丙",   "战舰ㄐ级"),
        (269811969, "BB_扶桑",         "战舰ㄐ级"),
        (538263809, "BB_扶桑改",       "战舰ㄐ级"),
        (806699265, "BB_扶桑改二",     "战舰ㄐ级"),
    ]

    all_tests = dd_tests + dd_mix + cl_tests + ca_tests + bb_tests
    for entry in all_tests:
        sids_or_sid, tag, acls = entry
        fids = sids_or_sid if isinstance(sids_or_sid, list) else [sids_or_sid]
        names = [SHIP_NAMES.get(s, "?") for s in fids]
        write_test(tag, fids, names, acls)

    print(f"Wrote test files to {OUTDIR}")

if __name__ == "__main__":
    main()
