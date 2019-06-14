/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public interface BRCryptoWalletEventType {

    int CRYPTO_WALLET_EVENT_CREATED = 0;
    int CRYPTO_WALLET_EVENT_CHANGED = 1;
    int CRYPTO_WALLET_EVENT_DELETED = 2;
    int CRYPTO_WALLET_EVENT_TRANSFER_ADDED = 3;
    int CRYPTO_WALLET_EVENT_TRANSFER_CHANGED = 4;
    int CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED = 5;
    int CRYPTO_WALLET_EVENT_TRANSFER_DELETED = 6;
    int CRYPTO_WALLET_EVENT_BALANCE_UPDATED = 7;
    int CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED = 8;
}
