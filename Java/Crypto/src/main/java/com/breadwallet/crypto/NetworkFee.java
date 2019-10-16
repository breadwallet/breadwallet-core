/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import java.lang.Object;

import com.google.common.primitives.UnsignedLong;

/**
 * A Network Fee represents the 'amount per cost factor' paid to mine a transfer. For BTC this
 * amount is 'SAT/BYTE'; for ETH this amount is 'gasPrice'.  The actual fee for the transfer depends
 * on properties of the transfer; for BTC, the cost factor is 'size in kB'; for ETH, the cost
 * factor is 'gas'.
 *
 * A Network supports a variety of fees.  Essentially the higher the fee the more enticing the
 * transfer is to a miner and thus the more quickly the transfer gets into the block chain.
 *
 * A NetworkFee implements {@link Object#equals(Object)} on the underlying Core representation.
 * It is natural to compare NetworkFee based on timeIntervalInMilliseconds.
 */
public interface NetworkFee {

    UnsignedLong getConfirmationTimeInMilliseconds();

    boolean equals(Object fee);
}
