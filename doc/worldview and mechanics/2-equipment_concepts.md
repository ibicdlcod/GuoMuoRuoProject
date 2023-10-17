# Equipment concepts

## Father and mother equipment

Many equipment have a father, and in this case you must possess the father in order to develop it.

Less commonly some equipment has a mother, in case of virtual one (>16384) you need to fulfill some specific condition in order to develop it. If it is a real one, developing the "son" cost skill points of the "mother" and further effects are described below.

## Skill points

Each equipment have a standard skill point determined by its tech level:
$$
S=(\frac{5}{4})^{tech}10000
$$
Gain: equal to kanmusu's, unless its "mother" is a real equipment, then it could not gain skill points in battle but can transfer skill points from its "mother equipment" at a ratio of (tech of "son equipment" : 1)

Losses: Kanmusu mounting it get medium damage: ?x,

Heavy damage ?x (TBD)

At 100% standard skill point the equipment is 100% effective. Maximum effectiveness is lower than 141.4%.
$$
Efficiency = \frac{\sqrt{2}y}{\sqrt{x^2+y^2}}\space where\space x=stand\space skillpoint,\space y=actual\space skillpoint
$$

## Improvement

You can improve any equipment at the cost of 100% of standard skill point. An equipment improved a times would have its actual skill point added by (log_10(a))*standard (a<=10);

## Prototype equipment

Some equipment have "disallow mass production" attribute, when developing it while possessing above maximum (determined by the value of "disallow mass production") the cost goes up exponentially.
