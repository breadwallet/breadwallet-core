/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.support.UInt256;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.google.common.primitives.Ints;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.List;

public class BRCryptoWalletMigrator extends PointerType {

    public BRCryptoWalletMigrator(Pointer address) {
        super(address);
    }

    public BRCryptoWalletMigrator() {
        super();
    }

    public static Optional<BRCryptoWalletMigrator> create(BRCryptoNetwork network, String storagePath) {
        return Optional.fromNullable(
            CryptoLibrary.INSTANCE.cryptoWalletMigratorCreate(network, storagePath)
        );
    }

    public boolean handleTransactionAsBtc(byte[] bytes, UnsignedInteger blockHeight, UnsignedInteger timestamp) {
        BRCryptoWalletMigratorStatus status = CryptoLibrary.INSTANCE.cryptoWalletMigratorHandleTransactionAsBTC(
                this, bytes, new SizeT(bytes.length), blockHeight.intValue(), timestamp.intValue()
        );
        return BRCryptoWalletMigratorStatus.CRYPTO_WALLET_MIGRATOR_SUCCESS == status.type;
    }

    public boolean handlePeerAsBtc(UnsignedInteger address, UnsignedInteger port, UnsignedLong services,
                                   UnsignedInteger timestamp) {
        BRCryptoWalletMigratorStatus status = CryptoLibrary.INSTANCE.cryptoWalletMigratorHandlePeerAsBTC(
                this,
                address.intValue(),
                port.shortValue(),
                services.longValue(),
                timestamp.intValue()
        );
        return BRCryptoWalletMigratorStatus.CRYPTO_WALLET_MIGRATOR_SUCCESS == status.type;
    }

    public boolean handleBlockAsBtc(byte[] block,
                                    UnsignedInteger height) {
        BRCryptoWalletMigratorStatus status = CryptoLibrary.INSTANCE.cryptoWalletMigratorHandleBlockBytesAsBTC(
                this,
                block,
                new SizeT(block.length),
                height.intValue()
        );
        return BRCryptoWalletMigratorStatus.CRYPTO_WALLET_MIGRATOR_SUCCESS == status.type;
    }

    public static class OwnedBRCryptoWalletMigrator extends BRCryptoWalletMigrator {

        public OwnedBRCryptoWalletMigrator(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoWalletMigrator() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoWalletMigratorRelease(this);
            }
        }
    }
}
