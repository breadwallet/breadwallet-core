/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoTransferDirection;
import com.breadwallet.corenative.crypto.BRCryptoTransferState;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerState;
import com.breadwallet.corenative.crypto.BRCryptoWalletState;
import com.breadwallet.corenative.support.BRSyncMode;
import com.breadwallet.crypto.TransferDirection;
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.WalletState;

/* package */
final class Utilities {

    /* package */
    static int walletManagerModeToCrypto(WalletManagerMode mode) {
        switch (mode) {
            case API_ONLY: return BRSyncMode.SYNC_MODE_BRD_ONLY;
            case API_WITH_P2P_SUBMIT: return BRSyncMode.SYNC_MODE_BRD_WITH_P2P_SEND;
            case P2P_ONLY: return BRSyncMode.SYNC_MODE_P2P_ONLY;
            case P2P_WITH_API_SYNC: return BRSyncMode.SYNC_MODE_P2P_WITH_BRD_SYNC;
            default: throw new IllegalArgumentException("Unsupported mode");
        }
    }

    /* package */
    static int walletManagerStateToCrypto(WalletManagerState state) {
        switch (state) {
            case CREATED: return BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_CREATED;
            case DELETED: return BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_DELETED;
            case CONNECTED: return BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_CONNECTED;
            case DISCONNECTED: return BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED;
            case SYNCING: return BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_SYNCING;
            default: throw new IllegalArgumentException("Unsupported state");
        }
    }

    /* package */
    static WalletManagerState walletManagerStateFromCrypto(int state) {
        switch (state) {
            case BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_CREATED: return WalletManagerState.CREATED;
            case BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_DELETED: return WalletManagerState.DELETED;
            case BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_CONNECTED: return WalletManagerState.CONNECTED;
            case BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED: return WalletManagerState.DISCONNECTED;
            case BRCryptoWalletManagerState.CRYPTO_WALLET_MANAGER_STATE_SYNCING: return WalletManagerState.SYNCING;
            default: throw new IllegalArgumentException("Unsupported state");
        }
    }

    /* package */
    static int walletStateToCrypto(WalletState state) {
        switch (state) {
            case CREATED: return BRCryptoWalletState.CRYPTO_WALLET_STATE_CREATED;
            case DELETED: return BRCryptoWalletState.CRYPTO_WALLET_STATE_DELETED;
            default: throw new IllegalArgumentException("Unsupported state");
        }
    }

    /* package */
    static WalletState walletStateFromCrypto(int state) {
        switch (state) {
            case BRCryptoWalletState.CRYPTO_WALLET_STATE_CREATED: return WalletState.CREATED;
            case BRCryptoWalletState.CRYPTO_WALLET_STATE_DELETED: return WalletState.DELETED;
            default: throw new IllegalArgumentException("Unsupported state");
        }
    }

    /* package */
    static TransferDirection transferDirectionFromCrypto(int direction) {
        switch (direction) {
            case BRCryptoTransferDirection.CRYPTO_TRANSFER_RECEIVED: return TransferDirection.RECEIVED;
            case BRCryptoTransferDirection.CRYPTO_TRANSFER_SENT: return TransferDirection.SENT;
            case BRCryptoTransferDirection.CRYPTO_TRANSFER_RECOVERED: return TransferDirection.RECOVERED;
            default: throw new IllegalArgumentException("Unsupported direction");
        }
    }

    /* package */
    static TransferState transferStateFromCrypto(int state) {
        switch (state) {
            // TODO(fix): state contains parameters (FAILED and INCLUDED)
            case BRCryptoTransferState.CRYPTO_TRANSFER_STATE_CREATED: return TransferState.CREATED();
            case BRCryptoTransferState.CRYPTO_TRANSFER_STATE_DELETED: return TransferState.DELETED();
            case BRCryptoTransferState.CRYPTO_TRANSFER_STATE_ERRORRED: return TransferState.FAILED(null);
            case BRCryptoTransferState.CRYPTO_TRANSFER_STATE_INCLUDED: return TransferState.INCLUDED(null);
            case BRCryptoTransferState.CRYPTO_TRANSFER_STATE_SIGNED: return TransferState.SIGNED();
            case BRCryptoTransferState.CRYPTO_TRANSFER_STATE_SUBMITTED: return TransferState.SUBMITTED();
            default: throw new IllegalArgumentException("Unsupported state");
        }
    }
}
