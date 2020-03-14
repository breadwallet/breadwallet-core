/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/10/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoWalletMigrator;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

//
// This migrator requires a 'release' in order to fully relinquish migrator resources
// in a timely manner.  Specifically the migrator writes data to a SQLite DB (in Core)
// that is also used by a WalletManager.  In our implementation, there can not be two
// SQLite DB connections to the same DB file.
//

/* package */
final class WalletMigrator {

    /* package */
    static Optional<WalletMigrator> create(com.breadwallet.crypto.Network network, String storagePath) {
        Network cryptoNetwork = Network.from(network);
        Optional<BRCryptoWalletMigrator> core = BRCryptoWalletMigrator.create(cryptoNetwork.getCoreBRCryptoNetwork(), storagePath);
        return core.transform(WalletMigrator::create);
    }

    /* package */
    static WalletMigrator create(BRCryptoWalletMigrator core) {
        return new WalletMigrator(core);
    }

    private final BRCryptoWalletMigrator core;

    private WalletMigrator(BRCryptoWalletMigrator core) {
        this.core = core;
    }

    /* package */
    boolean handleTransactionAsBtc(byte[] bytes, UnsignedInteger blockHeight, UnsignedInteger timestamp) {
        return core.handleTransactionAsBtc(bytes, blockHeight, timestamp);
    }

    /* package */
    boolean handleBlockAsBtc(byte[] block, UnsignedInteger height) {
        return core.handleBlockAsBtc(block, height);
    }

    /* package */
    boolean handlePeerAsBtc(UnsignedInteger address, UnsignedInteger port, UnsignedLong services, UnsignedInteger timestamp) {
        return core.handlePeerAsBtc(address, port, services, timestamp);
    }

    // This must be called explicitly once migration is complete.  After `release`,
    // `this` must not be used again.
    void release () {
        core.give();
    }
}
