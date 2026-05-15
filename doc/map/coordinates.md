# Map Coordinate Conversion

The game's world map is `resources/map/globe.png` (5632×2048 pixels), adapted from the Hearts of Iron IV wiki map. Each map union ID (1–86) has `x`/`y` pixel coordinates in `Map_nodes.csv` indicating its position on this image.

## Terminology

- **globe.png** — The world map image (5632×2048 px), hand-tuned projection approximating Miller cylindrical
- **Map (union ID)** — A battle playground, NOT the globe itself. Each map (union 1–86) is accompanied by a 5632×2048 viewport of the real-world OSM tile grid centered on its lat/lon location.
- **MapNode (node index)** — Individual nodes within a battle map (not relevant to globe positioning)

## globe.png → latitude/longitude

```
lon = (x / 5632) × 360 − 180

lat = c₂ × y² + c₁ × y + c₀

c₂ = −0.0000054474
c₁ = −0.049682
c₀ =  77.112
```

The latitude coefficients were calibrated from 10 known reference points (Tokyo, Pearl Harbor, Gibraltar, Cape Town, Panama, Port Moresby, Aberdeen, Marshall Islands, Malvinas, Barents Sea) using quadratic least-squares.

## latitude/longitude → OSM viewport tiles (zoom 8)

The viewport is 5632×2048 OSM pixels at zoom 8, which is **22 tiles wide × 8 tiles tall**. The top-left tile is stored in `Map_nodes.csv`.

```
n = 2⁸ = 256

center_tile_x = ⌊(lon + 180) / 360 × n⌋
center_tile_y = ⌊(1 − ln(tan(φ) + 1/cos(φ)) / π) / 2 × n⌋       (φ in radians, clamped ±85.05°)

openstreetmapx = center_tile_x − 11       (viewport left edge)
openstreetmapy = center_tile_y − 4        (viewport top edge)
```

The viewport covers tiles [openstreetmapx, openstreetmapx+21] × [openstreetmapy, openstreetmapy+7].

## OSM tile URL

```
https://tile.openstreetmap.org/{Z}/{tile_x}/{tile_y}.png
```
