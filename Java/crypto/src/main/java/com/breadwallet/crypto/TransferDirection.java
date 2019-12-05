/*
 * TransferDirection
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public enum TransferDirection {
    SENT,
    RECEIVED,
    RECOVERED;

    @Override
    public String toString() {
        switch (this) {
            case RECOVERED:
                return "Recovered";
            case SENT:
                return "Sent";
            case RECEIVED:
                return "Received";
            default:
                return super.toString();
        }
    }
}
