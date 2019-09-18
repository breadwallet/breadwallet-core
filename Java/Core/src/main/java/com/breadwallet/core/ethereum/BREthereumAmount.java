/*
 * EthereumAmount
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/21/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core.ethereum;

public abstract class BREthereumAmount {

    //
    // The Unit to use when displaying amounts - such as a wallet balance.
    //
    public enum Unit {
        TOKEN_DECIMAL(0),
        TOKEN_INTEGER(1),

        ETHER_WEI(0),
        ETHER_GWEI(3),
        ETHER_ETHER(6);

        // jniValue must match Core enum for:
        //    BREthereumUnit and BREthereumTokenQuantityUnit
        protected long jniValue;

        Unit(long jniValue) {
            this.jniValue = jniValue;
        }

        public boolean isTokenUnit () {
            switch (this) {
                case TOKEN_DECIMAL:
                case TOKEN_INTEGER:
                    return true;
                default:
                    return false;
            }
        }
    };
}
