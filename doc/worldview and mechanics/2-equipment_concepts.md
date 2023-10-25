# Equipment concepts

## Developing

Each player starts with 4 factory slots where they develop equipment and construct ships. Developing takes time(unlike KC) and have a success rate mentioned in technology page. As the game progress one may gain more factory slots up to 24.

## Father and mother equipment

Many equipment have a father, and in this case you must possess the father equipment in order to develop it.

Less commonly some equipment has a mother, in case of virtual one (id>=16384) you need to fulfill some specific condition in order to develop it. If it is a real one, developing the son equipment cost skill points of the mother equipment and further effects are described below.

## Skill points

Each equipment have a standard skill point determined by its tech level:
$$
S=(\frac{5}{4})^{tech}\times10000
$$
Gain: equal to kanmusu's, unless its "mother" is a real equipment, then it could NOT gain skill points in battle but can transfer skill points from its "mother equipment" at a ratio of (\sqrt{tech of "son equipment"} : 1)

Losses: Kanmusu mounting it get medium damage: ?x (TBD)

Heavy damage ?x (TBD)

At 0% stand skill points the equipment is 29.29% effective, at 100% points 100% effective. Maximum effectiveness is 129.29% (2-\sqrt{0.5}). Therefore an inferior equipment with enough skill points may outperform a superior but unskilled one.
$$
Efficiency = 1-\sqrt{\frac{1}{2}}+\frac{y}{\sqrt{x^2+y^2}}\space where\space x=stand\space skillpoint,\space y=actual\space skillpoint
$$

## Improvement

You can improve any equipment at the cost of 100% of standard skill point. An equipment improved a times would have its efficiency added by
$$
(star/10)*(\sqrt{\frac{1}{2}}-\frac{1}{2})
$$


## Prototype equipment

Some equipment have "disallow mass production" attribute, when developing it while possessing above maximum (determined by the value of "disallow mass production") the equipment can no longer be developed.

