# Ship classes lacking WoWS concealment data

These ship classes have no World of Warships equivalent, so their concealment values
remain unchanged from their original settings in `Ship.xlsx`.

## Japanese ships (no WoWS equivalent)

### Seaplane Tenders / Auxiliary
- 千歳型
- 日進型
- 瑞穂型
- 秋津洲型
- 明石型
- 神威型
- 迅鯨型
- 大鯨型
- 改氷川丸級

### Carriers (no WoWS page / removed post-rework)
- 赤城型 (Akagi – premium; wiki page lacks performance data)
- 大鷹型
- 飛鷹型
- 祥鳳型
- 龍鳳型
- 雲龍型
- 特設航空母艦
- 春日丸級

### Escort / Coastal Defense
- 占守型
- 択捉型
- 日振型
- 御蔵型
- 鵜来型
- 丁型海防艦
- 松型

### Submarines
- 巡潜乙型
- 巡潜乙型改一
- 巡潜乙型改二
- 巡潜丙型
- 巡潜甲型改二
- 巡潜3型
- 海大VI型
- 潜特型
- 潜高型
- 呂号潜水艦
- 三式潜航輸送艇

### Amphibious / Transport / Misc
- 大淀型
- 改敷島型
- 改風早型
- 大泊型
- 特1TL型
- 特2TL型
- 特種船丙型
- 特種船M丙型
- 二等輸送艦
- 陸軍特種船(R1)
- 香取型
- 改装陽炎型 (Taiwan/R.O.C. ship)
- 耐氷型雑用運用艦
- （陸軍）

## Foreign ships (no WoWS wiki page or 404)

### Italy
- Aquila級 (carrier; wiki page not found)
- Marcello級 (submarine; not in WoWS)
- Guglielmo Marconi級 (submarine; not in WoWS)

### Germany
- UボートIXC型 (submarine; not in WoWS)

### USA
- Casablanca級 (escort carrier; wiki page not found)
- Gato級 (submarine; not in WoWS)
- Salmon級 (submarine; not in WoWS)
- John C.Butler級 (destroyer escort; not in WoWS)

### France
- C.Teste級 (seaplane tender; wiki page not found)

### Sweden
- Gotland級 (wiki page not found)

## Methodology

Concealment is calculated as `round(500 / surface_detectability_km)` using
data from the [World of Warships wiki](https://wiki.wargaming.net).

## Notes on substitutions

Some classes use a **sister-ship substitution** when the exact class page is unavailable:

| Spreadsheet class | WoWS page used | Reason |
|---|---|---|
| 綾波型 | Fubuki | Ayanami is Fubuki-subclass |
| 暁型 | Akatsuki | Direct match |
| 長良型 | Kuma | Nagara not a separate WoWS ship |
| 川内型 | Kuma | Sendai not a separate WoWS ship |
| 蒼龍型 | Ryūjō | Hiryū wiki page lacks performance data |
| 飛龍型 | Ryūjō | Hiryū wiki page lacks performance data |
| 大鳳型 | Shōkaku | Taihō wiki page times out (504 Gateway) |
| Brooklyn級 | Helena | Brooklyn-class uses St. Louis subclass |
| South Dakota級 | Massachusetts | Same class |
| Nevada級 | New Mexico | Same tier US Standard BB |
| Northampton級 | New Orleans | Successor heavy cruiser class |
| Illustrious級 | Indomitable | Same class |
