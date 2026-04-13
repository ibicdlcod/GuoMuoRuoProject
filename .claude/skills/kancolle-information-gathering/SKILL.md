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

1. Normal maps are at https://wikiwiki.jp/kancolle/%E5%87%BA%E6%92%83. Note [鎮守府海域](https://wikiwiki.jp/kancolle/出撃#k9353bd7) is X=1, [南西諸島海域](https://wikiwiki.jp/kancolle/出撃#e9a8e47c) is X=2, [北方海域](https://wikiwiki.jp/kancolle/出撃#q2b974dd) is X=3, [南西海域](https://wikiwiki.jp/kancolle/出撃#q2b9jkdf) is X=7, [西方海域](https://wikiwiki.jp/kancolle/出撃#m93e7305) is X=4, [南方海域](https://wikiwiki.jp/kancolle/出撃#r83debbc) = X=5, [中部海域](https://wikiwiki.jp/kancolle/出撃#n3e79a3d) is X=6
2. Event maps are at https://wikiwiki.jp/kancolle/%E6%9C%9F%E9%96%93%E9%99%90%E5%AE%9A%E5%87%BA%E6%92%83. Note [逆転！ナルヴィク攻防戦](https://wikiwiki.jp/kancolle/期間限定出撃#q0922cf7) is X=61 and each link below that in https://wikiwiki.jp/kancolle/%E6%9C%9F%E9%96%93%E9%99%90%E5%AE%9A%E5%87%BA%E6%92%83 is X one less
3. Combine and update the detailed design philosophy in doc/design_philosophy/kancolle-map/X-Y.md