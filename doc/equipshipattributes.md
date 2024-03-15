|         Field          |                      Equipment meaning                       |                         Ship meaning                         |
| :--------------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
|          No.           |                   Equipment definition ID                    |                      Ship definition ID                      |
|         OldNo.         |                              -                               |                Ship ID in KC (yyb is 0xb00yy)                |
|        Remodel         |                              -                               |         Remodel level 0, 1, 2... ya/yb is 0xay/0xby          |
|       Remodelto        |                              -                               |                                                              |
|      Remodellevel      |                              -                               |                                                              |
|          Uuid          |                         Instance ID                          |                         Instance ID                          |
|         ja_JP          |                        Japanese name                         |                        Japanese name                         |
|         zh_CN          |              Chinese name (not yet implemented)              |              Chinese name (not yet implemented)              |
|         en_US          |              English name (not yet implemented)              |              English name (not yet implemented)              |
|   equiptype/shiptype   |                        Equipment type                        |                          Ship type                           |
|       shipclass        |                              -                               |                          Ship class                          |
|          種別          |                           (Unused)                           |                                                              |
|          Tech          | Tech level (may not correspond to actual year said equipment appears, for game balance reasons) | Tech level (may not correspond to actual year said ship appears, for game balance reasons) |
|         Father         |  You must own "Father" equipment to develop this equipment   |                              -                               |
|        Father2         |                            ditto                             |                              -                               |
|         Mother         | You must have enough skillpoints at "Mother" equipment to develop this equipment |                              -                               |
|     Equipfather1~4     |                              -                               |    Equipment that you must possess to construct this ship    |
| Disallowmassproduction | Maximum number of this equipment you may have (infinite if not present, -1 means production disabled) | Always 1, you need to specifically clone ships rather than construct more of them |
|       Hitpoints        |              Equivalent to 0.1 hitpoints in KC.              |                            ditto                             |
|       Firepower        |  Equivalent to 0.1 firepower in KC, countered by hitpoints.  |                            ditto                             |
|         Armor          | Equivalent to 0.5 armor in KC, countered by armorpenetration. Have stacking penalty. |                            ditto                             |
|    Armorpenetration    |                    Have stacking penalty.                    |                            ditto                             |
|        Accuracy        |   Equivalent to 0.1 accuracy in KC, countered by evasion.    |                            ditto                             |
|    Torpedoaccuracy     |      Like accuracy but only applies to torpedo attacks.      |                            ditto                             |
|        Evasion         |               Equivalent to 0.1 evasion in KC.               |                            ditto                             |
|          Los           |      Have stacking penalty (on same type of equipment).      |                            ditto                             |
|      Concealment       | Decrease the likelihood of enemy targeting this ship. A well-concealed fleet can prevent enemy from firing, countered by los. |                            ditto                             |
|      Firingrange       | A long range and high los may increase the effective battle time. |                            ditto                             |
|      Firingspeed       | A high firing speed may increase the shots fired in a given battle time. |                       0 for all ships                        |
|         Speed          | in knots. A higher speed than enemy increases battle time, evasion and los. |                            ditto                             |
|        Torpedo         |               Equivalent to 0.1 torpedo in KC.               |                            ditto                             |
|        Bombing         |               Equivalent to 0.1 bombing in KC.               |                       0 for all ships                        |
|        Antiair         |               Equivalent to 0.1 antiair in KC.               |                            ditto                             |
|          Asw           |                 Equivalent to 0.1 asw in KC.                 |                            ditto                             |
|      Interception      |                          Unchanged.                          |                       0 for all ships                        |
|       Antibomber       |                          Unchanged.                          |                       0 for all ships                        |
|        Antiland        | Anti land-based enemy capabilities. Bomber with antiland=0 does not attack them. |                            ditto                             |
|       Transport        |                   Transport capabilities.                    |                            ditto                             |
|      Flightrange       |                          Unchanged.                          |                       0 for all ships                        |