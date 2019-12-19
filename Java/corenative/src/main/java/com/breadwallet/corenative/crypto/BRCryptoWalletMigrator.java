/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoWalletMigrator extends PointerType {

    public static Optional<BRCryptoWalletMigrator> create(BRCryptoNetwork network, String storagePath) {
        return Optional.fromNullable(
            CryptoLibraryDirect.cryptoWalletMigratorCreate(
                    network.getPointer(),
                    storagePath
            )
        ).transform(BRCryptoWalletMigrator::new);
    }

    public BRCryptoWalletMigrator() {
        super();
    }

    public BRCryptoWalletMigrator(Pointer address) {
        super(address);
    }

    public boolean handleTransactionAsBtc(byte[] bytes, UnsignedInteger blockHeight, UnsignedInteger timestamp) {
        Pointer thisPtr = this.getPointer();

        BRCryptoWalletMigratorStatus status = CryptoLibraryDirect.cryptoWalletMigratorHandleTransactionAsBTC(
                thisPtr,
                bytes,
                new SizeT(bytes.length),
                blockHeight.intValue(),
                timestamp.intValue()
        );
        return BRCryptoWalletMigratorStatus.CRYPTO_WALLET_MIGRATOR_SUCCESS == status.type;
    }

    public boolean handlePeerAsBtc(UnsignedInteger address, UnsignedInteger port, UnsignedLong services,
                                   UnsignedInteger timestamp) {
        Pointer thisPtr = this.getPointer();

        BRCryptoWalletMigratorStatus status = CryptoLibraryDirect.cryptoWalletMigratorHandlePeerAsBTC(
                thisPtr,
                address.intValue(),
                port.shortValue(),
                services.longValue(),
                timestamp.intValue()
        );
        return BRCryptoWalletMigratorStatus.CRYPTO_WALLET_MIGRATOR_SUCCESS == status.type;
    }

    public boolean handleBlockAsBtc(byte[] block,
                                    UnsignedInteger height) {
        Pointer thisPtr = this.getPointer();

        BRCryptoWalletMigratorStatus status = CryptoLibraryDirect.cryptoWalletMigratorHandleBlockBytesAsBTC(
                thisPtr,
                block,
                new SizeT(block.length),
                height.intValue()
        );
        return BRCryptoWalletMigratorStatus.CRYPTO_WALLET_MIGRATOR_SUCCESS == status.type;
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletMigratorRelease(thisPtr);
    }
}
