## Technology

You control a fleet headquarters that mainly consists of Kanmusu(ship girl)s and their equipment.

Technology level is the most important aspect of the game, defined by the weighted average of your eligible ships and equipment.

Constructing ships and Developing equipment which is precisely on your current technology level have a 50% success rate. For deprecated technology the rate is higher but always lesser than 100%. On the other hand, forcefully constructing or developing something far ahead than your current technology level give heavy penalties. You are better off trying something 1-3 level ahead to boost your overall/local level and avoid a hell-like success rate of less than 2%.

## How the level is calculated

$$
level=\sum_{i=1}^{\infty}v_i(a^{w_i}-1)a^{-\sum_{j=1}^iw_j}
$$

v_i is each component's own characteristic tech level, in descending order. w_i is the weight of v_i.

Note the level never drops during gameplay (unless a is changed)

a: TBD (would be different for global and local)

## Weight of components

A Kanmusu's weight when calculating its technology is its [level]/10.

An equipment's weight when calculating its technology is
$$
\frac{9x}{\sqrt{1+x^2}}+1
$$
where x is [actual skill points]/[required skill points].

We will deal with skill points later.

## Bonuses and penalties

$$
successrate=\int_{t_{wanted}-t_{current}}^{\infty}\frac{1}{\sigma\sqrt{2\pi}}e^{-\frac{1}{2}(\frac{x}{\sigma})^2}dx
$$

σ: TBD

(just a normal distribution, chill)

## Global technology

All Kanmusu and equipment will count toward the calculation.

## Local technology

For ships: All ships of the same class and all equipment she can handle will count toward the calculation.

For equipment: All equipment of this exact type and [its children on the technology tree], and [all Kanmusu it is particularly suited to] will count toward the calculation. The two concepts will be dealt later.

## Combined Effects

The highest among global and local would be in effect. However local technology x level behind will cause a detrimental effect of
$$
-\frac{cx}{\sqrt{1+x^2}}
$$
be applied on global technology (the reverse is also true)

c: Difference of global and local (may subject to change)

## How characteristic tech level are determined

Generally (variations may exist): [(date this ship/equipment is functional)-(Jul 1 1921)]/500 days