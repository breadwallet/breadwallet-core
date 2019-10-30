/*
 * Address
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface Address {

    /**
     * Create an Address from `string` and `network`.
     *
     * The provided `string` must be valid for the provided `network` - that is, an ETH address
     * (as a string) differs from a BTC address and a BTC mainnet address differs from a BTC
     * testnet address.  If `string` is not appropriate for `network`, then `nil` is returned.
     *
     * This function is typically used to convert User input - of a 'target address' as a string -
     * into an Address.
     *
     * @param address A string representing a crypto address
     * @param network The network for which the string is value
     * @return An address or absent if `string` is invalid for `network`
     */
    static Optional<Address> create(String address, Network network) {
        return CryptoApi.getProvider().addressProvider().create(address, network);
    }

    String toString();

    boolean equals(Object o);

    int hashCode();
}
