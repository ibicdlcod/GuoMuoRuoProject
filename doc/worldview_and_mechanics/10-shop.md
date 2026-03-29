# Shop

The game features a shop for spending real money or premium currency. The shop is only accessible while online.

## ARD Coupons

ARD coupons are the premium currency of the game. Each coupon unit represents 0.01 HKD of value at face rate.

### Purchasing coupons

[Implemented in Server::handleInitARDPurchase]

[Implemented in Server::handleARDPurchaseAuth]

ARD coupons are purchased through Steam's microtransaction system. The client initiates a purchase; Steam presents the player with a payment dialog. Once the player authorises, the server finalises the transaction and credits the coupons.

The actual price paid includes a volume discount: the more you buy at once, the lower the cost per coupon.

$$
P = \frac{n \cdot 0.01}{\log_{65537}(0.01 \cdot 65536n + 1)}
$$

$P$: actual price in HKD

$n$: number of coupon units purchased

### Refunds and chargebacks

[Implemented in Server::pollARDRefunds]

The server periodically polls Steam for refunded, partially refunded, and charged-back transactions. When such a transaction is found the corresponding coupon units are clawed back from the player's balance. The balance may go negative as a result.

## Buying equipment

[Implemented in Server::doBuyFromStore]

[Implemented in Equipment::availableInStore]

Certain equipment items are listed in the store and can be purchased directly with ARD coupons. An equipment item is available in the store if and only if its `Storeprice` attribute is positive; that attribute gives the price in coupon units. Purchasing deducts the coupons and immediately adds one instance of the equipment to the player's inventory.

## Medals

[Implemented in Server::doBuyMedal]

Medals are purchased with ARD coupons at a fixed rate of 999 coupons per medal. Medals are used for ship decoration, which raises a ship's experience cap — see [experience and modernization](5.7-experience.md).
