# Technology

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
\frac{9y}{\sqrt{x^2+y^2}}+1
$$
where y is [actual skill points], x is [required skill points].

We will deal with skill points later.

## Bonuses and penalties 

When developing/constructing the success rate would be:
$$
successrate=\int_{t_{wanted}-t_{current}}^{\infty}\frac{1}{\sigma\sqrt{2\pi}}e^{-\frac{1}{2}(\frac{x}{\sigma})^2}dx
$$

σ: TBD

(just a normal distribution, chill)

## Global technology

All Kanmusu and equipment will count toward the calculation.

## Local technology

For ships: All ships of the same class and all equipment she can handle will count toward the calculation.

For equipment: All equipment of this exact type and [its "father" and all paternal children on the technology tree], and [all Kanmusu it is particularly suited to] will count toward the calculation. The two concepts will be dealt later.

An equipment that does not have a predecessor will have it's local technology not counted when developing.

## Combined Effects

The highest among global and local would be in effect. However local technology x level behind will cause a detrimental effect of
$$
-\frac{cx}{\sqrt{1+x^2}}
$$
be applied on global technology (the reverse is also true)

c: 3

## How characteristic tech level are determined

Generally (variations may exist):

| year this ship/equipment is functional | tech level |
| :------------------------------------: | :--------: |
|                  1908                  |     0      |
|                  1912                  |    1/4     |
|                  1916                  |    1/2     |
|                  1920                  |    3/4     |
|                  1924                  |     1      |
|                  1925                  |    4/3     |
|                  1926                  |    5/3     |
|                  1927                  |     2      |
|                  1928                  |    7/3     |
|                  1929                  |    8/3     |
|                  1930                  |     3      |
|                  1931                  |    10/3    |
|                  1932                  |    11/3    |
|                  1933                  |     4      |
|                  1934                  |    4.5     |
|                  1935                  |     5      |
|                  1936                  |    5.5     |
|                  1937                  |     6      |
|                  1938                  |    6.5     |
|                  1939                  |     7      |
|                  1940                  |    7.5     |
|                  1941                  |     8      |
|                  1942                  |     9      |
|                  1943                  |     10     |
|                  1944                  |     11     |
|                  1945                  |     12     |
|                  1946                  |     13     |
|                  1947                  |    13.5    |
|                  1948                  |     14     |
|                  1949                  |   14.25    |
|                  1950                  |    14.5    |
|                  1951                  |   14.75    |
|                  1952                  |     15     |
|                  1953                  |   15.25    |
|                  1954                  |    15.5    |
|                  1955                  |   15.75    |
|                  1956                  |     16     |

