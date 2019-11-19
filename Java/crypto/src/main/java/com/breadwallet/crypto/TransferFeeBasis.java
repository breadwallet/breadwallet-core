/*
 * TransferFeeBasis
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public interface TransferFeeBasis {

    Unit getUnit();

    Currency getCurrency();

    Amount getPricePerCostFactor();

    double getCostFactor();

    Amount getFee();
}
