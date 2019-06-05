/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.bitcoin;

public interface BRTransactionEventType {

    int BITCOIN_TRANSACTION_ADDED = 0;
    int BITCOIN_TRANSACTION_UPDATED = 1;
    int BITCOIN_TRANSACTION_DELETED = 2;
}
