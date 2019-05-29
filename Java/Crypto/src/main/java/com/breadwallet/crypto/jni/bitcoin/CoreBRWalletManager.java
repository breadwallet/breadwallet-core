package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.BRMasterPubKey;

public interface CoreBRWalletManager {

    static OwnedBRWalletManager create(BRWalletManagerClient.ByValue client, BRMasterPubKey.ByValue pubKey,
                                       BRChainParams chainParams, int timestamp, int mode, String path) {
        return new OwnedBRWalletManager(CryptoLibrary.INSTANCE.BRWalletManagerNew(client, pubKey, chainParams,
                timestamp, mode, path));
    }

    BRWallet getWallet();

    BRPeerManager getPeerManager();

    void connect();

    void disconnect();

    void scan();

    boolean matches(BRWalletManager walletManager);
}
