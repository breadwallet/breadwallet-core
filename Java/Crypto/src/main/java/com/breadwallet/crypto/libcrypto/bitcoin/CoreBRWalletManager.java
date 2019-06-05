/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.bitcoin;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;
import com.google.common.primitives.UnsignedInts;

import java.util.Date;
import java.util.List;

public interface CoreBRWalletManager {

    static OwnedBRWalletManager create(BRWalletManagerClient.ByValue client, BRMasterPubKey.ByValue pubKey,
                                       BRChainParams chainParams, Date earliestKeyTime, int mode, String path) {
        int timestamp = UnsignedInts.checkedCast(earliestKeyTime.getTime() / 1000);
        return new OwnedBRWalletManager(CryptoLibrary.INSTANCE.BRWalletManagerNew(client, pubKey, chainParams,
                timestamp, mode, path));
    }

    BRWallet getWallet();

    void generateUnusedAddrs(int limit);

    List<String> getAllAddrs();

    List<String> getAllAddrsLegacy();

    void connect();

    void disconnect();

    void scan();

    void submitTransaction(CoreBRTransaction tx, byte[] seed);

    boolean matches(BRWalletManager walletManager);

    void announceBlockNumber(int rid, long blockNumber);

    void announceSubmit(int rid, CoreBRTransaction transaction, int error);

    void announceTransaction(int rid, CoreBRTransaction transaction);

    void announceTransactionComplete(int rid, boolean success);
}
