---
name: kancolle-information-gathering
description: Understanding map design philosophy and patterns of Kantai Colletion thus aid the design of maps in this project
---

Kancolle have maps X-Y where normal maps have 1<=X<=7 and 1<=Y<=6. Event maps have 42<=X<=61 and 1<=Y<=7. (not all combinations are present)

Rules for gathering information from tsunkit:

1. map [X-Y] have its information at https://tsunkit.net/nav/X-Y
2. The page contains many useful information, especially when click on "Node" or "Edge" and following up on links.  If you find a good way to extract a specific type of information from the html/API, memorize it by updating this skill file.
3. If you can't determine map routing characteristics for a node based on fleet composition, it might be a LOS check.
4. For understanding various ship type's role, refer to https://wikiwiki.jp/kancolle/%E8%89%A6%E7%A8%AE%E3%81%94%E3%81%A8%E3%81%AE%E7%89%B9%E5%BE%B4
5. Output detailed design philosophy in doc/design_philosophy/kancolle-map/X-Y.md

Rules for gathering information from kancolle wiki:

1. (TBD)