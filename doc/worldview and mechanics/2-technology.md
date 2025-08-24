# Technology

You control a fleet headquarters that mainly consists of Kanmusu(ship girl)s and their equipment.

Technology level is the most important aspect of the game progression, defined by the weighted average of your eligible ships and equipment.

Constructing ships and Developing equipment which is precisely on your current technology level have a 50% success rate. For deprecated technology the rate is higher but always lesser than 100%. On the other hand, forcefully constructing or developing something far ahead than your current technology level give heavy penalties. You are better off trying something 1-3 level ahead to boost your overall/local level and avoid a hell-like success rate of less than 2%.

## How a combined level is calculated from its components

[Implemented in: Tech::calLevel]
$$
\text{Level}=\sum_{i=1}^{\infty}v_i(a^{w_i}-1)a^{-\sum_{j=1}^iw_j}
$$

$v_i$ is each component's own characteristic tech level, in descending order. $w_i$ is the weight of $v_i$ (for example, more experience on an equipment have more weight).

Note the level never drops unless you remove some of the components(e.g. destruct an equipment)

### Constants

$a$: [settings: rule/techglobalscope](settings.md) when calculating global tech, [settings: rule/techlocalscope](settings.md) when calculating local tech.

## Weight of components

### Weight of ships

[Implemented in Tech::calWeightShip] A Kanmusu's weight when calculating its technology is its [level]/[settings: rule/shiplevelperweight](settings.md).

### Weight of equipment, taking skill points into account 

[Implemented in Tech::calWeightEquip] An equipment's weight when calculating its technology is
$$
\frac{by}{\sqrt{x^2+y^2}}+1
$$
where $y$ is [actual skill points], $x$ is [required skill points], $b$ is [settings: rule/skillpointweightcontrib](settings.md).

We will deal with skill points later.

## Success rate

[Implemented in Tech::calExperiment]

When developing/constructing the success rate would be:
$$
\text{Successrate}=\int_{t_{\text{wanted}}-t_{\text{current}}}^{\infty}\frac{1}{\sigma\sqrt{2\pi}}e^{-\frac{1}{2}(\frac{x}{\sigma})^2}dx
$$

$σ$: [rule/sigmaconstant](settings.md)

$t_{\text{wanted}}$: the tech of the equipment/ship you want.

$t_{\text{current}}$: the highest among your global tech and your local tech for this equipment/ship, minus [combined effects] (see below).

## Global technology

[Implemented in Server::calculateTech]

All Kanmusu and equipment will count toward the calculation.

## Local technology

[Implemented in Server::calculateTech]

For ships: All ships of the same class and all equipment she can handle will count toward the calculation. [NOTYETIMPLEMENTED]

For equipment: All equipment of this exact type and a virtual equipment of this exact type (without base weight of 1.0, meaning only skill point effect will count [Implemented in Server::calculateTech#virtual_skill_point_effect]) and [its "father" and all paternal children on the technology tree], and [all Kanmusu who have visible bonuses related to this equipment] [NOTYETIMPLEMENTED] will count toward the calculation. The two concepts will be dealt later.

An equipment that does not have a predecessor will have it's local technology not counted when developing.

## Combined effects 

[Implemented in Tech::calCapable]

The highest among global and local would be in effect. However local technology x level behind will cause a detrimental effect of
$$
-\frac{cx}{\sqrt{1+x^2}}
$$
be applied on global technology (the reverse is also true). The resulting technology will NOT be lower than the lowest among global and local technology.

$c$: [rule/techcombinedeffects](settings.md)

## How characteristic tech level are determined

[Implemented in Tech::techYearToCompact]

Generally (variations may exist):

| year this ship/equipment is functional | tech level |
| :------------------------------------: | :--------: |
|                  1908                  |     0      |
|                  1912                  |    0.25    |
|                  1916                  |    0.5     |
|                  1920                  |    0.75    |
|                  1924                  |     1      |
|                  1925                  |    1.33    |
|                  1926                  |    1.67    |
|                  1927                  |     2      |
|                  1928                  |    2.33    |
|                  1929                  |    2.67    |
|                  1930                  |     3      |
|                  1931                  |    3.33    |
|                  1932                  |    3.67    |
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

