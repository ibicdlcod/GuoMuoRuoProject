# Battle mechanic

The game features a battle mechanic somewhat different from Kantai Collection; while not intented to be entirely realistic, some improvements are made.

Unlike KC (but similar to KC arcade), some phase of battle have an internal clock which together with the firing speed attribute (absent in KC), simulates reloading of guns (torpedos have their own reloading mechanism).

Sunk ships (HP ≤ 0) cannot make any attacks — their pending
events are silently dropped when the attack-processing function
encounters the HP guard.

The battle have the following characteristics:

- [Tactical Goal (for friend fleet)](9.c1-goal-friend.md)
  [Implemented in Battle::battleProcessor#friend-goal,
   Battle::isProtectedShip]
- [Tactical Goal (against enemy fleet)](9.c2-goal-enemy.md)
  [Implemented in Battle::battleProcessor#enemy-goal,
   Battle::isPrioritizedTarget]
- Target selection (see below)
  [Implemented in Battle::selectEnemyTarget,
   Battle::selectFriendTarget]
- [Communication efficiency](9.c3-communication.md)
  [Implemented in Battle::battleProcessor#communication-init]
- [LOS, undiscovered ships and surprise attacks](9.c4-los.md)
  [Implemented in Battle::decideHidden,
   Battle::forceVisible]
- [Air superiority coefficient](9.c5-air-superiority.md)
  [Implemented in Battle::computeAirSuperiority,
   Battle::fleetAirSuperiority]
- [Guided strikes (触接)](9.c6-guidedstrikes.md)
  [Implemented in Battle::processReconGuidedStrike,
   Battle::processMidgetGuidedStrike]
- [Formation efficiency](9.c7-formation.md)
  [Implemented in Battle::computeFormationEfficiency]
- [Commander's abilities](9.c8-abilities.md)
- [LBAS](9.c9-lbas.md)
- Radiation (TBD)

In the battle the following attack against enemies can occur:

- [Air attack ](9.a1-airattack.md)
  [Implemented in Battle::airBattle,
   Battle::processAirAttack]
   
  - Air bombing + individual anti-air
  - Air torpedo + individual anti-air
  - Air attack cut-in + individual anti-air
  - Jet plane attack (TBD)

- [Gunshot](9.a2-gunshot.md)
  [Implemented in Battle::processMainGunAttack,
   Battle::processSecondaryGunAttack]
  
  - Main gun gunshot
  - Secondary gun gunshot
  - Gunshot cut-in

- [Torpedo attack](9.a3-torpedoattack.md)
  [Implemented in Battle::processTorpedoAttack]
   
  - Torpedo reloading device
  - Torpedo attack
  - Torpedo cut-in
  - Gunshot+Torpedo cut-in

- [ASW attack](9.a4-asw.md) [NOTIMPLEMENTED]
  
  - ASW detection vs concealment
  - ASW depth charge attack
  - ASW aerial attack
  - ASW cut-in

- Land attack [NOTIMPLEMENTED]
  
  - General land attack
  - Soft attack
  - Hard attack

The battle is organized into the following phases:

- [Air battle](9.p1-airbattle.md) (0s clock)
- [Approaching phase](9.p2-approaching.md) (20s clock, actual attack clock depending on max firing range, speed and LOS)
- [Central phase](9.p3-central.md) (90s clock)
- [Disengaging phase](9.p4-disengage.md) (clock depending on speed and LOS) (If we want night battle, we prevent them disengage; if we don't, we try to disengage)
- [Night battle](9.p5-nightbattle.md) (30s clock)

The battle have the following special modes worthy of attention:

- [Night-battle-start node](9.s1-nightstart.md) 
- [Air-battle-only node](9.s2-aironly.md) [NOTIMPLEMENTED]

## Target selection

When friend fleet attack enemy fleet, "Tactical Goal (against enemy fleet)" is activated and ships will select randomly from a list of **visible** enemy ships. If the target is prioritized, it is selected; else another random selection would be made.

When enemy fleet attack friend fleet, "Tactical Goal (for friend fleet)" is activated and enemy ships will select randomly from a list of **visible** friend ships. If the target is not protected, it is selected; else another random selection would be made.

Of course, ships that are sunk (hp <= 0) is excluded from target selection.
