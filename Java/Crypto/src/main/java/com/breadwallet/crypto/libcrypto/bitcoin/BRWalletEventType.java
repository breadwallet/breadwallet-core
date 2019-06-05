/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.bitcoin;

public interface BRWalletEventType {

    int BITCOIN_WALLET_CREATED = 0;
    int BITCOIN_WALLET_BALANCE_UPDATED = 1;
    int BITCOIN_WALLET_TRANSACTION_SUBMITTED = 2;
    int BITCOIN_WALLET_DELETED = 3;
}
