package com.breadwallet.crypto.libcrypto.support;

public interface BRSyncMode {

    int SYNC_MODE_BRD_ONLY = 0;
    int SYNC_MODE_BRD_WITH_P2P_SEND = 1;
    int SYNC_MODE_P2P_WITH_BRD_SYNC = 2;
    int SYNC_MODE_P2P_ONLY = 3;
}
