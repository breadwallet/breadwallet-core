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

    // these values must be in sync with the enum values for BRCryptoWalletMigratorStatusType
    private static final int CRYPTO_WALLET_MIGRATOR_SUCCESS = 0;

    public BRCryptoWalletMigrator(Pointer address) {
        super(address);
    }

    public BRCryptoWalletMigrator() {
        super();
    }

    public static Optional<BRCryptoWalletMigrator> create(CoreBRCryptoNetwork network, String storagePath) {
        return Optional.fromNullable(
            CryptoLibrary.INSTANCE.cryptoWalletMigratorCreate(network.asBRCryptoNetwork(), storagePath)
        );
    }

    public boolean handleTransactionAsBtc(byte[] bytes, UnsignedInteger blockHeight, UnsignedInteger timestamp) {
        return CRYPTO_WALLET_MIGRATOR_SUCCESS == CryptoLibrary.INSTANCE.cryptoWalletMigratorHandleTransactionAsBTC(
                this,
                bytes,
                new SizeT(bytes.length),
                blockHeight.intValue(),
                timestamp.intValue()
        );
    }

    public boolean handlePeerAsBtc(UnsignedInteger address, UnsignedInteger port, UnsignedLong services,
                                   UnsignedInteger timestamp) {
        return CRYPTO_WALLET_MIGRATOR_SUCCESS == CryptoLibrary.INSTANCE.cryptoWalletMigratorHandlePeerAsBTC(
                this,
                address.intValue(),
                port.shortValue(),
                services.longValue(),
                timestamp.intValue()
        );
    }

    public boolean handleBlockAsBtc(byte[] block,
                                    UnsignedInteger height) {
        return CRYPTO_WALLET_MIGRATOR_SUCCESS == CryptoLibrary.INSTANCE.cryptoWalletMigratorHandleBlockBytesAsBTC(
                this,
                block, new SizeT(block.length),
                height.intValue()
        );
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
