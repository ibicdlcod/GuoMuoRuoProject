# Battle mechanic

The game features a battle mechanic somewhat different from Kantai Collection; while not intented to be entirely realistic, some improvements are made.

Unlike KC (but similar to KC arcade), some phase of battle have an internal clock which together with the firing speed attribute (absent in KC), simulates reloading of guns (torpedos have their own reloading mechanism).

The battle have the following characteristics:

- [Tactical Goal (for friend fleet)](9.c1-goal-friend.md)
- [Tactical Goal (against enemy fleet)](9.c2-goal-enemy.md)
- [Communication efficiency](9.c3-communication.md)
- [LOS, undiscovered ships and surprise attacks](9.c4-los.md)
- [Air superiority coefficient](9.c5-air-superiority.md)
- [Guided strikes (触接)](9.c6-guidedstrikes.md)
- [Formation efficiency](9.c7-formation.md)
- [Commander's abilities](9.c8-abilities.md)
- [LBAS](9.c9-lbas.md)
- Radiation (TBD)

In the battle the following attack against enemies can occur:

- [Air attack](9.a1-airattack.md)
  - Air bombing + individual anti-air
  - Air torpedo + individual anti-air
  - Air attack cut-in + individual anti-air
  - Jet plane attack (TBD)
- [Gunshot](9.a2-gunshot.md)
  - Main gun gunshot
  - Secondary gun gunshot
  - Gunshot cut-in
- [Torpedo attack](9.a3-torpedoattack.md)
  - Torpedo attack
  - Torpedo reloading device
  - Torpedo cut-in
  - Gunshot+Torpedo cut-in
- [ASW attack](9.a4-asw.md)
  - ASW detection vs concealment
  - ASW cut-in
- [Land attack](9.5-landattack.md)
  - General land attack
  - Soft attack
  - Hard attack

The battle is organized into the following phases:

- [Air battle](9.p1-airbattle.md)
- [Approaching phase](9.p2-approching.md) (clock depending on max firing range and LOS)
- [Central phase](9.p3-central.md) (90s clock)
- [Disengaging phase](9.p4-disengage.md) (clock depending on speed and LOS) (If we want night battle, we prevent them disengage; if we don't, we try to disengage)
- [Night battle](9.p5-nightbattle.md) (30s clock)

The battle have the following special modes worthy of attention:

- [Night-battle-start node](9.s1-nightstart.md)
- [Air-battle-only node](9.s2-aironly.md)
