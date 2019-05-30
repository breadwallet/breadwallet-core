package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.BRMasterPubKey;

import java.util.List;

public interface CoreBRWalletManager {

    static OwnedBRWalletManager create(BRWalletManagerClient.ByValue client, BRMasterPubKey.ByValue pubKey,
                                       BRChainParams chainParams, int timestamp, int mode, String path) {
        return new OwnedBRWalletManager(CryptoLibrary.INSTANCE.BRWalletManagerNew(client, pubKey, chainParams,
                timestamp, mode, path));
    }

    BRWallet getWallet();

    BRPeerManager getPeerManager();

    List<String> getUnusedAddrsLegacy(int limit);

    void connect();

    void disconnect();

    void scan();

    boolean matches(BRWalletManager walletManager);

    void announceBlockNumber(int rid, long blockNumber);

    void announceSubmit(int rid, CoreBRTransaction transaction, int error);

    void announceTransaction(int rid, CoreBRTransaction transaction);

    void announceTransactionComplete(int rid, boolean success);
}
