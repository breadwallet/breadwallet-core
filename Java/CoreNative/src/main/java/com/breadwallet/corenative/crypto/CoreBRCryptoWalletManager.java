/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.primitives.UnsignedLong;

public interface CoreBRCryptoWalletManager {

    static CoreBRCryptoWalletManager create(BRCryptoWalletManager manager) {
        return new OwnedBRCryptoWalletManager(manager);
    }

    static CoreBRCryptoWalletManager create(BRCryptoCWMListener.ByValue listener, BRCryptoCWMClient.ByValue client,
                                            CoreBRCryptoAccount account, CoreBRCryptoNetwork network, int mode,
                                            String path) {
        return new OwnedBRCryptoWalletManager(CryptoLibrary.INSTANCE.cryptoWalletManagerCreate(
                listener, client, account.asBRCryptoAccount(), network.asBRCryptoNetwork(), mode, path));
    }

    CoreBRCryptoWallet getWallet();

    UnsignedLong getWalletsCount();

    CoreBRCryptoWallet getWallet(UnsignedLong index);

    boolean containsWallet(CoreBRCryptoWallet wallet);

    int getMode();

    String getPath();

    int getState();

    void connect();

    void disconnect();

    void sync();

    void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, String paperKey);

    void announceBlockNumber(BRCryptoCallbackHandle handle, UnsignedLong blockchainHeight, boolean success);

    void announceTransaction(BRCryptoCallbackHandle handle, byte[] transaction, UnsignedLong timestamp, UnsignedLong blockHeight);

    void announceTransactionComplete(BRCryptoCallbackHandle handle, boolean success);

    void announceSubmit(BRCryptoCallbackHandle handle, boolean success);

    void announceBalance(BRCryptoCallbackHandle handle, UnsignedLong balance, boolean success);
}
